#include <jni.h>
#include "boofcv/binary_ops.h"
#include "boofcv/contour.h"
#include "JNIBoofCPP.h"

using namespace boofcv;

namespace {
    /// IDs of NativeChang2004, looked up once
    struct Chang2004Ids {
        jfieldID nativePtr, storagePoints, packedPoints;
        jmethodID addContour;

        explicit Chang2004Ids( JNIEnv* env ) {
            jclass cls = global_class(env, "org/boofcpp/contour/NativeChang2004");
            nativePtr = safe_GetFieldID(env, cls, "nativePtr", "J");
            storagePoints = safe_GetFieldID(env, cls, "storagePoints", "Lorg/ddogleg/struct/DogArray_I32;");
            packedPoints = safe_GetFieldID(env, cls, "packedPoints", "Lboofcv/struct/PackedSetsPoint2D_I32;");
            addContour = safe_GetMethodID(env, cls, "addContour", "(II)V");
        }
    };

    const Chang2004Ids& ids( JNIEnv* env ) {
        static const Chang2004Ids ids(env);
        return ids;
    }

    LinearContourLabelChang2004* get_contour( JNIEnv* env, jobject obj ) {
        return (LinearContourLabelChang2004*)env->GetLongField(obj, ids(env).nativePtr);
    }
}

extern "C" {

JNIEXPORT void JNICALL Java_org_boofcpp_contour_NativeChang2004_native_1init
        (JNIEnv *env, jobject obj)
{
    jni_call(env, [&] {
        LinearContourLabelChang2004* ptr = new LinearContourLabelChang2004(ConnectRule::FOUR);
        ptr->maxContourSize = 0x7fffffff; // Java's max is a signed int
        env->SetLongField(obj, ids(env).nativePtr, (jlong)ptr);
    });
}

JNIEXPORT void JNICALL Java_org_boofcpp_contour_NativeChang2004_native_1destroy
        (JNIEnv *env, jobject obj)
{
    jni_call(env, [&] {
        delete get_contour(env, obj);
        env->SetLongField(obj, ids(env).nativePtr, 0);
    });
}

JNIEXPORT void JNICALL Java_org_boofcpp_contour_NativeChang2004_native_1process
        (JNIEnv *env, jobject obj, jobject jbinary, jobject jlabel)
{
    jni_call(env, [&] {
        LinearContourLabelChang2004* contour = get_contour(env, obj);

        ImageAndInfo<Gray<U8>,JImageInfo> input = wrapCriticalGrayU8(env,jbinary);
        ImageAndInfo<Gray<S32>,JImageInfo> label = wrapCriticalGrayS32(env,jlabel);

        {
            CriticalArray input_data(env, input.info.jdata, JNI_ABORT);
            CriticalArray label_data(env, label.info.jdata, 0);
            input.image.data = (U8*)input_data.data;
            label.image.data = (S32*)label_data.data;
            contour->process(input.image, label.image);
        }

        jobject jpoints = env->GetObjectField(obj, ids(env).storagePoints);
        WrapJDogArray_I32 grow_queue(env,jpoints);

        jmethodID midAdd = ids(env).addContour;

        for( uint32_t i = 0; i < contour->contours.size(); i++ ) {
            ContourPacked &p = contour->contours.at(i);

            // internal contour ids will be grabbed from storagePoints
            grow_queue.setTo(p.internalIndexes);
            env->CallVoidMethod(obj,midAdd,(jint)p.id,(jint)p.externalIndex);
        }

        jobject jpacked = env->GetObjectField(obj, ids(env).packedPoints);
        copy_into_java(env,contour->packedPoints,jpacked);

        // TODO storageContours
    });
}

JNIEXPORT void JNICALL Java_org_boofcpp_contour_NativeChang2004_setSaveInnerContour
        (JNIEnv *env, jobject obj, jboolean enabled)
{
    jni_call(env, [&] {
        LinearContourLabelChang2004* contour = get_contour(env, obj);

        contour->saveInternalContours = enabled;
    });
}

JNIEXPORT jboolean JNICALL Java_org_boofcpp_contour_NativeChang2004_isSaveInternalContours
        (JNIEnv *env, jobject obj)
{
    return jni_call(env, [&] {
        LinearContourLabelChang2004* contour = get_contour(env, obj);

        return (jboolean)contour->saveInternalContours;
    });
}

JNIEXPORT void JNICALL Java_org_boofcpp_contour_NativeChang2004_nativeSetMinContour
        (JNIEnv *env, jobject obj, jint size)
{
    jni_call(env, [&] {
        LinearContourLabelChang2004* contour = get_contour(env, obj);

        contour->minContourSize = static_cast<uint32_t>(size);
    });
}

JNIEXPORT jint JNICALL Java_org_boofcpp_contour_NativeChang2004_nativeGetMinContour
        (JNIEnv *env, jobject obj)
{
    return jni_call(env, [&] {
        LinearContourLabelChang2004* contour = get_contour(env, obj);

        return static_cast<jint>(contour->minContourSize);
    });
}

JNIEXPORT void JNICALL Java_org_boofcpp_contour_NativeChang2004_nativeSetMaxContour
        (JNIEnv *env, jobject obj, jint size)
{
    jni_call(env, [&] {
        LinearContourLabelChang2004* contour = get_contour(env, obj);

        contour->maxContourSize = static_cast<uint32_t>(size);
    });
}

JNIEXPORT jint JNICALL Java_org_boofcpp_contour_NativeChang2004_nativeGetMaxContour
        (JNIEnv *env, jobject obj)
{
    return jni_call(env, [&] {
        LinearContourLabelChang2004* contour = get_contour(env, obj);

        return static_cast<jint>(contour->maxContourSize);
    });
}

JNIEXPORT void JNICALL Java_org_boofcpp_contour_NativeChang2004_setConnectRule
        (JNIEnv *env, jobject obj, jobject jrule )
{
    jni_call(env, [&] {
        LinearContourLabelChang2004* contour = get_contour(env, obj);

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
        LinearContourLabelChang2004* contour = get_contour(env, obj);

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