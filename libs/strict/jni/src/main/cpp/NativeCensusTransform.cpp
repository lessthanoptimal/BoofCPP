#include "boofcv/census_transform.h"
#include "CensusJNI.h"

using namespace boofcv;

extern "C" {
BOOFCPP_CENSUS_JNI(Java_org_boofcpp_transform_NativeCensusTransform, boofcv::CensusTransform, U8)
BOOFCPP_CENSUS_JNI(Java_org_boofcpp_transform_NativeCensusTransform, boofcv::CensusTransform, U16)
BOOFCPP_CENSUS_JNI(Java_org_boofcpp_transform_NativeCensusTransform, boofcv::CensusTransform, F32)
}
