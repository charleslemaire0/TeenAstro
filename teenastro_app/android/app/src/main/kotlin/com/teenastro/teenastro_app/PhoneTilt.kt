package com.teenastro.teenastro_app

import android.content.Context
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import kotlin.math.asin
import kotlin.math.sqrt

/**
 * Back-camera altitude from the gravity vector. Android device +Z comes out of
 * the screen, so the back lens looks along −Z. When the phone rests face-down
 * (lens at the zenith), gz is negative and the altitude is +90°.
 */
object PhoneTilt {
    /**
     * Altitude of the back camera axis above the horizon, in degrees, or null
     * when no gravity sample arrives in time.
     */
    fun cameraAltitudeDeg(context: Context, waitMs: Long = 400L): Double? {
        val manager = context.getSystemService(Context.SENSOR_SERVICE) as SensorManager
        val sensor = manager.getDefaultSensor(Sensor.TYPE_GRAVITY)
            ?: manager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER)
            ?: return null
        val latch = CountDownLatch(1)
        var sample: FloatArray? = null
        val listener = object : SensorEventListener {
            override fun onSensorChanged(event: SensorEvent) {
                if (sample != null) return
                sample = event.values.copyOf(3)
                latch.countDown()
            }

            override fun onAccuracyChanged(sensor: Sensor?, accuracy: Int) {}
        }
        return try {
            manager.registerListener(listener, sensor, SensorManager.SENSOR_DELAY_GAME)
            latch.await(waitMs, TimeUnit.MILLISECONDS)
            altitudeFromGravity(sample)
        } finally {
            manager.unregisterListener(listener)
        }
    }

    /** Pure math for tests: [g] is the Android gravity/accelerometer sample. */
    fun altitudeFromGravity(g: FloatArray?): Double? {
        if (g == null || g.size < 3) return null
        val gx = g[0].toDouble()
        val gy = g[1].toDouble()
        val gz = g[2].toDouble()
        val norm = sqrt(gx * gx + gy * gy + gz * gz)
        if (norm < 1.0) return null
        // Camera axis (−Z) dotted with “up” (g/|g|) → sin(altitude).
        return asin((-gz / norm).coerceIn(-1.0, 1.0)) * 180.0 / Math.PI
    }
}
