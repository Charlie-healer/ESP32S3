package com.friday.motonav

import android.annotation.SuppressLint
import android.bluetooth.BluetoothManager
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.os.Handler
import android.os.Looper
import android.util.Log
import java.util.LinkedHashMap

/**
 * BLE TPMS advertisement decoder for the common aftermarket formats used by
 * the OBD2 reference project (MIT): Sysgration/ZEEPIN, SYTPMS, Michelin,
 * WODHMIEY, FOBO and Bekubee. The phone relays one compact line:
 *   #TP|frontKpa|rearKpa|frontC|rearC
 * The first two distinct sensor ids are treated as front/rear. A Sysgration
 * position code (1/2) wins when present.
 */
@SuppressLint("MissingPermission")
object TpmsBle {
    private const val TAG = "MotoNavTPMS"
    private const val ATM_PSI = 6.894757f

    private var ctx: Context? = null
    private val handler = Handler(Looper.getMainLooper())
    private var scanning = false
    private var enabled = false
    private val sensors = LinkedHashMap<String, Reading>()
    private var lastSend = 0L

    private data class Reading(val id: String, var kpa: Float, var temp: Float, val position: Int, var at: Long)

    private data class Adv(
        val name: String,
        val uuids: List<Int>,
        val mfr: ByteArray?,
        val serviceUuid: Int,
        val serviceData: ByteArray?
    ) {
        val company: Int get() = if (mfr != null && mfr.size >= 2) u16le(mfr, 0) else -1
    }

    fun isEnabled(context: Context): Boolean =
        context.getSharedPreferences("motonav", Context.MODE_PRIVATE).getBoolean("tpms_enabled", false)

    fun setEnabled(context: Context, on: Boolean) {
        context.getSharedPreferences("motonav", Context.MODE_PRIVATE).edit().putBoolean("tpms_enabled", on).apply()
        if (on) start(context) else stop()
    }

    fun start(context: Context) {
        if (enabled) return
        ctx = context.applicationContext
        enabled = true
        sensors.clear()
        val mgr = ctx!!.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        val scanner = mgr.adapter?.bluetoothLeScanner ?: return
        scanning = true
        scanner.startScan(null, ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_POWER).build(), cb)
        handler.postDelayed({ if (enabled) restart() }, 30_000)
        Log.i(TAG, "scan start")
    }

    private fun restart() {
        stopScan()
        if (!enabled) return
        val mgr = ctx?.getSystemService(Context.BLUETOOTH_SERVICE) as? BluetoothManager ?: return
        val scanner = mgr.adapter?.bluetoothLeScanner ?: return
        scanning = true
        scanner.startScan(null, ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_POWER).build(), cb)
        handler.postDelayed({ if (enabled) restart() }, 30_000)
    }

    fun stop() {
        enabled = false
        handler.removeCallbacksAndMessages(null)
        stopScan()
        sensors.clear()
    }

    private fun stopScan() {
        scanning = false
        val c = ctx ?: return
        val mgr = c.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        try { mgr.adapter?.bluetoothLeScanner?.stopScan(cb) } catch (_: Exception) {}
    }

    private val cb = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult?) {
            val rec = result?.scanRecord ?: return
            val raw = rec.bytes ?: return
            val adv = parseAdv(raw, rec.deviceName.orEmpty())
            val r = decode(adv) ?: return
            val key = if (r.id.isNotEmpty()) r.id else (result.device?.address ?: return)
            val old = sensors[key]
            if (old != null) {
                old.kpa = r.kpa; old.temp = r.temp; old.at = System.currentTimeMillis()
            } else {
                if (sensors.size >= 2) {
                    val first = sensors.keys.iterator()
                    if (first.hasNext()) sensors.remove(first.next())
                }
                sensors[key] = Reading(key, r.kpa, r.temp, r.position, System.currentTimeMillis())
            }
            send()
        }
        override fun onScanFailed(errorCode: Int) { Log.w(TAG, "scan failed $errorCode"); scanning = false }
    }

    private fun send() {
        val now = System.currentTimeMillis()
        if (now - lastSend < 1500) return
        lastSend = now
        val list = sensors.values.filter { now - it.at < 120_000 && it.kpa.isFinite() }
        if (list.isEmpty()) return
        val front = list.firstOrNull { it.position == 1 } ?: list.getOrNull(0)
        val rear = list.firstOrNull { it.position == 2 } ?: list.getOrNull(1)
        val fk = front?.kpa?.toInt() ?: -1
        val rk = rear?.kpa?.toInt() ?: -1
        val fc = front?.temp?.toInt() ?: -99
        val rc = rear?.temp?.toInt() ?: -99
        BleLink.send("#TP|$fk|$rk|$fc|$rc")
    }

    private fun parseAdv(raw: ByteArray, name: String): Adv {
        val uuids = ArrayList<Int>()
        var mfr: ByteArray? = null
        var su = -1
        var sd: ByteArray? = null
        var i = 0
        while (i < raw.size) {
            val len = raw[i].toInt() and 0xFF
            if (len == 0 || i + len >= raw.size) break
            val type = raw[i + 1].toInt() and 0xFF
            val v = i + 2
            val n = len - 1
            when (type) {
                0x02, 0x03 -> {
                    var k = 0
                    while (k + 1 < n) { uuids.add(u16le(raw, v + k)); k += 2 }
                }
                0x16 -> if (n >= 2) {
                    su = u16le(raw, v)
                    sd = raw.copyOfRange(v + 2, v + n)
                }
                0xFF -> if (n >= 2) mfr = raw.copyOfRange(v, v + n)
            }
            i += len + 1
        }
        return Adv(name, uuids, mfr, su, sd)
    }

    private fun decode(a: Adv): Reading? {
        val m = a.mfr
        if (m != null && m.size >= 12 && a.company == 0x0100 && u8(m, 3) == 0xEA && u8(m, 4) == 0xCA &&
            (u8(m, 2) and 0xF0) == 0x80) {
            val kpa = i32le(m, 8) / 1000.0f
            val temp = if (m.size >= 16) i32le(m, 12) / 100.0f else Float.NaN
            return Reading(id = hex(m, 2, 6), kpa = kpa.coerceAtLeast(0f), temp = temp, position = u8(m, 2) and 0x0F, at = 0)
        }
        if (m != null && a.uuids.contains(0x27A5) && (m.size == 7 || m.size >= 5)) {
            val kpa = ((u16be(m, 3) - 145) / 10.0f) * ATM_PSI
            return Reading(id = hex(m, 0, m.size), kpa = kpa.coerceAtLeast(0f), temp = s8(m, 2).toFloat(), position = -1, at = 0)
        }
        if (m != null && m.size >= 4 && a.company == 0x0828 && u8(m, 2) == 0x01) {
            val frame = u8(m, 3)
            val at = when (frame) { 0x03, 0x04, 0x0C -> 6; 0x05 -> 14; else -> -1 }
            val temp = if (m.size >= 5) (u8(m, 4) - 60).toFloat() else Float.NaN
            val kpa = if (at >= 0 && m.size >= at + 2) (u16le(m, at) - 1000) / 10.0f else Float.NaN
            if (kpa.isFinite()) return Reading(id = hex(m, 0, m.size), kpa = kpa.coerceAtLeast(0f), temp = temp, position = -1, at = 0)
        }
        if (m != null && m.size >= 9 && a.company == 0x00AC) {
            val kpa = (u8(m, 3) + 256 * u8(m, 8)) * 3.144f
            val temp = if (m.size >= 5) (u8(m, 4) - 55).toFloat() else Float.NaN
            return Reading(id = hex(m, 14, 3), kpa = kpa, temp = temp, position = -1, at = 0)
        }
        if (m != null && m.size >= 6 && a.company == 0x0127) {
            val q = m.copyOfRange(m.size - 4, m.size)
            val word = u16be(q, 2)
            return Reading(id = hex(m, 0, m.size), kpa = (word and 0x3FF).toFloat(), temp = ((q[1].toInt() and 0x7F) - 50).toFloat(), position = -1, at = 0)
        }
        if (a.serviceUuid == 0x012A && a.serviceData != null && a.serviceData.size >= 4) {
            val q = a.serviceData.copyOfRange(a.serviceData.size - 4, a.serviceData.size)
            val word = u16be(q, 2)
            return Reading(id = hex(q, 0, q.size), kpa = (word and 0x3FF).toFloat(), temp = ((q[1].toInt() and 0x7F) - 40).toFloat(), position = -1, at = 0)
        }
        if (m != null && m.size >= 6 && (a.uuids.contains(0xA827) || (a.name == "TPMS" && m.size == 11))) {
            return Reading(id = hex(m, 6, 3), kpa = (u16be(m, 4) - 100).coerceAtLeast(0).toFloat(), temp = (u8(m, 3) - 50).toFloat(), position = -1, at = 0)
        }
        return null
    }

    private fun u8(b: ByteArray, i: Int) = b[i].toInt() and 0xFF
    private fun u16le(b: ByteArray, i: Int) = u8(b, i) or (u8(b, i + 1) shl 8)
    private fun u16be(b: ByteArray, i: Int) = (u8(b, i) shl 8) or u8(b, i + 1)
    private fun i32le(b: ByteArray, i: Int): Int = u8(b, i) or (u8(b, i + 1) shl 8) or (u8(b, i + 2) shl 16) or (u8(b, i + 3) shl 24)
    private fun s8(b: ByteArray, i: Int) = b[i].toInt()
    private fun hex(b: ByteArray, off: Int, n: Int): String {
        val e = (off + n).coerceAtMost(b.size)
        if (off >= e) return ""
        val sb = StringBuilder()
        for (i in off until e) sb.append(String.format("%02X", u8(b, i)))
        return sb.toString()
    }
}
