#include <jni.h>
#include <string>
#include <android/log.h>
#include "daas.hpp"

#define LOG_TAG "DaaS-Native"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

static JavaVM* g_vm = nullptr;
static DaasAPI* g_daas = nullptr;

static const typeset_t SIMPLE_TYPESET = 1;

/* ---------------- Events ---------------- */

class DaasEvents : public IDaasApiEvent {
public:
    void dinAccepted(din_t din) override {
        LOGD("[DaaS] dinAccepted %lu", din);
    }

    void ddoReceived(int payload_size, typeset_t typeset, din_t origin) override {
        LOGD("[DaaS] ddoReceived from %lu size=%d", origin, payload_size);

        if (typeset != SIMPLE_TYPESET) return;

        DDO* ddo = nullptr;
        if (g_daas->pull(origin, &ddo) != ERROR_NONE) {
            LOGD("[DaaS] pull() failed");
            return;
        }

        int value = 0;
        ddo->getPayloadAsBinary((uint8_t*)&value, 0, 1);

        JNIEnv* env;
        g_vm->AttachCurrentThread(&env, nullptr);

        jclass cls = env->FindClass("sebyone/libdaas/ddotest/DaasManager");
        jmethodID mid = env->GetStaticMethodID(cls, "onDDOReceived", "(JI)V");
        env->CallStaticVoidMethod(cls, mid, (jlong)origin, (jint)value);
    }

    void nodeConnectedToNetwork(din_t sid, din_t din) override {
        LOGD("[DaaS] nodeConnectedToNetwork sid=%lu din=%lu", sid, din);
    }

    void frisbeeReceived(din_t) override {}
    void nodeStateReceived(din_t) override {}
    void atsSyncCompleted(din_t) override {}
    void frisbeeDperfCompleted(din_t, uint32_t, uint32_t) override {}
    void nodeDiscovered(din_t, link_t) override {}
};

static DaasEvents g_events;

/* ---------------- JNI lifecycle ---------------- */

extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    g_vm = vm;
    return JNI_VERSION_1_6;
}

/* ---------------- API wrappers ---------------- */

extern "C"
JNIEXPORT void JNICALL
Java_sebyone_libdaas_ddotest_DaasManager_nativeCreate(JNIEnv*, jclass) {
    LOGD("[DaaS] Creating DaasAPI instance...");
    g_daas = new DaasAPI(&g_events);
    LOGD("[DaaS] Library version: %s", g_daas->getVersion());
}

extern "C"
JNIEXPORT jint JNICALL
Java_sebyone_libdaas_ddotest_DaasManager_nativeInit(
        JNIEnv* env, jclass, din_t sid, din_t din) {

    LOGD("[DaaS] Initializing with SID=%lu DIN=%lu", sid, din);
    auto err = g_daas->doInit(sid, din);
    LOGD("[DaaS] doInit() -> %d", err);
    return err;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_sebyone_libdaas_ddotest_DaasManager_nativeListDrivers(JNIEnv* env, jclass) {
    return env->NewStringUTF(g_daas->listAvailableDrivers());
}

extern "C"
JNIEXPORT jint JNICALL
Java_sebyone_libdaas_ddotest_DaasManager_nativeEnableDriver(
        JNIEnv* env, jclass, jstring uri) {

    const char* c_uri = env->GetStringUTFChars(uri, nullptr);
    LOGD("[DaaS] Enabling driver LINK_INET4 with URI %s", c_uri);

    auto err = g_daas->enableDriver(_LINK_INET4, c_uri);

    LOGD("[DaaS] enableDriver() -> %d", err);

    env->ReleaseStringUTFChars(uri, c_uri);
    return err;
}

extern "C"
JNIEXPORT jint JNICALL
Java_sebyone_libdaas_ddotest_DaasManager_nativeMap(
        JNIEnv* env, jclass, din_t din, jstring uri) {

    const char* c_uri = env->GetStringUTFChars(uri, nullptr);

    LOGD("[DaaS] Mapping DIN %lu to %s", din, c_uri);
    auto err = g_daas->map((din_t)din, _LINK_INET4, c_uri);

    LOGD("[DaaS] map() -> %d", err);

    env->ReleaseStringUTFChars(uri, c_uri);
    return err;
}

extern "C"
JNIEXPORT jint JNICALL
Java_sebyone_libdaas_ddotest_DaasManager_nativePerform(
        JNIEnv*, jclass) {
    return g_daas->doPerform(PERFORM_CORE_NO_THREAD);
}

extern "C"
JNIEXPORT jint JNICALL
Java_sebyone_libdaas_ddotest_DaasManager_nativeSendDDO(
        JNIEnv*, jclass, din_t remoteDin, jbyte value) {

    DDO ddo(SIMPLE_TYPESET);
    ddo.setPayload(&value, 1);

    LOGD("[DaaS] push() value=%d -> DIN=%lu", value, remoteDin);

    auto err = g_daas->push((din_t)remoteDin, &ddo);

    LOGD("[DaaS] push() -> %d", err);
    return err;
}

extern "C"
JNIEXPORT void JNICALL
Java_sebyone_libdaas_ddotest_DaasManager_nativeAutoPull(
        JNIEnv* env, jclass, jlong remoteDin) {

    if (!g_daas) return;

    DDO* inbound = nullptr;
    auto err = g_daas->pull((din_t)remoteDin, &inbound);

    if (err != ERROR_NONE || inbound == nullptr) {
        return; // no data available
    }

    int value = 0;
    inbound->getPayloadAsBinary((uint8_t*)&value, 0, sizeof(value));

    din_t origin = inbound->getOrigin();
    stime_t ts = inbound->getTimestamp();

    LOGD("[AUTO-PULL] origin=%lu value=%d ts=%llu", origin, value, ts);

    // notify Java UI
    JNIEnv* jni = nullptr;
    g_vm->AttachCurrentThread(&jni, nullptr);

    jclass cls = jni->FindClass("sebyone/libdaas/ddotest/DaasManager");
    jmethodID mid = jni->GetStaticMethodID(cls, "onAutoPull", "(JI)V");

    if (mid)
        jni->CallStaticVoidMethod(cls, mid, (jlong)origin, (jint)value);

    delete inbound;
}
