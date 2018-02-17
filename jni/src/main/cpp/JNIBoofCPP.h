#ifndef BOOFCPP_JNIBOOFCPP_H
#define BOOFCPP_JNIBOOFCPP_H

#include <jni.h>
#include <config_types.h>
#include "image_types.h"
#include "base_types.h"
#include "convolve.h"

class WrapJGrowQueue_I32 {
public:
    JNIEnv *env;
    jobject jobj;
    jarray array_object;
    jint* data;
    jint size;
    jint array_length;

    jfieldID fid_data;
    jfieldID fid_size;

    WrapJGrowQueue_I32(JNIEnv *env, jobject jobj);

    ~WrapJGrowQueue_I32();

    void setTo( const std::vector<uint32_t>& input );

    void resize( uint32_t desired );
};

struct JImageInfo {
    jobject jdata;


    jsize dataLength;

    jint width,height;
    jint offset,stride;

    JImageInfo() {
        jdata = nullptr;
        dataLength = 0;
        width=height=offset=stride=0;
    }
};

struct JImageCritical : public JImageInfo {
    void *data;

    JImageCritical() {
        data = nullptr;
    }
};

struct JImageInfoU8 : public JImageInfo {
    jbyte *data;

    JImageInfoU8() {
        data = nullptr;
    }
};

struct JImageInfoF32 : public JImageInfo {
    jfloat *data;

    JImageInfoF32() {
        data = nullptr;
    }
};

template<class Image,class Info>
struct ImageAndInfo {
    Image image;
    Info info;
};

jfieldID safe_GetFieldID( JNIEnv *env, jclass& objClass, const char* name , const char* type);

JImageInfoU8 extractInfoU8( JNIEnv *env, jobject& jimage );
JImageInfoF32 extractInfoF32( JNIEnv *env, jobject& jimage );

JImageCritical extractInfoCriticalU8( JNIEnv *env, jobject& jimage );
JImageCritical extractInfoCriticalS32( JNIEnv *env, jobject& jimage );
JImageCritical extractInfoCriticalF32( JNIEnv *env, jobject& jimage );

ImageAndInfo<boofcv::Gray<boofcv::U8>,JImageInfoU8> wrapGrayU8( JNIEnv *env, jobject& jimage );
ImageAndInfo<boofcv::Gray<boofcv::F32>,JImageInfoF32> wrapGrayF32( JNIEnv *env, jobject& jimage );

ImageAndInfo<boofcv::Gray<boofcv::U8>,JImageCritical> wrapCriticalGrayU8( JNIEnv *env, jobject& jimage );
ImageAndInfo<boofcv::Gray<boofcv::S32>,JImageCritical> wrapCriticalGrayS32( JNIEnv *env, jobject& jimage );
ImageAndInfo<boofcv::Gray<boofcv::F32>,JImageCritical> wrapCriticalGrayF32( JNIEnv *env, jobject& jimage );

boofcv::ConfigLength extractConfigLength( JNIEnv *env, jobject& jconfig );

// Returns a convolution corner from a java object. The kernel needs to be deleted when finished
boofcv::Kernel1D<boofcv::S32>* extractKernel1D_S32( JNIEnv *env, jobject& jkernel );
boofcv::Kernel1D<boofcv::F32>* extractKernel1D_F32( JNIEnv *env, jobject& jkernel );

#endif