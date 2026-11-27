#include "boofcv/census_transform.h"
#include "boofcv/census_transform_mt.h"
#include "CensusJNI.h"

using namespace boofcv;

extern "C" {
BOOFCPP_CENSUS_JNI(Java_org_boofcpp_transform_NativeCensusTransform, CensusTransform, CensusTransform_MT, U8)
BOOFCPP_CENSUS_JNI(Java_org_boofcpp_transform_NativeCensusTransform, CensusTransform, CensusTransform_MT, U16)
BOOFCPP_CENSUS_JNI(Java_org_boofcpp_transform_NativeCensusTransform, CensusTransform, CensusTransform_MT, F32)
}
