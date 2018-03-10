#include <jni.h>
#include <convolve.h>
#include "JNIBoofCPP.h"

#include <time.h>

#include <stdio.h>

using namespace boofcv;

extern "C" {

BorderType borderJavaToCpp( int type ) {
    switch( type ) {
        case 1:
            return BorderType::EXTENDED;

        case 3:
            return BorderType::REFLECT;

        case 4:
            return BorderType::WRAP;

        case 5:
            return BorderType::ZERO;
    }
    throw std::invalid_argument("Unknown Java border type");
}

JNIEXPORT void JNICALL Java_org_boofcpp_convolve_NativeConvolveImage_nativeHorizontal(
    JNIEnv *env, jobject obj, jobject jkernel, jobject jinput, jobject joutput, jint jborderType ) {

    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "isInteger", "Z");
    jboolean isInteger = env->GetBooleanField(obj, fid);
    jint inputBits = safe_GetInt(env,objClass,obj,"inputBits");
    jint outputBits = safe_GetInt(env,objClass,obj,"outputBits");

    BorderType borderType = borderJavaToCpp((int)jborderType);

    if( isInteger ) {
        if( inputBits != 8 )
            throw std::runtime_error("Can only handle 8-bit input integer images currently");
        boofcv::Kernel1D<S32>* kernel = extractKernel1D_S32(env,jkernel);
        std::shared_ptr<ImageBorder<U8>> border = FactoryImageBorder::create_SB<U8>(borderType);

        ImageAndInfo<Gray<U8>,JImageInfo> input = wrapCriticalGrayU8(env,jinput);
        border.get()->setImage(input.image);

        if( outputBits == 16 ) {
            ImageAndInfo<Gray<S16>,JImageInfo> output = wrapCriticalGrayS16(env,joutput);
            input.image.data = (U8*)env->GetPrimitiveArrayCritical((jarray)input.info.jdata, 0);
            output.image.data = (S16*)env->GetPrimitiveArrayCritical((jarray)output.info.jdata, 0);

            // Trying to figure out why this runs slower than a C++ application
            // JNI overhead appears to be negligible based on the printed times below
//            clock_t t;
//            t = clock();
            ConvolveImage::horizontal(*kernel, *border.get(),output.image);
//            t = clock() - t;
//            printf("horizontal time %f kernel %d\n",(float)1000*t/CLOCKS_PER_SEC,kernel->width);

            env->ReleasePrimitiveArrayCritical((jarray)output.info.jdata, output.image.data, 0);
        } else if( outputBits == 32 ) {
            ImageAndInfo<Gray<S32>,JImageInfo> output = wrapCriticalGrayS32(env,joutput);
            input.image.data = (U8*)env->GetPrimitiveArrayCritical((jarray)input.info.jdata, 0);
            output.image.data = (S32*)env->GetPrimitiveArrayCritical((jarray)output.info.jdata, 0);

            ConvolveImage::horizontal(*kernel, *border.get(),output.image);

            env->ReleasePrimitiveArrayCritical((jarray)output.info.jdata, output.image.data, 0);
        } else {
            throw std::runtime_error("Can only handle 16-bit or 32-bit integer output images currently");
        }

        env->ReleasePrimitiveArrayCritical((jarray)input.info.jdata, input.image.data, JNI_ABORT);
        delete kernel;
    } else {
        if( inputBits != 32 || outputBits != 32 ) {
            throw std::runtime_error("Can only handle 32-bit floats currently");
        }
        ImageAndInfo<Gray<F32>,JImageInfo> input = wrapCriticalGrayF32(env,jinput);
        ImageAndInfo<Gray<F32>,JImageInfo> output = wrapCriticalGrayF32(env,joutput);
        boofcv::Kernel1D<F32>* kernel = extractKernel1D_F32(env,jkernel);
        std::shared_ptr<ImageBorder<F32>> border = FactoryImageBorder::create_SB<F32>(borderType);
        border.get()->setImage(input.image);

        input.image.data = (F32*)env->GetPrimitiveArrayCritical((jarray)input.info.jdata, 0);
        output.image.data = (F32*)env->GetPrimitiveArrayCritical((jarray)output.info.jdata, 0);

        ConvolveImage::horizontal(*kernel, *border.get(),output.image);

        env->ReleasePrimitiveArrayCritical((jarray)input.info.jdata, input.image.data, JNI_ABORT);
        env->ReleasePrimitiveArrayCritical((jarray)output.info.jdata, output.image.data, 0);
        delete kernel;
    }
}

JNIEXPORT void JNICALL Java_org_boofcpp_convolve_NativeConvolveImage_nativeVertical(
    JNIEnv *env, jobject obj, jobject jkernel, jobject jinput, jobject joutput, jint jborderType ) {

    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "isInteger", "Z");
    jboolean isInteger = env->GetBooleanField(obj, fid);
    jint inputBits = safe_GetInt(env,objClass,obj,"inputBits");
    jint outputBits = safe_GetInt(env,objClass,obj,"outputBits");

    BorderType borderType = borderJavaToCpp((int)jborderType);

    if( isInteger ) {
        if( inputBits != 8 )
            throw std::runtime_error("Can only handle 8-bit input integer images currently");
        boofcv::Kernel1D<S32>* kernel = extractKernel1D_S32(env,jkernel);
        std::shared_ptr<ImageBorder<U8>> border = FactoryImageBorder::create_SB<U8>(borderType);

        ImageAndInfo<Gray<U8>,JImageInfo> input = wrapCriticalGrayU8(env,jinput);
        border.get()->setImage(input.image);

        if( outputBits == 16 ) {
            ImageAndInfo<Gray<S16>,JImageInfo> output = wrapCriticalGrayS16(env,joutput);
            input.image.data = (U8*)env->GetPrimitiveArrayCritical((jarray)input.info.jdata, 0);
            output.image.data = (S16*)env->GetPrimitiveArrayCritical((jarray)output.info.jdata, 0);

            ConvolveImage::vertical(*kernel, *border.get(),output.image);

            env->ReleasePrimitiveArrayCritical((jarray)output.info.jdata, output.image.data, 0);
        } else if( outputBits == 32 ) {
            ImageAndInfo<Gray<S32>,JImageInfo> output = wrapCriticalGrayS32(env,joutput);
            input.image.data = (U8*)env->GetPrimitiveArrayCritical((jarray)input.info.jdata, 0);
            output.image.data = (S32*)env->GetPrimitiveArrayCritical((jarray)output.info.jdata, 0);

            ConvolveImage::vertical(*kernel, *border.get(),output.image);

            env->ReleasePrimitiveArrayCritical((jarray)output.info.jdata, output.image.data, 0);
        } else {
            throw std::runtime_error("Can only handle 16-bit or 32-bit integer output currently");
        }

        env->ReleasePrimitiveArrayCritical((jarray)input.info.jdata, input.image.data, JNI_ABORT);
        delete kernel;
    } else {
        if( inputBits != 32 || outputBits != 32 ) {
            throw std::runtime_error("Can only handle 32-bit floats currently");
        }
        ImageAndInfo<Gray<F32>,JImageInfo> input = wrapCriticalGrayF32(env,jinput);
        ImageAndInfo<Gray<F32>,JImageInfo> output = wrapCriticalGrayF32(env,joutput);
        boofcv::Kernel1D<F32>* kernel = extractKernel1D_F32(env,jkernel);
        std::shared_ptr<ImageBorder<F32>> border = FactoryImageBorder::create_SB<F32>(borderType);
        border.get()->setImage(input.image);

        input.image.data = (F32*)env->GetPrimitiveArrayCritical((jarray)input.info.jdata, 0);
        output.image.data = (F32*)env->GetPrimitiveArrayCritical((jarray)output.info.jdata, 0);

        ConvolveImage::vertical(*kernel, *border.get(),output.image);

        env->ReleasePrimitiveArrayCritical((jarray)input.info.jdata, input.image.data, JNI_ABORT);
        env->ReleasePrimitiveArrayCritical((jarray)output.info.jdata, output.image.data, 0);
        delete kernel;
    }
}

JNIEXPORT void JNICALL Java_org_boofcpp_convolve_NativeConvolveImage_nativeConvolve(
    JNIEnv *env, jobject obj, jobject jkernel, jobject jinput, jobject joutput, jint jborderType ) {

   jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "isInteger", "Z");
    jboolean isInteger = env->GetBooleanField(obj, fid);
    jint inputBits = safe_GetInt(env,objClass,obj,"inputBits");
    jint outputBits = safe_GetInt(env,objClass,obj,"outputBits");

    BorderType borderType = borderJavaToCpp((int)jborderType);

    if( isInteger ) {
        if( inputBits != 8 )
            throw std::runtime_error("Can only handle 8-bit input integer images currently");
        boofcv::Kernel2D<S32>* kernel = extractKernel2D_S32(env,jkernel);
        std::shared_ptr<ImageBorder<U8>> border = FactoryImageBorder::create_SB<U8>(borderType);

        ImageAndInfo<Gray<U8>,JImageInfo> input = wrapCriticalGrayU8(env,jinput);
        border.get()->setImage(input.image);

        if( outputBits == 16 ) {
            ImageAndInfo<Gray<S16>,JImageInfo> output = wrapCriticalGrayS16(env,joutput);
            input.image.data = (U8*)env->GetPrimitiveArrayCritical((jarray)input.info.jdata, 0);
            output.image.data = (S16*)env->GetPrimitiveArrayCritical((jarray)output.info.jdata, 0);

            ConvolveImage::convolve(*kernel, *border.get(),output.image);

            env->ReleasePrimitiveArrayCritical((jarray)output.info.jdata, output.image.data, 0);
        } else if( outputBits == 32 ) {
            ImageAndInfo<Gray<S32>,JImageInfo> output = wrapCriticalGrayS32(env,joutput);
            input.image.data = (U8*)env->GetPrimitiveArrayCritical((jarray)input.info.jdata, 0);
            output.image.data = (S32*)env->GetPrimitiveArrayCritical((jarray)output.info.jdata, 0);

            ConvolveImage::convolve(*kernel, *border.get(),output.image);

            env->ReleasePrimitiveArrayCritical((jarray)output.info.jdata, output.image.data, 0);
        } else {
            throw std::runtime_error("Can only handle 16-bit or 32-bit integer output images currently");
        }

        env->ReleasePrimitiveArrayCritical((jarray)input.info.jdata, input.image.data, JNI_ABORT);
        delete kernel;
    } else {
        if( inputBits != 32 || outputBits != 32 ) {
            throw std::runtime_error("Can only handle 32-bit floats currently");
        }
        ImageAndInfo<Gray<F32>,JImageInfo> input = wrapCriticalGrayF32(env,jinput);
        ImageAndInfo<Gray<F32>,JImageInfo> output = wrapCriticalGrayF32(env,joutput);
        boofcv::Kernel2D<F32>* kernel = extractKernel2D_F32(env,jkernel);
        std::shared_ptr<ImageBorder<F32>> border = FactoryImageBorder::create_SB<F32>(borderType);
        border.get()->setImage(input.image);

        input.image.data = (F32*)env->GetPrimitiveArrayCritical((jarray)input.info.jdata, 0);
        output.image.data = (F32*)env->GetPrimitiveArrayCritical((jarray)output.info.jdata, 0);

        ConvolveImage::convolve(*kernel, *border.get(),output.image);

        env->ReleasePrimitiveArrayCritical((jarray)input.info.jdata, input.image.data, JNI_ABORT);
        env->ReleasePrimitiveArrayCritical((jarray)output.info.jdata, output.image.data, 0);
        delete kernel;
    }
}


}