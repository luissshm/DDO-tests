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
    external fun nativeListNodes(): LongArray
    external fun nativeAutoPull(remoteDin: Long)
    external fun nativeDiscovery()
    external fun nativeSetDiscoveryStateFull()

    external fun nativeLocate(din: Long, timeoutMs: Int = 1000): Int


    fun startAgent(sid: Int, din: Int, localUri: String) {
        Log.d(TAG, "Starting agent...")
        nativeCreate()

        val init = nativeInit(sid, din)
        Log.d(TAG, "Initialization result = $init")

        val drivers = nativeListDrivers()
        Log.d(TAG, "Available drivers = $drivers")

        val enable = nativeEnableDriver(localUri)
        Log.d(TAG, "enableDriver result = $enable")
        setDiscoveryStateFull()
        Log.d(TAG, "Node ready")
    }

    fun mapNode(din: Long, uri: String) {
        val r = nativeMap(din, uri)
        Log.d(TAG, "map result = $r")
    }

    fun discovery() {
        val r = nativeDiscovery()
        Log.d(TAG, "discovery result = $r")
    }

    fun setDiscoveryStateFull() {
        nativeSetDiscoveryStateFull()
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

    fun listNodes() {
        Log.d(TAG, "Listing Nodes...")

        val nodes: LongArray? = nativeListNodes() // call the JNI function

        if (nodes == null || nodes.isEmpty()) {
            Log.d(TAG, "No nodes found")
            return
        }

        // Log each node DIN
        nodes.forEachIndexed { index, din ->
            Log.d(TAG, "Node #$index -> DIN=$din")
        }
    }

    fun locateNode(din: Long, timeoutMs: Int = 1000): Int {
        val err = nativeLocate(din, timeoutMs)
        Log.d(TAG, "locateNode($din) -> $err")
        return err
    }


    @JvmStatic
    fun onNodeDiscovered(din: Long) {
        Log.d(TAG, "Node discovered: $din")
        (ddoCallback as? dynamicListener)?.onNodeDiscovered(din)
    }

    @JvmStatic
    fun onDDOReceivedExtended(origin: Long, typeset: Int, value: Int) {
        Log.d(TAG, "DDO RECEIVED origin=$origin typeset=$typeset value=$value")
        (ddoCallback as? dynamicListener)
            ?.onDDOReceivedExtended(origin, typeset, value)
    }

    interface dynamicListener {
        fun onNodeDiscovered(din: Long)
        fun onDDOReceivedExtended(origin: Long, typeset: Int, value: Int)
        fun onAutoPull(origin: Long, value: Int)
    }

}