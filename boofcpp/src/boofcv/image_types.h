#ifndef BOOFCPP_IMAGE_TYPES_H
#define BOOFCPP_IMAGE_TYPES_H

#include <cstdint>
#include <exception>
#include <stdexcept>
#include <vector>

using namespace std;

// TODO Design issue. What happens if the parent of a subimage is deleted or resized? The subimage will now
//      point to bad data. Would be nice if this was caught and prevented

namespace boofcv {

    /**
     * Base class for all image types
     */
    class ImageBase
    {
    public:
        uint32_t width,height;
        uint32_t offset;
        uint32_t stride;
        bool subimage;

        ImageBase() {
            this->width = 0;
            this->height = 0;
            this->stride = 0;
            this->subimage = false;
        }

        /**
         * Total number of pixels in the image
         */
        uint32_t total_pixels() const {
            return this->width*this->height;
        }

        /**
         * Changes the image's shapes. Value of pixels after this function is called is undefined.
         * @param width New image width
         * @param height New image height
         */
        virtual void reshape( uint32_t width , uint32_t height ) = 0;

        /**
         * Tests to see if the coordinate is within the image.
         */
        bool isInBounds( uint32_t x , uint32_t y ) const {
            return x < this->width && y < this->height;
        }
    };

    template<class _PT>
    class ImageBaseT : public ImageBase
    {
    public:
        // data type of a pixel value
        typedef _PT pixel_type;
    };


    /**
     * Gray scale image
     *
     * @tparam T primitive type of data array
     */
    template<class T>
    class Gray : public ImageBaseT<T> {
    public:
        // NOTE: Decided not to use a vector here since wrapping it around an array isn't trivial
        T* data;
        // length of the array. This might be larger than the number of elements in the image because of reshaping
        uint32_t data_length;

        Gray( uint32_t width , uint32_t height ) {
            this->width = width;
            this->height = height;
            this->data = new T[width*height]();
            this->data_length = width*height;
            this->offset = 0;
            this->stride = width;
            this->subimage = false;
        }

        /**
         * Constructor which creates a sub-image. Memory will not be freed when deconstructor is called
         */
        Gray( T* data , uint32_t data_length, uint32_t width , uint32_t height , uint32_t offset , uint32_t stride ) {
            this->data = data;
            this->data_length = data_length;
            this->width = width;
            this->height = height;
            this->offset = offset;
            this->stride = stride;
            this->subimage = true;
        }

        Gray(std::initializer_list<std::initializer_list<T>> l ){
            this->height = static_cast<uint32_t >(l.size());

            if( this->height == 0 ) {
                this->width = 0;
            } else {
                this->width = static_cast<uint32_t >(l.begin()->size());
            }
            this->data = new T[this->width*this->height]();
            this->data_length = this->width*this->height;
            this->offset = 0;
            this->stride = this->width;
            this->subimage = false;
            
            T* ptr = this->data;
            for (auto& x : l ) {
                if( x.size() != this->width ) {
                    throw invalid_argument("width missmatch");
                }
                for(auto& y : x ) {
                    *ptr++ = y;
                }
            }
        }

        Gray() : Gray(0,0) {
        }

        ~Gray() {
            if( !this->subimage ) {
                delete[]this->data;
            }
            this->width = 0;
            this->height = 0;
            this->data_length = 0;
            this->offset = 0;
            this->stride = 0;
            this->data = NULL;
        }

        void reshape( uint32_t width , uint32_t height ) override {
            if( this->width == width && this->height == height )
                return;
            else if( this->subimage ) {
                throw invalid_argument("Can't reshape a subimage");
            }

            uint32_t desired_length = width*height;
            if( desired_length > data_length ) {
                delete []data;
                data = new T[desired_length]();
                this->data_length = desired_length;
            }
            this->width = width;
            this->height = height;
            this->stride = width;
        }

        T& at( uint32_t x , uint32_t y ) const {
            if( x >= this->width || y >= this->height )
                throw invalid_argument("out of range");
            return data[this->offset + y*this->stride + x];
        }

        T& unsafe_at( uint32_t x , uint32_t y ) const {
            return data[this->offset + y*this->stride + x];
        }

        void setTo( const Gray<T>& src ) {
            if (this->subimage && (this->width != src.width || this->height != src.height) ) {
                throw invalid_argument("Shapes must match for sub images");
            } else {
                reshape(src.width, src.height);
            }
            // This will handle the situation where all of some of the images are sub-images
            for( uint32_t y = 0; y < this->height; y++ ) {
                memcpy(&this->data[this->offset+y*this->stride],&src.data[src.offset+y*src.stride], sizeof(T)*src.width);
            }
        }

        Gray<T> makeSubimage( uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1 ) const {
            if( x1 > this->width || y1 > this->height || x1 < x0 || y1 < y0 )
                throw invalid_argument("Bounds are illegal");
            return Gray<T>(this->data,this->data_length,x1-x0,y1-y0,this->offset + y0*this->stride+x0, this->stride );
        }

        uint32_t index_of( uint32_t x , uint32_t y ) const {
            return this->offset + y*this->stride + x;
        }

        Gray<T>& operator=(const Gray<T>& other) // copy assignment
        {
            if (this != &other) { // self-assignment check expected
                this->setTo(other);
            }
            return *this;
        }
    };

    /**
     * Planar image. Each band in a planar image is a Gray image.
     * @tparam T A Gray image.
     */
    template<class T>
    class Planar : public ImageBaseT<typename T::pixel_type> {
    public:
        T** bands;
        uint32_t num_bands;

        Planar( uint32_t width , uint32_t height , uint32_t num_bands ) {
            this->width = width;
            this->height = height;
            this->offset = 0;
            this->stride = width;
            this->subimage = false;

            this->num_bands = num_bands;
            bands = new T*[this->num_bands];
            for( uint32_t i = 0; i < this->num_bands; i++ ) {
                bands[i] = new T(width,height);
            }
        }

        Planar() : Planar(0,0,0) {
        }

        ~Planar() {
            this->width = 0;
            this->height = 0;
            this->offset = 0;
            this->stride = 0;
            for( uint32_t i = 0; i < num_bands; i++ ) {
                delete bands;
            }
            delete []bands;
            bands = NULL;
        }

        void reshape( uint32_t width , uint32_t height ) override {
            if( this->subimage ) {
                throw invalid_argument("Can't reshape a subimage");
            }
            for( uint32_t i = 0; i < num_bands; i++ ) {
                bands[i]->reshape(width,height);
            }
            this->width = width;
            this->height = height;
            this->stride = width;
        }

        void setNumberOfBands( uint32_t desired ) {
            T* new_bands;
            if( desired > this->num_bands ) {
                new_bands = new T*[desired];
                for( uint32_t i = 0; i < this->num_bands; i++ ) {
                    new_bands[i] = this->bands[i];
                }
                for( uint32_t i = num_bands; i < desired; i++ ) {
                    new_bands[i] = new T(this->width,this->height);
                }
            } else if( desired < this->num_bands ) {
                new_bands = new T*[desired];
                for( uint32_t i = 0; i < desired; i++ ) {
                    new_bands[i] = this->bands[i];
                }
                for( uint32_t i = desired; i < this->num_bands; i++ ) {
                    delete this->bands[i];
                }
            } else {
                return;
            }
            delete []this->bands;
            this->bands = new_bands;
            this->num_bands = desired;
        }

        T& at( uint32_t x , uint32_t y , uint32_t band ) const {
            if( band >= this->num_bands )
                throw invalid_argument("Band out of range");

            return this->bands[band]->at(x,y);
        }

        void setTo( const Planar<T>& src ) {
            if (this->subimage && (this->width != src.width || this->height != src.height || this->num_bands != src.num_bands) ) {
                throw invalid_argument("Shapes must match for sub images");
            } else {
                reshape(src.width, src.height);
            }
            setNumberOfBands(src.num_bands);
            for( uint32_t i = 0; i < this->num_bands; i++ ) {
                this->bands[i]->setTo(src.bands[i]);
            }
        }

        T& getBand( uint32_t which ) const {
            if( which >= this->num_bands )
                throw invalid_argument("Requested an out of bounds band");
            return *(this->bands[which]);
        }
    };

    /**
     * Banded interleaved image
     *
     * @tparam T primitive type of data array
     */
    template<class T>
    class Interleaved : public ImageBaseT<T> {
    public:
        // NOTE: Decided not to use a vector here since wrapping it around an array isn't trivial
        T* data;
        // length of the array. This might be larger than the number of elements in the image because of reshaping
        uint32_t data_length;

        // number of bands in the image
        uint32_t num_bands;

        Interleaved( uint32_t width , uint32_t height , uint32_t num_bands ) {
            this->width = width;
            this->height = height;
            this->num_bands = num_bands;
            this->data_length = width*height*num_bands;
            this->data = new T[this->data_length]();
            this->offset = 0;
            this->stride = width*num_bands;
            this->subimage = false;
        }

        /**
         * Constructor which creates a sub-image. Memory will not be freed when deconstructor is called
         */
        Interleaved( T* data , uint32_t data_length, uint32_t width , uint32_t height , uint32_t num_bands, uint32_t offset , uint32_t stride ) {
            this->data = data;
            this->data_length = data_length;
            this->width = width;
            this->height = height;
            this->num_bands = num_bands;
            this->offset = offset;
            this->stride = stride;
            this->subimage = true;
        }

        Interleaved() : Interleaved(0,0,0) {
        }

        ~Interleaved() {
            if( !this->subimage ) {
                this->width = 0;
                this->height = 0;
                this->num_bands = 0;
                this->data_length = 0;
                this->offset = 0;
                this->stride = 0;
                delete[]this->data;
                this->data = NULL;
            }
        }

        void reshape( uint32_t width , uint32_t height , uint32_t bands ) {
            if( this->width == width && this->height == height && this->num_bands == bands ) {
                return;
            } else if( this->subimage ) {
                throw invalid_argument("Can't reshape a subimage");
            }

            uint32_t new_length = width*height*bands;
            if( new_length > data_length ) {
                if( this->subimage ) {
                    throw invalid_argument("Grow the data array in a subimage");
                }
                delete []data;
                data = new T[new_length]();
                this->data_length = new_length;
            }

            this->width = width;
            this->height = height;
            this->num_bands = bands;
            this->stride = width*bands;
        }

        void reshape( uint32_t width , uint32_t height ) override {
            if( this->width == width && this->height == height )
                return;
            else if( this->subimage ) {
                throw invalid_argument("Can't reshape a subimage");
            }

            uint32_t desired_length = width*height*this->num_bands;
            if( desired_length > this->data_length ) {
                delete []data;
                this->data = new T[desired_length]();
                this->data_length = desired_length;
            }
            this->width = width;
            this->height = height;
            this->stride = width*this->num_bands;
        }

        T& at( uint32_t x , uint32_t y , uint32_t band ) const {
            if( x >= this->width || y >= this->height )
                throw invalid_argument("out of range");
            return data[this->offset + y*this->stride + x*this->num_bands + band];
        }

        T& unsafe_at( uint32_t x , uint32_t y , uint32_t band ) const {
            return data[this->offset + y*this->stride + x*this->num_bands + band];
        }

        void setNumberOfBands( uint32_t desired ) {
            if( this->num_bands == desired ) {
                return;
            } else if( this->subimage ) {
                throw invalid_argument("Can't reshape a subimage");
            }

            uint32_t new_length = this->width*this->height*desired;
            if( new_length > data_length ) {
                if( this->subimage ) {
                    throw invalid_argument("Grow the data array in a subimage");
                }
                delete []data;
                data = new T[new_length]();
                this->data_length = new_length;
            }
            this->num_bands = desired;
            this->stride = this->width*desired;
        }

        void setTo( const Interleaved<T>& src ) {
            if (this->subimage && (this->width != src.width || this->height != src.height || this->num_bands != src.num_bands) ) {
                throw invalid_argument("Shapes must match for sub images");
            } else {
                setNumberOfBands(src.num_bands);
                reshape(src.width, src.height);
            }
            // This will handle the situation where all of some of the images are sub-images
            for( uint32_t y = 0; y < this->height; y++ ) {
                memcpy(this->data+this->offset+y*this->stride,src.data+src.offset+y*src.stride,src.width*this->num_bands);
            }
        }

        Interleaved<T> makeSubimage( uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1 ) {
            if( x1 < x0 || y1 < y0 )
                throw invalid_argument("Upper bounds can't be lower than lower bounds");
            if( x1 > this->width || y1 > this->height )
                throw invalid_argument("Subimage must be inside the image");

            return Interleaved<T>(this->data,this->data_length,x1-x0,y1-y0,
                                  this->num_bands,this->offset + y0*this->stride+x0*num_bands, this->stride );
        }

        uint32_t index_of( uint32_t x , uint32_t y , uint32_t band) const {
            return this->offset + y*this->stride + x*this->num_bands + band;
        }
    };
}


#endif