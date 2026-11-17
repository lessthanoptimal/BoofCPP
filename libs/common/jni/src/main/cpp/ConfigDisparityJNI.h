#ifndef BOOFCPP_CONFIG_DISPARITY_JNI_H
#define BOOFCPP_CONFIG_DISPARITY_JNI_H

#include <jni.h>
#include "boofcv/config_disparity.h"
#include "JNIBoofCPP.h"

// Java types of the disparity enums and nested configurations, used by config_from_java() and config_to_java()
// in JNIBoofCPP.h

template<>
struct JavaEnum<boofcv::DisparityError> {
    static constexpr const char* signature = "Lboofcv/factory/disparity/DisparityError;";
    static constexpr const char* const* names = boofcv::DISPARITY_ERROR_NAMES;
    static constexpr size_t count = std::size(boofcv::DISPARITY_ERROR_NAMES);
};

template<>
struct JavaEnum<boofcv::CensusVariants> {
    static constexpr const char* signature = "Lboofcv/factory/transform/census/CensusVariants;";
    static constexpr const char* const* names = boofcv::CENSUS_VARIANTS_NAMES;
    static constexpr size_t count = std::size(boofcv::CENSUS_VARIANTS_NAMES);
};

template<>
struct JavaConfig<boofcv::ConfigDisparityError::Census> {
    static constexpr const char* signature = "Lboofcv/factory/disparity/ConfigDisparityError$Census;";
};

template<>
struct JavaConfig<boofcv::ConfigDisparityError::NCC> {
    static constexpr const char* signature = "Lboofcv/factory/disparity/ConfigDisparityError$NCC;";
};

#endif
