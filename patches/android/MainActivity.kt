package com.godoy.nexora

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.provider.Settings
import android.util.Size
import android.widget.EditText
import android.widget.Toast
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AlertDialog
import androidx.camera.core.CameraSelector
import androidx.camera.core.ImageAnalysis
import androidx.camera.core.ImageProxy
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import com.godoy.nexora.databinding.ActivityMainBinding
import com.godoy.nexora.networking.ConnectionManager
import com.godoy.nexora.util.Logger
import com.godoy.nexora.video.Camera
import com.google.android.material.dialog.MaterialAlertDialogBuilder

class MainActivity : AppCompatActivity(), ConnectionManager.ConnectionStateCallback {
    private lateinit var viewBinding: ActivityMainBinding
    private val qrscanner = QRScanner()
    private var connectionManager = ConnectionManager.getInstance(this)
    private var camera: Camera? = null
    private var pendingQrPairing = false
    private var pendingPairAddress: String? = null
    private var pendingPairPort: Int? = null
    private val prefs by lazy { getSharedPreferences("mycam_connection", MODE_PRIVATE) }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        viewBinding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(viewBinding.root)
        viewBinding.logReportButton.setOnClickListener { startActivity(Intent(this, LogActivity::class.java)) }
        viewBinding.manualConnectButton.setOnClickListener { showManualConnectDialog() }
        enableEdgeToEdge()
        initialize()
    }

    override fun onResume() {
        super.onResume()
        connectionManager = ConnectionManager.getInstance(this)
        if (camera != null) {
            camera!!.start(Size(1280, 720), CameraSelector.DEFAULT_BACK_CAMERA)
            connectSavedDeviceOrScan()
        }
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == 1000) {
            if (grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED) initialize()
            else MaterialAlertDialogBuilder(this).setTitle("Требуется доступ к камере").setMessage("Разрешите доступ к камере в настройках приложения.").setPositiveButton("Открыть настройки") { _, _ -> startActivity(Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS).apply { data = Uri.fromParts("package", packageName, null) }) }.setNegativeButton("Отмена", null).show()
        }
    }

    override fun onConnectionSuccessful(connectionMode: ConnectionManager.Mode) {
        runOnUiThread {
            if (connectionMode == ConnectionManager.Mode.WIFI && pendingQrPairing) {
                val address = pendingPairAddress
                val port = pendingPairPort
                if (address != null && port != null) prefs.edit().putBoolean("paired", true).putString("address", address).putInt("port", port).apply()
                pendingQrPairing = false
                Logger.log("MAIN", "QR pairing saved: $address:$port")
            }
            qrscanner.stop()
            Logger.log("MAIN", "Connection successful $connectionMode")
            startActivity(Intent(this, StreamActivity::class.java))
        }
    }

    override fun onConnectionFailed(connectionMode: ConnectionManager.Mode) {
        runOnUiThread {
            pendingQrPairing = false
            qrscanner.start()
            Logger.log("MAIN", "Error: Cannot connect!")
            Toast.makeText(this, if (connectionMode == ConnectionManager.Mode.WIFI) "Не удалось подключиться. Отсканируйте QR-код ещё раз." else "USB-подключение не удалось. Отсканируйте QR-код.", Toast.LENGTH_LONG).show()
        }
    }

    private fun checkPermissions(): Boolean {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.CAMERA), 1000)
            return false
        }
        return true
    }

    private fun initialize() {
        if (checkPermissions()) {
            camera = Camera(viewBinding.viewFinder.surfaceProvider, ImageAnalysis.OUTPUT_IMAGE_FORMAT_YUV_420_888, ::processImage, this, this)
            camera!!.start(Size(1280, 720), CameraSelector.DEFAULT_BACK_CAMERA)
            connectSavedDeviceOrScan()
        }
    }

    private fun connectSavedDeviceOrScan() {
        val paired = prefs.getBoolean("paired", false)
        val address = prefs.getString("address", null)
        val port = prefs.getInt("port", 0)
        if (paired && !address.isNullOrBlank() && port in 1..65535) {
            Logger.log("MAIN", "Auto-connecting to saved device $address:$port")
            qrscanner.stop()
            connectionManager.connect(address, port)
        } else {
            Logger.log("MAIN", "No trusted device. Waiting for first QR scan.")
            qrscanner.start()
        }
    }

    private fun processImage(imageProxy: ImageProxy) {
        qrscanner.launchScanTask(imageProxy) { result ->
            runOnUiThread {
                qrscanner.stop()
                pendingQrPairing = true
                pendingPairAddress = result.address
                pendingPairPort = result.port
                MaterialAlertDialogBuilder(this).setTitle("Подключение по Wi-Fi").setMessage("Подключиться к ${result.address}:${result.port}?").setPositiveButton("Подключить") { _, _ -> connectionManager.connect(result.address, result.port) }.setNegativeButton("Отмена") { _, _ -> pendingQrPairing = false; qrscanner.start() }.show()
            }
        }
    }

    private fun showManualConnectDialog() {
        qrscanner.stop()
        val input = EditText(this).apply { hint = getString(R.string.ip_address_hint); setSingleLine(true) }
        val dialog = MaterialAlertDialogBuilder(this).setTitle(R.string.connect_by_ip).setView(input).setPositiveButton("Подключить", null).setNegativeButton("Отмена") { _, _ -> qrscanner.start() }.create()
        dialog.setOnShowListener {
            dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener {
                val endpoint = QRScanner.parseEndpoint(input.text.toString())
                if (endpoint == null) input.error = "Введите корректный IPv4-адрес и порт"
                else { dialog.dismiss(); pendingQrPairing = false; connectionManager.connect(endpoint.address, endpoint.port) }
            }
        }
        dialog.setOnCancelListener { qrscanner.start() }
        dialog.show()
    }
}
