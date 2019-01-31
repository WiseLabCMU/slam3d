package com.example.arslam;

import com.google.ar.core.Pose;
import com.google.ar.sceneform.math.Quaternion;
import com.google.ar.sceneform.math.Vector3;

import java.io.IOException;

public class PoseManager {

    private static final Pose theirDeviceToOurDevice = Pose.IDENTITY;
    private static final Pose theirWorldToOurVio;
    private Pose theirDeviceToTheirWorld;
    private Pose ourDeviceToOurVio;
    private Pose ourWorldToTheirWorld;

    private Slam3dJni slam3d;
    private PoseLogger logger;

    static {
        Quaternion q = Quaternion.axisAngle(new Vector3(1.0f, 0.0f, 0.0f), 90.0f);
        theirWorldToOurVio = Pose.makeRotation(q.x, q.y, q.z, q.w);
    }

    public PoseManager() {
        slam3d = new Slam3dJni();
        updatePose();
    }

    public PoseManager(String tagFilename, String vioFilename, String bcnFilename) {
        slam3d = new Slam3dJni();
        logger = new PoseLogger(tagFilename, vioFilename, bcnFilename);
        updatePose();
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
        theirDeviceToTheirWorld = null;
        ourWorldToTheirWorld = null;
    }

    public void depositArCore(long elapsedRealtimeMillis, Pose arCorePose) {
        theirDeviceToTheirWorld = arCorePose;
        ourDeviceToOurVio = theirWorldToOurVio.compose(theirDeviceToTheirWorld.compose(theirDeviceToOurDevice.inverse()));
        slam3d.depositVio(elapsedRealtimeMillis / 1000.0, ourDeviceToOurVio.tx(), ourDeviceToOurVio.ty(), ourDeviceToOurVio.tz());
        updatePose();
        if (logger != null) {
            logger.logVio(elapsedRealtimeMillis, ourDeviceToOurVio);
            logger.logTag(slam3d.tagLocation);
        }
    }

    public void depositUwb(String bcnName, float range, float stdRange) {
        slam3d.depositUwb(bcnName, range, stdRange);
        updatePose();
    }

    public Pose getPoseToDraw(Pose ourWorldPose) {
        return ourWorldToTheirWorld.compose(ourWorldPose);
    }

    private void updatePose() {
        if (theirDeviceToTheirWorld == null) {
            theirDeviceToTheirWorld = Pose.IDENTITY;
        }
        if (ourDeviceToOurVio == null) {
            ourDeviceToOurVio = Pose.IDENTITY;
        }
        //TODO figure out rotation
        Quaternion q = Quaternion.axisAngle(new Vector3(0.0f, 0.0f, 1.0f), (float)Math.toDegrees(slam3d.tagLocation.theta));
        Pose additionalRotation = Pose.makeRotation(q.x, q.y, q.z, q.w);
        Pose theirRotationTheirDevice = theirDeviceToTheirWorld.extractRotation();
        Pose theirRotationTheirWorld = theirDeviceToTheirWorld.compose(theirRotationTheirDevice);
        Pose theirRotationOurVio = theirWorldToOurVio.compose(theirRotationTheirWorld);
        Pose ourRotationOurVio = additionalRotation.compose(theirRotationOurVio);
        Pose ourRotationOurDevice = ourDeviceToOurVio.inverse().compose(ourRotationOurVio);

        float[] t = new float[]{slam3d.tagLocation.x, slam3d.tagLocation.y, slam3d.tagLocation.z};
        Pose ourDeviceToOurWorld = new Pose(t, ourRotationOurDevice.getRotationQuaternion());
        ourWorldToTheirWorld = theirDeviceToTheirWorld.compose(theirDeviceToOurDevice.inverse().compose(ourDeviceToOurWorld.inverse()));
    }
}
