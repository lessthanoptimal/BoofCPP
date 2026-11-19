#include "JNIBoofCPP.h"
#include <cstring>
#include <stdexcept>
#include <string>
#include "boofcv/packed_sets.h"

using namespace boofcv;

namespace {
    /// IDs of DogArray_I32, looked up once
    struct DogArrayI32 {
        jclass cls;
        jmethodID resize;
        jfieldID data, size;

        explicit DogArrayI32( JNIEnv* env ) : cls(global_class(env, "org/ddogleg/struct/DogArray_I32")) {
            resize = safe_GetMethodID(env, cls, "resize", "(I)Lorg/ddogleg/struct/DogArray_I32;");
            data = safe_GetFieldID(env, cls, "data", "[I");
            size = safe_GetFieldID(env, cls, "size", "I");
        }
    };
}

WrapJDogArray_I32::WrapJDogArray_I32(JNIEnv *env, jobject jobj)
: env(env), jobj(jobj), data(nullptr), size(0)
{
    static const DogArrayI32 ids(env);
    if( !env->IsInstanceOf(jobj, ids.cls) )
        throw std::runtime_error("Expected a DogArray_I32");
    objClass = ids.cls;
    methodResize = ids.resize;
    fid_data = ids.data;
    fid_size = ids.size;

    this->array_object = (jarray)env->GetObjectField (jobj, fid_data);
    if( array_object == nullptr ) {
        throw std::runtime_error("array object is null");
    }

    this->array_length = env->GetArrayLength((jfloatArray)array_object);
    this->size = env->GetIntField(jobj,fid_size);
    this->data = nullptr;

//    printf("WrapJDogArray_I32 array_length=%d size=%d\n",this->array_length,this->size);
}

WrapJDogArray_I32::~WrapJDogArray_I32() {
    if( this->data != nullptr ) {
        throw std::runtime_error("You didn't release the critical array!");
    }
}

void WrapJDogArray_I32::criticalGet() {
    this->data = (jint *)env->GetPrimitiveArrayCritical(array_object, 0);
}

void WrapJDogArray_I32::criticalRelease() {
    this->env->ReleasePrimitiveArrayCritical(array_object, this->data, 0);
    this->data = nullptr;
}

void WrapJDogArray_I32::regularGet() {
    this->data = (jint *)env->GetIntArrayElements((jintArray)array_object, 0);
}

void WrapJDogArray_I32::regularRelease() {
    this->env->ReleaseIntArrayElements((jintArray)array_object, this->data, 0);
    this->data = nullptr;
}

void WrapJDogArray_I32::setTo( const std::vector<uint32_t>& input ) {
    if( input.size() != this->size ) {
        resize((uint32_t)input.size());
    }

    bool aquired = false;
    if( this->data == nullptr ) {
        aquired = true;
        criticalGet();
    }

    for (int i = 0; i < this->size; ++i) {
        this->data[i] = input[i];
    }

    if( aquired ) {
        criticalRelease();
    }
}

void WrapJDogArray_I32::resize( uint32_t desired ) {
    if( desired == this->size )
        return;
    if( this->data != nullptr ) {
        throw std::runtime_error("Unsafe resize. data must be null!");
    }

//    printf("ENTER resize. desired = %d\n",desired);

    env->DeleteLocalRef(env->CallObjectMethod(jobj,methodResize,(jint)desired));
    this->size = desired;

    // See if the array has been resized
    if( desired > this->array_length ) {
        this->array_object = (jarray) env->GetObjectField(jobj, fid_data);
        if (array_object == nullptr) {
            throw std::runtime_error("array object is null");
        }
        this->array_length = env->GetArrayLength((jfloatArray)array_object);
    }
}

namespace {
    /// IDs of PackedSetsPoint2D_I32, the DogArrays inside it and BlockIndexLength, looked up once
    struct PackedSetsIds {
        jclass packed;
        jfieldID blockLength, blocks, sets;
        jmethodID resize, get, set_to;

        explicit PackedSetsIds( JNIEnv* env ) : packed(global_class(env, "boofcv/struct/PackedSetsPoint2D_I32")) {
            blockLength = safe_GetFieldID(env, packed, "blockLength", "I");
            blocks = safe_GetFieldID(env, packed, "blocks", "Lorg/ddogleg/struct/DogArray;");
            sets = safe_GetFieldID(env, packed, "sets", "Lorg/ddogleg/struct/DogArray;");
            jclass dog_array = global_class(env, "org/ddogleg/struct/DogArray");
            resize = safe_GetMethodID(env, dog_array, "resize", "(I)Lorg/ddogleg/struct/DogArray;");
            get = safe_GetMethodID(env, dog_array, "get", "(I)Ljava/lang/Object;");
            jclass block_index_length = global_class(env, "boofcv/struct/BlockIndexLength");
            set_to = safe_GetMethodID(env, block_index_length, "setTo", "(III)V");
        }
    };
}

void copy_into_java( JNIEnv *env,
                     const boofcv::PackedSet<boofcv::Point2D<boofcv::S32>>& src ,
                     jobject dst )
{
    static const PackedSetsIds ids(env);
    if( !env->IsInstanceOf(dst, ids.packed) )
        throw std::runtime_error("Expected a PackedSetsPoint2D_I32");
    jfieldID field_blockLength = ids.blockLength;
    jfieldID field_blocks = ids.blocks;
    jfieldID field_sets = ids.sets;

    jint blockLength = env->GetIntField(dst,field_blockLength);

    if( blockLength != src._size_of_block*2 ) {
        printf("%d vs %d\n",blockLength,src._size_of_block);
        throw std::runtime_error("Block lengths do not match");
    }

    jobject object_blocks = env->GetObjectField (dst, field_blocks);
    jobject object_sets = env->GetObjectField (dst, field_sets);
    jmethodID method_resize = ids.resize;
    jmethodID method_get = ids.get;

    env->DeleteLocalRef(env->CallObjectMethod(object_blocks,method_resize,(jint)src._number_of_blocks));
    env->DeleteLocalRef(env->CallObjectMethod(object_sets,method_resize,(jint)src.set_info.size()));

    for( uint32_t block_idx = 0; block_idx < src._number_of_blocks; block_idx++ ) {
        jobject the_array = env->CallObjectMethod(object_blocks,method_get,(jint)block_idx);

        auto array_src = (boofcv::Point2D<boofcv::S32>*)src.blocks[block_idx];
        auto array_dst = (jint *)env->GetPrimitiveArrayCritical((jarray)the_array, 0);
        for( uint32_t j = 0; j < src._size_of_block; j++ ) {
            array_dst[j*2  ] = array_src[j].x;
            array_dst[j*2+1] = array_src[j].y;
        }
        env->ReleasePrimitiveArrayCritical((jarray)the_array, array_dst, 0);
    }

    jmethodID method_bil_set = ids.set_to;

    for( uint32_t i = 0; i < src.set_info.size(); i++ ) {
        const PackedSetInfo& set_info = src.set_info[i];

        jobject jset = env->CallObjectMethod(object_sets,method_get,(jint)i);
        env->CallVoidMethod(jset,method_bil_set,set_info.offset*2,set_info.size,set_info.block);
        env->DeleteLocalRef(jset);
    }
}

// The safe_ functions clear the pending Java exception and throw a C++ exception that names what wasn't found

jclass safe_FindClass( JNIEnv *env, const char* name )
{
    jclass theclass = env->FindClass(name);
    if( env->ExceptionCheck() || theclass == nullptr ) {
        env->ExceptionClear();
        throw std::runtime_error(std::string("Java class not found: ") + name);
    }
    return theclass;
}

jfieldID safe_GetFieldID( JNIEnv *env, jclass& objClass, const char* name , const char* type)
{
    jfieldID fid = env->GetFieldID(objClass, name, type);
    if( env->ExceptionCheck() || fid == nullptr ) {
        env->ExceptionClear();
        throw std::runtime_error(std::string("Java field not found: ") + name + " " + type);
    }
    return fid;
}

jmethodID safe_GetMethodID( JNIEnv *env, jclass& objClass, const char* name , const char* type)
{
    jmethodID fid = env->GetMethodID(objClass, name, type);
    if( env->ExceptionCheck() || fid == nullptr ) {
        env->ExceptionClear();
        throw std::runtime_error(std::string("Java method not found: ") + name + " " + type);
    }
    return fid;
}

jclass global_class( JNIEnv* env, const char* name ) {
    jclass local = safe_FindClass(env, name);
    auto global = (jclass)env->NewGlobalRef(local);
    env->DeleteLocalRef(local);
    return global;
}

jfieldID class_field_id( JNIEnv* env, jobject obj, const char* name, const char* type ) {
    jclass local = env->GetObjectClass(obj);
    jfieldID fid = safe_GetFieldID(env, local, name, type);
    env->NewGlobalRef(local);
    env->DeleteLocalRef(local);
    return fid;
}

const NativeBaseFields& NativeBaseFields::get( JNIEnv* env ) {
    static const NativeBaseFields fields = [env] {
        jclass cls = global_class(env, "org/boofcpp/NativeBase");
        NativeBaseFields f{};
        f.nativePtr = safe_GetFieldID(env, cls, "nativePtr", "J");
        f.isInteger = safe_GetFieldID(env, cls, "isInteger", "Z");
        f.inputBits = safe_GetFieldID(env, cls, "inputBits", "I");
        f.outputBits = safe_GetFieldID(env, cls, "outputBits", "I");
        return f;
    }();
    return fields;
}

void throw_java_exception( JNIEnv* env ) {
    // A Java exception that's already pending is more informative
    if (env->ExceptionCheck())
        return;
    try {
        throw;
    } catch (const std::invalid_argument& e) {
        env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"), e.what());
    } catch (const std::exception& e) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), e.what());
    } catch (const char* message) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), message);
    } catch (...) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), "Unknown C++ exception");
    }
}

int32_t java_enum_index( JNIEnv* env, jobject jenum, const char* const* names, size_t count ) {
    if (jenum == nullptr)
        throw std::invalid_argument("enum is null");
    jclass enum_class = safe_FindClass(env, "java/lang/Enum");
    jmethodID mid_name = safe_GetMethodID(env, enum_class, "name", "()Ljava/lang/String;");
    auto jname = (jstring)env->CallObjectMethod(jenum, mid_name);
    const char* name = env->GetStringUTFChars(jname, nullptr);
    std::string value(name);
    env->ReleaseStringUTFChars(jname, name);
    for (size_t i = 0; i < count; i++) {
        if (value == names[i])
            return (int32_t)i;
    }
    throw std::invalid_argument("Unknown enum value " + value);
}

jobject java_enum_value( JNIEnv* env, const char* signature, const char* name ) {
    // signature is "Lpackage/Class;"
    std::string class_name(signature + 1, std::strlen(signature) - 2);
    jclass enum_class = safe_FindClass(env, class_name.c_str());
    jfieldID fid = env->GetStaticFieldID(enum_class, name, signature);
    if (env->ExceptionCheck() || fid == nullptr) {
        env->ExceptionClear();
        throw std::invalid_argument(std::string("No enum value ") + name + " in " + class_name);
    }
    return env->GetStaticObjectField(enum_class, fid);
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
        throw std::runtime_error("Get field ID for data failed");
    }
    ret.jdata = env->GetObjectField (jimage, fid);
    if( ret.jdata == nullptr ) {
        throw std::runtime_error("data object is null");
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
        throw std::runtime_error("Get field ID for data failed");
    }
    ret.jdata = env->GetObjectField (jimage, fid);
    if( ret.jdata == nullptr ) {
        throw std::runtime_error("data object is null");
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

namespace {
    /// Field IDs of ImageBase, looked up once
    struct ImageBaseFields {
        jfieldID width, height, stride, startIndex;

        explicit ImageBaseFields( JNIEnv* env ) {
            jclass cls = global_class(env, "boofcv/struct/image/ImageBase");
            width = safe_GetFieldID(env, cls, "width", "I");
            height = safe_GetFieldID(env, cls, "height", "I");
            stride = safe_GetFieldID(env, cls, "stride", "I");
            startIndex = safe_GetFieldID(env, cls, "startIndex", "I");
        }
    };

    /// The 'data' field of the gray image class that declares it, looked up once
    struct GrayData {
        jclass cls;
        jfieldID data;

        GrayData( JNIEnv* env, const char* name, const char* type ) : cls(global_class(env, name)) {
            data = safe_GetFieldID(env, cls, "data", type);
        }
    };

    /// The gray image class with a data array of JNI signature 'type', or null if there's none
    const GrayData* gray_data( JNIEnv* env, const char* type ) {
        switch( type[1] ) {
            case 'B': { static const GrayData d(env, "boofcv/struct/image/GrayI8", "[B"); return &d; }
            case 'S': { static const GrayData d(env, "boofcv/struct/image/GrayI16", "[S"); return &d; }
            case 'I': { static const GrayData d(env, "boofcv/struct/image/GrayS32", "[I"); return &d; }
            case 'J': { static const GrayData d(env, "boofcv/struct/image/GrayS64", "[J"); return &d; }
            case 'F': { static const GrayData d(env, "boofcv/struct/image/GrayF32", "[F"); return &d; }
            case 'D': { static const GrayData d(env, "boofcv/struct/image/GrayF64", "[D"); return &d; }
            default: return nullptr;
        }
    }
}

JImageInfo extractInfoCritical( JNIEnv *env, jobject& jimage , const char*type ) {
    static const ImageBaseFields base(env);
    JImageInfo ret;

    // Images that aren't gray, e.g. interleaved, have their own data field
    const GrayData* gray = gray_data(env, type);
    jfieldID fid;
    if( gray != nullptr && env->IsInstanceOf(jimage, gray->cls) ) {
        fid = gray->data;
    } else {
        jclass objClass = env->GetObjectClass(jimage);
        fid = safe_GetFieldID(env, objClass, "data", type);
    }
    ret.jdata = env->GetObjectField (jimage, fid);
    if( ret.jdata == nullptr ) {
        throw std::runtime_error("data object is null");
    }

    ret.width = env->GetIntField(jimage, base.width);
    ret.height = env->GetIntField(jimage, base.height);
    ret.stride = env->GetIntField(jimage, base.stride);
    ret.offset = env->GetIntField(jimage, base.startIndex);
    ret.dataLength = env->GetArrayLength((jarray)ret.jdata);

    return ret;
}

JImageInfo extractInfoCriticalU8( JNIEnv *env, jobject& jimage ) {
    return extractInfoCritical(env,jimage,"[B");
}

JImageInfo extractInfoCriticalS16( JNIEnv *env, jobject& jimage ) {
    return extractInfoCritical(env,jimage,"[S");
}

JImageInfo extractInfoCriticalS32( JNIEnv *env, jobject& jimage ) {
    return extractInfoCritical(env,jimage,"[I");
}

JImageInfo extractInfoCriticalF32( JNIEnv *env, jobject& jimage ) {
    return extractInfoCritical(env,jimage,"[F");
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

ImageAndInfo<boofcv::Gray<boofcv::U8>,JImageInfo> wrapCriticalGrayU8( JNIEnv *env, jobject& jimage ) {
    ImageAndInfo<boofcv::Gray<boofcv::U8>,JImageInfo> output;

    JImageInfo inputInfo = extractInfoCriticalU8(env,jimage);

    output.info = inputInfo;
    output.image = Gray<U8>((U8*)nullptr,(uint32_t)inputInfo.dataLength,
                            (uint32_t)inputInfo.width,(uint32_t)inputInfo.height,
                            (uint32_t)inputInfo.offset,(uint32_t)inputInfo.stride);

    return output;
}

ImageAndInfo<boofcv::Gray<boofcv::S16>,JImageInfo> wrapCriticalGrayS16( JNIEnv *env, jobject& jimage ) {
    ImageAndInfo<boofcv::Gray<boofcv::S16>,JImageInfo> output;

    JImageInfo inputInfo = extractInfoCriticalS16(env,jimage);

    output.info = inputInfo;
    output.image = Gray<S16>((S16*)nullptr,(uint32_t)inputInfo.dataLength,
                            (uint32_t)inputInfo.width,(uint32_t)inputInfo.height,
                            (uint32_t)inputInfo.offset,(uint32_t)inputInfo.stride);

    return output;
}

ImageAndInfo<boofcv::Gray<boofcv::S32>,JImageInfo> wrapCriticalGrayS32( JNIEnv *env, jobject& jimage ) {
    ImageAndInfo<boofcv::Gray<boofcv::S32>,JImageInfo> output;

    JImageInfo inputInfo = extractInfoCriticalS32(env,jimage);

    output.info = inputInfo;
    output.image = Gray<S32>((S32*)nullptr,(uint32_t)inputInfo.dataLength,
                            (uint32_t)inputInfo.width,(uint32_t)inputInfo.height,
                            (uint32_t)inputInfo.offset,(uint32_t)inputInfo.stride);

    return output;
}

ImageAndInfo<boofcv::Gray<boofcv::F32>,JImageInfo> wrapCriticalGrayF32( JNIEnv *env, jobject& jimage ) {
    ImageAndInfo<boofcv::Gray<boofcv::F32>,JImageInfo> output;

    JImageInfo inputInfo = extractInfoCriticalF32(env,jimage);

    output.info = inputInfo;
    output.image = Gray<F32>((F32*)nullptr,(uint32_t)inputInfo.dataLength,
                             (uint32_t)inputInfo.width,(uint32_t)inputInfo.height,
                             (uint32_t)inputInfo.offset,(uint32_t)inputInfo.stride);

    return output;
}

// Set by CMake. Builds which don't set them, e.g. Android's, report them as unknown
#ifndef BOOFCPP_BUILD_TYPE
#define BOOFCPP_BUILD_TYPE "unknown"
#endif
#ifndef BOOFCPP_CXX_FLAGS
#define BOOFCPP_CXX_FLAGS "unknown"
#endif

jstring native_build_info( JNIEnv* env ) {
#if defined(__clang__)
    const char* compiler = "Clang " __clang_version__;
#elif defined(__GNUC__)
    const char* compiler = "GCC " __VERSION__;
#elif defined(_MSC_VER)
    const char* compiler = "MSVC";
#else
    const char* compiler = "unknown";
#endif
    std::string info = std::string(compiler) + ", " + BOOFCPP_BUILD_TYPE + ", " + BOOFCPP_CXX_FLAGS;
    return env->NewStringUTF(info.c_str());
}
