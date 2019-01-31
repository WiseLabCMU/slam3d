package com.example.arslam;

import com.google.ar.core.Pose;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class PoseLogger {
    private List<Slam3dJni.TagLocation> tag;
    private List<Slam3dJni.TagLocation> vio;
    private String tagFilename;
    private String vioFilename;
    private String bcnFilename;

    public PoseLogger(String tagFilename, String vioFilename, String bcnFilename) {
        tag = new ArrayList<>();
        vio = new ArrayList<>();
        this.tagFilename = tagFilename;
        this.vioFilename = vioFilename;
        this.bcnFilename = bcnFilename;
    }

    public void logTag(Slam3dJni.TagLocation tagLocation) {
        this.tag.add(new Slam3dJni.TagLocation(tagLocation));
    }

    public void logVio(long elapsedRealtimeMillis, Pose vioPose) {
        this.vio.add(new Slam3dJni.TagLocation(elapsedRealtimeMillis / 1000.0, vioPose.tx(), vioPose.ty(), vioPose.tz(), 0.0f));
    }

    public void writeLogs(HashMap<String, Slam3dJni.BcnLocation> bcns) throws IOException {
        saveTagToDisk();
        saveVioToDisk();
        saveBcnToDisk(bcns);
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

    private void saveVioToDisk() throws IOException {
        File out = new File(vioFilename);
        if (!out.getParentFile().exists()) {
            out.getParentFile().mkdirs();
        }
        try (FileOutputStream stream = new FileOutputStream(out)) {
            stream.write("t,x,y,z,theta\n".getBytes());
            for (Slam3dJni.TagLocation loc : vio) {
                stream.write((loc.toString() + "\n").getBytes());
            }
        }
        vio.clear();
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
