// Test-only JNI functions of NativeConfigs, built into JNIBoofCPPCommonTest. They make sure the C++ and Java
// configurations have the same fields, enum values, and defaults.
#include <jni.h>
#include <string>
#include <vector>
#include "JNIBoofCPP.h"
#include "ConfigDisparityJNI.h"

using namespace boofcv;

namespace {
    jobjectArray to_java_strings( JNIEnv* env, const std::vector<std::string>& values ) {
        jobjectArray ret = env->NewObjectArray((jsize)values.size(), env->FindClass("java/lang/String"), nullptr);
        for (size_t i = 0; i < values.size(); i++)
            env->SetObjectArrayElement(ret, (jsize)i, env->NewStringUTF(values[i].c_str()));
        return ret;
    }

    template<class C>
    jobjectArray field_names( JNIEnv* env ) {
        C config;
        std::vector<std::string> names;
        visit_fields(config, [&]( const char* name, auto& ) { names.emplace_back(name); });
        return to_java_strings(env, names);
    }

    template<class E>
    jobjectArray enum_names( JNIEnv* env ) {
        return to_java_strings(env, {JavaEnum<E>::names, JavaEnum<E>::names + JavaEnum<E>::count});
    }

    template<class C>
    void round_trip( JNIEnv* env, jobject jsrc, jobject jdst ) {
        try {
            config_to_java(env, config_from_java<C>(env, jsrc), jdst);
        } catch (...) {
            throw_java_exception(env);
        }
    }

    template<class C>
    void defaults( JNIEnv* env, jobject jdst ) {
        try {
            config_to_java(env, C(), jdst);
        } catch (...) {
            throw_java_exception(env);
        }
    }
}

extern "C" {
JNIEXPORT jobjectArray JNICALL Java_org_boofcpp_NativeConfigs_fieldNamesBM( JNIEnv* env, jclass ) {
    return field_names<ConfigDisparityBM>(env);
}

JNIEXPORT jobjectArray JNICALL Java_org_boofcpp_NativeConfigs_fieldNamesCensus( JNIEnv* env, jclass ) {
    return field_names<ConfigDisparityError::Census>(env);
}

JNIEXPORT jobjectArray JNICALL Java_org_boofcpp_NativeConfigs_fieldNamesNCC( JNIEnv* env, jclass ) {
    return field_names<ConfigDisparityError::NCC>(env);
}

JNIEXPORT jobjectArray JNICALL Java_org_boofcpp_NativeConfigs_fieldNamesLength( JNIEnv* env, jclass ) {
    return field_names<ConfigLength>(env);
}

JNIEXPORT jobjectArray JNICALL Java_org_boofcpp_NativeConfigs_disparityErrorNames( JNIEnv* env, jclass ) {
    return enum_names<DisparityError>(env);
}

JNIEXPORT jobjectArray JNICALL Java_org_boofcpp_NativeConfigs_borderTypeNames( JNIEnv* env, jclass ) {
    return enum_names<BorderType>(env);
}

JNIEXPORT jobjectArray JNICALL Java_org_boofcpp_NativeConfigs_censusVariantsNames( JNIEnv* env, jclass ) {
    return enum_names<CensusVariants>(env);
}

JNIEXPORT void JNICALL Java_org_boofcpp_NativeConfigs_roundTripBM( JNIEnv* env, jclass, jobject jsrc, jobject jdst ) {
    round_trip<ConfigDisparityBM>(env, jsrc, jdst);
}

JNIEXPORT void JNICALL Java_org_boofcpp_NativeConfigs_roundTripCensus(
        JNIEnv* env, jclass, jobject jsrc, jobject jdst ) {
    round_trip<ConfigDisparityError::Census>(env, jsrc, jdst);
}

JNIEXPORT void JNICALL Java_org_boofcpp_NativeConfigs_roundTripNCC( JNIEnv* env, jclass, jobject jsrc, jobject jdst ) {
    round_trip<ConfigDisparityError::NCC>(env, jsrc, jdst);
}

JNIEXPORT void JNICALL Java_org_boofcpp_NativeConfigs_roundTripLength( JNIEnv* env, jclass, jobject jsrc, jobject jdst ) {
    round_trip<ConfigLength>(env, jsrc, jdst);
}

JNIEXPORT void JNICALL Java_org_boofcpp_NativeConfigs_defaultsBM( JNIEnv* env, jclass, jobject jdst ) {
    defaults<ConfigDisparityBM>(env, jdst);
}

JNIEXPORT void JNICALL Java_org_boofcpp_NativeConfigs_defaultsCensus( JNIEnv* env, jclass, jobject jdst ) {
    defaults<ConfigDisparityError::Census>(env, jdst);
}

JNIEXPORT void JNICALL Java_org_boofcpp_NativeConfigs_defaultsNCC( JNIEnv* env, jclass, jobject jdst ) {
    defaults<ConfigDisparityError::NCC>(env, jdst);
}

JNIEXPORT void JNICALL Java_org_boofcpp_NativeConfigs_defaultsLength( JNIEnv* env, jclass, jobject jdst ) {
    defaults<ConfigLength>(env, jdst);
}
}
