package com.friday.motonav

import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Typeface
import kotlin.math.ceil

/**
 * 版本：跟随 App 版本，见 MainActivity.kt 里的 APP_VERSION（v1.7 新增）
 *
 * 把中文文字渲染成 4 位灰度小图片（每个像素 16 级灰度，两个像素占一个字节），
 * 发给圆屏显示。这样圆屏不需要中文字库，字体也是手机系统的，清晰好看。
 */
object TextImage {
    class Img(val w: Int, val h: Int, val data: ByteArray)

    /**
     * @param maxW  图片最大宽度（像素），文字太长会先缩小字号，再不行就截断加省略号
     * @param sizePx 期望字号（像素）
     * @return 文字为空时返回 null
     */
    fun render(text: String, maxW: Int, sizePx: Float, minSizePx: Float = 16f): Img? {
        if (text.isBlank()) return null
        val p = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            typeface = Typeface.DEFAULT_BOLD
            color = Color.WHITE
            textSize = sizePx
        }
        var t = text.trim()
        while (p.measureText(t) > maxW && p.textSize > minSizePx) p.textSize = p.textSize - 1f
        if (p.measureText(t) > maxW) {
            while (t.length > 1 && p.measureText("$t…") > maxW) t = t.dropLast(1)
            t = "$t…"
        }
        val fm = p.fontMetrics
        val h = ceil(fm.descent - fm.ascent).toInt().coerceIn(1, 60)
        val w = ceil(p.measureText(t)).toInt().coerceIn(1, maxW)

        val bmp = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888)
        Canvas(bmp).drawText(t, 0f, -fm.ascent, p)
        val px = IntArray(w * h)
        bmp.getPixels(px, 0, w, 0, 0, w, h)
        bmp.recycle()

        val bpr = (w + 1) / 2
        val out = ByteArray(bpr * h)
        for (y in 0 until h) {
            for (x in 0 until w) {
                val a = (px[y * w + x] ushr 24) and 0xFF
                val nib = ((a + 8) / 17).coerceIn(0, 15)
                if (nib == 0) continue
                val i = y * bpr + (x shr 1)
                out[i] = if (x and 1 == 0) (out[i].toInt() or (nib shl 4)).toByte()
                         else (out[i].toInt() or nib).toByte()
            }
        }
        return Img(w, h, out)
    }

    /**
     * 两行图片：第一行名称（粗体），第二行地址（半透明灰）。
     * 高度最多 52px，宽度最多 maxW，4 位灰度。
     */
    fun renderTwoLines(line1: String, line2: String, maxW: Int, size1Px: Float, size2Px: Float): Img? {
        if (line1.isBlank() && line2.isBlank()) return null
        val p1 = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            typeface = Typeface.DEFAULT_BOLD
            color = Color.WHITE
            textSize = size1Px
        }
        val p2 = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            typeface = Typeface.DEFAULT
            color = Color.argb(170, 255, 255, 255)
            textSize = size2Px
        }
        fun fit(p: Paint, raw: String, minSize: Float): String {
            var t = raw.trim()
            if (t.isEmpty()) return ""
            while (p.measureText(t) > maxW && p.textSize > minSize) p.textSize -= 1f
            if (p.measureText(t) > maxW) {
                while (t.length > 1 && p.measureText("$t…") > maxW) t = t.dropLast(1)
                t = "$t…"
            }
            return t
        }
        val t1 = fit(p1, line1, 14f)
        val t2 = fit(p2, line2, 10f)
        val fm1 = p1.fontMetrics
        val fm2 = p2.fontMetrics
        val h1 = ceil(fm1.descent - fm1.ascent).toInt().coerceAtLeast(1)
        val h2 = ceil(fm2.descent - fm2.ascent).toInt().coerceAtLeast(1)
        val gap = 2
        val h = (h1 + gap + h2).coerceIn(1, 52)
        val w = maxOf(ceil(p1.measureText(t1)).toInt(), ceil(p2.measureText(t2)).toInt()).coerceIn(1, maxW)

        val bmp = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888)
        val canvas = Canvas(bmp)
        canvas.drawText(t1, 0f, -fm1.ascent, p1)
        canvas.drawText(t2, 0f, h1 + gap - fm2.ascent, p2)
        val px = IntArray(w * h)
        bmp.getPixels(px, 0, w, 0, 0, w, h)
        bmp.recycle()

        val bpr = (w + 1) / 2
        val out = ByteArray(bpr * h)
        for (y in 0 until h) {
            for (x in 0 until w) {
                val a = (px[y * w + x] ushr 24) and 0xFF
                val nib = ((a + 8) / 17).coerceIn(0, 15)
                if (nib == 0) continue
                val i = y * bpr + (x shr 1)
                out[i] = if (x and 1 == 0) (out[i].toInt() or (nib shl 4)).toByte()
                         else (out[i].toInt() or nib).toByte()
            }
        }
        return Img(w, h, out)
    }

    /**
     * 多行自动换行；每行按字符测量，避免长歌名/歌手把字号缩得太小。
     * 高度最多 64px，超过 maxLines 时在最后一行加省略号。
     */
    fun renderWrapped(text: String, maxW: Int, sizePx: Float, minSizePx: Float, maxLines: Int): Img? {
        if (text.isBlank()) return null
        val p = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            typeface = Typeface.DEFAULT_BOLD
            color = Color.WHITE
            textSize = sizePx
        }
        val clean = text.trim().replace(Regex("\\s+"), " ")
        fun wrap(): MutableList<String> {
            val out = ArrayList<String>()
            var line = StringBuilder()
            for (ch in clean) {
                val cand = line.toString() + ch
                if (line.length > 0 && p.measureText(cand) > maxW) {
                    out.add(line.toString())
                    line = StringBuilder().append(ch)
                } else {
                    line.append(ch)
                }
            }
            if (line.length > 0) out.add(line.toString())
            return out
        }
        var lines = wrap()
        while (lines.size > maxLines && p.textSize > minSizePx) {
            p.textSize -= 1f
            lines = wrap()
        }
        if (lines.size > maxLines) {
            lines = lines.take(maxLines).toMutableList()
            var last = lines.last()
            while (last.length > 1 && p.measureText("$last…") > maxW) last = last.dropLast(1)
            lines[lines.size - 1] = "$last…"
        }
        val fm = p.fontMetrics
        val lineH = ceil(fm.descent - fm.ascent).toInt().coerceAtLeast(1) + 2
        val h = (lineH * lines.size).coerceIn(1, 64)
        val w = lines.maxOf { ceil(p.measureText(it)).toInt() }.coerceIn(1, maxW)

        val bmp = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888)
        val canvas = Canvas(bmp)
        lines.forEachIndexed { i, line -> canvas.drawText(line, 0f, -fm.ascent + i * lineH, p) }
        val px = IntArray(w * h)
        bmp.getPixels(px, 0, w, 0, 0, w, h)
        bmp.recycle()

        val bpr = (w + 1) / 2
        val out = ByteArray(bpr * h)
        for (y in 0 until h) {
            for (x in 0 until w) {
                val a = (px[y * w + x] ushr 24) and 0xFF
                val nib = ((a + 8) / 17).coerceIn(0, 15)
                if (nib == 0) continue
                val i = y * bpr + (x shr 1)
                out[i] = if (x and 1 == 0) (out[i].toInt() or (nib shl 4)).toByte()
                         else (out[i].toInt() or nib).toByte()
            }
        }
        return Img(w, h, out)
    }
}
