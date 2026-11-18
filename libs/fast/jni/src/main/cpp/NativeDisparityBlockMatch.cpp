#include "boofcv/fast/stereo_disparity.h"
#include "DisparityBlockMatchJNI.h"

extern "C" {
BOOFCPP_DISPARITY_BLOCK_MATCH_JNI(Java_org_boofcpp_fast_disparity_NativeDisparityBlockMatch,
                                  boofcv::fast::FactoryStereoDisparity)
}
