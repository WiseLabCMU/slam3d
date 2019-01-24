package com.example.arslam;

public class Slam3dJni {

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
