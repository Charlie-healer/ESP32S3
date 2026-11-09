package com.friday.motonav.countdown;
/** A transient message recomputed immediately before its BLE write. */
public interface TrafficSignalWrite {
    boolean expired(long now);
    String encode(long now);
}
