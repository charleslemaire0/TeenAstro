package com.teenastro.teenastro_app

import android.content.Context
import android.graphics.ImageFormat
import android.hardware.camera2.CameraCaptureSession
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CameraManager
import android.hardware.camera2.CaptureRequest
import android.hardware.camera2.CaptureResult
import android.media.ImageReader
import android.os.Handler
import android.os.HandlerThread
import android.view.Surface
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import kotlin.math.abs
import kotlin.math.max

/**
 * Still capture with an explicit shutter time. The preview camera must be
 * closed before [open], because a phone allows only one client per camera.
 */
class ManualSkyCapture(context: Context) {
    private val manager =
        context.getSystemService(Context.CAMERA_SERVICE) as CameraManager
    private val thread = HandlerThread("teenastro-shutter").also { it.start() }
    private val handler = Handler(thread.looper)

    private var device: CameraDevice? = null
    private var session: CameraCaptureSession? = null
    private var reader: ImageReader? = null

    var minUs: Int = 1
        private set
    var maxUs: Int = 1
        private set
    var minIso: Int = 100
        private set
    var maxIso: Int = 100
        private set

    fun open(cameraId: String): Boolean {
        close()
        val chars = try {
            manager.getCameraCharacteristics(cameraId)
        } catch (_: Exception) {
            return false
        }
        val exposure = chars.get(CameraCharacteristics.SENSOR_INFO_EXPOSURE_TIME_RANGE)
            ?: return false
        val sensitivity = chars.get(CameraCharacteristics.SENSOR_INFO_SENSITIVITY_RANGE)
            ?: return false
        val sizes = chars.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
            ?.getOutputSizes(ImageFormat.JPEG)
        if (sizes.isNullOrEmpty()) return false
        minUs = (exposure.lower / 1000L).toInt().coerceAtLeast(1)
        val samsungUpper = samsungExposureUpperNs(chars)
        val upperNs = maxOf(exposure.upper, samsungUpper ?: 0L)
        maxUs = (upperNs / 1000L).toInt().coerceAtLeast(minUs)
        minIso = sensitivity.lower
        maxIso = sensitivity.upper.coerceAtLeast(minIso)

        val size = sizes.minByOrNull { abs(max(it.width, it.height) - 1280) } ?: return false
        val imageReader = ImageReader.newInstance(size.width, size.height, ImageFormat.JPEG, 2)
        val opened = CountDownLatch(1)
        var camera: CameraDevice? = null
        try {
            manager.openCamera(cameraId, object : CameraDevice.StateCallback() {
                override fun onOpened(openedCamera: CameraDevice) {
                    camera = openedCamera
                    opened.countDown()
                }

                override fun onDisconnected(openedCamera: CameraDevice) {
                    openedCamera.close()
                    opened.countDown()
                }

                override fun onError(openedCamera: CameraDevice, error: Int) {
                    openedCamera.close()
                    opened.countDown()
                }
            }, handler)
        } catch (_: SecurityException) {
            imageReader.close()
            return false
        } catch (_: Exception) {
            imageReader.close()
            return false
        }
        if (!opened.await(4, TimeUnit.SECONDS) || camera == null) {
            camera?.close()
            imageReader.close()
            return false
        }
        val configured = CountDownLatch(1)
        var made: CameraCaptureSession? = null
        try {
            camera!!.createCaptureSession(
                listOf<Surface>(imageReader.surface),
                object : CameraCaptureSession.StateCallback() {
                    override fun onConfigured(captureSession: CameraCaptureSession) {
                        made = captureSession
                        configured.countDown()
                    }

                    override fun onConfigureFailed(captureSession: CameraCaptureSession) {
                        configured.countDown()
                    }
                },
                handler,
            )
        } catch (_: Exception) {
            camera?.close()
            imageReader.close()
            return false
        }
        if (!configured.await(4, TimeUnit.SECONDS) || made == null) {
            camera?.close()
            imageReader.close()
            return false
        }
        device = camera
        session = made
        reader = imageReader
        return true
    }

    fun capture(exposureUs: Int, iso: Int): SkyShot? {
        val camera = device ?: return null
        val captureSession = session ?: return null
        val imageReader = reader ?: return null
        val ns = exposureUs.coerceIn(minUs, maxUs).toLong() * 1000L
        val gain = iso.coerceIn(minIso, maxIso)
        while (true) {
            val leftover = imageReader.acquireNextImage() ?: break
            leftover.close()
        }
        val latch = CountDownLatch(1)
        var jpeg: ByteArray? = null
        var failed = false
        var exposureNs = ns
        imageReader.setOnImageAvailableListener({ pending ->
            val image = pending.acquireNextImage() ?: return@setOnImageAvailableListener
            try {
                val buffer = image.planes[0].buffer
                val bytes = ByteArray(buffer.remaining())
                buffer.get(bytes)
                jpeg = bytes
            } finally {
                image.close()
                latch.countDown()
            }
        }, handler)
        val request = try {
            camera.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE).apply {
                addTarget(imageReader.surface)
                put(CaptureRequest.CONTROL_MODE, CaptureRequest.CONTROL_MODE_OFF)
                put(CaptureRequest.CONTROL_AE_MODE, CaptureRequest.CONTROL_AE_MODE_OFF)
                put(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_OFF)
                put(CaptureRequest.CONTROL_AWB_MODE, CaptureRequest.CONTROL_AWB_MODE_AUTO)
                put(CaptureRequest.CONTROL_ENABLE_ZSL, false)
                put(CaptureRequest.NOISE_REDUCTION_MODE, CaptureRequest.NOISE_REDUCTION_MODE_OFF)
                put(CaptureRequest.EDGE_MODE, CaptureRequest.EDGE_MODE_OFF)
                put(CaptureRequest.FLASH_MODE, CaptureRequest.FLASH_MODE_OFF)
                put(CaptureRequest.LENS_FOCUS_DISTANCE, 0.0f)
                put(CaptureRequest.SENSOR_EXPOSURE_TIME, ns)
                put(CaptureRequest.SENSOR_FRAME_DURATION, ns.coerceAtLeast(33_000_000L))
                put(CaptureRequest.SENSOR_SENSITIVITY, gain)
                put(samsungExposureTimeKey, ns)
                put(CaptureRequest.JPEG_QUALITY, 92.toByte())
            }.build()
        } catch (_: Exception) {
            return null
        }
        return try {
            captureSession.capture(request, object : CameraCaptureSession.CaptureCallback() {
                override fun onCaptureCompleted(
                    session: CameraCaptureSession,
                    request: CaptureRequest,
                    result: android.hardware.camera2.TotalCaptureResult,
                ) {
                    val got = result.get(CaptureResult.SENSOR_EXPOSURE_TIME)
                    if (got != null && got > 0L) exposureNs = got
                }

                override fun onCaptureFailed(
                    session: CameraCaptureSession,
                    request: CaptureRequest,
                    failure: android.hardware.camera2.CaptureFailure,
                ) {
                    failed = true
                    latch.countDown()
                }
            }, handler)
            val waitMs = exposureUs / 1000L + 8000L
            if (!latch.await(waitMs, TimeUnit.MILLISECONDS)) return null
            val bytes = jpeg
            if (failed || bytes == null) return null
            SkyShot(bytes, (exposureNs / 1000L).toInt().coerceAtLeast(1))
        } catch (_: Exception) {
            null
        }
    }

    private fun <T> CaptureRequest.Builder.put(key: CaptureRequest.Key<T>, value: T) {
        try {
            set(key, value)
        } catch (_: Exception) {
        }
    }

    fun close() {
        try {
            session?.close()
        } catch (_: Exception) {
        }
        session = null
        try {
            device?.close()
        } catch (_: Exception) {
        }
        device = null
        try {
            reader?.close()
        } catch (_: Exception) {
        }
        reader = null
    }

    /** Samsung's own range reaches several seconds; the public Camera2 range stops at 0.1 s. */
    private fun samsungExposureUpperNs(chars: CameraCharacteristics): Long? {
        return try {
            val key = CameraCharacteristics.Key(
                "samsung.android.sensor.info.exposureTimeRange",
                LongArray::class.java,
            )
            val range = chars.get(key) ?: return null
            if (range.size < 2 || range[1] <= range[0]) null else range[1]
        } catch (_: Exception) {
            null
        }
    }

    fun release() {
        close()
        thread.quitSafely()
    }
}

class SkyShot(val jpeg: ByteArray, val exposureUs: Int)

private val samsungExposureTimeKey = CaptureRequest.Key(
    "samsung.android.sensor.exposureTimeInternal",
    Long::class.javaObjectType,
)
