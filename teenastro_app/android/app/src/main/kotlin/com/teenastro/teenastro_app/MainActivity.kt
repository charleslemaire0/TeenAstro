package com.teenastro.teenastro_app

import android.content.Context
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraManager
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.MethodChannel
import kotlin.math.PI
import kotlin.math.atan
import kotlin.math.max

class MainActivity : FlutterActivity() {
    private val skyCapture by lazy { ManualSkyCapture(this) }

    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)
        MethodChannel(
            flutterEngine.dartExecutor.binaryMessenger,
            "com.teenastro/camera",
        ).setMethodCallHandler { call, result ->
            when (call.method) {
                "longerSideFov" -> {
                    val id = call.argument<String>("cameraId")
                    val deg = if (id.isNullOrEmpty()) null else longerSideFov(id)
                    if (deg == null) {
                        result.error("unavailable", "The lens field is not reported", null)
                    } else {
                        result.success(deg)
                    }
                }
                "manualOpen" -> {
                    val id = call.argument<String>("cameraId")
                    if (id.isNullOrEmpty()) {
                        result.error("unavailable", "No camera", null)
                        return@setMethodCallHandler
                    }
                    Thread {
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

    override fun onDestroy() {
        skyCapture.release()
        super.onDestroy()
    }

    /// Degrees across the longer side of the sensor for this camera id.
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
}
