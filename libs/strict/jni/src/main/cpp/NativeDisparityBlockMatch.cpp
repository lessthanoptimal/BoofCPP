#include "boofcv/stereo_disparity.h"
#include "DisparityBlockMatchJNI.h"

extern "C" {
BOOFCPP_DISPARITY_BLOCK_MATCH_JNI(Java_org_boofcpp_disparity_NativeDisparityBlockMatch, boofcv::FactoryStereoDisparity)
}
