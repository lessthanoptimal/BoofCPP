#include "JNIBoofCPP.h"

using namespace boofcv;


WrapJGrowQueue_I32::WrapJGrowQueue_I32(JNIEnv *env, jobject jobj)
: env(env), jobj(jobj), data(nullptr), size(0)
{
    jclass objClass = env->GetObjectClass(jobj);
    fid_data = safe_GetFieldID(env,objClass,"data","[I");
    fid_size = safe_GetFieldID(env,objClass,"size","I");

    this->array_object = (jarray)env->GetObjectField (jobj, fid_data);
    if( array_object == nullptr ) {
        throw "array object is null";
    }

    this->array_length = env->GetArrayLength((jfloatArray)array_object);
    this->size = env->GetIntField(jobj,fid_size);
    this->data = (jint *)env->GetPrimitiveArrayCritical(array_object, 0);
}

WrapJGrowQueue_I32::~WrapJGrowQueue_I32() {
    if( this->data != nullptr ) {
        this->env->ReleasePrimitiveArrayCritical(array_object, this->data, 0);
        this->data = nullptr;
    }
}

void WrapJGrowQueue_I32::setTo( const std::vector<uint32_t>& input ) {
    if( input.size() != this->size ) {
        resize((uint32_t)input.size());
    }
    for (int i = 0; i < this->size; ++i) {
        this->data[i] = input[i];
    }
}

void WrapJGrowQueue_I32::resize( uint32_t desired ) {
    if( desired == this->size )
        return;

    jclass objClass = env->GetObjectClass(jobj);
    jmethodID mid =  env->GetMethodID(objClass, "resize", "(V)I");
    env->CallObjectMethod(jobj,mid,(jint)desired);
    this->size = desired;

    // See if the array has been resized
    if( desired > this->array_length ) {
        // Array could have been reallocated
        this->env->ReleasePrimitiveArrayCritical(array_object, this->data, 0);
        this->array_object = (jarray) env->GetObjectField(jobj, fid_data);
        if (array_object == nullptr) {
            throw "array object is null";
        }

        this->data = (jint *) env->GetPrimitiveArrayCritical(array_object, 0);
        this->array_length = env->GetArrayLength((jfloatArray)array_object);
    }
}

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

JImageCritical extractInfoCritical( JNIEnv *env, jobject& jimage , const char*type ) {
    JImageCritical ret;

    jclass objClass = env->GetObjectClass(jimage);
    jfieldID fid;

    fid = env->GetFieldID(objClass, "data", type);
    if( env->ExceptionCheck() ) {
        env->ExceptionDescribe();
        throw "Get field ID for data failed";
    }
    ret.jdata = env->GetObjectField (jimage, fid);
    if( ret.jdata == nullptr ) {
        throw "data object is null";
    }

    ret.width = safe_GetInt(env,objClass,jimage, "width");
    ret.height = safe_GetInt(env,objClass,jimage, "height");
    ret.stride = safe_GetInt(env,objClass,jimage, "stride");
    ret.offset = safe_GetInt(env,objClass,jimage, "startIndex");

    // Get the elements (you probably have to fetch the length of the array as well
    ret.dataLength = env->GetArrayLength((jarray)ret.jdata);
    ret.data = env->GetPrimitiveArrayCritical((jarray)ret.jdata, 0);

    return ret;
}

JImageCritical extractInfoCriticalU8( JNIEnv *env, jobject& jimage ) {
    return extractInfoCritical(env,jimage,"[B");
}

JImageCritical extractInfoCriticalS32( JNIEnv *env, jobject& jimage ) {
    return extractInfoCritical(env,jimage,"[I");
}

JImageCritical extractInfoCriticalF32( JNIEnv *env, jobject& jimage ) {
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

ImageAndInfo<boofcv::Gray<boofcv::U8>,JImageCritical> wrapCriticalGrayU8( JNIEnv *env, jobject& jimage ) {
    ImageAndInfo<boofcv::Gray<boofcv::U8>,JImageCritical> output;

    JImageCritical inputInfo = extractInfoCriticalU8(env,jimage);

    output.info = inputInfo;
    output.image = Gray<U8>((U8*)inputInfo.data,(uint32_t)inputInfo.dataLength,
                            (uint32_t)inputInfo.width,(uint32_t)inputInfo.height,
                            (uint32_t)inputInfo.offset,(uint32_t)inputInfo.stride);

    return output;
}

ImageAndInfo<boofcv::Gray<boofcv::S32>,JImageCritical> wrapCriticalGrayS32( JNIEnv *env, jobject& jimage ) {
    ImageAndInfo<boofcv::Gray<boofcv::S32>,JImageCritical> output;

    JImageCritical inputInfo = extractInfoCriticalS32(env,jimage);

    output.info = inputInfo;
    output.image = Gray<S32>((S32*)inputInfo.data,(uint32_t)inputInfo.dataLength,
                            (uint32_t)inputInfo.width,(uint32_t)inputInfo.height,
                            (uint32_t)inputInfo.offset,(uint32_t)inputInfo.stride);

    return output;
}

ImageAndInfo<boofcv::Gray<boofcv::F32>,JImageCritical> wrapCriticalGrayF32( JNIEnv *env, jobject& jimage ) {
    ImageAndInfo<boofcv::Gray<boofcv::F32>,JImageCritical> output;

    JImageCritical inputInfo = extractInfoCriticalF32(env,jimage);

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