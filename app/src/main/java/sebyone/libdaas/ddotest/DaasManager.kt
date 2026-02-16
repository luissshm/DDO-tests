package sebyone.libdaas.ddotest

import android.util.Log

object DaasManager {

    init {
        System.loadLibrary("daas_jni")
    }

    private const val TAG = "DaaS"
    // Listener for UI callbacks
    var ddoCallback: Any? = null

    external fun nativeCreate()
    external fun nativeInit(sid: Int, din: Int): Int
    external fun nativeEnableDriver(uri: String): Int
    external fun nativeMap(din: Long, uri: String): Int
    external fun nativePerform(): Int
    external fun nativeSendDDO(din: Long, value: Byte): Int
    external fun nativeListDrivers(): String
    external fun nativeAutoPull(remoteDin: Long)

    fun startAgent(sid: Int, din: Int, localUri: String) {
        Log.d(TAG, "Starting agent...")
        nativeCreate()

        val init = nativeInit(sid, din)
        Log.d(TAG, "Initialization result = $init")

        val drivers = nativeListDrivers()
        Log.d(TAG, "Available drivers = $drivers")

        val enable = nativeEnableDriver(localUri)
        Log.d(TAG, "enableDriver result = $enable")

        Log.d(TAG, "Node ready")
    }

    fun mapNode(din: Long, uri: String) {
        val r = nativeMap(din, uri)
        Log.d(TAG, "map result = $r")
    }

    fun sendTestDDO(din: Long, value: Byte) {
        Log.d(TAG, "Sending DDO value=$value")
        val r = nativeSendDDO(din, value)
        Log.d(TAG, "push result = $r")
    }

    fun loop() {
        nativePerform()
    }

    fun autoPull(remoteDin: Long) {
        nativeAutoPull(remoteDin)
    }

    @JvmStatic
    fun onDDOReceived(origin: Long, value: Int) {
        Log.d(TAG, "DDO RECEIVED from $origin value=$value")
        (ddoCallback as? dynamicListener)?.onDDOReceived(origin, value)
    }

    @JvmStatic
    fun onAutoPull(origin: Long, value: Int) {
        Log.d("DaaS-AUTO", "AUTO-PULL origin=$origin value=$value")
        (ddoCallback as? dynamicListener)?.onAutoPull(origin, value)
    }

    // Interface type to avoid casts
    interface dynamicListener {
        fun onDDOReceived(origin: Long, value: Int)
        fun onAutoPull(origin: Long, value: Int)
    }
}