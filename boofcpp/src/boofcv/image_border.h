#ifndef BOOFCPP_IMAGE_BORDER_H
#define BOOFCPP_IMAGE_BORDER_H

#include "image_types.h"
#include "sanity_checks.h"

namespace boofcv {

    /**
     * Image border class for a Gray image
     */
    template< class E>
    class ImageBorder {
    public:
        const Gray<E>* image = nullptr;

        virtual ~ImageBorder() {

        }

        uint32_t getWidth() const {
            return image->width;
        }

        uint32_t getHeight() const {
            return image->height;
        }

        void setImage( const Gray<E>& image ) {
            this->image = &image;
        }

        E get( int32_t x , int32_t y) const {
            if( x >= 0 && y >= 0 && x < image->width && y < image->height ) {
                return image->at(x,y);
            }
            return outside_get(x,y);
        }

        virtual E outside_get( int32_t x , int32_t y ) const = 0;
    };

    template< class E>
    class ImageBorderIndex : public ImageBorder<E> {
    public:
        E outside_get( int32_t x , int32_t y ) const override {
            adjust(x,this->image->width);
            adjust(y,this->image->height);
            return this->image->unsafe_at(x,y);
        }

        virtual void adjust( int32_t& x , const uint32_t& max_value ) const = 0;
    };


    /**
     * Pixels outside the image boundary are set to the specified value
     * @tparam E
     */
    template< class E>
    class ImageBorderValue : public ImageBorder<E> {
    public:
        E value;

        ImageBorderValue( E value ) {
            this->value = value;
        }

        ImageBorderValue( const Gray<E>& image , E value ) {
            this->setImage(image);
            this->value = value;
        }

        E outside_get( int32_t x , int32_t y ) const override {
            return this->value;
        }
    };

    /**
     * Pixels outside the image border at set to the closest image pixel by Manhattan distance
     */
    template< class E>
    class ImageBorderExtend : public ImageBorderIndex<E> {
    public:
        void adjust( int32_t& x , const uint32_t& max_value ) const override {
            if( x < 0 )
                x = 0;
            else if( x >= max_value )
                x = max_value-1;
        }
    };

    /**
     * Pixels outside the image border at set to the closest image pixel by Manhattan distance
     */
    template< class E>
    class ImageBorderReflect : public ImageBorderIndex<E> {
    public:
        void adjust( int32_t& x , const uint32_t& max_value ) const override {
            if( x < 0 )
                x = -x;
            else if( x >= max_value)
                x = max_value-2-(x-max_value);
        }
    };

    /**
     * Wraps a pixel value around to the other side of the image
     */
    template< class E>
    class ImageBorderWrap : public ImageBorderIndex<E> {
    public:
        void adjust( int32_t& x , const uint32_t& max_value ) const override {
            if( x < 0 )
                x = max_value+x;
            else if( x >= max_value)
                x = x-max_value;
        }
    };

    enum BorderType
    {
        /**
	     * Image borders are not processed and are simply skipped over.
	     */
        SKIP,

        /**
         * The pixels along the image border are extended outwards.  This is recommended for computing the gradient
         * and many convolution operations.
         */
        EXTENDED,

        /**
         * The kernel is renormalized to take in account that parts of it are not inside the image.
         * Typically only used with kernels that blur the image.
         */
        NORMALIZED,

        /**
         * Access to outside the array are reflected back into the array around the closest border.  This
         * is an even symmetric function, e.g. f(-1) = f(1) = 1, f(-2) = f(2) = 2.
         */
        REFLECT,

        /**
         * Also known as periodic. When a pixel outside of image is accessed it wraps around to the other side of the image
         * as if the image is a loop.  Primarily included for historical purposes and because other people use it.  In
         * most applications it has the potential of introducing structured noise, which is bad.
         */
        WRAP,

        /**
         * The image border is set to a fixed value of zero.  Generates harsh edges that can cause artifacts
         * in some applications.
         */
        ZERO
    };

    class FactoryImageBorder
    {
    public:

        template< class E >
        static ImageBorder<Gray<E>> create_SB( BorderType type ) {
            switch( type ) {
                case EXTENDED:
                    return ImageBorderExtend<E>();
                case REFLECT:
                    return ImageBorderReflect<E>();
                case WRAP:
                    return ImageBorderWrap<E>();
                case ZERO:
                    return value_SB((E)0);

                default:
                    throw invalid_argument("Unknown type");
            }
        }


        template< class E>
        static ImageBorder<Gray<E>> value_SB( E v ) {
            return ImageBorderValue<E>(v);
        }

        // TODO This doesn't seem to work well in C++. Is there a way to do this?
        template< class E>
        static ImageBorder<E> value( const Gray<E>& image , E v ) {
            ImageBorderValue<E> border(v);

            border.setImage(v);

            return border;
        }
    };
}

#endif

