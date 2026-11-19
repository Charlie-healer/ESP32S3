package com.friday.motonav

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothGattDescriptor
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothProfile
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.util.Log
import java.util.UUID

/**
 * Minimal BLE ELM327 client. The phone is the central; the round screen only
 * receives the compact "#OBD|..." line over the existing MotoNav link.
 *
 * Supported adapter layouts: FFE0/FFE1, FFF0/FFF1+FFF2, 18F0/2AF0+2AF1 and
 * Nordic UART. Polls a small standard OBD-II PID set at ~2 Hz.
 */
@SuppressLint("MissingPermission")
object ObdBle {
    private const val TAG = "MotoNavOBD"
    private val CCCD = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")
    private val SERVICES = listOf(
        UUID.fromString("0000ffe0-0000-1000-8000-00805f9b34fb"),
        UUID.fromString("0000fff0-0000-1000-8000-00805f9b34fb"),
        UUID.fromString("000018f0-0000-1000-8000-00805f9b34fb"),
        UUID.fromString("6e400001-b5a3-f393-e0a9-e50e24dcca9e")
    )
    private val WRITE_UUIDS = listOf(
        UUID.fromString("0000ffe1-0000-1000-8000-00805f9b34fb"),
        UUID.fromString("0000fff1-0000-1000-8000-00805f9b34fb"),
        UUID.fromString("0000fff2-0000-1000-8000-00805f9b34fb"),
        UUID.fromString("00002af0-0000-1000-8000-00805f9b34fb"),
        UUID.fromString("6e400002-b5a3-f393-e0a9-e50e24dcca9e")
    )
    private val NOTIFY_UUIDS = listOf(
        UUID.fromString("0000ffe1-0000-1000-8000-00805f9b34fb"),
        UUID.fromString("0000fff1-0000-1000-8000-00805f9b34fb"),
        UUID.fromString("0000fff2-0000-1000-8000-00805f9b34fb"),
        UUID.fromString("00002af1-0000-1000-8000-00805f9b34fb"),
        UUID.fromString("6e400003-b5a3-f393-e0a9-e50e24dcca9e")
    )

    private var ctx: Context? = null
    private val handler = Handler(Looper.getMainLooper())
    private var gatt: BluetoothGatt? = null
    private var writeChar: BluetoothGattCharacteristic? = null
    private var notifyChar: BluetoothGattCharacteristic? = null
    private var ready = false
    private var scanning = false
    private val rx = StringBuilder()
    private var pollIndex = 0
    private var lastSent = 0L
    @Volatile private var enabled = false

    private var rpm = -1
    private var speed = -1
    private var coolant = -99
    private var volt = -1.0
    private var load = -1
    private var fuel = -1.0

    private val pids = arrayOf("010C", "010D", "0105", "0142", "0104", "015E")

    fun isEnabled(context: Context): Boolean =
        context.getSharedPreferences("motonav", Context.MODE_PRIVATE).getBoolean("obd_enabled", false)

    fun setEnabled(context: Context, on: Boolean) {
        context.getSharedPreferences("motonav", Context.MODE_PRIVATE).edit().putBoolean("obd_enabled", on).apply()
        if (on) start(context) else stop()
    }

    fun start(context: Context) {
        if (enabled) return
        ctx = context.applicationContext
        enabled = true
        val mgr = ctx!!.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        val adapter = mgr.adapter ?: return
        if (!adapter.isEnabled) return
        val scanner = adapter.bluetoothLeScanner ?: return
        scanning = true
        scanner.startScan(null, ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build(), scanCb)
        handler.postDelayed({ if (scanning) stopScan() }, 12_000)
        Log.i(TAG, "scan start")
    }

    fun stop() {
        enabled = false
        handler.removeCallbacksAndMessages(null)
        stopScan()
        try { gatt?.disconnect() } catch (_: Exception) {}
        try { gatt?.close() } catch (_: Exception) {}
        gatt = null; writeChar = null; notifyChar = null; ready = false; scanning = false
    }

    private fun stopScan() {
        scanning = false
        val c = ctx ?: return
        val mgr = c.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        try { mgr.adapter?.bluetoothLeScanner?.stopScan(scanCb) } catch (_: Exception) {}
    }

    private val scanCb = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult?) {
            val dev = result?.device ?: return
            val name = try { dev.name.orEmpty() } catch (_: Exception) { "" }
            val hasService = result.scanRecord?.serviceUuids?.any { SERVICES.contains(it.uuid) } == true
            if (!hasService && !name.uppercase().contains("OBD") && !name.uppercase().contains("ELM") &&
                !name.uppercase().contains("VGATE") && !name.uppercase().contains("VEEPEAK")) return
            stopScan()
            connect(dev)
        }
        override fun onScanFailed(errorCode: Int) { Log.w(TAG, "scan failed $errorCode"); scanning = false }
    }

    private fun connect(dev: BluetoothDevice) {
        Log.i(TAG, "connect ${dev.address}")
        gatt = dev.connectGatt(ctx, false, gattCb, BluetoothDevice.TRANSPORT_LE)
    }

    private val gattCb = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(g: BluetoothGatt, status: Int, newState: Int) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                g.discoverServices()
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                ready = false
                if (enabled) handler.postDelayed({ start(ctx ?: return@postDelayed) }, 5000)
            }
        }

        override fun onServicesDiscovered(g: BluetoothGatt, status: Int) {
            if (status != BluetoothGatt.GATT_SUCCESS) return
            for (svc in g.services) {
                if (!SERVICES.contains(svc.uuid)) continue
                for (ch in svc.characteristics) {
                    if (WRITE_UUIDS.contains(ch.uuid) && (ch.properties and (BluetoothGattCharacteristic.PROPERTY_WRITE or BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE)) != 0) writeChar = ch
                    if (NOTIFY_UUIDS.contains(ch.uuid) && (ch.properties and BluetoothGattCharacteristic.PROPERTY_NOTIFY) != 0) notifyChar = ch
                }
            }
            if (writeChar == null) {
                for (svc in g.services) for (ch in svc.characteristics) {
                    if (writeChar == null && (ch.properties and (BluetoothGattCharacteristic.PROPERTY_WRITE or BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE)) != 0) writeChar = ch
                    if (notifyChar == null && (ch.properties and BluetoothGattCharacteristic.PROPERTY_NOTIFY) != 0) notifyChar = ch
                }
            }
            notifyChar?.let { ch ->
                g.setCharacteristicNotification(ch, true)
                val d = ch.getDescriptor(CCCD)
                if (d != null) {
                    if (Build.VERSION.SDK_INT >= 33) g.writeDescriptor(d, BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE)
                    else { @Suppress("DEPRECATION") d.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE; @Suppress("DEPRECATION") g.writeDescriptor(d) }
                }
            }
            ready = writeChar != null
            if (ready) {
                Log.i(TAG, "ready write=${writeChar?.uuid} notify=${notifyChar?.uuid}")
                handler.postDelayed({ send("ATZ") }, 300)
                handler.postDelayed({ send("ATE0") }, 1200)
                handler.postDelayed({ send("ATL0") }, 1500)
                handler.postDelayed({ send("ATS0") }, 1800)
                handler.postDelayed({ send("ATSP0") }, 2100)
                handler.postDelayed({ poll() }, 3000)
            }
        }

        override fun onCharacteristicChanged(g: BluetoothGatt, ch: BluetoothGattCharacteristic, value: ByteArray) {
            onData(value)
        }

        @Deprecated("Deprecated in Java")
        override fun onCharacteristicChanged(g: BluetoothGatt, ch: BluetoothGattCharacteristic) {
            @Suppress("DEPRECATION") onData(ch.value ?: return)
        }
    }

    private fun onData(value: ByteArray) {
        rx.append(String(value, Charsets.US_ASCII))
        if (rx.length > 4096) rx.delete(0, rx.length - 2048)
        if (rx.indexOf(">") < 0) return
        val text = rx.toString()
        rx.setLength(0)
        for (line in text.split('\r', '\n', '>')) {
            val t = line.replace(" ", "").trim()
            if (t.startsWith("41")) parse(t)
        }
        lastSentAt()
    }

    private fun parse(t: String) {
        val pid = t.substring(2, 4)
        val d = t.substring(4)
        if (d.length < 2) return
        fun b(i: Int) = d.substring(i * 2, i * 2 + 2).toInt(16)
        when (pid) {
            "0C" -> if (d.length >= 4) rpm = ((b(0) shl 8) + b(1)) / 4
            "0D" -> speed = b(0)
            "05" -> coolant = b(0) - 40
            "42" -> if (d.length >= 4) volt = ((b(0) shl 8) + b(1)) / 1000.0
            "04" -> load = b(0) * 100 / 255
            "5E" -> if (d.length >= 4) fuel = ((b(0) shl 8) + b(1)) / 20.0
        }
    }

    private fun lastSentAt() {
        val now = System.currentTimeMillis()
        if (now - lastSent < 800) return
        lastSent = now
        val line = "#OBD|" + rpm + "|" + speed + "|" + coolant + "|" +
            String.format(java.util.Locale.US, "%.2f", volt) + "|-1|" + load + "|" +
            String.format(java.util.Locale.US, "%.2f", fuel)
        BleLink.send(line)
    }

    private fun poll() {
        if (!enabled || !ready) return
        val pid = pids[pollIndex % pids.size]
        pollIndex++
        send(pid)
        handler.postDelayed({ poll() }, 450)
    }

    private fun send(cmd: String) {
        val g = gatt ?: return
        val ch = writeChar ?: return
        val data = (cmd + "\r").toByteArray(Charsets.US_ASCII)
        if (Build.VERSION.SDK_INT >= 33) {
            g.writeCharacteristic(ch, data, BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
        } else {
            @Suppress("DEPRECATION")
            ch.value = data
            @Suppress("DEPRECATION")
            ch.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
            @Suppress("DEPRECATION")
            g.writeCharacteristic(ch)
        }
    }
}
