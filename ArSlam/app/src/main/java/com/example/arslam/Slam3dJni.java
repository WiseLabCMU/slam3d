package com.example.arslam;

import java.util.ArrayList;

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

    static {
        System.loadLibrary("slam3d");
    }

    private native long particleFilter_newPf();
    private native long particleFilter_newBcn();
    private native void particleFilter_freePf(long pf);
    private native void particleFilter_freeBcn(long bcn);
    private native void particleFilter_depositVio(long pf, double t, float x, float y, float z, float dist);
    private native void particleFilter_depositUwb(long pf, long bcn, float range, float stdRange, ArrayList<Long> bcnArray);
    private native TagLocation particleFitler_getTagLoc(long pf);
    private native BcnLocation particleFilter_getBcnLoc(long pf, long bcn);

    public Slam3dJni() {

    }

}
