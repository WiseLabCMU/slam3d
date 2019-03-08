package com.example.arslam;

import android.util.Log;

import java.util.HashMap;
import java.util.Locale;

public class Slam3dJni {

    private static String LOG_TAG = "Slam3dJni";

    public static class VioMeasurement {
        public final double t;
        public final float x;
        public final float y;
        public final float z;
        public final float dist;

        public VioMeasurement(double t, float x, float y, float z, float dist) {
            this.t = t;
            this.x = x;
            this.y = y;
            this.z = z;
            this.dist = dist;
        }

        @Override
        public String toString() {
            return String.format(Locale.US, "%.3f", t) + "," + x + "," + y + "," + z + "," + dist;
        }
    }

    public static class UwbMeasurement {
        public final double t;
        public final String bcnName;
        public final float range;
        public final float stdRange;

        public UwbMeasurement(double t, String bcnName, float range, float stdRange) {
            this.t = t;
            this.bcnName = bcnName;
            this.range = range;
            this.stdRange = stdRange;
        }

        @Override
        public String toString() {
            return String.format(Locale.US, "%.3f", t) + "," + bcnName + "," + range + "," + stdRange;
        }
    }

    public static class TagLocation {
        public final double t;
        public final float x;
        public final float y;
        public final float z;
        public final float theta;

        public TagLocation(double t, float x, float y, float z, float theta) {
            this.t = t;
            this.x = x;
            this.y = y;
            this.z = z;
            this.theta = theta;
        }

        public TagLocation(TagLocation other) {
            this.t = other.t;
            this.x = other.x;
            this.y = other.y;
            this.z = other.z;
            this.theta = other.theta;
        }

        @Override
        public String toString() {
            return String.format(Locale.US, "%.3f", t) + "," + x + "," + y + "," + z + "," + theta;
        }
    }

    public static class BcnLocation {
        public final double t;
        public final float x;
        public final float y;
        public final float z;

        public BcnLocation(double t, float x, float y, float z) {
            this.t = t;
            this.x = x;
            this.y = y;
            this.z = z;
        }

        public BcnLocation(BcnLocation other) {
            this.t = other.t;
            this.x = other.x;
            this.y = other.y;
            this.z = other.z;
        }

        @Override
        public String toString() {
            return String.format(Locale.US, "%.3f", t) + "," + x + "," + y + "," + z;
        }
    }

    public TagLocation tagLocation;
    public HashMap<String, BcnLocation> bcnLocations = new HashMap<>();

    private long pf;
    private HashMap<String, Long> bcnMap = new HashMap<>();

    private static native long particleFilterNewPf();
    private static native long particleFilterNewBcn();
    private static native void particleFilterFreePf(long pf);
    private static native void particleFilterFreeBcn(long bcn);
    private static native void particleFilterDepositVio(long pf, double t, float x, float y, float z, float dist);
    private static native void particleFilterDepositUwb(long pf, long bcn, float range, float stdRange, long[] bcnArray);
    private static native TagLocation particleFilterGetTagLoc(long pf);
    private static native BcnLocation particleFilterGetBcnLoc(long pf, long bcn);

    static {
        System.loadLibrary("slam3d");
    }

    public Slam3dJni() {
        pf = particleFilterNewPf();
        tagLocation = particleFilterGetTagLoc(pf);
    }

    public void free() {
        if (pf == 0L) {
            throw new NullPointerException("Slam3d is not initialized");
        }
        for (Long bcn : bcnMap.values()) {
            particleFilterFreeBcn(bcn);
        }
        bcnMap.clear();
        particleFilterFreePf(pf);
        pf = 0L;

        tagLocation = null;
        bcnLocations.clear();
    }

    public void depositVio(double t, float x, float y, float z) {
        if (pf == 0L) {
            throw new NullPointerException("Slam3d is not initialized");
        }
        particleFilterDepositVio(pf, t, x, y, z, 0.0f);
        tagLocation = particleFilterGetTagLoc(pf);
    }

    public void depositUwb(String bcnName, float range, float stdRange) {
        if (pf == 0L) {
            throw new NullPointerException("Slam3d is not initialized");
        }
        if (!bcnMap.containsKey(bcnName)) {
            bcnMap.put(bcnName, particleFilterNewBcn());
        }
        long[] bcnArray = new long[bcnMap.size()];
        int i = 0;
        for (Long bcn : bcnMap.values()) {
            bcnArray[i++] = bcn;
        }
        particleFilterDepositUwb(pf, bcnMap.get(bcnName), range, stdRange, bcnArray);
        tagLocation = particleFilterGetTagLoc(pf);
        for (String bcn : bcnMap.keySet()) {
            bcnLocations.put(bcn, particleFilterGetBcnLoc(pf, bcnMap.get(bcn)));
        }
    }

}
