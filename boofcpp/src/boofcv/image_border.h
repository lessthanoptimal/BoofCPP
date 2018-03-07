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
        const Gray<E>* image;

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
            if( image->isInBounds(x,y)) {
                return image->at(x,y);
            }
            return outside_get(x,y);
        }

        virtual E outside_get( int32_t x , int32_t y ) const = 0;
    };

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

    class FactoryImageBorder
    {
    public:
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

