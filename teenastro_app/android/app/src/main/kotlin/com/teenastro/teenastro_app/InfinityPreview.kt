package com.teenastro.teenastro_app

import android.content.Context
import android.graphics.SurfaceTexture
import android.hardware.camera2.CameraCaptureSession
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CameraManager
import android.hardware.camera2.CaptureRequest
import android.os.Handler
import android.os.HandlerThread
import android.view.Surface
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import kotlin.math.abs
import kotlin.math.max

/**
 * Live preview with the lens fixed at infinity. The still-capture camera must
 * be closed while this preview runs: a phone allows only one client per camera.
 */
class InfinityPreview(context: Context) {
    private val manager =
        context.getSystemService(Context.CAMERA_SERVICE) as CameraManager
    private val thread = HandlerThread("teenastro-preview").also { it.start() }
    private val handler = Handler(thread.looper)

    private var device: CameraDevice? = null
    private var session: CameraCaptureSession? = null
    private var surface: Surface? = null

    var sensorOrientation: Int = 90
        private set
    var previewWidth: Int = 0
        private set
    var previewHeight: Int = 0
        private set

    fun start(cameraId: String, texture: SurfaceTexture): Boolean {
        stop()
        val chars = try {
            manager.getCameraCharacteristics(cameraId)
        } catch (_: Exception) {
            return false
        }
        sensorOrientation = chars.get(CameraCharacteristics.SENSOR_ORIENTATION) ?: 90
        val sizes = chars.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
            ?.getOutputSizes(SurfaceTexture::class.java)
        if (sizes.isNullOrEmpty()) return false
        val size = sizes.minByOrNull { abs(max(it.width, it.height) - 1280) } ?: return false
        previewWidth = size.width
        previewHeight = size.height
        texture.setDefaultBufferSize(size.width, size.height)
        val previewSurface = Surface(texture)

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
            previewSurface.release()
            return false
        } catch (_: Exception) {
            previewSurface.release()
            return false
        }
        if (!opened.await(4, TimeUnit.SECONDS) || camera == null) {
            camera?.close()
            previewSurface.release()
            return false
        }

        val configured = CountDownLatch(1)
        var made: CameraCaptureSession? = null
        try {
            camera!!.createCaptureSession(
                listOf(previewSurface),
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
            previewSurface.release()
            return false
        }
        if (!configured.await(4, TimeUnit.SECONDS) || made == null) {
            camera?.close()
            previewSurface.release()
            return false
        }

        val request = try {
            camera!!.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW).apply {
                addTarget(previewSurface)
                setInfinityFocus()
                put(CaptureRequest.CONTROL_AE_MODE, CaptureRequest.CONTROL_AE_MODE_ON)
                put(CaptureRequest.CONTROL_AWB_MODE, CaptureRequest.CONTROL_AWB_MODE_AUTO)
                put(CaptureRequest.FLASH_MODE, CaptureRequest.FLASH_MODE_OFF)
            }.build()
        } catch (_: Exception) {
            made?.close()
            camera?.close()
            previewSurface.release()
            return false
        }
        return try {
            made!!.setRepeatingRequest(request, null, handler)
            device = camera
            session = made
            surface = previewSurface
            true
        } catch (_: Exception) {
            made?.close()
            camera?.close()
            previewSurface.release()
            false
        }
    }

    fun stop() {
        try {
            session?.stopRepeating()
        } catch (_: Exception) {
        }
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
            surface?.release()
        } catch (_: Exception) {
        }
        surface = null
    }

    private fun CaptureRequest.Builder.setInfinityFocus() {
        put(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_OFF)
        // 0 dioptres is infinity on Camera2.
        put(CaptureRequest.LENS_FOCUS_DISTANCE, 0f)
    }

    private fun <T> CaptureRequest.Builder.put(key: CaptureRequest.Key<T>, value: T) {
        try {
            set(key, value)
        } catch (_: Exception) {
        }
    }
}
