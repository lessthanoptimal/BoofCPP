#ifndef BOOFCPP_IMAGE_TYPES_H
#define BOOFCPP_IMAGE_TYPES_H

#include <cstdint>
#include <exception>
#include <stdexcept>
#include <vector>

using namespace std;

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

        virtual void reshape( uint32_t width , uint32_t height ) = 0;

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

        Gray() : Gray(0,0) {
        }

        ~Gray() {
            if( !this->subimage ) {
                this->width = 0;
                this->height = 0;
                this->data_length = 0;
                this->offset = 0;
                this->stride = 0;
                delete[]this->data;
                this->data = NULL;
            }
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

        void setTo( const Gray<T>& src ) {
            if (this->subimage && (this->width != src.width || this->height != src.height) ) {
                throw invalid_argument("Shapes must match for sub images");
            } else {
                reshape(src.width, src.height);
            }
            // This will handle the situation where all of some of the images are sub-images
            for( uint32_t y = 0; y < this->height; y++ ) {
                memcpy(this->data+this->offset+y*this->stride,src.data+src.offset+y*src.stride,src.width);
            }
        }

        Gray<T> makeSubimage( uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1 ) {
            return Gray<T>(this->data,this->data_length,x1-x0,y1-y0,this->offset + y0*this->stride+x0, this->stride );
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
        uint32_t number_of_bands;

        Planar( uint32_t width , uint32_t height , uint32_t number_of_bands ) {
            this->width = width;
            this->height = height;
            this->offset = 0;
            this->stride = width;
            this->subimage = false;

            this->number_of_bands = number_of_bands;
            bands = new T*[number_of_bands];
            for( uint32_t i = 0; i < number_of_bands; i++ ) {
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
            for( uint32_t i = 0; i < number_of_bands; i++ ) {
                delete bands;
            }
            delete []bands;
            bands = NULL;
        }

        void reshape( uint32_t width , uint32_t height ) override {
            if( this->subimage ) {
                throw invalid_argument("Can't reshape a subimage");
            }
            for( uint32_t i = 0; i < number_of_bands; i++ ) {
                bands[i]->reshape(width,height);
            }
            this->width = width;
            this->height = height;
            this->stride = width;
        }

        void setNumberOfBands( uint32_t desired ) {
            T* new_bands;
            if( desired > this->number_of_bands ) {
                new_bands = new T*[desired];
                for( uint32_t i = 0; i < this->number_of_bands; i++ ) {
                    new_bands[i] = this->bands[i];
                }
                for( uint32_t i = number_of_bands; i < desired; i++ ) {
                    new_bands[i] = new T(this->width,this->height);
                }
            } else if( desired < this->number_of_bands ) {
                new_bands = new T*[desired];
                for( uint32_t i = 0; i < desired; i++ ) {
                    new_bands[i] = this->bands[i];
                }
                for( uint32_t i = desired; i < this->number_of_bands; i++ ) {
                    delete this->bands[i];
                }
            } else {
                return;
            }
            delete []this->bands;
            this->bands = new_bands;
            this->number_of_bands = desired;
        }

        T& at( uint32_t x , uint32_t y , uint32_t band ) const {
            if( band >= this->number_of_bands )
                throw invalid_argument("Band out of range");

            return this->bands[band]->at(x,y);
        }

        void setTo( const Planar<T>& src ) {
            if (this->subimage && (this->width != src.width || this->height != src.height || this->number_of_bands != src.number_of_bands) ) {
                throw invalid_argument("Shapes must match for sub images");
            } else {
                reshape(src.width, src.height);
            }
            setNumberOfBands(src.number_of_bands);
            for( uint32_t i = 0; i < this->number_of_bands; i++ ) {
                this->bands[i]->setTo(src.bands[i]);
            }
        }

        T& getBand( uint32_t which ) const {
            if( which >= this->number_of_bands )
                throw invalid_argument("Requested an out of bounds band");
            return *(this->bands[which]);
        }
    };
}


#endif