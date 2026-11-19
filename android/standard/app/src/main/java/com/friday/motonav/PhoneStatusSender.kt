package com.friday.motonav

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.BatteryManager
import android.os.Build

/** Sends the phone battery to the round screen as "#PB,<pct>,<charging>". */
object PhoneStatusSender {
    private var receiver: BroadcastReceiver? = null
    private var ctx: Context? = null
    private var lastSent = 0L
    private var lastPct = -1
    private var lastChg = false
    private var lastAt = 0L

    fun start(context: Context) {
        if (receiver != null) return
        ctx = context.applicationContext
        val r = object : BroadcastReceiver() {
            override fun onReceive(c: Context?, intent: Intent?) = send(force = false)
        }
        val f = IntentFilter(Intent.ACTION_BATTERY_CHANGED)
        try {
            if (Build.VERSION.SDK_INT >= 33) {
                context.registerReceiver(r, f, Context.RECEIVER_NOT_EXPORTED)
            } else {
                @Suppress("DEPRECATION")
                context.registerReceiver(r, f)
            }
            receiver = r
        } catch (_: Exception) {
            receiver = null
        }
        send(force = true)
    }

    fun send(force: Boolean) {
        val c = ctx ?: return
        val now = System.currentTimeMillis()
        val i = try {
            if (Build.VERSION.SDK_INT >= 33) c.registerReceiver(null, IntentFilter(Intent.ACTION_BATTERY_CHANGED), Context.RECEIVER_NOT_EXPORTED)
            else c.registerReceiver(null, IntentFilter(Intent.ACTION_BATTERY_CHANGED))
        } catch (_: Exception) { null } ?: return
        val level = i.getIntExtra(BatteryManager.EXTRA_LEVEL, -1)
        val scale = i.getIntExtra(BatteryManager.EXTRA_SCALE, 100).coerceAtLeast(1)
        if (level < 0) return
        val pct = (level * 100 / scale).coerceIn(0, 100)
        val status = i.getIntExtra(BatteryManager.EXTRA_STATUS, -1)
        val chg = status == BatteryManager.BATTERY_STATUS_CHARGING || status == BatteryManager.BATTERY_STATUS_FULL
        if (!force && pct == lastPct && chg == lastChg && now - lastAt < 60_000L) return
        if (!force && now - lastSent < 10_000L && pct == lastPct && chg == lastChg) return
        lastPct = pct; lastChg = chg; lastAt = now; lastSent = now
        BleLink.send("#PB,$pct,${if (chg) 1 else 0}")
    }

    fun stop() {
        val c = ctx ?: return
        receiver?.let { try { c.unregisterReceiver(it) } catch (_: Exception) {} }
        receiver = null
        ctx = null
    }
}
