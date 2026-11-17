#ifndef BOOFCPP_CONFIG_DISPARITY_H
#define BOOFCPP_CONFIG_DISPARITY_H

#include <cstdint>
#include <stdexcept>

#include "boofcv/census_variants.h"
#include "boofcv/config_types.h"
#include "boofcv/image_border.h"

namespace boofcv {

    /// How the error between two blocks is computed. Same as BoofCV's DisparityError
    enum class DisparityError {
        /// Sum of Absolute Difference
        SAD,
        /// Sum of Squared Difference
        SSD,
        /// Zero-mean Sum of Squared Difference
        ZSSD,
        /// Hamming distance between census transforms
        CENSUS,
        /// Normalized Cross Correlation
        NCC
    };

    /// Names of DisparityError values, in the same order. Same as the Java enum names
    inline const char* const DISPARITY_ERROR_NAMES[] = {"SAD", "SSD", "ZSSD", "CENSUS", "NCC"};

    /// Configurations for different types of disparity error metrics. Same as BoofCV's ConfigDisparityError
    struct ConfigDisparityError {
        /// Configuration for Census
        class Census : public Configuration {
        public:
            CensusVariants variant = CensusVariants::BLOCK_5_5;

            void checkValidity() const override {}
        };

        /// Configuration for NCC
        class NCC : public Configuration {
        public:
            /// Avoids dividing by zero when dividing by the standard deviation. Tuned for pixel values scaled to -1 to 1.
            double eps = 5.5e-6;

            /// If true then the input is normalized to zero mean and a maximum absolute value of one
            bool normalize_input = true;

            void checkValidity() const override {}
        };
    };

    /// Calls visitor(java_name, field) for every field. See visit_fields() for ConfigDisparityBM
    template<class Visitor>
    void visit_fields( ConfigDisparityError::Census& c, Visitor&& v ) {
        v("variant", c.variant);
    }

    /// Calls visitor(java_name, field) for every field. See visit_fields() for ConfigDisparityBM
    template<class Visitor>
    void visit_fields( ConfigDisparityError::NCC& c, Visitor&& v ) {
        v("eps", c.eps);
        v("normalizeInput", c.normalize_input);
    }

    /// Configuration for stereo block matching. Same fields and defaults as BoofCV's ConfigDisparityBM, in snake case.
    /// Every field must also be listed in visit_fields() below.
    class ConfigDisparityBM : public Configuration {
    public:
        /// Minimum disparity to consider. Can be positive or negative.
        int32_t disparity_min = 0;
        /// Number of disparity values considered. Must be > 0
        int32_t disparity_range = 100;
        /// Radius of the rectangular region along x-axis.
        int32_t region_radius_x = 3;
        /// Radius of the rectangular region along y-axis.
        int32_t region_radius_y = 3;
        /// Maximum allowed error in a region per pixel. Disable by setting to <= 0.
        double max_per_pixel_error = 0;
        /// Tolerance for how different the left to right associated values can be. Disable with -1
        int32_t validate_r_to_l = 1;
        /// Tolerance for how similar the optimal region is to other regions. Disable with a value <= 0
        double texture = 0.15;
        /// If true the disparity is subpixel and the disparity image is F32, otherwise it's U8
        bool subpixel = true;
        /// If true it will save the disparity score for each pixel
        bool save_score = true;
        /// How the error is computed for each block
        DisparityError error_type = DisparityError::CENSUS;
        /// How the image border is handled
        BorderType border = REFLECT;
        /// Recomputes the score from scratch every N rows. Only used with floating point scores
        int32_t catastrophic_reset = 25;
        /// Used if the error type is Census
        ConfigDisparityError::Census config_census;

        /// Used if the error type is NCC
        ConfigDisparityError::NCC config_ncc;

        void checkValidity() const override {
            if (disparity_range < 1)
                throw std::invalid_argument("rangeDisparity < 1");
            if (border == NORMALIZED || border == SKIP)
                throw std::invalid_argument("Normalized and Skip are not supported");
        }
    };

    /// Calls visitor(java_name, field) for every field in ConfigDisparityBM, where java_name is the field's name in
    /// Java. Used to copy to and from Java and to check that the C++ and Java configurations have the same fields.
    /// Update this when a field is added.
    template<class Visitor>
    void visit_fields( ConfigDisparityBM& c, Visitor&& v ) {
        v("disparityMin", c.disparity_min);
        v("disparityRange", c.disparity_range);
        v("regionRadiusX", c.region_radius_x);
        v("regionRadiusY", c.region_radius_y);
        v("maxPerPixelError", c.max_per_pixel_error);
        v("validateRtoL", c.validate_r_to_l);
        v("texture", c.texture);
        v("subpixel", c.subpixel);
        v("saveScore", c.save_score);
        v("errorType", c.error_type);
        v("border", c.border);
        v("catastrophicReset", c.catastrophic_reset);
        v("configCensus", c.config_census);
        v("configNCC", c.config_ncc);
    }
}

#endif
