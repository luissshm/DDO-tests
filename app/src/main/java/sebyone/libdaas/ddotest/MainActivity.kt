package sebyone.libdaas.ddotest

import android.content.Intent
import android.os.Bundle
import androidx.activity.ComponentActivity

class MainActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Start DAAS backend
        startService(Intent(this, DaasService::class.java))

        // No fragments
        // No background threads
        // No DAAS calls here
    }
}
