package com.example.arslam;

import com.google.ar.core.Pose;
import com.google.ar.sceneform.math.Quaternion;
import com.google.ar.sceneform.math.Vector3;

import java.io.IOException;
import java.util.Set;

public class PoseManager {

    private Pose theirDeviceToTheirWorld;
    private Pose ourDeviceToOurVio;
    private Pose ourWorldToOurVio;

    private Slam3dJni slam3d;
    private PoseLogger logger;

    private static final String LOG_TAG = "PoseManager";

    public PoseManager() {
        slam3d = new Slam3dJni();
        updatePose();
    }

    public PoseManager(String vioFilename, String uwbFilename, String rssiFilename, String tagFilename, String bcnFilename) {
        slam3d = new Slam3dJni();
        logger = new PoseLogger(vioFilename, uwbFilename, rssiFilename, tagFilename, bcnFilename);
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
        ourDeviceToOurVio = null;
        ourWorldToOurVio = null;
    }

    public void depositArCore(long elapsedRealtimeMillis, Pose arCorePose) {
        theirDeviceToTheirWorld = arCorePose;
        ourDeviceToOurVio = axisSwapThemToUs(theirDeviceToTheirWorld);
        slam3d.depositTagVio(elapsedRealtimeMillis / 1000.0, ourDeviceToOurVio.tx(), ourDeviceToOurVio.ty(), ourDeviceToOurVio.tz());
        updatePose();
        if (logger != null) {
            logger.logVio(elapsedRealtimeMillis, ourDeviceToOurVio);
            logger.logTag(slam3d.tagLocation);
        }
    }

    public void depositUwb(long elapsedRealtimeMillis, String bcnName, float range, float stdRange) {
        slam3d.depositRange(bcnName, range, stdRange);
        updatePose();
        if (logger != null) {
            logger.logUwb(elapsedRealtimeMillis, bcnName, range, stdRange);
        }
    }

    public void depositRssi(long elapsedRealtimeMillis, String bcnName, int rssi) {
        slam3d.depositRssi(bcnName, rssi);
        updatePose();
        if (logger != null) {
            logger.logRssi(elapsedRealtimeMillis, bcnName, rssi);
        }
    }

    public Pose getPoseToDraw(Pose ourWorldPose) {
        return axisSwapUsToThem(ourWorldToOurVio.compose(ourWorldPose));
    }

    public Pose getBcnWorldPose(String bcnName) {
        Slam3dJni.BcnLocation bcn = slam3d.bcnLocations.get(bcnName);
        return new Pose(new float[]{bcn.x, bcn.y, bcn.z}, new float[]{0.0f, 0.0f, 0.0f, 1.0f});
    }

    public Set<String> getBcnNames() {
        return slam3d.bcnLocations.keySet();
    }

    private Pose axisSwapThemToUs(Pose them) {
        return new Pose(new float[]{them.tx(), -them.tz(), them.ty()}, them.getRotationQuaternion());
    }

    private Pose axisSwapUsToThem(Pose us) {
        return new Pose(new float[]{us.tx(), us.tz(), -us.ty()}, us.getRotationQuaternion());
    }

    private void updatePose() {
        if (theirDeviceToTheirWorld == null) {
            theirDeviceToTheirWorld = Pose.IDENTITY;
        }
        if (ourDeviceToOurVio == null) {
            ourDeviceToOurVio = Pose.IDENTITY;
        }

        Quaternion q = Quaternion.axisAngle(new Vector3(0.0f, 0.0f, 1.0f), (float)Math.toDegrees(slam3d.tagLocation.theta));
        float[] t = new float[]{slam3d.tagLocation.x, slam3d.tagLocation.y, slam3d.tagLocation.z};

        Pose ourDeviceToOurWorld = new Pose(t, ourDeviceToOurVio.getRotationQuaternion());
        ourDeviceToOurWorld = Pose.makeRotation(q.x, q.y, q.z, q.w).compose(ourDeviceToOurWorld);
        ourWorldToOurVio = ourDeviceToOurVio.compose(ourDeviceToOurWorld.inverse());
    }
}
