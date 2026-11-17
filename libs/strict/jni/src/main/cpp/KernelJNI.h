#ifndef BOOFCPP_KERNELJNI_H
#define BOOFCPP_KERNELJNI_H

#include <jni.h>
#include "boofcv/convolve_kernels.h"

// Returns a convolution corner from a java object. The kernel needs to be deleted when finished
boofcv::Kernel1D<boofcv::S32>* extractKernel1D_S32( JNIEnv *env, jobject& jkernel );
boofcv::Kernel1D<boofcv::F32>* extractKernel1D_F32( JNIEnv *env, jobject& jkernel );

boofcv::Kernel2D<boofcv::S32>* extractKernel2D_S32( JNIEnv *env, jobject& jkernel );
boofcv::Kernel2D<boofcv::F32>* extractKernel2D_F32( JNIEnv *env, jobject& jkernel );

#endif
