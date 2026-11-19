package com.friday.motonav

import java.io.ByteArrayOutputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder

/**
 * Minimal FIT activity encoder (file_id + record messages) for RideRecorder.
 *
 * FIT is little-endian. Coordinates are semicircles, timestamps are seconds
 * since 1989-12-31 00:00 UTC, distance is centimetres and speed is mm/s. The
 * resulting file is intentionally small and imports in Garmin Connect /
 * fitfiletools; it does not contain laps/session summaries yet.
 */
object FitEncoder {
    private const val FIT_EPOCH = 631065600L
    private const val BASE_ENUM = 0x00
    private const val BASE_UINT16 = 0x84
    private const val BASE_SINT32 = 0x85
    private const val BASE_UINT32 = 0x86

    fun encode(points: List<RidePoint>): ByteArray {
        val body = ByteArrayOutputStream()
        var distanceM = 0.0
        var maxSpeedMmS = 0
        var prev: RidePoint? = null
        var prevWgs: DoubleArray? = null
        val startTimeMs = points.firstOrNull()?.time ?: System.currentTimeMillis()
        val endTimeMs = points.lastOrNull()?.time ?: startTimeMs

        // file_id (global 0): type=activity (4), time_created.
        definition(body, 0, listOf(Field(0, 1, BASE_ENUM), Field(4, 4, BASE_UINT32)))
        val firstTime = fitTime((points.firstOrNull()?.time ?: System.currentTimeMillis()) / 1000L)
        data(body) {
            u8(4)
            u32(firstTime)
        }

        // record (global 20): timestamp, lat, lon, distance, speed.
        definition(body, 20, listOf(
            Field(253, 4, BASE_UINT32),
            Field(0, 4, BASE_SINT32),
            Field(1, 4, BASE_SINT32),
            Field(5, 4, BASE_UINT32),
            Field(6, 2, BASE_UINT16)
        ))

        for (p in points) {
            val wgs = MapGeo.gcjToWgs(p.lat, p.lng)
            if (prev != null && prevWgs != null) {
                val d = haversine(prevWgs[0], prevWgs[1], wgs[0], wgs[1])
                val dt = ((p.time - prev.time) / 1000.0).coerceAtLeast(0.0)
                if (d < 1000.0 && dt > 0.0) distanceM += d
            }
            val speed = if (prev != null && prevWgs != null) {
                val d = haversine(prevWgs[0], prevWgs[1], wgs[0], wgs[1])
                val dt = ((p.time - prev.time) / 1000.0).coerceAtLeast(0.0)
                if (dt > 0.0) (d / dt * 1000.0).toInt().coerceIn(0, 65535) else 0
            } else 0
            if (speed > maxSpeedMmS) maxSpeedMmS = speed
            data(body) {
                u32(fitTime(p.time / 1000L))
                s32(semicircles(wgs[0]))
                s32(semicircles(wgs[1]))
                u32((distanceM * 100.0).toLong().coerceIn(0, 0xFFFFFFFFL).toInt())
                u16(speed)
            }
            prev = p
            prevWgs = wgs
        }

        // One lap + one session so Garmin Connect / bike computers see a
        // complete activity, not just raw record messages.
        val elapsedMs = (endTimeMs - startTimeMs).coerceAtLeast(0L)
        val distanceCm = (distanceM * 100.0).toLong().coerceIn(0L, 0xFFFFFFFFL)
        val avgSpeedMmS = if (elapsedMs > 0L)
            (distanceM / (elapsedMs / 1000.0) * 1000.0).toInt().coerceIn(0, 65535) else 0
        definition(body, 19, listOf(   // lap
            Field(253, 4, BASE_UINT32), Field(2, 4, BASE_UINT32),
            Field(7, 4, BASE_UINT32), Field(8, 4, BASE_UINT32),
            Field(9, 4, BASE_UINT32), Field(13, 2, BASE_UINT16),
            Field(14, 2, BASE_UINT16)
        ))
        data(body) {
            u32(fitTime(endTimeMs / 1000L))
            u32(fitTime(startTimeMs / 1000L))
            u32(elapsedMs)
            u32(elapsedMs)
            u32(distanceCm.toInt())
            u16(avgSpeedMmS)
            u16(maxSpeedMmS)
        }
        definition(body, 18, listOf(   // session
            Field(253, 4, BASE_UINT32), Field(2, 4, BASE_UINT32),
            Field(7, 4, BASE_UINT32), Field(8, 4, BASE_UINT32),
            Field(9, 4, BASE_UINT32), Field(14, 2, BASE_UINT16),
            Field(15, 2, BASE_UINT16), Field(5, 1, BASE_ENUM)
        ))
        data(body) {
            u32(fitTime(endTimeMs / 1000L))
            u32(fitTime(startTimeMs / 1000L))
            u32(elapsedMs)
            u32(elapsedMs)
            u32(distanceCm.toInt())
            u16(avgSpeedMmS)
            u16(maxSpeedMmS)
            u8(1)   // sport = cycling
        }

        val dataBytes = body.toByteArray()
        val header = ByteBuffer.allocate(12).order(ByteOrder.LITTLE_ENDIAN)
        header.put(12.toByte())        // header size
        header.put(0x10.toByte())      // protocol version 1.0
        header.putShort(2100.toShort()) // profile version 21.00
        header.putInt(dataBytes.size)
        header.put(".FIT".toByteArray(Charsets.US_ASCII))
        val headerBytes = header.array()
        val headerCrc = crc(headerBytes, 0, 12)
        val out = ByteArrayOutputStream()
        out.write(headerBytes)
        out.write(byteArrayOf((headerCrc and 0xFF).toByte(), ((headerCrc shr 8) and 0xFF).toByte()))
        out.write(dataBytes)
        val fileCrc = crc(out.toByteArray(), 0, out.size())
        out.write(byteArrayOf((fileCrc and 0xFF).toByte(), ((fileCrc shr 8) and 0xFF).toByte()))
        return out.toByteArray()
    }

    private data class Field(val num: Int, val size: Int, val base: Int)

    private fun definition(out: ByteArrayOutputStream, global: Int, fields: List<Field>) {
        out.write(0x40)                 // definition message
        out.write(0)                    // reserved
        out.write(0)                    // little-endian
        out.write(global and 0xFF)
        out.write((global shr 8) and 0xFF)
        out.write(fields.size)
        for (f in fields) {
            out.write(f.num)
            out.write(f.size)
            out.write(f.base)
        }
    }

    private inline fun data(out: ByteArrayOutputStream, block: ByteWriter.() -> Unit) {
        out.write(0)                    // data message
        block(ByteWriter(out))
    }

    private class ByteWriter(private val out: ByteArrayOutputStream) {
        fun u8(v: Int) { out.write(v and 0xFF) }
        fun u16(v: Int) { out.write(v and 0xFF); out.write((v shr 8) and 0xFF) }
        fun u32(v: Long) { for (i in 0 until 4) out.write(((v shr (8 * i)) and 0xFF).toInt()) }
        fun u32(v: Int) = u32(v.toLong() and 0xFFFFFFFFL)
        fun s32(v: Int) = u32(v.toLong() and 0xFFFFFFFFL)
    }

    private fun fitTime(unixSec: Long): Long = (unixSec - FIT_EPOCH).coerceAtLeast(0L)
    private fun semicircles(deg: Double): Int = Math.round(deg * (2147483648.0 / 180.0)).toInt()

    private fun haversine(lat1: Double, lon1: Double, lat2: Double, lon2: Double): Double {
        val r = 6371000.0
        val p1 = Math.toRadians(lat1)
        val p2 = Math.toRadians(lat2)
        val dp = Math.toRadians(lat2 - lat1)
        val dl = Math.toRadians(lon2 - lon1)
        val a = Math.sin(dp / 2) * Math.sin(dp / 2) + Math.cos(p1) * Math.cos(p2) * Math.sin(dl / 2) * Math.sin(dl / 2)
        return 2 * r * Math.asin(Math.sqrt(a))
    }

    private fun crc(data: ByteArray, off: Int, len: Int): Int {
        var crc = 0
        for (i in off until off + len) {
            crc = (crc ushr 8) or ((crc and 0xFF) shl 8)
            crc = crc xor (data[i].toInt() and 0xFF)
            crc = crc xor ((crc and 0xFF) ushr 4)
            crc = crc xor ((crc shl 12) and 0xFFFF)
            crc = crc xor (((crc and 0xFF) shl 5) and 0xFFFF)
        }
        return crc and 0xFFFF
    }
}
