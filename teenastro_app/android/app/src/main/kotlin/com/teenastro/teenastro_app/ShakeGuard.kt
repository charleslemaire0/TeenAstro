package com.teenastro.teenastro_app

import android.content.Context
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import kotlin.math.acos
import kotlin.math.sqrt

/**
 * High-rate motion veto while stacking. Gyroscope catches short jolts;
 * gravity angle from the armed reference catches a lasting tip. Either marks
 * a hold window so the next camera frames are dropped.
 */
class ShakeGuard(context: Context) : SensorEventListener {
    private val manager =
        context.getSystemService(Context.SENSOR_SERVICE) as SensorManager
    private val gyro = manager.getDefaultSensor(Sensor.TYPE_GYROSCOPE)
    private val gravity = manager.getDefaultSensor(Sensor.TYPE_GRAVITY)
        ?: manager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER)

    @Volatile private var armed = false
    @Volatile private var shakeUntilNs = 0L
    private var refX = 0.0
    private var refY = 0.0
    private var refZ = 0.0
    private var haveRef = false
    private val lock = Any()

    fun start() {
        armed = false
        haveRef = false
        shakeUntilNs = 0L
        val rate = SensorManager.SENSOR_DELAY_FASTEST
        gyro?.let { manager.registerListener(this, it, rate) }
        gravity?.let { manager.registerListener(this, it, rate) }
    }

    /** Lock the current gravity direction after auto-exposure has settled. */
    fun arm() {
        armed = true
    }

    fun stop() {
        armed = false
        manager.unregisterListener(this)
    }

    /** True while a jolt is in the hold window or the tip exceeds the limit. */
    fun isShaky(): Boolean {
        if (!armed) return false
        if (System.nanoTime() < shakeUntilNs) return true
        return false
    }

    override fun onSensorChanged(event: SensorEvent) {
        if (!armed) {
            if (event.sensor.type == Sensor.TYPE_GRAVITY ||
                event.sensor.type == Sensor.TYPE_ACCELEROMETER
            ) {
                rememberGravity(event.values)
            }
            return
        }
        when (event.sensor.type) {
            Sensor.TYPE_GYROSCOPE -> {
                val wx = event.values[0].toDouble()
                val wy = event.values[1].toDouble()
                val wz = event.values[2].toDouble()
                val rate = sqrt(wx * wx + wy * wy + wz * wz)
                if (rate >= GYRO_SHAKE_RAD_S) markShake()
            }
            Sensor.TYPE_GRAVITY, Sensor.TYPE_ACCELEROMETER -> {
                if (tilted(event.values)) markShake()
                else rememberGravity(event.values)
            }
        }
    }

    override fun onAccuracyChanged(sensor: Sensor?, accuracy: Int) {}

    private fun markShake() {
        shakeUntilNs = System.nanoTime() + HOLD_NS
    }

    private fun rememberGravity(v: FloatArray) {
        val n = norm(v) ?: return
        synchronized(lock) {
            if (!haveRef) {
                refX = v[0] / n
                refY = v[1] / n
                refZ = v[2] / n
                haveRef = true
            }
        }
    }

    private fun tilted(v: FloatArray): Boolean {
        val n = norm(v) ?: return false
        val ux = v[0] / n
        val uy = v[1] / n
        val uz = v[2] / n
        val dot = synchronized(lock) {
            if (!haveRef) {
                refX = ux
                refY = uy
                refZ = uz
                haveRef = true
                return false
            }
            (refX * ux + refY * uy + refZ * uz).coerceIn(-1.0, 1.0)
        }
        return acos(dot) >= TILT_SHAKE_RAD
    }

    private fun norm(v: FloatArray): Double? {
        if (v.size < 3) return null
        val n = sqrt(
            v[0].toDouble() * v[0] + v[1].toDouble() * v[1] + v[2].toDouble() * v[2],
        )
        return if (n < 1.0) null else n
    }

    companion object {
        /** Instantaneous gyro rate that counts as a jolt (~2°/s). */
        const val GYRO_SHAKE_RAD_S = 2.0 * Math.PI / 180.0

        /** Tip from the armed attitude (~0.4°). */
        const val TILT_SHAKE_RAD = 0.4 * Math.PI / 180.0

        /** Keep rejecting frames this long after a jolt. */
        const val HOLD_NS = 200_000_000L
    }
}
