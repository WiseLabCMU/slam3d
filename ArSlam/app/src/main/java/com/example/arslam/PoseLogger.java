package com.example.arslam;

import android.util.Log;

import com.google.ar.core.Pose;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class PoseLogger {
    private List<Slam3dJni.VioMeasurement> vio;
    private List<Slam3dJni.RangeMeasurement> uwb;
    private List<Slam3dJni.RssiMeasurement> rssi;
    private List<Slam3dJni.TagLocation> tag;
    private String vioFilename;
    private String uwbFilename;
    private String rssiFilename;
    private String tagFilename;
    private String bcnFilename;

    private static final String LOG_TAG = "PoseLogger";

    public PoseLogger(String vioFilename, String uwbFilename, String rssiFilename, String tagFilename, String bcnFilename) {
        vio = new ArrayList<>();
        uwb = new ArrayList<>();
        rssi = new ArrayList<>();
        tag = new ArrayList<>();
        this.vioFilename = vioFilename;
        this.uwbFilename = uwbFilename;
        this.rssiFilename = rssiFilename;
        this.bcnFilename = bcnFilename;
        this.tagFilename = tagFilename;
    }

    public void logVio(long elapsedRealtimeMillis, Pose vioPose) {
        this.vio.add(new Slam3dJni.VioMeasurement(elapsedRealtimeMillis / 1000.0, vioPose.tx(), vioPose.ty(), vioPose.tz(), 0.0f));
    }

    public void logUwb(long elapsedRealtimeMillis, String bcnName, float range, float stdRange) {
        this.uwb.add(new Slam3dJni.RangeMeasurement(elapsedRealtimeMillis / 1000.0, bcnName, range, stdRange));
    }

    public void logRssi(long elapsedRealtimeMillis, String bcnName, int rssi) {
        this.rssi.add(new Slam3dJni.RssiMeasurement(elapsedRealtimeMillis / 1000.0, bcnName, rssi));
    }

    public void logTag(Slam3dJni.TagLocation tagLocation) {
        this.tag.add(new Slam3dJni.TagLocation(tagLocation));
    }

    public void writeLogs(HashMap<String, Slam3dJni.BcnLocation> bcns) throws IOException {
        saveVioToDisk();
        saveUwbToDisk();
        saveRssiToDisk();
        saveTagToDisk();
        saveBcnToDisk(bcns);
    }

    private void saveVioToDisk() throws IOException {
        File out = new File(vioFilename);
        if (!out.getParentFile().exists()) {
            out.getParentFile().mkdirs();
        }
        try (FileOutputStream stream = new FileOutputStream(out)) {
            stream.write("t,x,y,z,theta\n".getBytes());
            for (Slam3dJni.VioMeasurement v : vio) {
                stream.write((v.toString() + "\n").getBytes());
            }
        }
        vio.clear();
    }

    private void saveUwbToDisk() throws IOException {
        File out = new File(uwbFilename);
        if (!out.getParentFile().exists()) {
            out.getParentFile().mkdirs();
        }
        try (FileOutputStream stream = new FileOutputStream(out)) {
            stream.write("t,b,r,s\n".getBytes());
            for (Slam3dJni.RangeMeasurement u : uwb) {
                stream.write((u.toString() + "\n").getBytes());
            }
        }
        uwb.clear();
    }

    private void saveRssiToDisk() throws IOException {
        File out = new File(rssiFilename);
        if (!out.getParentFile().exists()) {
            out.getParentFile().mkdirs();
        }
        try (FileOutputStream stream = new FileOutputStream(out)) {
            stream.write("t,b,r\n".getBytes());
            for (Slam3dJni.RssiMeasurement r : rssi) {
                stream.write((r.toString() + "\n").getBytes());
            }
        }
        rssi.clear();
    }

    private void saveTagToDisk() throws IOException {
        File out = new File(tagFilename);
        if (!out.getParentFile().exists()) {
            out.getParentFile().mkdirs();
        }
        try (FileOutputStream stream = new FileOutputStream(out)) {
            stream.write("t,x,y,z,theta\n".getBytes());
            for (Slam3dJni.TagLocation loc : tag) {
                stream.write((loc.toString() + "\n").getBytes());
            }
        }
        tag.clear();
    }

    private void saveBcnToDisk(HashMap<String, Slam3dJni.BcnLocation> bcns) throws IOException {
        File out = new File(bcnFilename);
        if (!out.getParentFile().exists()) {
            out.getParentFile().mkdirs();
        }
        try (FileOutputStream stream = new FileOutputStream(out)) {
            stream.write("b,t,x,y,z\n".getBytes());
            for (Map.Entry<String, Slam3dJni.BcnLocation> entry : bcns.entrySet()) {
                stream.write((entry.getKey() + "," + entry.getValue().toString() + "\n").getBytes());
            }
        }
    }
}
