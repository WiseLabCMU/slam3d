package com.example.arslam;

import com.google.ar.core.Pose;
import com.google.ar.sceneform.math.Quaternion;
import com.google.ar.sceneform.math.Vector3;

public class PoseManager {

    private static Pose arCoreToSlam3dPose;

    static {
        Quaternion q = Quaternion.axisAngle(new Vector3(1.0f, 0.0f, 0.0f), 90.0f);
        arCoreToSlam3dPose = Pose.makeRotation(q.x, q.y, q.z, q.w);
    }

    public static Pose arCoreToSlam3d(Pose arCorePose) {
        return arCoreToSlam3dPose.compose(arCorePose);
    }
}
