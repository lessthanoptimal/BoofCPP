#include <jni.h>
#include "JNIBoofCPP.h"

extern "C" {

JNIEXPORT jstring JNICALL Java_org_boofcpp_BoofCPP_nativeBuildInfo( JNIEnv* env, jclass ) {
    return native_build_info(env);
}

}
