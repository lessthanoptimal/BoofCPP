#include <jni.h>
#include <binary_ops.h>

using namespace boofcv;

struct JImageInfo {
    jbyteArray *jdata;

    jbyte *data;
    jsize dataLength;

    jint width,height;
    jint offset,stride;

    JImageInfo() {
        jdata = nullptr;
        data = nullptr;
        dataLength = 0;
        width=height=offset=stride=0;
    }
};

JImageInfo extractInfo( JNIEnv *env, jobject& jimage ) {
    JImageInfo ret;

    jclass objClass = env->GetObjectClass(jimage);
    jfieldID fid = env->GetFieldID(objClass, "data", "[B");
    jobject mvdata = env->GetObjectField (objClass, fid);
    ret.jdata = reinterpret_cast<jbyteArray*>(&mvdata);

    // Get the elements (you probably have to fetch the length of the array as well
    ret.data = env->GetByteArrayElements(*ret.jdata, NULL);
    ret.dataLength = env->GetArrayLength(*ret.jdata);

    fid = env->GetFieldID(objClass, "width", "I");
    ret.width = env->GetIntField(jimage, fid);

    fid = env->GetFieldID(objClass, "height", "I");
    ret.height = env->GetIntField(jimage, fid);

    fid = env->GetFieldID(objClass, "stride", "I");
    ret.stride = env->GetIntField(jimage, fid);

    fid = env->GetFieldID(objClass, "startIndex", "I");
    ret.offset = env->GetIntField(jimage, fid);

    return ret;
}

extern "C" {
JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeGlobalFixed_1U8_nativeinit(
    JNIEnv *env, jobject obj, jdouble threshold, jboolean down) {

    jclass objClass = env->GetObjectClass(obj);

    GlobalFixedBinaryFilter<U8> *alg = new GlobalFixedBinaryFilter<U8>((U8)threshold,(bool)down);

    jfieldID fid = env->GetFieldID(objClass, "nativePtr", "L");
    env->SetLongField(obj, fid, (jlong)alg);
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeGlobalFixed_1U8_nativedestroy(JNIEnv *env, jobject obj) {

    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "nativePtr", "L");
    jlong nativePtr = env->GetLongField(obj, fid);

    delete (GlobalFixedBinaryFilter<U8> *)nativePtr;
}

JNIEXPORT void JNICALL Java_org_boofcpp_threshold_NativeGlobalFixed_1U8_process
    (JNIEnv *env, jobject obj, jobject jinput, jobject joutput) {

    // Get the pointer to the thresholding algorithm
    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "nativePtr", "L");
    auto * alg = (GlobalFixedBinaryFilter<U8> *)env->GetLongField(obj, fid);

    JImageInfo inputInfo = extractInfo(env,jinput);
    JImageInfo outputInfo = extractInfo(env,joutput);

    Gray<U8> input((U8*)inputInfo.data,(uint32_t)inputInfo.dataLength,
                   (uint32_t)inputInfo.width,(uint32_t)inputInfo.height,
                   (uint32_t)inputInfo.offset,(uint32_t)inputInfo.stride);

    Gray<U8> output((U8*)outputInfo.data,(uint32_t)outputInfo.dataLength,
                   (uint32_t)outputInfo.width,(uint32_t)outputInfo.height,
                   (uint32_t)outputInfo.offset,(uint32_t)outputInfo.stride);

    try {
        alg->process(input, output);
    } catch( ... ) {
        // TODO do something here
    }

    // Release the arrays
    env->ReleaseByteArrayElements(*inputInfo.jdata, inputInfo.data, 0);
    env->ReleaseByteArrayElements(*outputInfo.jdata, outputInfo.data, 0);
}

}