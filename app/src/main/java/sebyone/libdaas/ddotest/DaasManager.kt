package sebyone.libdaas.ddotest

import android.util.Log

object DaasManager {

    private const val TAG = "DAAS"

    init {
        System.loadLibrary("daas_jni")
    }

    external fun init(sid: Long, din: Long, localUri: String)
    external fun mapNode(din: Long, uri: String)
    external fun sendSimpleDDO(remoteDin: Long, value: Int)
    external fun perform()

    // ===== DDO callback from native =====
    @JvmStatic
    fun onDDOReceived(origin: Long, value: Int) {

        if (value >= 0) {
            // ---------------- DATA ----------------
            Log.d("DAAS", "RX DATA=$value from DIN=$origin")

            // SEND ACK
            val ackValue = -value
            sendSimpleDDO(origin, ackValue)

            Log.d("DAAS", "TX ACK=$ackValue to DIN=$origin")

        } else {
            // ---------------- ACK ----------------
            val ackFor = -value
            Log.d("DAAS", "RX ACK for value=$ackFor from DIN=$origin")
        }
    }
}