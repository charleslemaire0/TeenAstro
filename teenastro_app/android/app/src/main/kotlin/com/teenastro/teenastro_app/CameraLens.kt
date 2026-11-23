package com.teenastro.teenastro_app

import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CaptureRequest
import android.util.Size
import kotlin.math.PI
import kotlin.math.abs
import kotlin.math.atan
import kotlin.math.max
import kotlin.math.min

/** Prefer the phone's 1× main lens, not the 0.5× ultra-wide. */
object CameraLens {
    private const val mainZoom = 1.0f
    private const val targetFovDeg = 70.0
    /** Same framing as the live preview, which is 16:9. */
    private const val previewAspect = 16.0 / 9.0

    /** [zoom] clamped to what the camera supports. */
    fun zoomRatio(chars: CameraCharacteristics, zoom: Float = mainZoom): Float? {
        val range = chars.get(CameraCharacteristics.CONTROL_ZOOM_RATIO_RANGE) ?: return null
        return zoom.coerceIn(range.lower, range.upper)
    }

    fun applyZoom(builder: CaptureRequest.Builder, chars: CameraCharacteristics, zoom: Float) {
        val ratio = zoomRatio(chars, zoom) ?: return
        try {
            builder.set(CaptureRequest.CONTROL_ZOOM_RATIO, ratio)
        } catch (_: Exception) {
        }
    }

    /**
     * Focal length for the 1× lens: among the advertised focals, the one whose
     * longer-side field is closest to a normal phone main camera (~70°).
     */
    fun mainFocalMm(chars: CameraCharacteristics): Double? {
        val size = chars.get(CameraCharacteristics.SENSOR_INFO_PHYSICAL_SIZE) ?: return null
        val focals = chars.get(CameraCharacteristics.LENS_INFO_AVAILABLE_FOCAL_LENGTHS)
        if (focals == null || focals.isEmpty()) return null
        val side = max(size.width.toDouble(), size.height.toDouble())
        if (side <= 0.0) return null
        return focals.minByOrNull { f ->
            val deg = 2.0 * atan(side / (2.0 * f.toDouble())) * 180.0 / PI
            abs(deg - targetFovDeg)
        }?.toDouble()
    }

    /** Longer-side field at [zoom] (digital zoom divides the half-angle tangent). */
    fun longerSideFovDeg(chars: CameraCharacteristics, zoom: Float = mainZoom): Double? {
        val size = chars.get(CameraCharacteristics.SENSOR_INFO_PHYSICAL_SIZE) ?: return null
        val focal = mainFocalMm(chars) ?: return null
        if (focal <= 0.0) return null
        val side = max(size.width.toDouble(), size.height.toDouble())
        if (side <= 0.0) return null
        val ratio = (zoomRatio(chars, zoom) ?: 1f).toDouble()
        val deg = 2.0 * atan(side / (2.0 * focal * ratio)) * 180.0 / PI
        if (deg < 5.0 || deg > 170.0) return null
        return deg
    }

    private fun aspect(size: Size): Double {
        val w = size.width.toDouble()
        val h = size.height.toDouble()
        return max(w, h) / min(w, h)
    }

    private fun sixteenNine(sizes: Array<Size>): List<Size> {
        val matched = sizes.filter { abs(aspect(it) - previewAspect) < 0.04 }
        return if (matched.isNotEmpty()) matched else sizes.toList()
    }

    /**
     * Largest still whose shape matches the 16:9 preview. The 4:3 12 MP frame
     * is the same lens but a taller field than the video.
     */
    fun stillSize(sizes: Array<Size>): Size? {
        if (sizes.isEmpty()) return null
        return sixteenNine(sizes).maxByOrNull { it.width.toLong() * it.height.toLong() }
    }

    /**
     * The 16:9 stream nearest half the full frame on each side, so the camera's
     * image processor averages about 2×2 sensor pixels into each one.
     */
    fun binnedSize(sizes: Array<Size>): Size? {
        val full = stillSize(sizes) ?: return null
        val target = full.width.toLong() * full.height.toLong() / 4
        val pool = sixteenNine(sizes)
        return pool.filter { it.width.toLong() * it.height.toLong() <= target * 105 / 100 }
            .maxByOrNull { it.width.toLong() * it.height.toLong() }
            ?: pool.minByOrNull { it.width.toLong() * it.height.toLong() }
    }
}
