#include <jni.h>
#include <binary_ops.h>
#include <contour.h>
#include "JNIBoofCPP.h"

#include <stdio.h>

using namespace boofcv;

extern "C" {

JNIEXPORT void JNICALL Java_org_boofcpp_contour_NativeChang2004_native_1init
        (JNIEnv *env, jobject obj)
{
    jclass objClass = env->GetObjectClass(obj);

    LinearContourLabelChang2004* ptr = new LinearContourLabelChang2004(ConnectRule::FOUR);
    ptr->maxContourSize = 0x7fffffff; // Java's max is a signed int

    jfieldID fid = safe_GetFieldID(env,objClass, "nativePtr", "J");
    env->SetLongField(obj, fid, (jlong)ptr);
}

JNIEXPORT void JNICALL Java_org_boofcpp_contour_NativeChang2004_native_1destroy
        (JNIEnv *env, jobject obj)
{
    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "nativePtr", "J");
    jlong nativePtr = env->GetLongField(obj, fid);

    delete (LinearContourLabelChang2004*) nativePtr;

    env->SetLongField(obj, fid, 0);
}

JNIEXPORT void JNICALL Java_org_boofcpp_contour_NativeChang2004_native_1process
        (JNIEnv *env, jobject obj, jobject jbinary, jobject jlabel)
{
    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = safe_GetFieldID(env,objClass, "nativePtr", "J");
    LinearContourLabelChang2004* contour = (LinearContourLabelChang2004*)env->GetLongField(obj, fid);

    ImageAndInfo<Gray<U8>,JImageInfo> input = wrapCriticalGrayU8(env,jbinary);
    ImageAndInfo<Gray<S32>,JImageInfo> label = wrapCriticalGrayS32(env,jlabel);

    input.image.data = (U8*)env->GetPrimitiveArrayCritical((jarray)input.info.jdata, 0);
    label.image.data = (S32*)env->GetPrimitiveArrayCritical((jarray)label.info.jdata, 0);

    contour->process(input.image, label.image);

    env->ReleasePrimitiveArrayCritical((jarray)input.info.jdata, input.image.data, 0);
    env->ReleasePrimitiveArrayCritical((jarray)label.info.jdata, label.image.data, 0);

    jfieldID fidPoints = safe_GetFieldID(env, objClass, "storagePoints", "Lorg/ddogleg/struct/GrowQueue_I32;");
    jobject jpoints = env->GetObjectField(obj,fidPoints);
    WrapJGrowQueue_I32 grow_queue(env,jpoints);

    jmethodID midAdd =  safe_GetMethodID(env, objClass, "addContour", "(II)V");

    for( uint32_t i = 0; i < contour->contours.size(); i++ ) {
        ContourPacked &p = contour->contours.at(i);

        // internal contour ids will be grabbed from storagePoints
        grow_queue.setTo(p.internalIndexes);
        env->CallVoidMethod(obj,midAdd,(jint)p.id,(jint)p.externalIndex);
    }

    jfieldID field_packed = safe_GetFieldID(env, objClass, "packedPoints", "Lboofcv/struct/PackedSetsPoint2D_I32;");
    jobject jpacked = env->GetObjectField(obj,field_packed);
    copy_into_java(env,contour->packedPoints,jpacked);

    // TODO storageContours
}

JNIEXPORT void JNICALL Java_org_boofcpp_contour_NativeChang2004_setSaveInnerContour
        (JNIEnv *env, jobject obj, jboolean enabled)
{
    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = safe_GetFieldID(env, objClass, "nativePtr", "J");
    auto * contour = (LinearContourLabelChang2004*)env->GetLongField(obj, fid);

    contour->saveInternalContours = enabled;
}

JNIEXPORT jboolean JNICALL Java_org_boofcpp_contour_NativeChang2004_isSaveInternalContours
        (JNIEnv *env, jobject obj)
{
    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = safe_GetFieldID(env,objClass, "nativePtr", "J");
    auto * contour = (LinearContourLabelChang2004*)env->GetLongField(obj, fid);

    return (jboolean)contour->saveInternalContours;
}

JNIEXPORT void JNICALL Java_org_boofcpp_contour_NativeChang2004_setMinContour
        (JNIEnv *env, jobject obj, jint size)
{
    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = safe_GetFieldID(env, objClass, "nativePtr", "J");
    auto* contour = (LinearContourLabelChang2004*)env->GetLongField(obj, fid);

    contour->minContourSize = static_cast<uint32_t>(size);
}

JNIEXPORT jint JNICALL Java_org_boofcpp_contour_NativeChang2004_getMinContour
        (JNIEnv *env, jobject obj)
{
    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = safe_GetFieldID(env,objClass, "nativePtr", "J");
    auto* contour = (LinearContourLabelChang2004*)env->GetLongField(obj, fid);

    return static_cast<jint>(contour->minContourSize);
}

JNIEXPORT void JNICALL Java_org_boofcpp_contour_NativeChang2004_setMaxContour
        (JNIEnv *env, jobject obj, jint size)
{
    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = safe_GetFieldID(env,objClass, "nativePtr", "J");
    auto* contour = (LinearContourLabelChang2004*)env->GetLongField(obj, fid);

    contour->maxContourSize = static_cast<uint32_t>(size);
}

JNIEXPORT jint JNICALL Java_org_boofcpp_contour_NativeChang2004_getMaxContour
        (JNIEnv *env, jobject obj)
{
    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = safe_GetFieldID(env, objClass, "nativePtr", "J");
    auto* contour = (LinearContourLabelChang2004*)env->GetLongField(obj, fid);

    return static_cast<jint>(contour->maxContourSize);
}

JNIEXPORT void JNICALL Java_org_boofcpp_contour_NativeChang2004_setConnectRule
        (JNIEnv *env, jobject obj, jobject jrule )
{
    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = safe_GetFieldID(env, objClass, "nativePtr", "J");
    auto* contour = (LinearContourLabelChang2004*)env->GetLongField(obj, fid);

    jclass enumClass = env->GetObjectClass(jrule);
    jmethodID mid =  safe_GetMethodID(env, enumClass, "ordinal", "()I");
    jint rule_oridinal = env->CallIntMethod(jrule,mid);

    switch( rule_oridinal ) {
        case 0:
            contour->setConnectRule(ConnectRule::FOUR);
            break;

        case 1:
            contour->setConnectRule(ConnectRule::EIGHT);
            break;

        default:
            throw "Unknown connect rule";
    }

}

JNIEXPORT jobject JNICALL Java_org_boofcpp_contour_NativeChang2004_getConnectRule
        (JNIEnv *env, jobject obj)
{
    jclass objClass = env->GetObjectClass(obj);
    jfieldID fid = env->GetFieldID(objClass, "nativePtr", "J");
    auto* contour = (LinearContourLabelChang2004*)env->GetLongField(obj, fid);

    jclass enumClass = safe_FindClass(env,"boofcv/struct/ConnectRule");

    jfieldID theRule;

    if( contour->getConnectRule() == ConnectRule::FOUR )
        theRule = env->GetStaticFieldID(enumClass , "FOUR", "Lboofcv/struct/ConnectRule;");
    else
        theRule = env->GetStaticFieldID(enumClass , "EIGHT", "Lboofcv/struct/ConnectRule;");

    if( env->ExceptionCheck() || theRule == nullptr ) {
        env->ExceptionDescribe();
        throw "Could lookup static field";
    }

    jobject ret = env->GetStaticObjectField(enumClass,theRule);
    if( env->ExceptionCheck() || ret == nullptr ) {
        env->ExceptionDescribe();
        throw "Could lookup static field";
    }
    return ret;
}
}