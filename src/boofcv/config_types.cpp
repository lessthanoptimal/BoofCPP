#include <cmath>
#include <stdexcept>
#include "config_types.h"

using namespace boofcv;

boofcv::ConfigLength::ConfigLength(double length, double fraction) {
    this->length = length;
    this->fraction = fraction;
}

boofcv::ConfigLength::ConfigLength() = default;

boofcv::ConfigLength boofcv::ConfigLength::fixed(double pixels ) {
    return ConfigLength(pixels,-1);
}

boofcv::ConfigLength boofcv::ConfigLength::relative(double fraction , int minimum ) {
    return ConfigLength(minimum,fraction);
}

double boofcv::ConfigLength::compute(double totalLength) const  {
    double size;
    if (fraction >= 0) {
        size = fraction*totalLength;
        size = fmax(size, length);
    } else {
        size = length;
    }

    return size;
}

bool boofcv::ConfigLength::isRelative() const  {
    return fraction >= 0;
}

bool boofcv::ConfigLength::isFixed() const  {
    return fraction < 0;
}

int boofcv::ConfigLength::computeI( double totalLength ) const {
    double size = compute(totalLength);
    if( size >= 0 )
        return (int)(size+0.5); // size is always more than zero. Ignore incorrect rounding
    else
        return -1;
}

int boofcv::ConfigLength::getLengthI() const {
    return (int)(length + 0.5);
}

void boofcv::ConfigLength::checkValidity() const  {
    if( length < 0 && fraction < 0 )
        throw std::invalid_argument("length and/or fraction must be >= 0");
}