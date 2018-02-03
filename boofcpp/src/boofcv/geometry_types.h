#ifndef BOOFCPP_GEOMETRY_TYPES_H
#define BOOFCPP_GEOMETRY_TYPES_H

#include <cstdint>
#include <stdexcept>
#include <cstring>

#include "base_types.h"

namespace boofcv {

    template<class E>
    class Point2D {
    public:
        typedef typename TypeInfo<E>::signed_type signed_type;
        typedef typename TypeInfo<E>::sum_type sum_type;

        E x;
        E y;

        Point2D( E x , E y) {
            this->x = x;
            this->y = y;
        }

        Point2D() {
            this->x = 0;
            this->y = 0;
        }

        sum_type distanceSq( E x , E y ) const {
            signed_type dx = x-this->x;
            signed_type dy = y-this->y;
            return static_cast<sum_type>(dx*dx + dy*dy);
        }

        sum_type distanceSq( const Point2D<E>& p ) const {
            return distanceSq(p.x,p.y);
        }
    };

}

#endif