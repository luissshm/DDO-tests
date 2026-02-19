package sebyone.libdaas.ddotest

import android.os.Bundle
import android.util.Log
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import java.net.Inet4Address
import java.net.NetworkInterface
import kotlin.concurrent.fixedRateTimer
import kotlin.random.Random

class MainActivity : AppCompatActivity() {

    private lateinit var logView: TextView
    private lateinit var scrollView: ScrollView
    private lateinit var ipView: TextView
    private lateinit var dinView: TextView
    private lateinit var payloadEdit: EditText
    private lateinit var lastPayloadView: TextView

    private val TAG = "DaaS-UI"
    private val localDin = 102L

    private var discoveredDin: Long? = null
    private var discoveryRunning = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        logView = findViewById(R.id.logView)
        scrollView = findViewById(R.id.logScrollView)
        ipView = findViewById(R.id.localIpText)
        dinView = findViewById(R.id.localDinText)
        payloadEdit = findViewById(R.id.payloadEdit)
        lastPayloadView = findViewById(R.id.lastPayloadText)

        val btnStart = findViewById<Button>(R.id.btnStart)
        val btnSend = findViewById<Button>(R.id.btnSend)
        val btnDiscovery = findViewById<Button>(R.id.btnDiscovery)

        val localIp = getLocalIpAddress()
        ipView.text = "Local IP: $localIp"
        dinView.text = "DIN: $localDin"
        log("Local IP detected: $localIp")

        // ---------------- Buttons ----------------
        btnStart.setOnClickListener {
            val uri = "$localIp:4000"
            log("[DaaS] Starting agent with URI $uri")

            DaasManager.startAgent(
                sid = 100,
                din = localDin.toInt(),
                localUri = uri
            )

            startPerformLoop()
        }

        btnSend.setOnClickListener {
            val value = payloadEdit.text.toString().toByteOrNull()
            val remoteDin = discoveredDin

            if (value == null || remoteDin == null) {
                log("Invalid payload or no discovered DIN")
                return@setOnClickListener
            }

            log("[DaaS] Sending DDO value=$value → DIN=$remoteDin")
            DaasManager.sendTestDDO(remoteDin, value)
        }

        btnDiscovery.setOnClickListener {
            if (!discoveryRunning) {
                discoveryRunning = true
                startDiscovery()
            } else {
                log("Discovery already running...")
            }
        }

        // ---------------- DDO Listener ----------------
        DaasManager.ddoCallback = object : DaasManager.dynamicListener {
            override fun onNodeDiscovered(din: Long) {
                runOnUiThread {
                    log("[DISCOVERED] DIN=$din")
                    discoveredDin = din
                    // Automatically locate and start auto-pull
                    DaasManager.locateNode(din)
                }
            }

            override fun onDDOReceivedExtended(origin: Long, typeset: Int, value: Int) {
                runOnUiThread {
                    lastPayloadView.text = "Last → typeset=$typeset value=$value"
                    log("[DDO] origin=$origin typeset=$typeset value=$value")
                }
            }

            override fun onAutoPull(origin: Long, value: Int) {
                runOnUiThread {
                    lastPayloadView.text = "Last → typeset=1 value=$value"
                    log("[AUTO-PULL] origin=$origin value=$value")
                }
            }
        }
    }

    // ---------------- Discovery every 10s ----------------
    private fun startDiscovery() {
        log("[DaaS] Starting discovery every 10s")

        fixedRateTimer("discoveryTimer", true, 0, 10_000) {
            if (!discoveryRunning) cancel()

            runOnUiThread {
                log("[DaaS] Discovery call...")
                DaasManager.discovery()
            }
        }
    }

    // ---------------- Perform / Auto-poll loop ----------------
    private fun startPerformLoop() {
        Thread {
            while (true) {
                DaasManager.loop()
                discoveredDin?.let { DaasManager.autoPull(it) }
                Thread.sleep(10)
            }
        }.start()
        log("[DaaS] Perform loop started + auto pull polling")
    }

    private fun log(msg: String) {
        Log.d(TAG, msg)
        runOnUiThread {
            logView.append(msg + "\n")
            scrollView.post { scrollView.fullScroll(ScrollView.FOCUS_DOWN) }
        }
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
