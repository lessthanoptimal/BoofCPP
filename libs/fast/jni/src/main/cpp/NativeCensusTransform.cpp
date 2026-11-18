#include "boofcv/fast/census_transform.h"
#include "CensusJNI.h"

using namespace boofcv;

extern "C" {
BOOFCPP_CENSUS_JNI(Java_org_boofcpp_fast_transform_NativeCensusTransform, boofcv::fast::CensusTransform, U8)
BOOFCPP_CENSUS_JNI(Java_org_boofcpp_fast_transform_NativeCensusTransform, boofcv::fast::CensusTransform, U16)
BOOFCPP_CENSUS_JNI(Java_org_boofcpp_fast_transform_NativeCensusTransform, boofcv::fast::CensusTransform, F32)
}
