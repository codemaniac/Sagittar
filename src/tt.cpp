#include "tt.h"
#include "search.h"

namespace sagittar {

    namespace search {

        TranspositionTable::TranspositionTable() :
            TranspositionTable(DEFAULT_TT_SIZE_MB) {}

        TranspositionTable::TranspositionTable(const std::size_t mb) :
            size_mb(mb) {
            setSize(mb);
            currentage = 0;
        }

        void TranspositionTable::setSize(const std::size_t mb) {
            size = (mb * 0x100000) / sizeof(TTEntry);
            size -= sizeof(TTEntry);
            entries.clear();
            entries.resize(size);
            entries.shrink_to_fit();
            clear();
        }

        std::size_t TranspositionTable::getSize() const { return size; }

        void TranspositionTable::clear() {
            for (u32 i = 0; i < size; i++)
            {
                entries.at(i) = TTEntry();
            }
            currentage = 0;
        }

        void TranspositionTable::resetForSearch() { currentage++; }

        void TranspositionTable::store(const board::Board& board,
                                       const i8            depth,
                                       const TTFlag        flag,
                                       i32                 value,
                                       const move::Move    move) {
            const u64 hash  = board.getHash();
            const u64 index = hash % size;
            TTEntry&  entry = entries.at(index);

            // Only handles empty indices or stale entires
            const bool replace =
              (entry.hash == 0ULL) || (entry.getAge() < currentage) || (entry.getDepth() <= depth);
            if (!replace)
            {
                return;
            }

            if (value < -MATE_SCORE)
            {
                value -= board.getPlyCount();
            }
            else if (value > MATE_SCORE)
            {
                value += board.getPlyCount();
            }

            // If current entry is from the current position AND if move is a null move,
            // DO NOT replace the move in the entry
            move::Move move_to_replace = move;
            if (move == move::Move() && entry.isValid(hash))
            {
                move_to_replace = entry.getMove();
            }

            // Store entry
            entry             = TTEntry(hash, depth, currentage, flag, value, move_to_replace);
            entries.at(index) = entry;
        }

        bool TranspositionTable::probe(TTData* data, const board::Board& board) const {
            const u64      hash  = board.getHash();
            const u64      index = hash % size;
            const TTEntry& entry = entries.at(index);
            if (entry.isValid(hash))
            {
                *data = entry.toTTData();
                return true;
            }
            return false;
        }

    }

}
