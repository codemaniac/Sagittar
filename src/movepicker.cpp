#include "movepicker.h"

namespace sagittar {

    namespace search {

        // clang-format off
        /*
            (Victims) Pawn   Knight Bishop Rook   Queen  King
        (Attackers)
        Pawn          105    205    305    405    505    605
        Knight        104    204    304    404    504    604
        Bishop        103    203    303    403    503    603
        Rook          102    202    302    402    502    602
        Queen         101    201    301    401    501    601
        King          100    200    300    400    500    600
        */
        static const u16 MVV_LVA_TABLE[36] = {
            105, 205, 305, 405, 505, 605,
            104, 204, 304, 404, 504, 604,
            103, 203, 303, 403, 503, 603,
            102, 202, 302, 402, 502, 602,
            101, 201, 301, 401, 501, 601,
            100, 200, 300, 400, 500, 600
        };
        // clang-format on

        static constexpr u16 MVVLVA_SCORE_OFFSET = 10000;

        static constexpr u8 mvvlvaIdx(const PieceType attacker, const PieceType victim) {
            return ((attacker - 1) * 6) + (victim - 1);
        }

        void scoreMoves(containers::ArrayList<move::Move>* moves, const board::Board& board) {
            for (u8 i = 0; i < moves->size(); i++)
            {
                const move::Move move = moves->at(i);

                if (move::isCapture(move.getFlag()))
                {
                    const PieceType attacker = pieceTypeOf(board.getPiece(move.getFrom()));
                    const PieceType victim   = pieceTypeOf(board.getPiece(move.getTo()));
                    const u16       score =
                      MVV_LVA_TABLE[mvvlvaIdx(attacker, victim)] + MVVLVA_SCORE_OFFSET;
                    moves->at(i).setScore(score);
                }
            }
        }

        void sortMoves(containers::ArrayList<move::Move>* moves, const u8 index) {
            for (u32 i = index + 1; i < moves->size(); i++)
            {
                if (moves->at(i).getScore() > moves->at(index).getScore())
                {
                    std::swap(moves->at(index), moves->at(i));
                }
            }
        }

    }

}