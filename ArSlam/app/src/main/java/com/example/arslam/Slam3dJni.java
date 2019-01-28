package com.example.arslam;

import java.util.ArrayList;
import java.util.HashMap;

public class Slam3dJni {

    public class TagLocation {
        public double t;
        public float x;
        public float y;
        public float z;
        public float theta;

        public TagLocation(double t, float x, float y, float z, float theta) {
            this.t = t;
            this.x = x;
            this.y = y;
            this.z = z;
            this.theta = theta;
        }
    }

    public class BcnLocation {
        public double t;
        public float x;
        public float y;
        public float z;

        public BcnLocation(double t, float x, float y, float z) {
            this.t = t;
            this.x = x;
            this.y = y;
            this.z = z;
        }
    }

    public TagLocation tagLocation = new TagLocation(0.0, 0.0f, 0.0f, 0.0f, 0.0f);
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
    }

    public void free() {
        for (Long bcn : bcnMap.values()) {
            particleFilter_freeBcn(bcn);
        }
        bcnMap.clear();
        particleFilter_freePf(pf);
        pf = 0L;

        tagLocation = new TagLocation(0.0, 0.0f, 0.0f, 0.0f, 0.0f);
        bcnLocations.clear();
    }

    public void depositVio(double t, float x, float y, float z) {
        particleFilter_depositVio(pf, t, x, y, z, 0.0f);
        tagLocation = particleFilter_getTagLoc(pf);
    }

    public void depositUwb(String bcnName, float range, float stdRange) {
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
