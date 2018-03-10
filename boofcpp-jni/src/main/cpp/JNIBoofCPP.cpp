#include "JNIBoofCPP.h"
#include <stdexcept>
#include <packed_sets.h>

using namespace boofcv;


WrapJGrowQueue_I32::WrapJGrowQueue_I32(JNIEnv *env, jobject jobj)
: env(env), jobj(jobj), data(nullptr), size(0)
{
    objClass = env->GetObjectClass(jobj);
    methodResize =  safe_GetMethodID(env, objClass, "resize", "(I)V");
    fid_data = safe_GetFieldID(env,objClass,"data","[I");
    fid_size = safe_GetFieldID(env,objClass,"size","I");

    this->array_object = (jarray)env->GetObjectField (jobj, fid_data);
    if( array_object == nullptr ) {
        throw std::runtime_error("array object is null");
    }

    this->array_length = env->GetArrayLength((jfloatArray)array_object);
    this->size = env->GetIntField(jobj,fid_size);
    this->data = nullptr;

//    printf("WrapJGrowQueue_I32 array_length=%d size=%d\n",this->array_length,this->size);
}

WrapJGrowQueue_I32::~WrapJGrowQueue_I32() {
    if( this->data != nullptr ) {
        throw std::runtime_error("You didn't release the critical array!");
    }
}

void WrapJGrowQueue_I32::criticalGet() {
    this->data = (jint *)env->GetPrimitiveArrayCritical(array_object, 0);
}

void WrapJGrowQueue_I32::criticalRelease() {
    this->env->ReleasePrimitiveArrayCritical(array_object, this->data, 0);
    this->data = nullptr;
}

void WrapJGrowQueue_I32::regularGet() {
    this->data = (jint *)env->GetIntArrayElements((jintArray)array_object, 0);
}

void WrapJGrowQueue_I32::regularRelease() {
    this->env->ReleaseIntArrayElements((jintArray)array_object, this->data, 0);
    this->data = nullptr;
}

void WrapJGrowQueue_I32::setTo( const std::vector<uint32_t>& input ) {
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

void WrapJGrowQueue_I32::resize( uint32_t desired ) {
    if( desired == this->size )
        return;
    if( this->data != nullptr ) {
        throw std::runtime_error("Unsafe resize. data must be null!");
    }

//    printf("ENTER resize. desired = %d\n",desired);

    env->CallVoidMethod(jobj,methodResize,(jint)desired);
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

void copy_into_java( JNIEnv *env,
                     const boofcv::PackedSet<boofcv::Point2D<boofcv::S32>>& src ,
                     jobject dst )
{
    jclass class_packed = env->GetObjectClass(dst);
    jfieldID field_blockLength = safe_GetFieldID(env,class_packed, "blockLength", "I");
    jfieldID field_blocks = safe_GetFieldID(env,class_packed, "blocks", "Lorg/ddogleg/struct/FastQueue;");
    jfieldID field_sets = safe_GetFieldID(env,class_packed, "sets", "Lorg/ddogleg/struct/FastQueue;");

    jint blockLength = env->GetIntField(dst,field_blockLength);

    if( blockLength != src._size_of_block*2 ) {
        printf("%d vs %d\n",blockLength,src._size_of_block);
        throw std::runtime_error("Block lengths do not match");
    }

    jobject object_blocks = env->GetObjectField (dst, field_blocks);
    jobject object_sets = env->GetObjectField (dst, field_sets);
    jclass class_fastqueue = env->GetObjectClass(object_blocks);

    jmethodID method_resize =  safe_GetMethodID(env, class_fastqueue, "resize", "(I)V");
    jmethodID method_get =  safe_GetMethodID(env, class_fastqueue, "get", "(I)Ljava/lang/Object;");

    env->CallVoidMethod(object_blocks,method_resize,(jint)src._number_of_blocks);
    env->CallVoidMethod(object_sets,method_resize,(jint)src.set_info.size());

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

    jclass class_blockindexlength = env->FindClass("boofcv/struct/BlockIndexLength");
    jmethodID method_bil_set =  safe_GetMethodID(env, class_blockindexlength, "set", "(III)V");

    for( uint32_t i = 0; i < src.set_info.size(); i++ ) {
        const PackedSetInfo& set_info = src.set_info[i];

        jobject jset = env->CallObjectMethod(object_sets,method_get,(jint)i);
        env->CallVoidMethod(jset,method_bil_set,set_info.offset*2,set_info.size,set_info.block);
        env->DeleteLocalRef(jset);
    }
}

jclass safe_FindClass( JNIEnv *env, const char* name )
{
    jclass theclass = env->FindClass(name);
    if( env->ExceptionCheck() || theclass == nullptr ) {
        env->ExceptionDescribe();
        throw "failed FindClass";
    }
    return theclass;
}

jfieldID safe_GetFieldID( JNIEnv *env, jclass& objClass, const char* name , const char* type)
{
    jfieldID fid = env->GetFieldID(objClass, name, type);
    if( env->ExceptionCheck() ) {
        env->ExceptionDescribe();
        throw "failed GetFieldID";
    }
    return fid;
}

jmethodID safe_GetMethodID( JNIEnv *env, jclass& objClass, const char* name , const char* type)
{
    jmethodID fid = env->GetMethodID(objClass, name, type);
    if( env->ExceptionCheck() ) {
        env->ExceptionDescribe();
        throw "failed GetMethodID";
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

JImageInfo extractInfoCritical( JNIEnv *env, jobject& jimage , const char*type ) {
    JImageInfo ret;

    jclass objClass = env->GetObjectClass(jimage);
    jfieldID fid;

    fid = env->GetFieldID(objClass, "data", type);
    if( env->ExceptionCheck() ) {
        env->ExceptionDescribe();
        throw std::runtime_error("Get field ID for data failed");
    }
    ret.jdata = env->GetObjectField (jimage, fid);
    if( ret.jdata == nullptr ) {
        throw std::runtime_error("data object is null");
    }

    ret.width = safe_GetInt(env,objClass,jimage, "width");
    ret.height = safe_GetInt(env,objClass,jimage, "height");
    ret.stride = safe_GetInt(env,objClass,jimage, "stride");
    ret.offset = safe_GetInt(env,objClass,jimage, "startIndex");

    // Get the elements (you probably have to fetch the length of the array as well
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

boofcv::ConfigLength extractConfigLength( JNIEnv *env, jobject& jconfig ) {
    boofcv::ConfigLength ret;

    jclass objClass = env->GetObjectClass(jconfig);
    ret.length = safe_GetDouble(env,objClass,jconfig, "length");
    ret.fraction = safe_GetDouble(env,objClass,jconfig, "fraction");

    return ret;
}

// make sure you free the kernel afterwards!
boofcv::Kernel1D<S32>* extractKernel1D_S32( JNIEnv *env, jobject& jkernel ) {
    jclass objClass = env->GetObjectClass(jkernel);
    jint width = safe_GetInt(env,objClass,jkernel, "width");
    jint offset = safe_GetInt(env,objClass,jkernel, "offset");

    jfieldID fid = env->GetFieldID(objClass, "data", "[I");
    jobject  jarray = env->GetObjectField (jkernel, fid);
    jint* data = env->GetIntArrayElements((jintArray)jarray, 0);
    jint length = env->GetArrayLength((jintArray)jarray);

    // copy the Java array into the kernel
    auto kernel1D = new boofcv::Kernel1D<S32>((uint32_t)width,(uint32_t)offset);
    kernel1D->data.set((S32*)data,(uint32_t)length);

    // release java objects
    env->ReleaseIntArrayElements((jintArray)jarray, data, 0);

    // return the results
    return kernel1D;
}

boofcv::Kernel1D<F32>* extractKernel1D_F32( JNIEnv *env, jobject& jkernel ) {
    jclass objClass = env->GetObjectClass(jkernel);
    jint width = safe_GetInt(env,objClass,jkernel, "width");
    jint offset = safe_GetInt(env,objClass,jkernel, "offset");

    jfieldID fid = env->GetFieldID(objClass, "data", "[F");
    jobject  jarray = env->GetObjectField (jkernel, fid);
    jfloat* data = env->GetFloatArrayElements((jfloatArray)jarray, 0);
    jint length = env->GetArrayLength((jfloatArray)jarray);

    // copy the Java array into the kernel
    auto kernel1D = new boofcv::Kernel1D<F32>((uint32_t)width,(uint32_t)offset);
    kernel1D->data.set((F32*)data,(uint32_t)length);

    // release java objects
    env->ReleaseFloatArrayElements((jfloatArray)jarray, data, 0);

    // return the results
    return kernel1D;
}

boofcv::Kernel2D<boofcv::S32>* extractKernel2D_S32( JNIEnv *env, jobject& jkernel ) {
    jclass objClass = env->GetObjectClass(jkernel);
    jint width = safe_GetInt(env,objClass,jkernel, "width");
    jint offset = safe_GetInt(env,objClass,jkernel, "offset");

    jfieldID fid = env->GetFieldID(objClass, "data", "[I");
    jobject  jarray = env->GetObjectField (jkernel, fid);
    jint* data = env->GetIntArrayElements((jintArray)jarray, 0);
    jint length = env->GetArrayLength((jintArray)jarray);

    // copy the Java array into the kernel
    auto kernel2D = new boofcv::Kernel2D<S32>((uint32_t)width,(uint32_t)offset);
    kernel2D->data.set((S32*)data,(uint32_t)length);

    // release java objects
    env->ReleaseIntArrayElements((jintArray)jarray, data, 0);

    // return the results
    return kernel2D;
}

boofcv::Kernel2D<boofcv::F32>* extractKernel2D_F32( JNIEnv *env, jobject& jkernel ) {
    jclass objClass = env->GetObjectClass(jkernel);
    jint width = safe_GetInt(env,objClass,jkernel, "width");
    jint offset = safe_GetInt(env,objClass,jkernel, "offset");

    jfieldID fid = env->GetFieldID(objClass, "data", "[F");
    jobject  jarray = env->GetObjectField (jkernel, fid);
    jfloat* data = env->GetFloatArrayElements((jfloatArray)jarray, 0);
    jint length = env->GetArrayLength((jfloatArray)jarray);

    // copy the Java array into the kernel
    auto kernel2D = new boofcv::Kernel2D<F32>((uint32_t)width,(uint32_t)offset);
    kernel2D->data.set((F32*)data,(uint32_t)length);

    // release java objects
    env->ReleaseFloatArrayElements((jfloatArray)jarray, data, 0);

    // return the results
    return kernel2D;
}
