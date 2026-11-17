#ifndef BOOFCPP_JNIBOOFCPP_H
#define BOOFCPP_JNIBOOFCPP_H

#include <jni.h>
#include "boofcv/config_types.h"
#include "boofcv/image_types.h"
#include "boofcv/base_types.h"
#include "boofcv/geometry_types.h"
#include "boofcv/packed_sets.h"
#include "boofcv/image_border.h"
#include <iterator>
#include <stdexcept>
#include <type_traits>

class WrapJDogArray_I32 {
public:
    JNIEnv *env;
    jobject jobj;
    jclass objClass;
    jarray array_object;
    jint* data;
    jint size;
    jint array_length;

    jfieldID fid_data;
    jfieldID fid_size;

    jmethodID methodResize;

    WrapJDogArray_I32(JNIEnv *env, jobject jobj);

    ~WrapJDogArray_I32();

    // there can't be any JNI call's between these two
    void criticalGet();
    void criticalRelease();

    void regularGet();
    void regularRelease();

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

/// Acquires a primitive array with GetPrimitiveArrayCritical and releases it when it goes out of scope, e.g. when an
/// exception is thrown. No other JNI calls are allowed while it's held.
class CriticalArray {
public:
    void* data;

    CriticalArray( JNIEnv* env, jobject array, jint release_mode ) :
            env(env), array((jarray)array), release_mode(release_mode) {
        data = env->GetPrimitiveArrayCritical(this->array, nullptr);
        if (data == nullptr)
            throw std::runtime_error("GetPrimitiveArrayCritical failed");
    }

    ~CriticalArray() { env->ReleasePrimitiveArrayCritical(array, data, release_mode); }

    CriticalArray( const CriticalArray& ) = delete;
    CriticalArray& operator=( const CriticalArray& ) = delete;

private:
    JNIEnv* env;
    jarray array;
    jint release_mode;
};

/// Throws a Java exception for the C++ exception currently being handled. Call it from a catch block, after all
/// critical arrays have been released. Does nothing if a Java exception is already pending.
void throw_java_exception( JNIEnv* env );

/// Calls func() for a JNI function and returns its result. A C++ exception becomes a Java exception, and a default
/// value is returned. Critical arrays must be held by CriticalArray inside func, so they're released first
template<class Func>
auto jni_call( JNIEnv* env, Func func ) -> decltype(func()) {
    try {
        return func();
    } catch (...) {
        throw_java_exception(env);
        if constexpr (!std::is_void_v<decltype(func())>)
            return {};
    }
}

/// Index of the Java enum's name() in 'names'. Throws if it isn't there
int32_t java_enum_index( JNIEnv* env, jobject jenum, const char* const* names, size_t count );

/// Java enum value with the given name. 'signature' is the enum's type, e.g. "Lboofcv/struct/border/BorderType;"
jobject java_enum_value( JNIEnv* env, const char* signature, const char* name );

/// The Java type of a C++ enum. Specialize it with the Java signature and the names of the values, in order
template<class E>
struct JavaEnum;

template<>
struct JavaEnum<boofcv::BorderType> {
    static constexpr const char* signature = "Lboofcv/struct/border/BorderType;";
    static constexpr const char* const* names = boofcv::BORDER_TYPE_NAMES;
    static constexpr size_t count = std::size(boofcv::BORDER_TYPE_NAMES);
};

/// Converts a Java enum into the C++ enum with the same name
template<class E>
E enum_from_java( JNIEnv* env, jobject jenum ) {
    return (E)java_enum_index(env, jenum, JavaEnum<E>::names, JavaEnum<E>::count);
}

/// Converts a C++ enum into the Java enum with the same name
template<class E>
jobject enum_to_java( JNIEnv* env, E value ) {
    return java_enum_value(env, JavaEnum<E>::signature, JavaEnum<E>::names[(size_t)value]);
}

void copy_into_java( JNIEnv *env, const boofcv::PackedSet<boofcv::Point2D<boofcv::S32>>& src , jobject dst );

jclass safe_FindClass( JNIEnv *env, const char* name );
jfieldID safe_GetFieldID( JNIEnv *env, jclass& objClass, const char* name , const char* type);
jmethodID safe_GetMethodID( JNIEnv *env, jclass& objClass, const char* name , const char* type);
jdouble safe_GetDouble( JNIEnv *env, jclass& objClass, jobject& obj, const char* name );
jint safe_GetInt( JNIEnv *env, jclass& objClass, jobject& obj, const char* name );

/// The C++ object stored in the Java object's 'nativePtr' field
template<class T>
T* get_native_ptr( JNIEnv* env, jobject obj ) {
    jclass cls = env->GetObjectClass(obj);
    return (T*)env->GetLongField(obj, safe_GetFieldID(env, cls, "nativePtr", "J"));
}

/// Java signature of a configuration that's a field inside another configuration. Specialize it when needed
template<class C>
struct JavaConfig;

/// Copies a Java configuration into C++. Works for any configuration with a visit_fields() function. Each field is
/// looked up by its Java name, so a field that's missing in Java throws instead of being silently skipped.
template<class C>
void config_from_java( JNIEnv* env, jobject jconfig, C& config ) {
    jclass cls = env->GetObjectClass(jconfig);
    visit_fields(config, [&]( const char* name, auto& field ) {
        using T = std::decay_t<decltype(field)>;
        if constexpr (std::is_same<T, int32_t>::value) {
            field = env->GetIntField(jconfig, safe_GetFieldID(env, cls, name, "I"));
        } else if constexpr (std::is_same<T, double>::value) {
            field = env->GetDoubleField(jconfig, safe_GetFieldID(env, cls, name, "D"));
        } else if constexpr (std::is_same<T, bool>::value) {
            field = env->GetBooleanField(jconfig, safe_GetFieldID(env, cls, name, "Z"));
        } else if constexpr (std::is_enum<T>::value) {
            jobject value = env->GetObjectField(jconfig, safe_GetFieldID(env, cls, name, JavaEnum<T>::signature));
            field = enum_from_java<T>(env, value);
        } else if constexpr (std::is_base_of<boofcv::Configuration, T>::value) {
            jobject nested = env->GetObjectField(jconfig, safe_GetFieldID(env, cls, name, JavaConfig<T>::signature));
            if (nested == nullptr)
                throw std::invalid_argument(std::string("Configuration is null: ") + name);
            config_from_java(env, nested, field);
        } else {
            static_assert(!sizeof(T), "Unsupported field type");
        }
    });
}

template<class C>
C config_from_java( JNIEnv* env, jobject jconfig ) {
    C config;
    config_from_java(env, jconfig, config);
    return config;
}

/// Copies a C++ configuration into an existing Java configuration. See config_from_java()
template<class C>
void config_to_java( JNIEnv* env, C config, jobject jconfig ) {
    jclass cls = env->GetObjectClass(jconfig);
    visit_fields(config, [&]( const char* name, auto& field ) {
        using T = std::decay_t<decltype(field)>;
        if constexpr (std::is_same<T, int32_t>::value) {
            env->SetIntField(jconfig, safe_GetFieldID(env, cls, name, "I"), field);
        } else if constexpr (std::is_same<T, double>::value) {
            env->SetDoubleField(jconfig, safe_GetFieldID(env, cls, name, "D"), field);
        } else if constexpr (std::is_same<T, bool>::value) {
            env->SetBooleanField(jconfig, safe_GetFieldID(env, cls, name, "Z"), (jboolean)field);
        } else if constexpr (std::is_enum<T>::value) {
            env->SetObjectField(jconfig, safe_GetFieldID(env, cls, name, JavaEnum<T>::signature),
                                enum_to_java(env, field));
        } else if constexpr (std::is_base_of<boofcv::Configuration, T>::value) {
            // Java configurations always have their nested configurations, so write into the existing one
            jobject nested = env->GetObjectField(jconfig, safe_GetFieldID(env, cls, name, JavaConfig<T>::signature));
            if (nested == nullptr)
                throw std::invalid_argument(std::string("Configuration is null: ") + name);
            config_to_java(env, field, nested);
        } else {
            static_assert(!sizeof(T), "Unsupported field type");
        }
    });
}

JImageInfoU8 extractInfoU8( JNIEnv *env, jobject& jimage );
JImageInfoF32 extractInfoF32( JNIEnv *env, jobject& jimage );

JImageInfo extractInfoCriticalU8( JNIEnv *env, jobject& jimage );
JImageInfo extractInfoCriticalS16( JNIEnv *env, jobject& jimage );
JImageInfo extractInfoCriticalS32( JNIEnv *env, jobject& jimage );
JImageInfo extractInfoCriticalF32( JNIEnv *env, jobject& jimage );

ImageAndInfo<boofcv::Gray<boofcv::U8>,JImageInfoU8> wrapGrayU8( JNIEnv *env, jobject& jimage );
ImageAndInfo<boofcv::Gray<boofcv::F32>,JImageInfoF32> wrapGrayF32( JNIEnv *env, jobject& jimage );

ImageAndInfo<boofcv::Gray<boofcv::U8>,JImageInfo> wrapCriticalGrayU8( JNIEnv *env, jobject& jimage );
ImageAndInfo<boofcv::Gray<boofcv::S16>,JImageInfo> wrapCriticalGrayS16( JNIEnv *env, jobject& jimage );
ImageAndInfo<boofcv::Gray<boofcv::S32>,JImageInfo> wrapCriticalGrayS32( JNIEnv *env, jobject& jimage );
ImageAndInfo<boofcv::Gray<boofcv::F32>,JImageInfo> wrapCriticalGrayF32( JNIEnv *env, jobject& jimage );

/// Width, height, stride, offset and the data array of a Java image. 'type' is the JNI signature of the data array,
/// e.g. "[B"
JImageInfo extractInfoCritical( JNIEnv *env, jobject& jimage , const char*type );

/// Wraps a Java gray image of the same pixel type. Its data pointer is null until it's set from a CriticalArray
template<class E>
ImageAndInfo<boofcv::Gray<E>,JImageInfo> wrapCriticalGray( JNIEnv *env, jobject jimage ) {
    const char* type;
    if constexpr (std::is_same<E, boofcv::U8>::value || std::is_same<E, boofcv::S8>::value)
        type = "[B";
    else if constexpr (std::is_same<E, boofcv::U16>::value || std::is_same<E, boofcv::S16>::value)
        type = "[S";
    else if constexpr (std::is_same<E, boofcv::S32>::value)
        type = "[I";
    else if constexpr (std::is_same<E, boofcv::F32>::value)
        type = "[F";
    else if constexpr (std::is_same<E, boofcv::F64>::value)
        type = "[D";
    else
        static_assert(!std::is_same<E, E>::value, "Unsupported pixel type");

    ImageAndInfo<boofcv::Gray<E>,JImageInfo> output;
    output.info = extractInfoCritical(env, jimage, type);
    output.image = boofcv::Gray<E>((E*)nullptr, (uint32_t)output.info.dataLength,
                                   (uint32_t)output.info.width, (uint32_t)output.info.height,
                                   (uint32_t)output.info.offset, (uint32_t)output.info.stride);
    return output;
}


#endif