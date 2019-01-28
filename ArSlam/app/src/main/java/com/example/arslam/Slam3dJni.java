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

    private long pf;
    private HashMap<String, Long> bcnMap;

    private static native long particleFilter_newPf();
    private static native long particleFilter_newBcn();
    private static native void particleFilter_freePf(long pf);
    private static native void particleFilter_freeBcn(long bcn);
    private static native void particleFilter_depositVio(long pf, double t, float x, float y, float z, float dist);
    private static native void particleFilter_depositUwb(long pf, long bcn, float range, float stdRange, ArrayList<Long> bcnArray);
    private static native TagLocation particleFitler_getTagLoc(long pf);
    private static native BcnLocation particleFilter_getBcnLoc(long pf, long bcn);

    static {
        System.loadLibrary("slam3d");
    }

    public Slam3dJni() {

    }

}
