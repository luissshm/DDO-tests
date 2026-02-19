package sebyone.libdaas.ddotest

import android.os.Bundle
import android.util.Log
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import java.net.Inet4Address
import java.net.NetworkInterface
import kotlin.collections.forEachIndexed
import kotlin.random.Random

class MainActivity : AppCompatActivity() {

    private lateinit var logView: TextView
    private lateinit var scrollView: ScrollView
    private lateinit var ipView: TextView
    private lateinit var dinView: TextView
    private lateinit var remoteIpEdit: EditText
    private lateinit var remoteDinEdit: EditText
    private lateinit var payloadEdit: EditText

    private val TAG = "DaaS-UI"
//    private val localDin = Random.nextInt(100, 10000).toLong()
    private val localDin = 103

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        logView = findViewById(R.id.logView)
        scrollView = findViewById<ScrollView>(R.id.logScrollView)
        ipView = findViewById(R.id.localIpText)
        dinView = findViewById(R.id.localDinText)
        remoteIpEdit = findViewById(R.id.remoteIpEdit)
        remoteDinEdit = findViewById(R.id.remoteDinEdit)
        payloadEdit = findViewById(R.id.payloadEdit)

        val btnStart = findViewById<Button>(R.id.btnStart)
        val btnMap = findViewById<Button>(R.id.btnMap)
        val btnSend = findViewById<Button>(R.id.btnSend)
        val btnListNodes = findViewById<Button>(R.id.btnListNodes)
        val btnDiscovery = findViewById<Button>(R.id.btnDiscovery)

        val localIp = getLocalIpAddress()
        ipView.text = "Local IP: $localIp"
        dinView.text = "DIN: $localDin"
        log("Local IP detected: $localIp")

        btnStart.setOnClickListener {
            val uri = "$localIp:4000"
            log("[DaaS] Starting agent with URI $uri")

            DaasManager.startAgent(
                sid = 100,
                din = localDin,
                localUri = uri
            )

            startPerformLoop()
        }

        btnMap.setOnClickListener {
            val remoteIp = remoteIpEdit.text.toString()
            val remoteDin = remoteDinEdit.text.toString().toLongOrNull()

            if (remoteIp.isEmpty() || remoteDin == null) {
                log("Invalid remote configuration")
                return@setOnClickListener
            }

            val uri = "$remoteIp:4000"
            log("[DaaS] Mapping remote DIN=$remoteDin → $uri")

            DaasManager.mapNode(remoteDin, uri)
        }

        btnSend.setOnClickListener {
            val remoteDin = remoteDinEdit.text.toString().toLongOrNull()
            val value = payloadEdit.text.toString().toByte()

            if (remoteDin == null || value.equals(null)) {
                log("Invalid payload or DIN")
                return@setOnClickListener
            }

            if (value < -127 || value > 126) {
                log("Payload must be a number between {-127,126}")
                return@setOnClickListener
            }

            log("[DaaS] Sending DDO value=$value → DIN=$remoteDin")
            DaasManager.sendTestDDO(remoteDin, value)
        }

        btnListNodes.setOnClickListener {
            val nodes: LongArray? = DaasManager.nativeListNodes()
            // Log each node DIN
            nodes?.forEachIndexed { index, din ->
                log("Node #$index -> DIN=$din")
            }
        }

        btnDiscovery.setOnClickListener {
            log("[DaaS] Starting Discovery")
            val discovery = DaasManager.discovery()
        }
    }

    // ------------------ DDO Listener Hook ------------------
    private val ddoListener = object : DaasManager.dynamicListener {
        override fun onDDOReceived(origin: Long, value: Int) {
            runOnUiThread { log("[DDO RECEIVED] from $origin value=$value") }
        }
        override fun onAutoPull(origin: Long, value: Int) {
            runOnUiThread { log("[AUTO-PULL] origin=$origin value=$value") }
        }
    }

    override fun onResume() {
        super.onResume()
        // Register listener so DaasManager callbacks go to this activity
        DaasManager.ddoCallback = ddoListener
    }

    override fun onPause() {
        super.onPause()
        // Unregister listener to avoid leaks
        DaasManager.ddoCallback = null
    }

    private fun startPerformLoop() {
        Thread {
            while (true) {
                DaasManager.loop()

                val remoteDin = remoteDinEdit.text.toString().toLongOrNull()
                if (remoteDin != null) {
                    DaasManager.autoPull(remoteDin)
                }

                Thread.sleep(10)
            }
        }.start()

        log("[DaaS] Perform loop started + auto pull polling")
    }

    private fun log(msg: String) {
        Log.d(TAG, msg)
        runOnUiThread {
            logView.append(msg + "\n")
            // Scroll to bottom
            scrollView.post {
                scrollView.fullScroll(ScrollView.FOCUS_DOWN)
            }
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