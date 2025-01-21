#include "search.h"
#include "containers.h"
#include "eval.h"
#include "movegen.h"
#include "movepicker.h"
#include "utils.h"

namespace sagittar {

    namespace search {

        Searcher::Searcher() { stop.store(false, std::memory_order_relaxed); }

        void Searcher::shouldStopSearchNow(const SearchInfo& info) {
            if (info.timeset && (utils::currtimeInMilliseconds() >= info.stoptime))
            {
                stop.store(true, std::memory_order_relaxed);
            }
        }

        i32 Searcher::quiescencesearch(
          board::Board& board, i32 alpha, i32 beta, const SearchInfo& info, SearchResult* result) {
            if ((result->nodes & 2047) == 0)
            {
                shouldStopSearchNow(info);
            }

            if (board.getPlyCount() >= MAX_DEPTH)
            {
                return eval::evaluateBoard(board);
            }

            const i32 stand_pat = eval::evaluateBoard(board);
            if (stand_pat >= beta)
            {
                return beta;
            }
            if (alpha < stand_pat)
            {
                alpha = stand_pat;
            }

            containers::ArrayList<move::Move> moves;
            movegen::generatePseudolegalMoves(&moves, board, movegen::MovegenType::CAPTURES);
            scoreMoves(&moves, board, pvmove, tt);

            for (u8 i = 0; i < moves.size(); i++)
            {
                sortMoves(&moves, i);
                const move::Move move = moves.at(i);

                const board::DoMoveResult do_move_result = board.doMove(move);

                if (do_move_result == board::DoMoveResult::ILLEGAL)
                {
                    board.undoMove();
                    continue;
                }

                result->nodes++;

                const i32 score = -quiescencesearch(board, -beta, -alpha, info, result);

                board.undoMove();

                if (stop.load(std::memory_order_relaxed))
                {
                    return 0;
                }

                if (score > alpha)
                {
                    alpha = score;
                    if (score >= beta)
                    {
                        return beta;
                    }
                }
            }

            return alpha;
        }

        i32 Searcher::search(board::Board&     board,
                             i8                depth,
                             i32               alpha,
                             i32               beta,
                             const SearchInfo& info,
                             SearchResult*     result) {
            if ((result->nodes & 2047) == 0)
            {
                shouldStopSearchNow(info);
            }

            const i32 alpha_orig = alpha;

            if (board.getPlyCount() >= MAX_DEPTH) [[unlikely]]
            {
                return eval::evaluateBoard(board);
            }

            if (board.hasPositionRepeated() || board.getHalfmoveClock() >= 100)
            {
                return 0;
            }

            const bool is_in_check = movegen::isInCheck(board);

            if (is_in_check)
            {
                depth++;
            }

            if (depth <= 0)
            {
                return quiescencesearch(board, alpha, beta, info, result);
            }

            if (board.getPlyCount() > 0)
            {
                tt::TTEntry ttentry;
                const bool  tthit = tt.probe(&ttentry, board);
                if (tthit && ttentry.depth >= depth)
                {
                    i32 ttvalue = ttentry.value;

                    if (ttvalue < -MATE_SCORE)
                    {
                        ttvalue += board.getPlyCount();
                    }
                    else if (ttvalue > MATE_SCORE)
                    {
                        ttvalue -= board.getPlyCount();
                    }

                    if (ttentry.flag == tt::TTFlag::EXACT)
                    {
                        return ttvalue;
                    }
                }
            }

            i32        best_score = -INF;
            move::Move best_move_so_far;
            u32        legal_moves_count = 0;

            containers::ArrayList<move::Move> moves;
            movegen::generatePseudolegalMoves(&moves, board, movegen::MovegenType::ALL);
            scoreMoves(&moves, board, pvmove, tt);

            for (u8 i = 0; i < moves.size(); i++)
            {
                sortMoves(&moves, i);
                const move::Move move = moves.at(i);

                const board::DoMoveResult do_move_result = board.doMove(move);

                if (do_move_result == board::DoMoveResult::ILLEGAL)
                {
                    board.undoMove();
                    continue;
                }

                result->nodes++;
                legal_moves_count++;

                const i32 score = -search(board, depth - 1, -beta, -alpha, info, result);

                board.undoMove();

                if (stop.load(std::memory_order_relaxed))
                {
                    return 0;
                }

                if (score > best_score)
                {
                    best_score = score;
                    if (score > alpha)
                    {
                        alpha            = score;
                        best_move_so_far = move;
                        if (board.getPlyCount() == 0 && !stop.load(std::memory_order_relaxed))
                        {
                            pvmove = move;
                        }
                        if (score >= beta)
                        {
                            break;
                        }
                    }
                }
            }

            if (legal_moves_count == 0)
            {
                if (is_in_check)
                {
                    return -MATE_VALUE + board.getPlyCount();
                }
                else
                {
                    return 0;
                }
            }

            if (!stop.load(std::memory_order_relaxed))
            {
                tt::TTFlag flag = tt::TTFlag::NONE;
                if (best_score <= alpha_orig)
                {
                    flag = tt::TTFlag::UPPERBOUND;
                }
                else if (best_score >= beta)
                {
                    flag = tt::TTFlag::LOWERBOUND;
                }
                else
                {
                    flag = tt::TTFlag::EXACT;
                }
                tt.store(board, depth, flag, best_score, best_move_so_far);
            }

            if (board.getPlyCount() == 0 && !stop.load(std::memory_order_relaxed))
            {
                result->bestmove = pvmove;
            }
            return best_score;
        }

        SearchResult Searcher::searchRoot(
          board::Board&                                    board,
          const SearchInfo&                                info,
          std::function<void(const search::SearchResult&)> searchProgressReportHandler,
          std::function<void(const search::SearchResult&)> searchCompleteReportHander) {
            SearchResult bestresult{};

            for (u8 currdepth = 1; currdepth <= info.depth; currdepth++)
            {
                SearchResult result{};
                const u64    starttime = utils::currtimeInMilliseconds();
                i32          score     = search(board, currdepth, -INF, INF, info, &result);
                const u64    time      = utils::currtimeInMilliseconds() - starttime;
                if (stop.load(std::memory_order_relaxed))
                {
                    break;
                }

                bestresult = result;

                result.score = score;
                if (score > -MATE_VALUE && score < -MATE_SCORE)
                {
                    result.is_mate = true;
                    result.mate_in = (-(score + MATE_VALUE) / 2 - 1);
                }
                else if (score > MATE_SCORE && score < MATE_VALUE)
                {
                    result.is_mate = true;
                    result.mate_in = ((MATE_VALUE - score) / 2 + 1);
                }
                else
                {
                    result.is_mate = false;
                    result.mate_in = 0;
                }
                result.depth = currdepth;
                result.time  = time;
                result.pv    = {result.bestmove};
                searchProgressReportHandler(result);
            }

            searchCompleteReportHander(bestresult);

            return bestresult;
        }

        void Searcher::reset() {
            pvmove = move::Move();
            stop.store(false, std::memory_order_relaxed);
            tt.clear();
        }

        void Searcher::resetForSearch() { tt.resetForSearch(); }

        void Searcher::setTranspositionTableSize(const std::size_t size) { tt.setSize(size); }

        SearchResult Searcher::startSearch(
          board::Board&                                    board,
          const SearchInfo&                                info,
          std::function<void(const search::SearchResult&)> searchProgressReportHandler,
          std::function<void(const search::SearchResult&)> searchCompleteReportHander) {
            pvmove = move::Move();
            stop.store(false, std::memory_order_relaxed);
            return searchRoot(board, info, searchProgressReportHandler, searchCompleteReportHander);
        }

        SearchResult Searcher::startSearch(board::Board& board, const SearchInfo& info) {
            return startSearch(board, info, [](auto&) {}, [](auto&) {});
        }

        void Searcher::stopSearch() { stop.store(true, std::memory_order_relaxed); }

    }

}
