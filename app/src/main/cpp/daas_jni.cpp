#include <jni.h>
#include <string>
#include <android/log.h>
#include "daas.hpp"
#include "daas_types.hpp"

#define LOG_TAG "DAAS-JNI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

static JavaVM* g_vm = nullptr;
static jobject g_daasObject = nullptr;
static DaasAPI* g_daas = nullptr;

static const typeset_t SIMPLE_TYPESET = 1;

/* ---------------- Event handler ---------------- */

class DaasEvents : public IDaasApiEvent {
public:
    void dinAccepted(din_t) override {}
    void frisbeeReceived(din_t) override {}
    void nodeStateReceived(din_t) override {}
    void atsSyncCompleted(din_t) override {}
    void frisbeeDperfCompleted(din_t, uint32_t, uint32_t) override {}
    void nodeDiscovered(din_t, link_t) override {}
    void nodeConnectedToNetwork(din_t, din_t) override {}

    void ddoReceived(int payload_size, typeset_t typeset, din_t origin) override {
        if (!g_daas || typeset != SIMPLE_TYPESET || payload_size != sizeof(int))
            return;

        DDO* ddo = nullptr;
        g_daas->pull(origin, &ddo);
        if (!ddo) return;

        int value = 0;
        ddo->getPayloadAsBinary(
                reinterpret_cast<uint8_t*>(&value), 0, sizeof(int)
        );

        JNIEnv* env = nullptr;
        if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK)
            return;

        jclass cls = env->GetObjectClass(g_daasObject);
        if (!cls) return;

        jmethodID mid = env->GetStaticMethodID(
                cls,
                "onDDOReceived",
                "(JI)V"
        );
        if (!mid) return;

        env->CallStaticVoidMethod(
                cls,
                mid,
                static_cast<jlong>(origin),
                static_cast<jint>(value)
        );
    }
};

static DaasEvents g_events;

/* ---------------- JNI ---------------- */

extern "C"
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM* vm, void*) {
    g_vm = vm;
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT void JNICALL
Java_sebyone_libdaas_ddotest_DaasManager_init(
        JNIEnv* env,
        jobject /* thiz */,
        jlong sid,
        jlong din,
        jstring uri
) {
    const char* c_uri = env->GetStringUTFChars(uri, nullptr);

    g_daas = new DaasAPI(&g_events);
    g_daas->doInit(static_cast<din_t>(sid), static_cast<din_t>(din));
    g_daas->enableDriver(_LINK_INET4, c_uri);
    g_daas->doPerform(PERFORM_CORE_THREAD);

    jclass cls = env->FindClass("sebyone/libdaas/ddotest/DaasManager");
    g_daasObject = env->NewGlobalRef(cls);

    env->ReleaseStringUTFChars(uri, c_uri);
}

extern "C"
JNIEXPORT void JNICALL
Java_sebyone_libdaas_ddotest_DaasManager_mapNode(
        JNIEnv* env,
        jobject /* thiz */,
        jlong din,
        jstring uri
) {
    const char* c_uri = env->GetStringUTFChars(uri, nullptr);
    g_daas->map(static_cast<din_t>(din), _LINK_INET4, c_uri);
    env->ReleaseStringUTFChars(uri, c_uri);
}

extern "C"
JNIEXPORT void JNICALL
Java_sebyone_libdaas_ddotest_DaasManager_sendSimpleDDO(
        JNIEnv*,
        jobject /* thiz */,
        jlong remoteDin,
        jint value
) {
    DDO ddo(SIMPLE_TYPESET);
    ddo.setPayload(&value, sizeof(int));
    g_daas->push(static_cast<din_t>(remoteDin), &ddo);
}

extern "C"
JNIEXPORT void JNICALL
Java_sebyone_libdaas_ddotest_DaasManager_perform(
        JNIEnv*,
        jobject /* thiz */
) {
    g_daas->doPerform(PERFORM_CORE_NO_THREAD);
}