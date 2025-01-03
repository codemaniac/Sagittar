#include "movegen.h"
#include "utils.h"

namespace sagittar {

    namespace movegen {

        static const u64 MAGIC_BISHOP[64] = {
          0x89a1121896040240ULL, 0x2004844802002010ULL, 0x2068080051921000ULL,
          0x62880a0220200808ULL, 0x4042004000000ULL,    0x100822020200011ULL,
          0xc00444222012000aULL, 0x28808801216001ULL,   0x400492088408100ULL,
          0x201c401040c0084ULL,  0x840800910a0010ULL,   0x82080240060ULL,
          0x2000840504006000ULL, 0x30010c4108405004ULL, 0x1008005410080802ULL,
          0x8144042209100900ULL, 0x208081020014400ULL,  0x4800201208ca00ULL,
          0xf18140408012008ULL,  0x1004002802102001ULL, 0x841000820080811ULL,
          0x40200200a42008ULL,   0x800054042000ULL,     0x88010400410c9000ULL,
          0x520040470104290ULL,  0x1004040051500081ULL, 0x2002081833080021ULL,
          0x400c00c010142ULL,    0x941408200c002000ULL, 0x658810000806011ULL,
          0x188071040440a00ULL,  0x4800404002011c00ULL, 0x104442040404200ULL,
          0x511080202091021ULL,  0x4022401120400ULL,    0x80c0040400080120ULL,
          0x8040010040820802ULL, 0x480810700020090ULL,  0x102008e00040242ULL,
          0x809005202050100ULL,  0x8002024220104080ULL, 0x431008804142000ULL,
          0x19001802081400ULL,   0x200014208040080ULL,  0x3308082008200100ULL,
          0x41010500040c020ULL,  0x4012020c04210308ULL, 0x208220a202004080ULL,
          0x111040120082000ULL,  0x6803040141280a00ULL, 0x2101004202410000ULL,
          0x8200000041108022ULL, 0x21082088000ULL,      0x2410204010040ULL,
          0x40100400809000ULL,   0x822088220820214ULL,  0x40808090012004ULL,
          0x910224040218c9ULL,   0x402814422015008ULL,  0x90014004842410ULL,
          0x1000042304105ULL,    0x10008830412a00ULL,   0x2520081090008908ULL,
          0x40102000a0a60140ULL,
        };

        static const u64 MAGIC_ROOK[64] = {
          0xa8002c000108020ULL,  0x6c00049b0002001ULL,  0x100200010090040ULL,
          0x2480041000800801ULL, 0x280028004000800ULL,  0x900410008040022ULL,
          0x280020001001080ULL,  0x2880002041000080ULL, 0xa000800080400034ULL,
          0x4808020004000ULL,    0x2290802004801000ULL, 0x411000d00100020ULL,
          0x402800800040080ULL,  0xb000401004208ULL,    0x2409000100040200ULL,
          0x1002100004082ULL,    0x22878001e24000ULL,   0x1090810021004010ULL,
          0x801030040200012ULL,  0x500808008001000ULL,  0xa08018014000880ULL,
          0x8000808004000200ULL, 0x201008080010200ULL,  0x801020000441091ULL,
          0x800080204005ULL,     0x1040200040100048ULL, 0x120200402082ULL,
          0xd14880480100080ULL,  0x12040280080080ULL,   0x100040080020080ULL,
          0x9020010080800200ULL, 0x813241200148449ULL,  0x491604001800080ULL,
          0x100401000402001ULL,  0x4820010021001040ULL, 0x400402202000812ULL,
          0x209009005000802ULL,  0x810800601800400ULL,  0x4301083214000150ULL,
          0x204026458e001401ULL, 0x40204000808000ULL,   0x8001008040010020ULL,
          0x8410820820420010ULL, 0x1003001000090020ULL, 0x804040008008080ULL,
          0x12000810020004ULL,   0x1000100200040208ULL, 0x430000a044020001ULL,
          0x280009023410300ULL,  0xe0100040002240ULL,   0x200100401700ULL,
          0x2244100408008080ULL, 0x8000400801980ULL,    0x2000810040200ULL,
          0x8010100228810400ULL, 0x2000009044210200ULL, 0x4080008040102101ULL,
          0x40002080411d01ULL,   0x2005524060000901ULL, 0x502001008400422ULL,
          0x489a000810200402ULL, 0x1004400080a13ULL,    0x4000011008020084ULL,
          0x26002114058042ULL,
        };

        struct AttackMask {
            board::BitBoard mask;
            u8              mask_bits;
        };

        static AttackMask ATTACK_MASK_TABLE_BISHOP[64];
        static AttackMask ATTACK_MASK_TABLE_ROOK[64];

        static board::BitBoard ATTACK_TABLE_PAWN[2][64];
        static board::BitBoard ATTACK_TABLE_KNIGHT[64];
        static board::BitBoard ATTACK_TABLE_BISHOP[64][512];
        static board::BitBoard ATTACK_TABLE_ROOK[64][4096];
        static board::BitBoard ATTACK_TABLE_KING[64];

        static board::BitBoard bishopMask(const Square sq) {
            i8 r, f;

            const u8 tr = sq2rank(sq);
            const u8 tf = sq2file(sq);

            board::BitBoard attack_mask = 0ULL;

            for (r = tr + 1, f = tf + 1; r <= 6 && f <= 6; r++, f++)
            {
                attack_mask |= (1ULL << rf2sq(r, f));
            }
            for (r = tr + 1, f = tf - 1; r <= 6 && f >= 1; r++, f--)
            {
                attack_mask |= (1ULL << rf2sq(r, f));
            }
            for (r = tr - 1, f = tf + 1; r >= 1 && f <= 6; r--, f++)
            {
                attack_mask |= (1ULL << rf2sq(r, f));
            }
            for (r = tr - 1, f = tf - 1; r >= 1 && f >= 1; r--, f--)
            {
                attack_mask |= (1ULL << rf2sq(r, f));
            }

            return attack_mask;
        }

        static board::BitBoard bishopAttacks(const Square sq, const board::BitBoard blockers) {
            i8 r, f;

            const u8 tr = sq2rank(sq);
            const u8 tf = sq2file(sq);

            board::BitBoard attack_mask = 0ULL;
            board::BitBoard sqb;

            for (r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++)
            {
                sqb = 1ULL << rf2sq(r, f);
                attack_mask |= sqb;
                if (blockers & sqb)
                {
                    break;
                }
            }
            for (r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--)
            {
                sqb = 1ULL << rf2sq(r, f);
                attack_mask |= sqb;
                if (blockers & sqb)
                {
                    break;
                }
            }
            for (r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++)
            {
                sqb = 1ULL << rf2sq(r, f);
                attack_mask |= sqb;
                if (blockers & sqb)
                {
                    break;
                }
            }
            for (r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--)
            {
                sqb = 1ULL << rf2sq(r, f);
                attack_mask |= sqb;
                if (blockers & sqb)
                {
                    break;
                }
            }

            return attack_mask;
        }

        static void initAttackMaskTableBishop(void) {
            board::BitBoard mask;
            u8              mask_bits;

            for (u8 sq = 0; sq < 64; sq++)
            {
                mask      = bishopMask(static_cast<Square>(sq));
                mask_bits = utils::bitCount1s(mask);

                ATTACK_MASK_TABLE_BISHOP[sq].mask      = mask;
                ATTACK_MASK_TABLE_BISHOP[sq].mask_bits = mask_bits;
            }
        }

        static board::BitBoard rookMask(const Square sq) {
            i8 r, f;

            const u8 tr = sq2rank(sq);
            const u8 tf = sq2file(sq);

            board::BitBoard attack_mask = 0ULL;

            for (r = tr + 1; r <= 6; r++)
            {
                attack_mask |= (1ULL << rf2sq(r, tf));
            }
            for (r = tr - 1; r >= 1; r--)
            {
                attack_mask |= (1ULL << rf2sq(r, tf));
            }
            for (f = tf + 1; f <= 6; f++)
            {
                attack_mask |= (1ULL << rf2sq(tr, f));
            }
            for (f = tf - 1; f >= 1; f--)
            {
                attack_mask |= (1ULL << rf2sq(tr, f));
            }

            return attack_mask;
        }

        static board::BitBoard rookAttacks(const Square sq, const board::BitBoard blockers) {
            i8 r, f;

            const u8 tr = sq2rank(sq);
            const u8 tf = sq2file(sq);

            board::BitBoard attack_mask = 0ULL;
            board::BitBoard sqb;

            for (r = tr + 1; r <= 7; r++)
            {
                sqb = 1ULL << rf2sq(r, tf);
                attack_mask |= sqb;
                if (blockers & sqb)
                {
                    break;
                }
            }
            for (r = tr - 1; r >= 0; r--)
            {
                sqb = 1ULL << rf2sq(r, tf);
                attack_mask |= sqb;
                if (blockers & sqb)
                {
                    break;
                }
            }
            for (f = tf + 1; f <= 7; f++)
            {
                sqb = 1ULL << rf2sq(tr, f);
                attack_mask |= sqb;
                if (blockers & sqb)
                {
                    break;
                }
            }
            for (f = tf - 1; f >= 0; f--)
            {
                sqb = 1ULL << rf2sq(tr, f);
                attack_mask |= sqb;
                if (blockers & sqb)
                {
                    break;
                }
            }

            return attack_mask;
        }

        static void initAttackMaskTableRook(void) {
            board::BitBoard mask;
            u8              mask_bits;

            for (u8 sq = 0; sq < 64; sq++)
            {
                mask      = rookMask(static_cast<Square>(sq));
                mask_bits = utils::bitCount1s(mask);

                ATTACK_MASK_TABLE_ROOK[sq].mask      = mask;
                ATTACK_MASK_TABLE_ROOK[sq].mask_bits = mask_bits;
            }
        }

        static board::BitBoard getVariant(u32 index, u8 bits, board::BitBoard m) {
            i32             i, j;
            board::BitBoard result = 0ULL;
            for (i = 0; i < bits; i++)
            {
                j = utils::bitScanForward(&m);
                if (index & (1 << i))
                {
                    result |= (1ULL << j);
                }
            }
            return result;
        }

        static u32 transform(const board::BitBoard b, const board::BitBoard magic, const u8 bits) {
            return (u32) ((b * magic) >> (64 - bits));
        }

        static void initAttackTableBishop() {
            board::BitBoard mask, b;
            u8              n;
            u32             magic_index;

            for (u8 sq = 0; sq < 64; sq++)
            {
                mask = ATTACK_MASK_TABLE_BISHOP[sq].mask;
                n    = ATTACK_MASK_TABLE_BISHOP[sq].mask_bits;

                for (i32 i = 0; i < (1 << n); i++)
                {
                    b           = getVariant(i, n, mask);
                    magic_index = transform(b, MAGIC_BISHOP[sq], n);
                    ATTACK_TABLE_BISHOP[sq][magic_index] =
                      bishopAttacks(static_cast<Square>(sq), b);
                }
            }
        }

        static void initAttackTableRook() {
            board::BitBoard mask, b;
            u8              n;
            u32             magic_index;

            for (u8 sq = 0; sq < 64; sq++)
            {
                mask = ATTACK_MASK_TABLE_ROOK[sq].mask;
                n    = ATTACK_MASK_TABLE_ROOK[sq].mask_bits;

                for (i32 i = 0; i < (1 << n); i++)
                {
                    b                                  = getVariant(i, n, mask);
                    magic_index                        = transform(b, MAGIC_ROOK[sq], n);
                    ATTACK_TABLE_ROOK[sq][magic_index] = rookAttacks(static_cast<Square>(sq), b);
                }
            }
        }

        static void initAttackTablePawn() {
            Square          sq;
            board::BitBoard b;
            board::BitBoard attacks;

            // White pawn attacks
            for (u8 r = RANK_1; r <= RANK_8; r++)
            {
                for (u8 f = FILE_A; f <= FILE_H; f++)
                {
                    sq                           = rf2sq(r, f);
                    b                            = 1ULL << sq;
                    attacks                      = board::northEast(b) | board::northWest(b);
                    ATTACK_TABLE_PAWN[WHITE][sq] = attacks;
                }
            }

            // Black pawn attacks
            for (i8 r = RANK_8; r >= RANK_1; r--)
            {
                for (u8 f = FILE_A; f <= FILE_H; f++)
                {
                    sq                           = rf2sq(r, f);
                    b                            = 1ULL << sq;
                    attacks                      = board::southEast(b) | board::southWest(b);
                    ATTACK_TABLE_PAWN[BLACK][sq] = attacks;
                }
            }
        }

        static void initAttackTableKnight() {
            board::BitBoard attacks = 0ULL;
            board::BitBoard b;

            for (u8 sq = 0; sq < 64; sq++)
            {
                b = 1ULL << sq;

                attacks = 0ULL;
                attacks |= (b & board::MASK_NOT_H_FILE) << 17;
                attacks |= (b & board::MASK_NOT_GH_FILE) << 10;
                attacks |= (b & board::MASK_NOT_GH_FILE) >> 6;
                attacks |= (b & board::MASK_NOT_H_FILE) >> 15;
                attacks |= (b & board::MASK_NOT_A_FILE) << 15;
                attacks |= (b & board::MASK_NOT_AB_FILE) << 6;
                attacks |= (b & board::MASK_NOT_AB_FILE) >> 10;
                attacks |= (b & board::MASK_NOT_A_FILE) >> 17;

                ATTACK_TABLE_KNIGHT[sq] = attacks;
            }
        }

        static void initAttackTableKing() {
            board::BitBoard attacks = 0ULL;
            board::BitBoard b;

            for (u8 sq = 0; sq < 64; sq++)
            {
                b = 1ULL << sq;

                attacks = 0ULL;
                attacks |= board::north(b);
                attacks |= board::south(b);
                attacks |= board::east(b);
                attacks |= board::west(b);
                attacks |= board::northEast(b);
                attacks |= board::southEast(b);
                attacks |= board::southWest(b);
                attacks |= board::northWest(b);

                ATTACK_TABLE_KING[sq] = attacks;
            }
        }

        static board::BitBoard
        getPawnAttacks(const Square sq, const Color c, const board::BitBoard occupancy) {
            return ATTACK_TABLE_PAWN[c][sq] & occupancy;
        }

        static board::BitBoard getKnightAttacks(const Square sq, const board::BitBoard occupancy) {
            return ATTACK_TABLE_KNIGHT[sq] & occupancy;
        }

        static board::BitBoard getBishopAttacks(const Square sq, board::BitBoard occupancy) {
            occupancy = occupancy & ATTACK_MASK_TABLE_BISHOP[sq].mask;
            const u32 index =
              transform(occupancy, MAGIC_BISHOP[sq], ATTACK_MASK_TABLE_BISHOP[sq].mask_bits);
            return ATTACK_TABLE_BISHOP[sq][index];
        }

        static board::BitBoard getRookAttacks(const Square sq, board::BitBoard occupancy) {
            occupancy = occupancy & ATTACK_MASK_TABLE_ROOK[sq].mask;
            const u32 index =
              transform(occupancy, MAGIC_ROOK[sq], ATTACK_MASK_TABLE_ROOK[sq].mask_bits);
            return ATTACK_TABLE_ROOK[sq][index];
        }

        static board::BitBoard getQueenAttacks(const Square sq, board::BitBoard occupancy) {
            return getBishopAttacks(sq, occupancy) | getRookAttacks(sq, occupancy);
        }

        static board::BitBoard getKingAttacks(const Square sq, const board::BitBoard occupancy) {
            return ATTACK_TABLE_KING[sq] & occupancy;
        }

        static void generatePseudolegalMovesPawn(std::vector<move::Move>* moves,
                                                 const board::Board&      board,
                                                 const MovegenType        type) {
            const Color  active_color   = board.getActiveColor();
            const Square ep_target      = board.getEnpassantTarget();
            const Rank   promotion_rank = promotionRankDestOf(active_color);
            const Piece  pawn           = pieceCreate(PieceType::PAWN, active_color);
            auto         bb             = board.getBitboard(pawn);
            while (bb)
            {
                const Square from = static_cast<Square>(utils::bitScanForward(&bb));
                auto         occupancy =
                  board.getBitboard(board::bitboardColorSlot(colorFlip(active_color)));
                if (ep_target != Square::NO_SQ)
                {
                    occupancy |= (1ULL << ep_target);
                }
                auto attacks = getPawnAttacks(from, active_color, occupancy);
                while (attacks)
                {
                    const Square to       = static_cast<Square>(utils::bitScanForward(&attacks));
                    const Piece  captured = board.getPiece(to);
                    if (sq2rank(to) == promotion_rank)
                    {
                        moves->emplace_back(from, to, captured,
                                            move::MoveFlag::MOVE_CAPTURE_PROMOTION_QUEEN);
                        moves->emplace_back(from, to, captured,
                                            move::MoveFlag::MOVE_CAPTURE_PROMOTION_ROOK);
                        moves->emplace_back(from, to, captured,
                                            move::MoveFlag::MOVE_CAPTURE_PROMOTION_BISHOP);
                        moves->emplace_back(from, to, captured,
                                            move::MoveFlag::MOVE_CAPTURE_PROMOTION_KNIGHT);
                    }
                    else if (to == ep_target)
                    {
                        const Piece captured =
                          pieceCreate(PieceType::PAWN, colorFlip(active_color));
                        moves->emplace_back(from, to, captured, move::MoveFlag::MOVE_CAPTURE_EP);
                    }
                    else
                    {
                        moves->emplace_back(from, to, captured, move::MoveFlag::MOVE_CAPTURE);
                    }
                }

                if (type == MovegenType::CAPTURES)
                {
                    continue;
                }
                attacks   = (active_color == Color::WHITE) ? board::north((1ULL << from))
                                                           : board::south((1ULL << from));
                occupancy = board.getBitboard(Piece::NO_PIECE);
                attacks &= occupancy;
                if (attacks)
                {
                    const Square to = static_cast<Square>(utils::bitScanForward(&attacks));
                    if (sq2rank(to) == promotion_rank)
                    {
                        moves->emplace_back(from, to, Piece::NO_PIECE,
                                            move::MoveFlag::MOVE_PROMOTION_QUEEN);
                        moves->emplace_back(from, to, Piece::NO_PIECE,
                                            move::MoveFlag::MOVE_PROMOTION_ROOK);
                        moves->emplace_back(from, to, Piece::NO_PIECE,
                                            move::MoveFlag::MOVE_PROMOTION_BISHOP);
                        moves->emplace_back(from, to, Piece::NO_PIECE,
                                            move::MoveFlag::MOVE_PROMOTION_KNIGHT);
                    }
                    else
                    {
                        moves->emplace_back(from, to, Piece::NO_PIECE, move::MoveFlag::MOVE_QUIET);
                    }
                }
                // Pawn Double push
                if (active_color == WHITE)
                {
                    attacks = board::north((1ULL << from)) & occupancy;
                    attacks = board::north(attacks) & occupancy & board::MASK_RANK_4;
                }
                else
                {
                    attacks = board::south((1ULL << from)) & occupancy;
                    attacks = board::south(attacks) & occupancy & board::MASK_RANK_5;
                }
                if (attacks)
                {
                    const Square to = static_cast<Square>(utils::bitScanForward(&attacks));
                    moves->emplace_back(from, to, Piece::NO_PIECE,
                                        move::MoveFlag::MOVE_QUIET_PAWN_DBL_PUSH);
                }
            }
        }

        static void generatePseudolegalMovesPiece(std::vector<move::Move>* moves,
                                                  const board::Board&      board,
                                                  const PieceType          piece_type,
                                                  const MovegenType        type) {
            const Color active_color = board.getActiveColor();
            const Piece piece        = pieceCreate(piece_type, active_color);
            auto        bb           = board.getBitboard(piece);
            while (bb)
            {
                const Square from = static_cast<Square>(utils::bitScanForward(&bb));

                board::BitBoard occupancy = 0ULL;
                board::BitBoard attacks   = 0ULL;
                switch (piece_type)
                {
                    case KNIGHT :
                        occupancy =
                          board.getBitboard(board::bitboardColorSlot(colorFlip(active_color)))
                          | board.getBitboard(Piece::NO_PIECE);
                        attacks = getKnightAttacks(from, occupancy);
                        break;

                    case BISHOP :
                        occupancy = ~board.getBitboard(Piece::NO_PIECE);
                        attacks   = getBishopAttacks(from, occupancy);
                        break;

                    case ROOK :
                        occupancy = ~board.getBitboard(Piece::NO_PIECE);
                        attacks   = getRookAttacks(from, occupancy);
                        break;

                    case QUEEN :
                        occupancy = ~board.getBitboard(Piece::NO_PIECE);
                        attacks   = getQueenAttacks(from, occupancy);
                        break;

                    case KING :
                        occupancy =
                          board.getBitboard(board::bitboardColorSlot(colorFlip(active_color)))
                          | board.getBitboard(Piece::NO_PIECE);
                        attacks = getKingAttacks(from, occupancy);
                        break;

                    default :
                        break;
                }

                while (attacks)
                {
                    const Square to       = static_cast<Square>(utils::bitScanForward(&attacks));
                    const Piece  captured = board.getPiece(to);
                    if (type == MovegenType::ALL && captured == Piece::NO_PIECE)
                    {
                        moves->emplace_back(from, to, Piece::NO_PIECE, move::MoveFlag::MOVE_QUIET);
                    }
                    else if (captured != Piece::NO_PIECE && pieceColorOf(captured) != active_color)
                    {
                        moves->emplace_back(from, to, captured, move::MoveFlag::MOVE_CAPTURE);
                    }
                }
            }
        }

        static void generatePseudolegalMovesCastle(std::vector<move::Move>* moves,
                                                   const board::Board&      board) {

            if (isInCheck(board))
            {
                return;
            }

            const Color active_color     = board.getActiveColor();
            const u8    casteling_rights = board.getCastelingRights();

            if (active_color == Color::WHITE)
            {
                if (casteling_rights & board::CastleFlag::WKCA)
                {
                    if (board.getPiece(Square::E1) == Piece::WHITE_KING
                        && board.getPiece(Square::F1) == Piece::NO_PIECE
                        && board.getPiece(Square::G1) == Piece::NO_PIECE
                        && board.getPiece(Square::H1) == Piece::WHITE_ROOK
                        && !isSquareAttacked(board, Square::F1, Color::BLACK))
                    {
                        moves->emplace_back(Square::E1, Square::G1, Piece::NO_PIECE,
                                            move::MoveFlag::MOVE_CASTLE_KING_SIDE);
                    }
                }
                if (casteling_rights & board::CastleFlag::WQCA)
                {
                    if (board.getPiece(Square::E1) == Piece::WHITE_KING
                        && board.getPiece(Square::D1) == Piece::NO_PIECE
                        && board.getPiece(Square::C1) == Piece::NO_PIECE
                        && board.getPiece(Square::B1) == Piece::NO_PIECE
                        && board.getPiece(Square::A1) == Piece::WHITE_ROOK
                        && !isSquareAttacked(board, Square::D1, Color::BLACK))
                    {
                        moves->emplace_back(Square::E1, Square::C1, Piece::NO_PIECE,
                                            move::MoveFlag::MOVE_CASTLE_QUEEN_SIDE);
                    }
                }
            }
            else
            {
                if (casteling_rights & board::CastleFlag::BKCA)
                {
                    if (board.getPiece(Square::E8) == Piece::BLACK_KING
                        && board.getPiece(Square::F8) == Piece::NO_PIECE
                        && board.getPiece(Square::G8) == Piece::NO_PIECE
                        && board.getPiece(Square::H8) == Piece::BLACK_ROOK
                        && !isSquareAttacked(board, Square::F8, Color::WHITE))
                    {
                        moves->emplace_back(Square::E8, Square::G8, Piece::NO_PIECE,
                                            move::MoveFlag::MOVE_CASTLE_KING_SIDE);
                    }
                }
                if (casteling_rights & board::CastleFlag::BQCA)
                {
                    if (board.getPiece(Square::E8) == Piece::BLACK_KING
                        && board.getPiece(Square::D8) == Piece::NO_PIECE
                        && board.getPiece(Square::C8) == Piece::NO_PIECE
                        && board.getPiece(Square::B8) == Piece::NO_PIECE
                        && board.getPiece(Square::A8) == Piece::BLACK_ROOK
                        && !isSquareAttacked(board, Square::D8, Color::WHITE))
                    {
                        moves->emplace_back(Square::E8, Square::C8, Piece::NO_PIECE,
                                            move::MoveFlag::MOVE_CASTLE_QUEEN_SIDE);
                    }
                }
            }
        }

        void initialize() {
            initAttackTablePawn();

            initAttackTableKnight();

            initAttackMaskTableBishop();
            initAttackTableBishop();

            initAttackMaskTableRook();
            initAttackTableRook();

            initAttackTableKing();
        }

        bool isSquareAttacked(const board::Board& board, const Square sq, const Color attacked_by) {
            const board::BitBoard occupancy = ~(board.getBitboard(Piece::NO_PIECE));

            Piece piece = pieceCreate(PieceType::QUEEN, attacked_by);
            if (getQueenAttacks(sq, occupancy) & board.getBitboard(piece))
            {
                return true;
            }

            piece = pieceCreate(PieceType::ROOK, attacked_by);
            if (getRookAttacks(sq, occupancy) & board.getBitboard(piece))
            {
                return true;
            }

            piece = pieceCreate(PieceType::BISHOP, attacked_by);
            if (getBishopAttacks(sq, occupancy) & board.getBitboard(piece))
            {
                return true;
            }

            piece = pieceCreate(PieceType::KNIGHT, attacked_by);
            if (ATTACK_TABLE_KNIGHT[sq] & board.getBitboard(piece))
            {
                return true;
            }

            piece = pieceCreate(PieceType::PAWN, attacked_by);
            if (ATTACK_TABLE_PAWN[colorFlip(attacked_by)][sq] & board.getBitboard(piece))
            {
                return true;
            }

            piece = pieceCreate(PieceType::KING, attacked_by);
            if (ATTACK_TABLE_KING[sq] & board.getBitboard(piece))
            {
                return true;
            }

            return false;
        }

        bool isInCheck(const board::Board& board) {
            const Piece     king        = pieceCreate(PieceType::KING, board.getActiveColor());
            board::BitBoard bb          = board.getBitboard(king);
            const Square    sq          = static_cast<Square>(utils::bitScanForward(&bb));
            const Color     attacked_by = colorFlip(board.getActiveColor());
            return isSquareAttacked(board, sq, attacked_by);
        }


        void generatePseudolegalMoves(std::vector<move::Move>* moves,
                                      const board::Board&      board,
                                      const MovegenType        type) {
            generatePseudolegalMovesPawn(moves, board, type);
            generatePseudolegalMovesPiece(moves, board, PieceType::KNIGHT, type);
            generatePseudolegalMovesPiece(moves, board, PieceType::BISHOP, type);
            generatePseudolegalMovesPiece(moves, board, PieceType::ROOK, type);
            generatePseudolegalMovesPiece(moves, board, PieceType::QUEEN, type);
            generatePseudolegalMovesPiece(moves, board, PieceType::KING, type);
            generatePseudolegalMovesCastle(moves, board);
        }

    }

}