package com.example.arslam;

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

    private native void particleFilter_init();

    public Slam3dJni() {

    }

    public void bar() {
        foo();
    }
}
