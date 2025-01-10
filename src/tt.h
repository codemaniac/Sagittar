#pragma once

#include "board.h"
#include "move.h"
#include "pch.h"
#include "types.h"

namespace sagittar {

    namespace search {

        enum TTFlag : u8 {
            NONE,
            LOWERBOUND,
            UPPERBOUND,
            EXACT
        };

        constexpr i32 INF_BOUND = 52000;

        struct TTData {
            i8         depth;
            TTFlag     flag;
            i32        value;
            move::Move move;
        };

        struct TTEntry {
            u64        hash;
            i8         depth;
            u8         age;
            TTFlag     flag;
            i32        value;
            move::Move move;


            TTEntry() :
                hash(0ULL),
                depth(0),
                age(0),
                flag(TTFlag::NONE),
                value(0),
                move(move::Move()) {}

            TTEntry(const u64        hash,
                    const u8         depth,
                    const u8         age,
                    const TTFlag     flag,
                    const i32        value,
                    const move::Move move) :
                hash(hash),
                depth(depth),
                age(age),
                flag(flag),
                value(value + INF_BOUND),
                move(move) {}

            u8 getDepth() const { return depth; }

            u8 getAge() const { return age; }

            move::Move getMove() const { return move; }

            bool isValid(const u64 h) const { return (hash == h); }

            TTData toTTData() const {
                TTData ttdata;
                ttdata.depth = getDepth();
                ttdata.flag  = flag;
                ttdata.value = value - INF_BOUND;
                ttdata.move  = getMove();
                return ttdata;
            }
        };

        class TranspositionTable {
           private:
            std::vector<TTEntry> entries;
            std::size_t          size_mb;
            std::size_t          size;
            u8                   currentage;

           public:
            TranspositionTable();
            TranspositionTable(const std::size_t mb);
            void               setSize(const std::size_t mb);
            std::size_t        getSize() const;
            void               clear();
            void               resetForSearch();
            void               store(const board::Board& board,
                                     const i8            depth,
                                     const TTFlag        flag,
                                     const i32           value,
                                     const move::Move    move);
            [[nodiscard]] bool probe(TTData* data, const board::Board& board) const;
        };

    }

}
