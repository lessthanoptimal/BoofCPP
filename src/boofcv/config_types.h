#ifndef BOOFCPP_BINARY_OPS_H
#define BOOFCPP_BINARY_OPS_H

#include "base_types.h"

namespace boofcv
{
    /**
     * Base class for configuration paramters
     */
    class Configuration {
    public:
        virtual void checkValidity() = 0;
    };

    /**
     * Specifies a length as a fixed length or relative to the total size of some other object.
     *
     * @author Peter Abeles
     */
    class ConfigLength : public Configuration {
    public:
        /**
         * If in fixed mode this is the length or it's the minimum length of a relative length is being specified
         */
        double length = -1;
        /**
         * If >= 0 the length is relative to the total size and the 'fixed' number is treated
         * as a minium size.
         */
        double fraction = -1;

        ConfigLength(double length, double fraction);

        ConfigLength();

        /**
         * Creates one fixed length configuration
         */
        static ConfigLength fixed(double pixels );

        /**
         * Creates a relative length configuration with the specified minimum size
         */
        static ConfigLength relative(double fraction , int minimum );

        double compute(double totalLength);

        bool isRelative();

        bool isFixed();

        /**
         * Computes the length as an integer
         */
        int computeI( double totalLength );

        int getLengthI();

        void checkValidity() override;
    };
}

#endif