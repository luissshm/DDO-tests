package sebyone.libdaas.ddotest

import android.app.*
import android.content.Intent
import android.os.Build
import android.os.IBinder
import android.util.Log
import java.net.Inet4Address
import java.net.NetworkInterface

class DaasService : Service() {

    override fun onCreate() {
        super.onCreate()
        startForegroundService()
        initDaas()
    }

    override fun onBind(intent: Intent?): IBinder? = null

    private fun initDaas() {
        val ip = getLocalIpAddress()
        Log.d("DAAS", "Service IP = $ip")

        if (ip == "0.0.0.0") {
            Log.e("DAAS", "No valid IP, aborting DAAS init")
            return
        }

        DaasManager.init(
            sid = 1L,
            din = 1001L,
            localUri = "$ip:4000"
        )

        DaasManager.mapNode(
            din = 1002L,
            uri = "10.42.0.89:4000" // CHANGE on 2nd phone
        )

        // Send test DDO after startup
        Thread {
            Thread.sleep(1500)
            Log.d("DAAS", "Sending test DDO")
            DaasManager.sendSimpleDDO(
                remoteDin = 1002L,
                value = 42
            )
        }.start()
    }

    private fun startForegroundService() {
        val channelId = "daas_channel"

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(
                channelId,
                "DAAS Service",
                NotificationManager.IMPORTANCE_LOW
            )
            getSystemService(NotificationManager::class.java)
                .createNotificationChannel(channel)
        }

        val notification = Notification.Builder(this, channelId)
            .setContentTitle("DAAS running")
            .setContentText("DDO node active")
            .setSmallIcon(android.R.drawable.stat_sys_data_bluetooth)
            .build()

        startForeground(1, notification)
    }

    private fun getLocalIpAddress(): String {
        return try {
            NetworkInterface.getNetworkInterfaces().asSequence()
                .flatMap { it.inetAddresses.asSequence() }
                .firstOrNull { !it.isLoopbackAddress && it is Inet4Address }
                ?.hostAddress ?: "0.0.0.0"
        } catch (e: Exception) {
            "0.0.0.0"
        }
    }

}