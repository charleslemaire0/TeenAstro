package com.teenastro.teenastro_app

import android.Manifest
import android.content.Context
import android.content.pm.PackageManager
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraManager
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.MethodChannel
import kotlin.math.PI
import kotlin.math.atan
import kotlin.math.max

class MainActivity : FlutterActivity() {
    private val skyCapture by lazy { ManualSkyCapture(this) }
    private var pendingPermission: MethodChannel.Result? = null

    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)
        flutterEngine
            .platformViewsController
            .registry
            .registerViewFactory("teenastro/infinity_preview", InfinityPreviewFactory(this))

        MethodChannel(
            flutterEngine.dartExecutor.binaryMessenger,
            "com.teenastro/camera",
        ).setMethodCallHandler { call, result ->
            when (call.method) {
                "ensureCameraPermission" -> ensureCameraPermission(result)
                "longerSideFov" -> {
                    val id = call.argument<String>("cameraId")
                    val deg = if (id.isNullOrEmpty()) null else longerSideFov(id)
                    if (deg == null) {
                        result.error("unavailable", "The lens field is not reported", null)
                    } else {
                        result.success(deg)
                    }
                }
                "previewGamma" -> {
                    val g = call.argument<Double>("gamma") ?: 1.0
                    InfinityPreviewView.current?.setGamma(g.toFloat())
                    result.success(null)
                }
                "listBackCameras" -> {
                    result.success(listBackCameras())
                }
                "manualOpen" -> {
                    val id = call.argument<String>("cameraId")
                    if (id.isNullOrEmpty()) {
                        result.error("unavailable", "No camera", null)
                        return@setMethodCallHandler
                    }
                    if (!hasCameraPermission()) {
                        result.error("permission", "Camera permission is required", null)
                        return@setMethodCallHandler
                    }
                    Thread {
                        // Preview PlatformView must be removed first; give it time to release.
                        Thread.sleep(300)
                        val opened = skyCapture.open(id)
                        runOnUiThread {
                            if (!opened) {
                                result.error("unavailable", "The shutter could not be set", null)
                            } else {
                                result.success(
                                    mapOf(
                                        "minUs" to skyCapture.minUs,
                                        "maxUs" to skyCapture.maxUs,
                                        "minIso" to skyCapture.minIso,
                                        "maxIso" to skyCapture.maxIso,
                                    ),
                                )
                            }
                        }
                    }.start()
                }
                "manualCapture" -> {
                    val us = call.argument<Int>("exposureUs") ?: 0
                    val iso = call.argument<Int>("iso") ?: 0
                    Thread {
                        val shot = skyCapture.capture(us, iso)
                        runOnUiThread {
                            if (shot == null) {
                                result.error("unavailable", "The shutter did not fire", null)
                            } else {
                                result.success(
                                    mapOf(
                                        "jpeg" to shot.jpeg,
                                        "exposureUs" to shot.exposureUs,
                                    ),
                                )
                            }
                        }
                    }.start()
                }
                "manualClose" -> {
                    skyCapture.close()
                    result.success(null)
                }
                else -> result.notImplemented()
            }
        }
    }

    private fun hasCameraPermission(): Boolean =
        ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) ==
            PackageManager.PERMISSION_GRANTED

    private fun ensureCameraPermission(result: MethodChannel.Result) {
        if (hasCameraPermission()) {
            result.success(true)
            return
        }
        if (pendingPermission != null) {
            result.error("busy", "A permission request is already open", null)
            return
        }
        pendingPermission = result
        ActivityCompat.requestPermissions(
            this,
            arrayOf(Manifest.permission.CAMERA),
            REQ_CAMERA,
        )
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray,
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode != REQ_CAMERA) return
        val pending = pendingPermission
        pendingPermission = null
        val granted = grantResults.isNotEmpty() &&
            grantResults[0] == PackageManager.PERMISSION_GRANTED
        pending?.success(granted)
    }

    override fun onDestroy() {
        pendingPermission?.error("destroyed", "The activity closed", null)
        pendingPermission = null
        skyCapture.release()
        super.onDestroy()
    }

    private fun listBackCameras(): List<Map<String, Any>> {
        val manager = getSystemService(Context.CAMERA_SERVICE) as CameraManager
        val out = mutableListOf<Map<String, Any>>()
        for (id in manager.cameraIdList) {
            val chars = try {
                manager.getCameraCharacteristics(id)
            } catch (_: Exception) {
                continue
            }
            val facing = chars.get(CameraCharacteristics.LENS_FACING)
                ?: continue
            if (facing != CameraCharacteristics.LENS_FACING_BACK) continue
            out.add(
                mapOf(
                    "id" to id,
                    "sensorOrientation" to (chars.get(CameraCharacteristics.SENSOR_ORIENTATION) ?: 90),
                ),
            )
        }
        return out
    }

    private fun longerSideFov(cameraId: String): Double? {
        val manager = getSystemService(Context.CAMERA_SERVICE) as CameraManager
        val chars = try {
            manager.getCameraCharacteristics(cameraId)
        } catch (_: Exception) {
            return null
        }
        val size = chars.get(CameraCharacteristics.SENSOR_INFO_PHYSICAL_SIZE) ?: return null
        val focals = chars.get(CameraCharacteristics.LENS_INFO_AVAILABLE_FOCAL_LENGTHS)
        if (focals == null || focals.isEmpty()) return null
        val focal = focals[0].toDouble()
        if (focal <= 0.0) return null
        val side = max(size.width.toDouble(), size.height.toDouble())
        if (side <= 0.0) return null
        val deg = 2.0 * atan(side / (2.0 * focal)) * 180.0 / PI
        if (deg < 5.0 || deg > 170.0) return null
        return deg
    }

    companion object {
        private const val REQ_CAMERA = 4401
    }
}
