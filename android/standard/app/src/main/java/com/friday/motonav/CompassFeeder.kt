package com.friday.motonav

import android.annotation.SuppressLint
import android.content.Context
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.location.Location
import android.location.LocationListener
import android.location.LocationManager
import android.os.Bundle
import android.os.Looper
import android.util.Log

/**
 * Feeds the round screen's compass page while it is visible:
 *   #C,<heading>,<kmh>
 * Heading comes from the phone's rotation-vector sensor (works standing still);
 * speed comes from the best available location provider. Started/stopped by the
 * firmware's T,C,1 / T,C,0 events.
 */
@SuppressLint("MissingPermission")
object CompassFeeder {
    private const val TAG = "MotoNavCompass"
    private var ctx: Context? = null
    private var sensorManager: SensorManager? = null
    private var locationManager: LocationManager? = null
    private var sensor: Sensor? = null
    private var running = false
    private var heading = -1f
    private var speedMps = 0f
    private var lastSend = 0L

    private val sensorListener = object : SensorEventListener {
        override fun onSensorChanged(event: SensorEvent?) {
            val e = event ?: return
            val rot = FloatArray(9)
            SensorManager.getRotationMatrixFromVector(rot, e.values)
            val ori = FloatArray(3)
            SensorManager.getOrientation(rot, ori)
            var deg = Math.toDegrees(ori[0].toDouble()).toFloat()
            if (deg < 0) deg += 360f
            heading = deg
            send()
        }
        override fun onAccuracyChanged(sensor: Sensor?, accuracy: Int) {}
    }

    private val locationListener = object : LocationListener {
        override fun onLocationChanged(location: Location) {
            if (location.hasSpeed()) speedMps = location.speed
            if (heading < 0 && location.hasBearing()) heading = location.bearing
            send()
        }
        @Deprecated("Deprecated in Java")
        override fun onStatusChanged(provider: String?, status: Int, extras: Bundle?) {}
        override fun onProviderEnabled(provider: String) {}
        override fun onProviderDisabled(provider: String) {}
    }

    fun start(context: Context) {
        if (running) return
        ctx = context.applicationContext
        running = true
        sensorManager = ctx!!.getSystemService(Context.SENSOR_SERVICE) as? SensorManager
        sensor = sensorManager?.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR)
            ?: sensorManager?.getDefaultSensor(Sensor.TYPE_GEOMAGNETIC_ROTATION_VECTOR)
        if (sensor != null) sensorManager?.registerListener(sensorListener, sensor, SensorManager.SENSOR_DELAY_UI)
        locationManager = ctx!!.getSystemService(Context.LOCATION_SERVICE) as? LocationManager
        try {
            for (p in listOf(LocationManager.GPS_PROVIDER, LocationManager.NETWORK_PROVIDER)) {
                if (locationManager?.isProviderEnabled(p) == true) {
                    locationManager?.requestLocationUpdates(p, 1000L, 0f, locationListener, Looper.getMainLooper())
                }
            }
        } catch (e: Exception) {
            Log.w(TAG, "location start failed: ${e.message}")
        }
        Log.i(TAG, "start sensor=${sensor?.name ?: "none"}")
    }

    fun stop() {
        if (!running) return
        running = false
        try { sensorManager?.unregisterListener(sensorListener) } catch (_: Exception) {}
        try { locationManager?.removeUpdates(locationListener) } catch (_: Exception) {}
        sensorManager = null; locationManager = null; sensor = null
        heading = -1f; speedMps = 0f
        Log.i(TAG, "stop")
    }

    private fun send() {
        val now = System.currentTimeMillis()
        if (now - lastSend < 400) return
        lastSend = now
        val h = if (heading.isFinite() && heading >= 0f) heading.toInt() % 360 else -1
        val kmh = (speedMps * 3.6f).coerceIn(0f, 300f)
        BleLink.send("#C,$h,${String.format(java.util.Locale.US, "%.1f", kmh)}")
    }
}
