#include <jni.h>
#include "boofcv/binary_ops.h"
#include "boofcv/contour.h"
#include "JNIBoofCPP.h"

using namespace boofcv;

extern "C" {

JNIEXPORT void JNICALL Java_org_boofcpp_contour_NativeChang2004_native_1init
        (JNIEnv *env, jobject obj)
{
    jni_call(env, [&] {
        jclass objClass = env->GetObjectClass(obj);
        jfieldID fid = safe_GetFieldID(env,objClass, "nativePtr", "J");

        LinearContourLabelChang2004* ptr = new LinearContourLabelChang2004(ConnectRule::FOUR);
        ptr->maxContourSize = 0x7fffffff; // Java's max is a signed int
        env->SetLongField(obj, fid, (jlong)ptr);
    });
}

JNIEXPORT void JNICALL Java_org_boofcpp_contour_NativeChang2004_native_1destroy
        (JNIEnv *env, jobject obj)
{
    jni_call(env, [&] {
        jclass objClass = env->GetObjectClass(obj);
        jfieldID fid = env->GetFieldID(objClass, "nativePtr", "J");
        jlong nativePtr = env->GetLongField(obj, fid);

        delete (LinearContourLabelChang2004*) nativePtr;

        env->SetLongField(obj, fid, 0);
    });
}

JNIEXPORT void JNICALL Java_org_boofcpp_contour_NativeChang2004_native_1process
        (JNIEnv *env, jobject obj, jobject jbinary, jobject jlabel)
{
    jni_call(env, [&] {
        jclass objClass = env->GetObjectClass(obj);
        jfieldID fid = safe_GetFieldID(env,objClass, "nativePtr", "J");
        LinearContourLabelChang2004* contour = (LinearContourLabelChang2004*)env->GetLongField(obj, fid);

        ImageAndInfo<Gray<U8>,JImageInfo> input = wrapCriticalGrayU8(env,jbinary);
        ImageAndInfo<Gray<S32>,JImageInfo> label = wrapCriticalGrayS32(env,jlabel);

        {
            CriticalArray input_data(env, input.info.jdata, JNI_ABORT);
            CriticalArray label_data(env, label.info.jdata, 0);
            input.image.data = (U8*)input_data.data;
            label.image.data = (S32*)label_data.data;
            contour->process(input.image, label.image);
        }

        jfieldID fidPoints = safe_GetFieldID(env, objClass, "storagePoints", "Lorg/ddogleg/struct/DogArray_I32;");
        jobject jpoints = env->GetObjectField(obj,fidPoints);
        WrapJDogArray_I32 grow_queue(env,jpoints);

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
    });
}

JNIEXPORT void JNICALL Java_org_boofcpp_contour_NativeChang2004_setSaveInnerContour
        (JNIEnv *env, jobject obj, jboolean enabled)
{
    jni_call(env, [&] {
        jclass objClass = env->GetObjectClass(obj);
        jfieldID fid = safe_GetFieldID(env, objClass, "nativePtr", "J");
        auto * contour = (LinearContourLabelChang2004*)env->GetLongField(obj, fid);

        contour->saveInternalContours = enabled;
    });
}

JNIEXPORT jboolean JNICALL Java_org_boofcpp_contour_NativeChang2004_isSaveInternalContours
        (JNIEnv *env, jobject obj)
{
    return jni_call(env, [&] {
        jclass objClass = env->GetObjectClass(obj);
        jfieldID fid = safe_GetFieldID(env,objClass, "nativePtr", "J");
        auto * contour = (LinearContourLabelChang2004*)env->GetLongField(obj, fid);

        return (jboolean)contour->saveInternalContours;
    });
}

JNIEXPORT void JNICALL Java_org_boofcpp_contour_NativeChang2004_nativeSetMinContour
        (JNIEnv *env, jobject obj, jint size)
{
    jni_call(env, [&] {
        jclass objClass = env->GetObjectClass(obj);
        jfieldID fid = safe_GetFieldID(env, objClass, "nativePtr", "J");
        auto* contour = (LinearContourLabelChang2004*)env->GetLongField(obj, fid);

        contour->minContourSize = static_cast<uint32_t>(size);
    });
}

JNIEXPORT jint JNICALL Java_org_boofcpp_contour_NativeChang2004_nativeGetMinContour
        (JNIEnv *env, jobject obj)
{
    return jni_call(env, [&] {
        jclass objClass = env->GetObjectClass(obj);
        jfieldID fid = safe_GetFieldID(env,objClass, "nativePtr", "J");
        auto* contour = (LinearContourLabelChang2004*)env->GetLongField(obj, fid);

        return static_cast<jint>(contour->minContourSize);
    });
}

JNIEXPORT void JNICALL Java_org_boofcpp_contour_NativeChang2004_nativeSetMaxContour
        (JNIEnv *env, jobject obj, jint size)
{
    jni_call(env, [&] {
        jclass objClass = env->GetObjectClass(obj);
        jfieldID fid = safe_GetFieldID(env,objClass, "nativePtr", "J");
        auto* contour = (LinearContourLabelChang2004*)env->GetLongField(obj, fid);

        contour->maxContourSize = static_cast<uint32_t>(size);
    });
}

JNIEXPORT jint JNICALL Java_org_boofcpp_contour_NativeChang2004_nativeGetMaxContour
        (JNIEnv *env, jobject obj)
{
    return jni_call(env, [&] {
        jclass objClass = env->GetObjectClass(obj);
        jfieldID fid = safe_GetFieldID(env, objClass, "nativePtr", "J");
        auto* contour = (LinearContourLabelChang2004*)env->GetLongField(obj, fid);

        return static_cast<jint>(contour->maxContourSize);
    });
}

JNIEXPORT void JNICALL Java_org_boofcpp_contour_NativeChang2004_setConnectRule
        (JNIEnv *env, jobject obj, jobject jrule )
{
    jni_call(env, [&] {
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

    });
}

JNIEXPORT jobject JNICALL Java_org_boofcpp_contour_NativeChang2004_getConnectRule
        (JNIEnv *env, jobject obj)
{
    return jni_call(env, [&] {
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
            throw "Couldn't look up the static field";
        }

        jobject ret = env->GetStaticObjectField(enumClass,theRule);
        if( env->ExceptionCheck() || ret == nullptr ) {
            env->ExceptionDescribe();
            throw "Couldn't look up the static field";
        }
        return ret;
    });
}
}