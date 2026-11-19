package com.friday.motonav

import android.app.Notification
import android.content.Context
import android.service.notification.StatusBarNotification
import android.util.Log
import java.util.LinkedHashMap

/**
 * Forwards phone notifications to the round screen as:
 *   #N|<app>|<title>|<body>
 *
 * The firmware has no CJK font, so non-ASCII characters are replaced with '?'
 * here instead of drawing meaningless bytes. Apps whose names are Latin (Gmail,
 * Telegram, WhatsApp, Maps...) read well; Chinese-only notifications still
 * arrive but their text is intentionally sanitized.
 */
object NotificationForwarder {
    private val recent = LinkedHashMap<String, Long>()
    private const val DEDUPE_MS = 30_000L
    private var seq = 0

    private fun enabled(context: Context): Boolean =
        context.getSharedPreferences("motonav", Context.MODE_PRIVATE)
            .getBoolean("notify_show", true)

    private fun ascii(s: CharSequence?): String {
        if (s == null) return ""
        val b = StringBuilder()
        for (ch in s) {
            when {
                ch == '\n' || ch == '\r' || ch == '|' -> b.append(' ')
                ch.code in 32..126 -> b.append(ch)
                else -> b.append('?')
            }
        }
        return b.toString().trim()
    }

    fun forward(context: Context, sbn: StatusBarNotification?) {
        if (sbn == null || !enabled(context)) return
        val n = sbn.notification ?: return
        val pkg = sbn.packageName ?: return
        if (pkg == context.packageName || pkg == "android" || pkg == "com.android.systemui" ||
            pkg.startsWith("com.autonavi")) return
        if (n.flags and Notification.FLAG_ONGOING_EVENT != 0) return
        if (n.flags and Notification.FLAG_GROUP_SUMMARY != 0) return
        if (n.category == Notification.CATEGORY_PROGRESS || n.category == Notification.CATEGORY_SERVICE) return

        val extras = n.extras ?: return
        val title = ascii(extras.getCharSequence(Notification.EXTRA_TITLE))
        val body = ascii(
            extras.getCharSequence(Notification.EXTRA_BIG_TEXT)
                ?: extras.getCharSequence(Notification.EXTRA_TEXT)
        )
        if (title.isEmpty() && body.isEmpty()) return

        val app = try {
            val pm = context.packageManager
            ascii(pm.getApplicationLabel(pm.getApplicationInfo(pkg, 0)))
        } catch (_: Exception) {
            ascii(pkg.substringAfterLast('.'))
        }.ifEmpty { "APP" }

        val key = "$pkg:${sbn.id}:$title:$body"
        val now = System.currentTimeMillis()
        val old = recent[key]
        if (old != null && now - old < DEDUPE_MS) return
        recent[key] = now
        if (recent.size > 64) {
            val oldest = recent.keys.iterator()
            if (oldest.hasNext()) recent.remove(oldest.next())
        }

        val line = "#N|" + app.take(18) + "|" + title.take(38) + "|" + body.take(72)
        Log.i("MotoNavNotify", "forward pkg=$pkg")
        BleLink.sendEventKey("N${seq++ and 0xFFFF}", line)
    }
}
