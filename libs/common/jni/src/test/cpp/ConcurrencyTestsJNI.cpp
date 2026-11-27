// Test-only JNI functions of NativeToggleCall, built into JNIBoofCPPCommonTest
#include <jni.h>
#include "boofcv/concurrency.h"

using namespace boofcv;

extern "C" {
/// A concurrent loop with more than one thread, otherwise nothing, like a hook's switch
JNIEXPORT void JNICALL Java_org_boofcpp_testing_NativeToggleCall_call( JNIEnv*, jclass ) {
    if (BoofConcurrency::max_threads() > 1)
        BoofConcurrency::loop_blocks(0, 10).run([]( uint32_t, uint32_t ) {});
}
}
