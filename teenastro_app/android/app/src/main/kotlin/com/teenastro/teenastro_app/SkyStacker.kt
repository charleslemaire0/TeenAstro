package com.teenastro.teenastro_app

import android.graphics.Bitmap
import android.media.Image
import kotlin.math.abs
import kotlin.math.roundToInt
import kotlin.math.sqrt

/**
 * Aligns YUV frames on their stars and adds them. The first frame with enough
 * stars is the reference; every later frame is shifted onto it before adding.
 * Only translation is solved: over a few tens of seconds on a tripod, field
 * rotation is below a pixel at the frame edge.
 */
class SkyStacker(private val w: Int, private val h: Int) {
    private val cw = w / 2
    private val ch = h / 2
    private val sumY = IntArray(w * h)
    private val sumU = IntArray(cw * ch)
    private val sumV = IntArray(cw * ch)
    private val count = ShortArray(w * h)

    private var yBytes = ByteArray(0)
    private var uBytes = ByteArray(0)
    private var vBytes = ByteArray(0)
    private val binned = FloatArray((w / BIN) * (h / BIN))

    private var reference: List<Star>? = null
    private var lastDx = 0
    private var lastDy = 0

    var frames = 0
        private set
    var aligned = 0
        private set

    private class Star(val x: Float, val y: Float, val flux: Float)

    fun add(image: Image) {
        val planes = image.planes
        val yPlane = planes[0]
        val yRow = yPlane.rowStride
        val yPix = yPlane.pixelStride
        yBytes = copy(yPlane.buffer, yBytes)
        val uPlane = planes[1]
        val vPlane = planes[2]
        uBytes = copy(uPlane.buffer, uBytes)
        vBytes = copy(vPlane.buffer, vBytes)

        val stars = detect(yRow, yPix)
        val ref = reference
        var dx = 0
        var dy = 0
        if (ref == null) {
            if (stars.size >= MIN_STARS) reference = stars
        } else {
            val shift = match(ref, stars)
            if (shift != null) {
                dx = shift.first
                dy = shift.second
                lastDx = dx
                lastDy = dy
                aligned++
            } else {
                // Too few stars this frame: assume the drift has not changed.
                dx = lastDx
                dy = lastDy
            }
        }

        for (y in 0 until h) {
            val sy = y - dy
            if (sy < 0 || sy >= h) continue
            val src = sy * yRow
            val out = y * w
            for (x in 0 until w) {
                val sx = x - dx
                if (sx < 0 || sx >= w) continue
                sumY[out + x] += yBytes[src + sx * yPix].toInt() and 0xff
                count[out + x]++
            }
        }
        val cdx = dx / 2
        val cdy = dy / 2
        val uRow = uPlane.rowStride
        val vRow = vPlane.rowStride
        val uPix = uPlane.pixelStride
        val vPix = vPlane.pixelStride
        for (y in 0 until ch) {
            val sy = y - cdy
            if (sy < 0 || sy >= ch) continue
            val out = y * cw
            for (x in 0 until cw) {
                val sx = x - cdx
                if (sx < 0 || sx >= cw) continue
                val ui = sy * uRow + sx * uPix
                val vi = sy * vRow + sx * vPix
                if (ui < uBytes.size) sumU[out + x] += uBytes[ui].toInt() and 0xff
                if (vi < vBytes.size) sumV[out + x] += vBytes[vi].toInt() and 0xff
            }
        }
        frames++
    }

    /**
     * Mean luma as little-endian u16, scaled by 256 so fractional ADUs from
     * averaging survive (mean 12.4 → 3174). Star detection uses this buffer;
     * the 8-bit JPEG is only for on-screen display.
     */
    fun resultY16(): ByteArray {
        val out = ByteArray(w * h * 2)
        var o = 0
        for (i in 0 until w * h) {
            val n = count[i].toInt()
            val v =
                if (n == 0) 0
                else ((sumY[i].toLong() * 256L) / n).toInt().coerceIn(0, 65535)
            out[o++] = (v and 0xff).toByte()
            out[o++] = (v shr 8 and 0xff).toByte()
        }
        return out
    }

    /** The mean frame as an RGB bitmap (8-bit, for the preview JPEG only). */
    fun result(): Bitmap {
        val bitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888)
        val row = IntArray(w)
        for (y in 0 until h) {
            val cRow = (y / 2).coerceAtMost(ch - 1) * cw
            for (x in 0 until w) {
                val i = y * w + x
                val n = count[i].toInt()
                if (n == 0) {
                    row[x] = 0xff000000.toInt()
                    continue
                }
                val yy = sumY[i] / n
                val ci = cRow + (x / 2).coerceAtMost(cw - 1)
                val u = sumU[ci] / n - 128
                val v = sumV[ci] / n - 128
                val r = (yy + 1.402f * v).toInt().coerceIn(0, 255)
                val g = (yy - 0.344f * u - 0.714f * v).toInt().coerceIn(0, 255)
                val b = (yy + 1.772f * u).toInt().coerceIn(0, 255)
                row[x] = (0xff shl 24) or (r shl 16) or (g shl 8) or b
            }
            bitmap.setPixels(row, 0, w, 0, y, w, 1)
        }
        return bitmap
    }

    private fun copy(buffer: java.nio.ByteBuffer, into: ByteArray): ByteArray {
        val size = buffer.remaining()
        val out = if (into.size >= size) into else ByteArray(size)
        buffer.get(out, 0, size)
        return out
    }

    /** Bright stars on a [BIN]×[BIN] binned copy, in full-resolution pixels. */
    private fun detect(yRow: Int, yPix: Int): List<Star> {
        val bw = w / BIN
        val bh = h / BIN
        for (by in 0 until bh) {
            for (bx in 0 until bw) {
                var s = 0
                for (j in 0 until BIN) {
                    val base = (by * BIN + j) * yRow + bx * BIN * yPix
                    for (i in 0 until BIN) s += yBytes[base + i * yPix].toInt() and 0xff
                }
                binned[by * bw + bx] = s.toFloat()
            }
        }
        var mean = 0.0
        for (v in binned) mean += v
        mean /= binned.size
        var variance = 0.0
        for (v in binned) {
            val d = v - mean
            variance += d * d
        }
        val sigma = sqrt(variance / binned.size).coerceAtLeast(1.0)
        val threshold = (mean + 5 * sigma).toFloat()
        val found = ArrayList<Star>()
        for (by in 1 until bh - 1) {
            for (bx in 1 until bw - 1) {
                val c = binned[by * bw + bx]
                if (c < threshold) continue
                var peak = true
                loop@ for (j in -1..1) for (i in -1..1) {
                    if ((i != 0 || j != 0) && binned[(by + j) * bw + bx + i] > c) {
                        peak = false
                        break@loop
                    }
                }
                if (!peak) continue
                var sw = 0f
                var sx = 0f
                var sy = 0f
                for (j in -1..1) for (i in -1..1) {
                    val v = binned[(by + j) * bw + bx + i] - mean.toFloat()
                    if (v <= 0f) continue
                    sw += v
                    sx += v * (bx + i)
                    sy += v * (by + j)
                }
                if (sw <= 0f) continue
                found.add(
                    Star(
                        (sx / sw + 0.5f) * BIN - 0.5f,
                        (sy / sw + 0.5f) * BIN - 0.5f,
                        sw,
                    ),
                )
            }
        }
        found.sortByDescending { it.flux }
        return if (found.size > MAX_STARS) found.subList(0, MAX_STARS) else found
    }

    /** Shift that carries [cur] onto [ref], by voting over all star pairs. */
    private fun match(ref: List<Star>, cur: List<Star>): Pair<Int, Int>? {
        if (cur.size < MIN_STARS) return null
        val votes = HashMap<Long, Int>()
        for (r in ref) for (c in cur) {
            val dx = r.x - c.x
            val dy = r.y - c.y
            if (abs(dx) > MAX_SHIFT || abs(dy) > MAX_SHIFT) continue
            val key = ((dx / VOTE).roundToInt().toLong() shl 32) or
                ((dy / VOTE).roundToInt().toLong() and 0xffffffffL)
            votes[key] = (votes[key] ?: 0) + 1
        }
        val best = votes.maxByOrNull { it.value } ?: return null
        if (best.value < MIN_MATCHES) return null
        val bx = (best.key shr 32).toInt() * VOTE
        val by = best.key.toInt() * VOTE
        var sx = 0f
        var sy = 0f
        var n = 0
        for (r in ref) for (c in cur) {
            val dx = r.x - c.x
            val dy = r.y - c.y
            if (abs(dx - bx) <= VOTE && abs(dy - by) <= VOTE) {
                sx += dx
                sy += dy
                n++
            }
        }
        if (n < MIN_MATCHES) return null
        return Pair((sx / n).roundToInt(), (sy / n).roundToInt())
    }

    private companion object {
        const val BIN = 2
        const val MAX_STARS = 40
        const val MIN_STARS = 4
        const val MIN_MATCHES = 3
        const val MAX_SHIFT = 200f
        const val VOTE = 3f
    }
}
