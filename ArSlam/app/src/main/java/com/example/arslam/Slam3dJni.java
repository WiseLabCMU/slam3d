package com.example.arslam;

import java.util.ArrayList;
import java.util.HashMap;

public class Slam3dJni {

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
            return t + "," + x + "," + y + "," + z + "," + theta;
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
            return t + "," + x + "," + y + "," + z;
        }
    }

    public TagLocation tagLocation;
    public HashMap<String, BcnLocation> bcnLocations = new HashMap<>();

    private long pf;
    private HashMap<String, Long> bcnMap = new HashMap<>();

    private static native long particleFilter_newPf();
    private static native long particleFilter_newBcn();
    private static native void particleFilter_freePf(long pf);
    private static native void particleFilter_freeBcn(long bcn);
    private static native void particleFilter_depositVio(long pf, double t, float x, float y, float z, float dist);
    private static native void particleFilter_depositUwb(long pf, long bcn, float range, float stdRange, ArrayList<Long> bcnArray);
    private static native TagLocation particleFilter_getTagLoc(long pf);
    private static native BcnLocation particleFilter_getBcnLoc(long pf, long bcn);

    static {
        System.loadLibrary("slam3d");
    }

    public Slam3dJni() {
        pf = particleFilter_newPf();
        tagLocation = particleFilter_getTagLoc(pf);
    }

    public void free() {
        if (pf == 0L) {
            throw new NullPointerException("Slam3d is not initialized");
        }
        for (Long bcn : bcnMap.values()) {
            particleFilter_freeBcn(bcn);
        }
        bcnMap.clear();
        particleFilter_freePf(pf);
        pf = 0L;

        tagLocation = null;
        bcnLocations.clear();
    }

    public void depositVio(double t, float x, float y, float z) {
        if (pf == 0L) {
            throw new NullPointerException("Slam3d is not initialized");
        }
        particleFilter_depositVio(pf, t, x, y, z, 0.0f);
        tagLocation = particleFilter_getTagLoc(pf);
    }

    public void depositUwb(String bcnName, float range, float stdRange) {
        if (pf == 0L) {
            throw new NullPointerException("Slam3d is not initialized");
        }
        if (!bcnMap.containsKey(bcnName)) {
            bcnMap.put(bcnName, particleFilter_newBcn());
        }
        particleFilter_depositUwb(pf, bcnMap.get(bcnName), range, stdRange, new ArrayList<>(bcnMap.values()));
        tagLocation = particleFilter_getTagLoc(pf);
        for (String bcn : bcnMap.keySet()) {
            bcnLocations.put(bcn, particleFilter_getBcnLoc(pf, bcnMap.get(bcn)));
        }
    }

}
