#ifndef BOOFCPP_PACKED_SETS_H
#define BOOFCPP_PACKED_SETS_H

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace boofcv {


    /**
     * Describes the location of elements that belong to a set.
     */
    struct PackedSetInfo {
        // The block the start of this set resides in
        uint32_t block;
        // The index in the block that the set starts in
        uint32_t offset;
        // number of points in this set
        uint32_t size;

        PackedSetInfo() : block(0),offset(0),size(0) {
        }
    };

    /**
     * Specifies a collection of sets of variable lengths. Instead of storing an element in a set directly they
     * are stored in an array. The array is broken up into blocks to avoid a large copy when it needs to grow. A
     * set is specified by the block its first element appears in, the offset in the block it first appears in,
     * and its size/number of elements.
     *
     * This data structure is designed to make manipulating the last or tail set very in expensive. creation/destruction
     * of memory is minimized
     *
     * @tparam T Data type of an element
     */
    template<typename T>
    class PackedSet {
    public:
        // storage for each block. All blocks are the same size
        T** blocks;
        // number of blocks which have been declared
        uint32_t _number_of_blocks;
        // number of elements in each block
        uint32_t _size_of_block;
        // total number of elements which are used in a set
        uint32_t _total_element;
        // Describes where to find elements that belong to each set
        std::vector<PackedSetInfo> set_info;

        PackedSet( uint32_t size_of_block ) :
                _size_of_block(size_of_block) ,_number_of_blocks(1), _total_element(0)
        {
            blocks = new T*[_number_of_blocks];
            for( uint32_t i = 0; i < _number_of_blocks; i++ ) {
                blocks[i] = new T[_size_of_block];
            }
        }

        PackedSet() : PackedSet(2000) {}

        ~PackedSet() {
            for( uint32_t i = 0; i < _number_of_blocks; i++ ) {
                delete []blocks[i];
            }
            delete []blocks;
            blocks = nullptr;
            _number_of_blocks = 0;
            _size_of_block = 0;
            _total_element = 0;

        }

        void clear() {
            this->set_info.clear();
            this->_total_element = 0;
        }

        /**
         * Creates a new set with zero elements and adds it to the tail
         */
        void start_new_set() {
            PackedSetInfo set;

            set.size = 0;
            set.block = _total_element/_size_of_block;
            set.offset = _total_element%_size_of_block;

            this->set_info.push_back(set);
        }

        /**
         * Size of the tail set
         */
        uint32_t size_of_tail() const {
            if( set_info.size() == 0 )
                return 0;
            return set_info.back().size;
        }

        /**
         * Pushes the element on to the end of the tail set
         */
        void push_tail( const T& element ) {
            // if it's empty create a new set
            if( set_info.size() == 0 )
                start_new_set();

            _add_element(element);

            // increase the size of the tail set
            set_info.back().size++;
        }

        /**
         * Adds an element to the end of the queue but does not modify any sets. For internal use only.
         */
        void _add_element( const T& element ) {
            uint32_t block = _total_element/_size_of_block;
            if( block == _number_of_blocks ) {
                T** tmp = new T*[block+1];
                for( uint32_t i = 0; i < _number_of_blocks; i++ ) {
                    tmp[i] = blocks[i];
                }
                tmp[_number_of_blocks] = new T[_size_of_block];
                delete []blocks;
                this->blocks = tmp;
                this->_number_of_blocks++;
            }
            this->blocks[block][_total_element%_size_of_block] = element;
            _total_element++;
        }

        /**
         * Removes the set that's at the end.
         */
        void remove_tail() {
            if( set_info.size() > 0 ) {
                this->_total_element -= set_info.back().size;
                this->set_info.pop_back();
            }
        }

        /**
         * A slow function for accessing an element in a set
         * @param index_set Which set the element belongs to
         * @param index_element Which element in the set
         * @return The element
         */
        T& at( uint32_t index_set , uint32_t index_element ) const
        {
            if( index_set >= set_info.size()  )
                throw std::invalid_argument("set out of range");

            const PackedSetInfo& set = set_info.at(index_set);

            if( index_element >= set.size )
                throw std::invalid_argument("element out of range");

            uint32_t b = set.block*_size_of_block + set.offset + index_element;

            return this->blocks[b / _size_of_block][b % _size_of_block];
        }

        void load_set( uint32_t index_set , std::vector<T>& output ) const
        {
            const PackedSetInfo& set = set_info.at(index_set);

            for( uint32_t i = 0; i < set.size; i++ ) {
                uint32_t b = set.block*_size_of_block + set.offset + i;
                output.push_back( this->blocks[b / _size_of_block][b % _size_of_block] );
            }
        }

        void write_set( uint32_t index_set , const std::vector<T>& input )
        {
            const PackedSetInfo& set = set_info.at(index_set);

            if( input.size() != set.size )
                throw std::invalid_argument("sizes do not match");

            for( uint32_t i = 0; i < set.size; i++ ) {
                uint32_t b = set.block*_size_of_block + set.offset + i;
                this->blocks[b / _size_of_block][b % _size_of_block] = input[i];
            }
        }

        uint32_t number_of_sets() {
            return static_cast<uint32_t>(set_info.size());
        }
    };
}


#endif