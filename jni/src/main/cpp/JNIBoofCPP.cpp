#include "JNIBoofCPP.h"

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