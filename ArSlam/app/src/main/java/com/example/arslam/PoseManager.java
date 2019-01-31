package com.example.arslam;

import com.google.ar.core.Pose;
import com.google.ar.sceneform.math.Quaternion;
import com.google.ar.sceneform.math.Vector3;

import java.io.IOException;

public class PoseManager {

    private static final Pose arCoreToSlam3dPose;

    private Slam3dJni slam3d;
    private PoseLogger logger;

    static {
        Quaternion q = Quaternion.axisAngle(new Vector3(1.0f, 0.0f, 0.0f), 90.0f);
        arCoreToSlam3dPose = Pose.makeRotation(q.x, q.y, q.z, q.w);
    }

    public PoseManager() {
        slam3d = new Slam3dJni();
    }

    public PoseManager(String tagFilename, String vioFilename, String bcnFilename) {
        slam3d = new Slam3dJni();
        logger = new PoseLogger(tagFilename, vioFilename, bcnFilename);
    }

    public void free() {
        if (logger != null) {
            try {
                logger.writeLogs(slam3d.bcnLocations);
            } catch (IOException ignored) { }
            logger = null;
        }
        slam3d.free();
        slam3d = null;
    }

    public void depositArCore(long elapsedRealtimeMillis, Pose arCorePose) {
        Pose vio = arCoreToSlam3dPose.compose(arCorePose);
        slam3d.depositVio(elapsedRealtimeMillis / 1000.0, vio.tx(), vio.ty(), vio.tz());

        if (logger != null) {
            logger.logVio(elapsedRealtimeMillis, vio);
            logger.logTag(slam3d.tagLocation);
        }
    }

    public void depositUwb(String bcnName, float range, float stdRange) {
        slam3d.depositUwb(bcnName, range, stdRange);
    }


}
