#ifndef BOOFCPP_JNIBOOFCPP_H
#define BOOFCPP_JNIBOOFCPP_H

#include <jni.h>

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

JImageInfoU8 extractInfoU8( JNIEnv *env, jobject& jimage );

JImageInfoF32 extractInfoF32( JNIEnv *env, jobject& jimage );

#endif