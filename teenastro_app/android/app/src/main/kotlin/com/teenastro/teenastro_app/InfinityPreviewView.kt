package com.teenastro.teenastro_app

import android.app.Activity
import android.content.Context
import android.graphics.Matrix
import android.graphics.SurfaceTexture
import android.hardware.camera2.CameraCaptureSession
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CameraManager
import android.hardware.camera2.CaptureRequest
import android.hardware.camera2.params.TonemapCurve
import android.os.Handler
import android.os.HandlerThread
import android.util.Size
import android.view.Surface
import android.view.TextureView
import android.view.View
import io.flutter.plugin.common.StandardMessageCodec
import io.flutter.plugin.platform.PlatformView
import io.flutter.plugin.platform.PlatformViewFactory
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import kotlin.math.abs
import kotlin.math.max

/**
 * Live camera preview on a TextureView, lens fixed at infinity.
 * Uses the Camera2Basic transform so the buffer is upright and center-cropped.
 */
class InfinityPreviewView(
    private val activity: Activity,
    private val cameraId: String,
    @Volatile private var gamma: Float,
) : PlatformView, TextureView.SurfaceTextureListener {
    companion object {
        @Volatile
        var current: InfinityPreviewView? = null
    }

    private var previewSurface: Surface? = null
    private var toneModes: IntArray = IntArray(0)
    private var maxCurvePoints: Int = 0
    private var aeCompMax: Int = 0
    private val textureView = TextureView(activity)
    private val manager = activity.getSystemService(Context.CAMERA_SERVICE) as CameraManager
    private val thread = HandlerThread("teenastro-preview-view").also { it.start() }
    private val handler = Handler(thread.looper)

    private var device: CameraDevice? = null
    private var session: CameraCaptureSession? = null
    private var previewSize: Size = Size(1280, 720)
    private var sensorOrientation: Int = 90

    init {
        textureView.surfaceTextureListener = this
        try {
            val chars = manager.getCameraCharacteristics(cameraId)
            sensorOrientation = chars.get(CameraCharacteristics.SENSOR_ORIENTATION) ?: 90
            toneModes = chars.get(CameraCharacteristics.TONEMAP_AVAILABLE_TONE_MAP_MODES)
                ?: IntArray(0)
            maxCurvePoints = chars.get(CameraCharacteristics.TONEMAP_MAX_CURVE_POINTS) ?: 0
            aeCompMax = chars.get(CameraCharacteristics.CONTROL_AE_COMPENSATION_RANGE)?.upper ?: 0
            val sizes = chars.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
                ?.getOutputSizes(SurfaceTexture::class.java)
            if (!sizes.isNullOrEmpty()) {
                previewSize = sizes.minByOrNull { abs(max(it.width, it.height) - 1280) }
                    ?: previewSize
            }
        } catch (_: Exception) {
        }
    }

    override fun getView(): View = textureView

    override fun dispose() {
        if (current === this) current = null
        closeCamera()
        thread.quitSafely()
    }

    /** Change the display gamma of the running preview. */
    fun setGamma(value: Float) {
        gamma = value
        handler.post { startRepeating() }
    }

    private fun startRepeating() {
        val camera = device ?: return
        val captureSession = session ?: return
        val surface = previewSurface ?: return
        try {
            val request = camera.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW).apply {
                addTarget(surface)
                try {
                    set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_OFF)
                    set(CaptureRequest.LENS_FOCUS_DISTANCE, 0f)
                } catch (_: Exception) {
                }
                try {
                    set(CaptureRequest.CONTROL_AE_MODE, CaptureRequest.CONTROL_AE_MODE_ON)
                    set(CaptureRequest.CONTROL_AWB_MODE, CaptureRequest.CONTROL_AWB_MODE_AUTO)
                    set(CaptureRequest.FLASH_MODE, CaptureRequest.FLASH_MODE_OFF)
                    // Dark sky: let auto-exposure brighten as much as it can.
                    set(CaptureRequest.CONTROL_AE_EXPOSURE_COMPENSATION, aeCompMax)
                } catch (_: Exception) {
                }
                applyTone(this)
            }.build()
            captureSession.setRepeatingRequest(request, null, handler)
        } catch (_: Exception) {
        }
    }

    private fun applyTone(builder: CaptureRequest.Builder) {
        val g = gamma
        if (g <= 1.01f) return
        try {
            if (toneModes.contains(CaptureRequest.TONEMAP_MODE_GAMMA_VALUE)) {
                builder.set(CaptureRequest.TONEMAP_MODE, CaptureRequest.TONEMAP_MODE_GAMMA_VALUE)
                builder.set(CaptureRequest.TONEMAP_GAMMA, g)
            } else if (toneModes.contains(CaptureRequest.TONEMAP_MODE_CONTRAST_CURVE) &&
                maxCurvePoints >= 2
            ) {
                val n = minOf(maxCurvePoints, 64)
                val curve = FloatArray(n * 2)
                for (i in 0 until n) {
                    val x = i / (n - 1f)
                    curve[2 * i] = x
                    curve[2 * i + 1] = Math.pow(x.toDouble(), 1.0 / g).toFloat()
                }
                builder.set(CaptureRequest.TONEMAP_MODE, CaptureRequest.TONEMAP_MODE_CONTRAST_CURVE)
                builder.set(CaptureRequest.TONEMAP_CURVE, TonemapCurve(curve, curve, curve))
            }
        } catch (_: Exception) {
        }
    }

    override fun onSurfaceTextureAvailable(surface: SurfaceTexture, width: Int, height: Int) {
        openCamera(surface)
        configureTransform(width, height)
    }

    override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture, width: Int, height: Int) {
        configureTransform(width, height)
    }

    override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean {
        closeCamera()
        return true
    }

    override fun onSurfaceTextureUpdated(surface: SurfaceTexture) {}

    private fun openCamera(texture: SurfaceTexture) {
        closeCamera()
        if (activity.checkSelfPermission(android.Manifest.permission.CAMERA) !=
            android.content.pm.PackageManager.PERMISSION_GRANTED
        ) {
            return
        }
        texture.setDefaultBufferSize(previewSize.width, previewSize.height)
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
        } catch (_: Exception) {
            previewSurface.release()
            return
        }
        if (!opened.await(4, TimeUnit.SECONDS) || camera == null) {
            camera?.close()
            previewSurface.release()
            return
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
            return
        }
        if (!configured.await(4, TimeUnit.SECONDS) || made == null) {
            camera?.close()
            previewSurface.release()
            return
        }
        device = camera
        session = made
        this.previewSurface = previewSurface
        startRepeating()
        activity.runOnUiThread {
            configureTransform(textureView.width, textureView.height)
        }
    }

    private fun closeCamera() {
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
        previewSurface?.release()
        previewSurface = null
    }

    /**
     * The stream arrives already rotated upright, then stretched to the view.
     * Shrink one axis so the whole upright frame shows undistorted (letterboxed).
     */
    private fun configureTransform(viewWidth: Int, viewHeight: Int) {
        if (viewWidth == 0 || viewHeight == 0) return
        val displayDeg = when (activity.windowManager.defaultDisplay.rotation) {
            Surface.ROTATION_90 -> 90
            Surface.ROTATION_180 -> 180
            Surface.ROTATION_270 -> 270
            else -> 0
        }
        val rotate = (sensorOrientation - displayDeg + 360) % 360
        val swap = rotate == 90 || rotate == 270
        val contentW = (if (swap) previewSize.height else previewSize.width).toFloat()
        val contentH = (if (swap) previewSize.width else previewSize.height).toFloat()
        val contentAspect = contentW / contentH
        val viewAspect = viewWidth.toFloat() / viewHeight
        val matrix = Matrix()
        if (contentAspect > viewAspect) {
            matrix.setScale(1f, viewAspect / contentAspect, viewWidth / 2f, viewHeight / 2f)
        } else {
            matrix.setScale(contentAspect / viewAspect, 1f, viewWidth / 2f, viewHeight / 2f)
        }
        textureView.setTransform(matrix)
    }
}

class InfinityPreviewFactory(private val activity: Activity) :
    PlatformViewFactory(StandardMessageCodec.INSTANCE) {
    override fun create(context: Context, viewId: Int, args: Any?): PlatformView {
        val params = args as? Map<*, *>
        val cameraId = params?.get("cameraId") as? String ?: "0"
        val gamma = (params?.get("gamma") as? Number)?.toFloat() ?: 1f
        return InfinityPreviewView(activity, cameraId, gamma).also {
            InfinityPreviewView.current = it
        }
    }
}
