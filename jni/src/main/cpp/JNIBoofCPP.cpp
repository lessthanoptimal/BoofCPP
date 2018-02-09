#include "JNIBoofCPP.h"

using namespace boofcv;

jfieldID safe_GetFieldID( JNIEnv *env, jclass& objClass, const char* name , const char* type)
{
    jfieldID fid = env->GetFieldID(objClass, name, type);
    if( env->ExceptionCheck() ) {
        env->ExceptionDescribe();
        throw "filed ID failed";
    }
    return fid;
}

jdouble safe_GetDouble( JNIEnv *env, jclass& objClass, jobject& obj, const char* name ) {
    jfieldID fid = safe_GetFieldID(env,objClass,name,"D");
    return env->GetDoubleField(obj, fid);
}

jint safe_GetInt( JNIEnv *env, jclass& objClass, jobject& obj, const char* name ) {
    jfieldID fid = safe_GetFieldID(env,objClass,name,"I");
    return env->GetIntField(obj, fid);
}

JImageInfoU8 extractInfoU8( JNIEnv *env, jobject& jimage ) {
    JImageInfoU8 ret;

    jclass objClass = env->GetObjectClass(jimage);
    jfieldID fid = env->GetFieldID(objClass, "data", "[B");
    if( env->ExceptionCheck() ) {
        env->ExceptionDescribe();
        throw "Get field ID for data failed";
    }
    ret.jdata = env->GetObjectField (jimage, fid);
    if( ret.jdata == nullptr ) {
        throw "data object is null";
    }

    // Get the elements (you probably have to fetch the length of the array as well
    ret.data = env->GetByteArrayElements((jbyteArray)ret.jdata, 0);
    ret.dataLength = env->GetArrayLength((jbyteArray)ret.jdata);

    ret.width = safe_GetInt(env,objClass,jimage, "width");
    ret.height = safe_GetInt(env,objClass,jimage, "height");
    ret.stride = safe_GetInt(env,objClass,jimage, "stride");
    ret.offset = safe_GetInt(env,objClass,jimage, "startIndex");

    return ret;
}

JImageInfoF32 extractInfoF32( JNIEnv *env, jobject& jimage ) {
    JImageInfoF32 ret;

    jclass objClass = env->GetObjectClass(jimage);
    jfieldID fid = env->GetFieldID(objClass, "data", "[F");
    if( env->ExceptionCheck() ) {
        env->ExceptionDescribe();
        throw "Get field ID for data failed";
    }
    ret.jdata = env->GetObjectField (jimage, fid);
    if( ret.jdata == nullptr ) {
        throw "data object is null";
    }

    // Get the elements (you probably have to fetch the length of the array as well
    ret.data = env->GetFloatArrayElements((jfloatArray)ret.jdata, 0);
    ret.dataLength = env->GetArrayLength((jbyteArray)ret.jdata);

    ret.width = safe_GetInt(env,objClass,jimage, "width");
    ret.height = safe_GetInt(env,objClass,jimage, "height");
    ret.stride = safe_GetInt(env,objClass,jimage, "stride");
    ret.offset = safe_GetInt(env,objClass,jimage, "startIndex");

    return ret;
}

ImageAndInfo<boofcv::Gray<boofcv::U8>,JImageInfoU8> wrapGrayU8( JNIEnv *env, jobject& jimage ) {
    ImageAndInfo<boofcv::Gray<boofcv::U8>,JImageInfoU8> output;

    JImageInfoU8 inputInfo = extractInfoU8(env,jimage);

    output.info = inputInfo;
    output.image = Gray<U8>((U8*)inputInfo.data,(uint32_t)inputInfo.dataLength,
                            (uint32_t)inputInfo.width,(uint32_t)inputInfo.height,
                            (uint32_t)inputInfo.offset,(uint32_t)inputInfo.stride);

    return output;
}

ImageAndInfo<boofcv::Gray<boofcv::F32>,JImageInfoF32> wrapGrayF32( JNIEnv *env, jobject& jimage ) {
    ImageAndInfo<boofcv::Gray<boofcv::F32>,JImageInfoF32> output;
    JImageInfoF32 inputInfo = extractInfoF32(env,jimage);

    output.info = inputInfo;
    output.image = Gray<F32>((F32*)inputInfo.data,(uint32_t)inputInfo.dataLength,
                             (uint32_t)inputInfo.width,(uint32_t)inputInfo.height,
                             (uint32_t)inputInfo.offset,(uint32_t)inputInfo.stride);

    return output;
}

boofcv::ConfigLength extractConfigLength( JNIEnv *env, jobject& jconfig ) {
    boofcv::ConfigLength ret;

    jclass objClass = env->GetObjectClass(jconfig);
    ret.length = safe_GetDouble(env,objClass,jconfig, "length");
    ret.fraction = safe_GetDouble(env,objClass,jconfig, "fraction");

    return ret;
}