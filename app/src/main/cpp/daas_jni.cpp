#include <jni.h>
#include <string>
#include <vector>
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
        LOGD("[DaaS] ddoReceived from %lu size=%d typeset=%d",
             origin, payload_size, typeset);

        DDO* ddo = nullptr;
        if (g_daas->pull(origin, &ddo) != ERROR_NONE || !ddo)
            return;

        int value = 0;
        ddo->getPayloadAsBinary((uint8_t*)&value, 0, 1);

        JNIEnv* env;
        g_vm->AttachCurrentThread(&env, nullptr);

        jclass cls = env->FindClass("sebyone/libdaas/ddotest/DaasManager");
        jmethodID mid = env->GetStaticMethodID(cls,
                                               "onDDOReceivedExtended",
                                               "(JII)V");

        if (mid)
            env->CallStaticVoidMethod(cls,
                                      mid,
                                      (jlong)origin,
                                      (jint)typeset,
                                      (jint)value);

        delete ddo;
    }

    void nodeDiscovered(din_t din, link_t link) override {
        LOGD("[DaaS] Node Discovered DIN=%lu LINK=%d", din, link);
    }

    void nodeConnectedToNetwork(din_t sid, din_t din) override {
        LOGD("[DaaS] nodeConnectedToNetwork sid=%lu din=%lu", sid, din);
    }

    void frisbeeReceived(din_t) override {}
    void nodeStateReceived(din_t) override {}
    void atsSyncCompleted(din_t) override {}
    void frisbeeDperfCompleted(din_t, uint32_t, uint32_t) override {}
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
JNIEXPORT jint JNICALL
Java_sebyone_libdaas_ddotest_DaasManager_nativeDiscovery(
        JNIEnv*, jclass) {

    LOGD("[DaaS] Initializing discovery INET4");

    auto err = g_daas->discovery(_LINK_INET4);

    LOGD("[DaaS] discovery(_LINK_INET4) -> %d", err);
    return err;
}

extern "C"
JNIEXPORT jint JNICALL
Java_sebyone_libdaas_ddotest_DaasManager_nativeLocate(
        JNIEnv*, jclass, jlong din, jint timeout_ms) {

    if (!g_daas) return -1;

    LOGD("[DaaS] Locating DIN=%lu with timeout=%dms", din, timeout_ms);
    auto err = g_daas->locate((din_t)din, timeout_ms);
    LOGD("[DaaS] locate() -> %d", err);
    return err;
}

extern "C"
JNIEXPORT void JNICALL
Java_sebyone_libdaas_ddotest_DaasManager_nativeSetDiscoveryStateFull(
        JNIEnv*, jclass) {

    discovery_state_t dis = discovery_full;
    LOGD("[DaaS] Setting discovery state to -> discovery_full");
    g_daas->setDiscoveryState(dis);

}

extern "C"
JNIEXPORT jlongArray JNICALL
Java_sebyone_libdaas_ddotest_DaasManager_nativeListNodes(
        JNIEnv* env, jclass) {

    if (!g_daas) {
        LOGD("[DaaS] List Nodes: g_daas is null");
        return nullptr;
    }

    // Get the local node list
    dinlist_t nodes = g_daas->listNodes();
    jsize count = static_cast<jsize>(nodes.size());

    LOGD("[DaaS] List Nodes -> %d nodes", count);

    // Create a Java long array to return
    jlongArray jnodes = env->NewLongArray(count);
    if (!jnodes) {
        LOGD("[DaaS] ListNodes: failed to allocate jlongArray");
        return nullptr;
    }

    // Copy values into Java array
    std::vector<jlong> tmp(count);
    for (jsize i = 0; i < count; ++i) {
        tmp[i] = static_cast<jlong>(nodes[i]);
    }
    env->SetLongArrayRegion(jnodes, 0, count, tmp.data());

    return jnodes;
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
