package com.teenastro.teenastro_app

import android.content.Context
import android.graphics.Bitmap
import android.graphics.ImageFormat
import android.hardware.camera2.CameraCaptureSession
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CameraManager
import android.hardware.camera2.CaptureRequest
import android.hardware.camera2.CaptureResult
import android.hardware.camera2.TotalCaptureResult
import android.media.ImageReader
import android.os.Handler
import android.os.HandlerThread
import android.util.Range
import android.view.Surface
import java.io.ByteArrayOutputStream
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicInteger
import java.util.concurrent.atomic.AtomicLong

/**
 * Sky still for phones that give apps no manual shutter (Samsung LIMITED
 * cameras ignore SENSOR_EXPOSURE_TIME and SENSOR_SENSITIVITY). Auto-exposure
 * runs at its brightest, the slowest frame rate is requested, and full-colour
 * YUV frames are averaged until the requested time has been collected.
 * The preview camera must be closed before [open].
 */
class ManualSkyCapture(private val context: Context) {
    private val manager =
        context.getSystemService(Context.CAMERA_SERVICE) as CameraManager
    private val thread = HandlerThread("teenastro-shutter").also { it.start() }
    private val handler = Handler(thread.looper)
    private val frameThread = HandlerThread("teenastro-frames").also { it.start() }
    private val frameHandler = Handler(frameThread.looper)

    private var device: CameraDevice? = null
    private var session: CameraCaptureSession? = null
    private var reader: ImageReader? = null
    private var characteristics: CameraCharacteristics? = null
    private var zoom: Float = 1f
    private var slowFps: Range<Int>? = null
    private var aeCompMax: Int = 0

    var minUs: Int = 1
        private set
    var maxUs: Int = 300_000_000
        private set
    var minIso: Int = 100
        private set
    var maxIso: Int = 100
        private set

    fun open(cameraId: String, zoomRatio: Float): Boolean {
        close()
        zoom = zoomRatio
        val chars = try {
            manager.getCameraCharacteristics(cameraId)
        } catch (_: Exception) {
            return false
        }
        val sensitivity = chars.get(CameraCharacteristics.SENSOR_INFO_SENSITIVITY_RANGE)
        minIso = sensitivity?.lower ?: 100
        maxIso = (sensitivity?.upper ?: 100).coerceAtLeast(minIso)
        val sizes = chars.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
            ?.getOutputSizes(ImageFormat.YUV_420_888)
        if (sizes.isNullOrEmpty()) return false
        slowFps = chars.get(CameraCharacteristics.CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES)
            ?.minWithOrNull(compareBy<Range<Int>> { it.lower }.thenBy { it.upper })
        aeCompMax = chars.get(CameraCharacteristics.CONTROL_AE_COMPENSATION_RANGE)?.upper ?: 0
        characteristics = chars

        // 16:9 like the preview, binned about 2×2 by the camera (1920×1080 here).
        val size = CameraLens.binnedSize(sizes) ?: return false
        val imageReader =
            ImageReader.newInstance(size.width, size.height, ImageFormat.YUV_420_888, 4)
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

    private fun skyRequest(target: Surface): CaptureRequest? {
        val camera = device ?: return null
        return try {
            camera.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE).apply {
                addTarget(target)
                put(CaptureRequest.CONTROL_MODE, CaptureRequest.CONTROL_MODE_AUTO)
                put(CaptureRequest.CONTROL_AE_MODE, CaptureRequest.CONTROL_AE_MODE_ON)
                put(CaptureRequest.CONTROL_AE_EXPOSURE_COMPENSATION, aeCompMax)
                slowFps?.let { put(CaptureRequest.CONTROL_AE_TARGET_FPS_RANGE, it) }
                put(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_OFF)
                put(CaptureRequest.LENS_FOCUS_DISTANCE, 0.0f)
                put(CaptureRequest.CONTROL_AWB_MODE, CaptureRequest.CONTROL_AWB_MODE_AUTO)
                put(CaptureRequest.FLASH_MODE, CaptureRequest.FLASH_MODE_OFF)
                // Noise reduction erases faint stars as noise; averaging does that job.
                put(CaptureRequest.NOISE_REDUCTION_MODE, CaptureRequest.NOISE_REDUCTION_MODE_OFF)
                put(CaptureRequest.EDGE_MODE, CaptureRequest.EDGE_MODE_OFF)
                characteristics?.let { CameraLens.applyZoom(this, it, zoom) }
            }.build()
        } catch (_: Exception) {
            null
        }
    }

    /**
     * Average steady frames for [exposureUs] of sensor time (at least one).
     * Shaky frames (gyro jolt or tip) are dropped and do not count toward the
     * total. [iso] is ignored: this camera chooses the gain itself.
     */
    @Suppress("UNUSED_PARAMETER")
    fun capture(exposureUs: Int, iso: Int): SkyShot? {
        val captureSession = session ?: return null
        val imageReader = reader ?: return null
        val request = skyRequest(imageReader.surface) ?: return null
        val stacker = SkyStacker(imageReader.width, imageReader.height)
        val wantedNs = exposureUs.toLong().coerceAtLeast(1L) * 1000L
        val collectedNs = AtomicLong(0L)
        val lastExposureNs = AtomicLong(0L)
        val counted = AtomicInteger(0)
        val shaken = AtomicInteger(0)
        val done = CountDownLatch(1)
        val armed = java.util.concurrent.atomic.AtomicBoolean(false)
        // Let auto-exposure settle in the dark before frames count.
        val settleUntil = System.nanoTime() + 1_500_000_000L
        val shake = ShakeGuard(context)
        shake.start()

        while (true) {
            val stale = imageReader.acquireNextImage() ?: break
            stale.close()
        }
        imageReader.setOnImageAvailableListener({ r ->
            val image = r.acquireNextImage() ?: return@setOnImageAvailableListener
            try {
                if (done.count == 0L) return@setOnImageAvailableListener
                if (System.nanoTime() < settleUntil) return@setOnImageAvailableListener
                if (armed.compareAndSet(false, true)) shake.arm()
                if (shake.isShaky()) {
                    shaken.incrementAndGet()
                    return@setOnImageAvailableListener
                }
                stacker.add(image)
                counted.incrementAndGet()
                val exp = lastExposureNs.get().takeIf { it > 0 } ?: 100_000_000L
                if (collectedNs.addAndGet(exp) >= wantedNs) done.countDown()
            } finally {
                image.close()
            }
        }, frameHandler)

        val callback = object : CameraCaptureSession.CaptureCallback() {
            override fun onCaptureCompleted(
                session: CameraCaptureSession,
                request: CaptureRequest,
                result: TotalCaptureResult,
            ) {
                val got = result.get(CaptureResult.SENSOR_EXPOSURE_TIME)
                if (got != null && got > 0L) lastExposureNs.set(got)
            }
        }
        try {
            captureSession.setRepeatingRequest(request, callback, handler)
        } catch (_: Exception) {
            imageReader.setOnImageAvailableListener(null, null)
            shake.stop()
            return null
        }
        // Extra headroom when wind drops many frames.
        val waitMs = wantedNs / 1_000_000L * 6 + 20_000L
        done.await(waitMs, TimeUnit.MILLISECONDS)
        try {
            captureSession.stopRepeating()
        } catch (_: Exception) {
        }
        imageReader.setOnImageAvailableListener(null, null)
        shake.stop()
        // Let an in-flight frame finish on the frame thread.
        val drained = CountDownLatch(1)
        frameHandler.post { drained.countDown() }
        drained.await(2, TimeUnit.SECONDS)
        val frames = counted.get()
        if (frames == 0) return null

        val y16 = stacker.resultY16()
        val bitmap = stacker.result()
        val stream = ByteArrayOutputStream()
        bitmap.compress(Bitmap.CompressFormat.JPEG, 95, stream)
        bitmap.recycle()
        val totalUs = (collectedNs.get() / 1000L).coerceIn(1L, Int.MAX_VALUE.toLong()).toInt()
        return SkyShot(
            stream.toByteArray(),
            y16,
            imageReader.width,
            imageReader.height,
            totalUs,
            frames,
            stacker.aligned,
            shaken.get(),
        )
    }

    private fun <T> CaptureRequest.Builder.put(key: CaptureRequest.Key<T>, value: T) {
        try {
            set(key, value)
        } catch (_: Exception) {
        }
    }

    /** Gallery copy under Pictures/TeenAstro, for later processing. */
    fun save(bytes: ByteArray, mime: String, extension: String): String? {
        val name = "TeenAstro_" +
            java.text.SimpleDateFormat("yyyyMMdd_HHmmss", java.util.Locale.US)
                .format(java.util.Date()) + ".$extension"
        val values = android.content.ContentValues().apply {
            put(android.provider.MediaStore.Images.Media.DISPLAY_NAME, name)
            put(android.provider.MediaStore.Images.Media.MIME_TYPE, mime)
            put(android.provider.MediaStore.Images.Media.RELATIVE_PATH, "Pictures/TeenAstro")
            put(android.provider.MediaStore.Images.Media.IS_PENDING, 1)
        }
        val uri = context.contentResolver.insert(
            android.provider.MediaStore.Images.Media.EXTERNAL_CONTENT_URI,
            values,
        ) ?: return null
        return try {
            context.contentResolver.openOutputStream(uri)?.use { it.write(bytes) }
            values.clear()
            values.put(android.provider.MediaStore.Images.Media.IS_PENDING, 0)
            context.contentResolver.update(uri, values, null, null)
            name
        } catch (_: Exception) {
            context.contentResolver.delete(uri, null, null)
            null
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
        characteristics = null
    }

    fun release() {
        close()
        thread.quitSafely()
        frameThread.quitSafely()
    }
}

/**
 * [jpeg] is 8-bit for display only; [y16] is little-endian mean×256 luma for
 * solving. [exposureUs] is the total collected; [frames] how many steady sensor
 * frames made it; [aligned] how many of those were registered on the reference
 * stars; [shaken] how many frames the IMU vetoed.
 */
class SkyShot(
    val jpeg: ByteArray,
    val y16: ByteArray,
    val width: Int,
    val height: Int,
    val exposureUs: Int,
    val frames: Int,
    val aligned: Int,
    val shaken: Int,
)
