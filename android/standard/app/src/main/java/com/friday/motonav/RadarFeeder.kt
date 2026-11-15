package com.friday.motonav

import android.content.Context
import android.util.Log
import com.amap.api.location.AMapLocationClient
import com.amap.api.location.AMapLocationClientOption
import org.json.JSONObject
import java.io.ByteArrayOutputStream
import java.net.HttpURLConnection
import java.net.URL
import java.util.Locale
import kotlin.math.abs
import kotlin.math.cos
import kotlin.math.max
import kotlin.math.min
import kotlin.math.roundToInt
import kotlin.math.sqrt

/**
 * ADS-B feeder for the round display.
 *
 * The device has no network and no absolute lat/lon, so this object polls a
 * public ADS-B API while the device's RADAR page is open, converts every
 * aircraft to dx/dy metres relative to the bike, and sends one bulk type 5
 * frame over the existing BLE link.
 *
 * Sources (no API key):
 *  - adsb.fi  https://opendata.adsb.fi/api/v3/lat/{lat}/lon/{lon}/dist/{nm}
 *    personal non-commercial use, attribution required, ~1 req/s
 *  - adsb.lol https://api.adsb.lol/v2/point/{lat}/{lon}/{nm}
 *    descriptive User-Agent required
 *
 * Note: the phone's location is GCJ-02 (AMap), ADS-B is WGS84. In mainland
 * China that is a ~500 m offset, acceptable at a 10-50 km radar range; convert
 * first if that ever matters.
 */
object RadarFeeder {
    private const val TAG = "MotoNavRadar"
    private const val MAX_PLANES = 30
    private const val POLL_MS = 2000L
    private const val RETRY_MS = 15000L

    @Volatile private var active = false
    @Volatile private var rangeM = 20000
    @Volatile private var lat = Double.NaN
    @Volatile private var lon = Double.NaN
    @Volatile private var failUntil = 0L
    @Volatile private var thread: Thread? = null
    private var locationClient: AMapLocationClient? = null
    private var seq = 0

    fun onLocation(la: Double, lo: Double) {
        lat = la
        lon = lo
    }

    /**
     * MapFeeder.onLocation only runs during navigation/cruise, so a parked bike
     * would have no coordinates. While the RADAR page is open this feeder runs
     * its own AMap location client and stops it again on exit.
     */
    fun setActive(on: Boolean, context: Context? = null) {
        if (!on) {
            active = false
            stopLocation()
            return
        }
        if (context != null) startLocation(context)
        if (active) return
        active = true
        failUntil = 0
        if (thread == null) {
            thread = Thread { run() }.apply {
                isDaemon = true
                name = "RadarFeeder"
                start()
            }
        }
    }

    private fun startLocation(context: Context) {
        if (locationClient != null) return
        try {
            val client = AMapLocationClient(context.applicationContext)
            client.setLocationOption(AMapLocationClientOption().apply {
                locationMode = AMapLocationClientOption.AMapLocationMode.Hight_Accuracy
                interval = 2000L
                isNeedAddress = false
                isLocationCacheEnable = false
                isOnceLocation = false
            })
            client.setLocationListener { fix ->
                if (fix != null && fix.errorCode == 0) onLocation(fix.latitude, fix.longitude)
            }
            client.startLocation()
            locationClient = client
            Log.i(TAG, "location client started")
        } catch (t: Throwable) {
            Log.w(TAG, "location start failed: ${t.message}")
        }
    }

    private fun stopLocation() {
        try {
            locationClient?.stopLocation()
            locationClient?.onDestroy()
        } catch (_: Throwable) {
        }
        locationClient = null
    }

    fun setRange(meters: Int) {
        if (meters in 5000..60000) rangeM = meters
    }

    private fun run() {
        while (active) {
            val t0 = System.currentTimeMillis()
            if (lat.isFinite() && lon.isFinite() && t0 >= failUntil) fetchAndSend()
            val spent = System.currentTimeMillis() - t0
            try {
                Thread.sleep(max(200L, POLL_MS - spent))
            } catch (_: InterruptedException) {
                break
            }
        }
        thread = null
    }

    private data class P(
        val dx: Int, val dy: Int, val alt: Int, val trk: Int,
        val gs: Int, val sq: Int, val flags: Int, val cs: String, val dist: Double
    )

    private fun fetchAndSend() {
        val nm = max(1, ((rangeM / 1000.0) * 0.539957).roundToInt())
        val candidates = listOf(
            String.format(Locale.US, "https://opendata.adsb.fi/api/v3/lat/%.5f/lon/%.5f/dist/%d", lat, lon, nm) to 0,
            String.format(Locale.US, "https://api.adsb.lol/v2/point/%.5f/%.5f/%d", lat, lon, nm) to 1
        )
        for ((url, source) in candidates) {
            val body = httpGet(url) ?: continue
            val planes = parse(body) ?: continue
            val payload = encode(planes, source)
            BleLink.sendBulk(5, seq++ and 0xFF, payload)
            Log.i(TAG, "sent ${planes.size} planes src=$source bytes=${payload.size}")
            return
        }
        failUntil = System.currentTimeMillis() + RETRY_MS
    }

    private fun httpGet(url: String): String? {
        val conn = try {
            (URL(url).openConnection() as HttpURLConnection).apply {
                connectTimeout = 4000
                readTimeout = 6000
                setRequestProperty("User-Agent", "MotoNav/8.3.26 (Android; personal non-commercial)")
                setRequestProperty("Accept", "application/json")
            }
        } catch (e: Exception) {
            Log.w(TAG, "open failed: ${e.message}")
            return null
        }
        return try {
            if (conn.responseCode != 200) {
                Log.w(TAG, "HTTP ${conn.responseCode} $url")
                null
            } else {
                conn.inputStream.bufferedReader().use { it.readText() }
            }
        } catch (e: Exception) {
            Log.w(TAG, "fetch failed: ${e.message}")
            null
        } finally {
            conn.disconnect()
        }
    }

    private fun parse(body: String): List<P>? {
        val root = try {
            JSONObject(body)
        } catch (_: Exception) {
            return null
        }
        val arr = root.optJSONArray("ac") ?: root.optJSONArray("aircraft") ?: return null
        val cosLat = cos(Math.toRadians(lat))
        val out = ArrayList<P>(arr.length())
        for (i in 0 until arr.length()) {
            val o = arr.optJSONObject(i) ?: continue
            val ala = o.optDouble("lat", Double.NaN)
            val alo = o.optDouble("lon", Double.NaN)
            if (!ala.isFinite() || !alo.isFinite()) continue
            val dx = ((alo - lon) * 111320.0 * cosLat).roundToInt()
            val dy = ((ala - lat) * 110540.0).roundToInt()
            if (abs(dx) > 32000 || abs(dy) > 32000) continue
            val dist = sqrt(dx.toDouble() * dx + dy.toDouble() * dy)
            if (dist > rangeM) continue

            val altRaw = o.opt("alt_baro")
            val ground = altRaw is String && altRaw.equals("ground", true)
            val alt = when {
                ground -> 0
                altRaw is Number -> altRaw.toInt().coerceIn(0, 60000)
                else -> 0x7FFF
            }
            val trkRaw = if (o.has("track")) o.optDouble("track", Double.NaN)
                         else o.optDouble("true_heading", Double.NaN)
            val trk = if (trkRaw.isFinite())
                (trkRaw * 100).roundToInt().coerceIn(0, 35999) else 0xFFFF
            val gsRaw = o.optDouble("gs", Double.NaN)
            val gs = if (gsRaw.isFinite())
                (gsRaw * 10).roundToInt().coerceIn(0, 65534) else 0xFFFF
            val squawk = o.optString("squawk", "").toIntOrNull() ?: -1
            val sq = if (squawk >= 0) squawk else 0xFFFF
            val cs = o.optString("flight", "").trim().uppercase().padEnd(6).take(6)

            var flags = 0
            if (ground) flags = flags or 1
            if (squawk == 7500 || squawk == 7600 || squawk == 7700) flags = flags or 2
            if ((o.optInt("dbFlags", 0) and 1) != 0) flags = flags or 4

            out.add(P(dx, dy, alt, trk, gs, sq, flags, cs, dist))
        }
        out.sortBy { it.dist }
        return if (out.size > MAX_PLANES) ArrayList(out.subList(0, MAX_PLANES)) else out
    }

    private fun encode(list: List<P>, source: Int): ByteArray {
        val out = ByteArrayOutputStream(6 + list.size * 20)
        out.write(1)                 // version
        out.write(list.size)
        out.write(source)
        out.write(0)
        le16(out, rangeM)
        for (p in list) {
            le16(out, p.dx)
            le16(out, p.dy)
            le16(out, p.alt)
            le16(out, p.trk)
            le16(out, p.gs)
            le16(out, p.sq)
            out.write(p.flags)
            out.write(0)
            val cs = p.cs.toByteArray(Charsets.US_ASCII)
            for (k in 0 until 6) out.write(if (k < cs.size) cs[k].toInt() else 32)
        }
        return out.toByteArray()
    }

    private fun le16(out: ByteArrayOutputStream, value: Int) {
        out.write(value and 0xFF)
        out.write((value shr 8) and 0xFF)
    }
}
