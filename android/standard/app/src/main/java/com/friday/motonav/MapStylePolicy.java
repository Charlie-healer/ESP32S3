package com.friday.motonav;

/** Map content policy, independent of Android so every style can be tested. */
public final class MapStylePolicy {
    public static final int ORIGINAL = 0, VICE_CITY = 1, GTA5 = 2, NEON = 3, PAPER = 4;
    public static final int MINIMAL = 5, BLUEPRINT = 6, PIXEL = 7, RADAR = 8, COMPASS = 9;
    public static final int SATELLITE = 10, DRAGON_RADAR = 11;
    private MapStylePolicy() {}
    /** All twelve styles are rendered by the firmware palette; the phone only
     *  chooses which base raster to capture (VICE/ORIG/MINIMAL/satellite). */
    public static boolean isValid(int theme) { return theme >= 0 && theme <= 11; }
    public static int normalize(int theme) { return isValid(theme) ? theme : VICE_CITY; }
    public static String cacheKey(int theme, boolean buildings) {
        if(theme==ORIGINAL)return "amap-original-v1";
        if(theme==MINIMAL)return "amap-roads-only-v1";
        if(theme==SATELLITE)return "amap-sat-v1";
        if(theme==DRAGON_RADAR)return "dragon";
        if(theme==VICE_CITY)return buildings ? "amap-buildings-v1" : "amap-sdk-v3";
        // 2/3/4/6/7/8/9 reuse the VICE base; the firmware recolours it.
        return "amap-sdk-v3";
    }
    public static boolean usesBasemap(int theme) { return theme != DRAGON_RADAR; }
    public static boolean usesAmapCapture(int theme) { return usesBasemap(theme); }
}
