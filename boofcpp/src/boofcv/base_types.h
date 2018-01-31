#ifndef BOOFCPP_BASE_TYPES_H
#define BOOFCPP_BASE_TYPES_H

#include <cstdint>
#include <stdexcept>
#include <cstring>

namespace boofcv {
    typedef uint8_t U8;
    typedef uint16_t U16;
    typedef int16_t S16;
    typedef int32_t S32;
    typedef int64_t S64;
    typedef float F32;
    typedef double F64;

    template<class _T> class TypeInfo;

    template<> class TypeInfo<U8> {
    public:
        typedef uint32_t sum_type;
    };
    template<> class TypeInfo<S16> {
    public:
        typedef int32_t sum_type;
    };
    template<> class TypeInfo<U16> {
    public:
        typedef uint32_t sum_type;
    };
    template<> class TypeInfo<S32> {
    public:
        typedef int32_t sum_type;
    };
    template<> class TypeInfo<S64> {
    public:
        typedef int64_t sum_type;
    };
    template<> class TypeInfo<F32> {
    public:
        typedef float sum_type;
    };
    template<> class TypeInfo<F64> {
    public:
        typedef double sum_type;
    };

    /**
     * Growable array with similiar behavor to GrowArray
     * @tparam T
     */
    template<class T>
    class GrowArray {
    public:

        T* data;
        uint32_t size;
        uint32_t array_length;
        // value that new elements are set to
        T default_value = 0;

    private:
        // indicate if the array can be resized. If it can't be resized that means this class doesn't
        // own the data
        bool can_resize;

    public:
        GrowArray( T*data , uint32_t array_length, uint32_t size ) {
            this->data = data;
            this->array_length = array_length;
            this->size = size;
            can_resize = false;
        }

        GrowArray( uint32_t size ) {
            this->data = new T[size];
            this->size = size;
            this->array_length = size;
            this->can_resize = true;
            fill(default_value);
        }

        GrowArray() : GrowArray(10) {

        }

        ~GrowArray() {
            if( !can_resize )
                return;
            delete []data;
            this->size = 0;
            this->array_length = 0;
        }

        /**
         * Increases the array_length and sets the size to zero. Previous data is not saved.
         * @param new_size
         */
        void grow_array( uint32_t new_size ) {
            if( !can_resize )
                throw std::invalid_argument("Can't resize array");
            if( new_size > array_length ) {
                delete []this->data;
                this->data = new T[new_size];
                this->array_length = new_size;
                memset(data,default_value,new_size);
            }
            this->size = 0;
        }

        /**
         * Resizes the internal array while saving its previous values.  Changes the size and array_length
         * @param new_size
         */
        void resize( uint32_t new_size ) {
            if( !can_resize )
                throw std::invalid_argument("Can't resize array");

            if( new_size > array_length ) {
                auto *tmp = new T[new_size];
                memmove(tmp,this->data,this->size);
                memset(&tmp[this->array_length],default_value,sizeof(T)*(new_size-this->size));
                delete []this->data;
                this->data = tmp;
                this->array_length = new_size;
            }
            this->size = new_size;
        }

        void fill( T value ) {
            std::memset(this->data,default_value,this->size*sizeof(T));
        }

        T& at( uint32_t index ) const {
            if( index >= this->size )
                throw std::invalid_argument("index out of bounds");
            return this->data[index];
        }

        T& operator[] (uint32_t index) const {
            return this->data[index];
        }
    };

}


#endif