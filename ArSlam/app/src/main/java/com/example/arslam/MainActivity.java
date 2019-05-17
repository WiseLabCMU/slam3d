package com.example.arslam;

import android.Manifest;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.SystemClock;
import android.support.annotation.NonNull;
import android.support.design.widget.FloatingActionButton;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Toast;

import com.google.ar.core.Frame;
import com.google.ar.core.HitResult;
import com.google.ar.core.Plane;
import com.google.ar.core.Pose;
import com.google.ar.core.Trackable;
import com.google.ar.core.TrackingState;
import com.google.ar.sceneform.Node;
import com.google.ar.sceneform.math.Quaternion;
import com.google.ar.sceneform.math.Vector3;
import com.google.ar.sceneform.rendering.ModelRenderable;
import com.google.ar.sceneform.rendering.Renderable;
import com.google.ar.sceneform.ux.ArFragment;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class MainActivity extends AppCompatActivity {

    private ArFragment fragment;
    private PointerDrawable pointer = new PointerDrawable();
    private boolean isTracking;
    private boolean isHitting;

    private HashMap<String, Node> bcnNodes;

    private BluetoothAdapter bluetoothAdapter;
    private BluetoothSystemManager bluetoothManager;
    private PoseManager poseManager;

    private static final int REQUEST_ENABLE_BT = 1;
    private static final int PERMISSION_REQUEST_COARSE_LOCATION = 2;
    private static final boolean ENABLE_LOG = true;

    private static final String LOG_TAG = "MainActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        FloatingActionButton fab = findViewById(R.id.fab);
        fab.setOnClickListener(view -> pressSave());

        fragment = (ArFragment) getSupportFragmentManager().findFragmentById(R.id.sceneform_fragment);
        fragment.getArSceneView().getScene().addOnUpdateListener(frameTime -> {
            fragment.onUpdate(frameTime);
            pointerUpdate();
            slam3dUpdate();
        });

        bcnNodes = new HashMap<>();

        Date date = new Date();
        poseManager = ENABLE_LOG ?
                new PoseManager(generateVioFilename(date), generateUwbFilename(date), generateRssiFilename(date), generateTagFilename(date), generateBcnFilename(date))
                : new PoseManager();

        if (!getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)) {
            Toast.makeText(this, "BLE Not Supported", Toast.LENGTH_LONG).show();
            finish();
        }
        final BluetoothManager bluetoothManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        bluetoothAdapter = bluetoothManager.getAdapter();
        requestPermissions(new String[]{Manifest.permission.ACCESS_COARSE_LOCATION}, PERMISSION_REQUEST_COARSE_LOCATION);
        this.bluetoothManager = new BluetoothSystemManager(this, new BluetoothSystemManager.RangeCallback() {
            @Override
            public void onBleConnect(String deviceName) {
                Log.i(LOG_TAG, "connected");
                runOnUiThread(() -> Toast.makeText(MainActivity.this, "BLE Connected: " + deviceName, Toast.LENGTH_LONG).show());
            }

            @Override
            public void onUwbRange(String bcnName, float range, float quality) {
                poseManager.depositUwb(SystemClock.elapsedRealtime(), bcnName, range, 0.1f);
            }

            @Override
            public void onBleRssi(String address, int rssi) {
                if (rssi > -45) {
                    Log.i(LOG_TAG, "Got strong BLE " + address);
                    poseManager.depositRssi(SystemClock.elapsedRealtime(), address, rssi);
                }
            }
        });
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (bluetoothAdapter == null || !bluetoothAdapter.isEnabled()) {
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
        } else {
            bluetoothManager.resume(bluetoothAdapter);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (bluetoothAdapter != null && bluetoothAdapter.isEnabled()) {
            bluetoothManager.pause();
        }
    }

    @Override
    protected void onDestroy() {
        bluetoothManager.destroy();
        poseManager.free();
        super.onDestroy();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == REQUEST_ENABLE_BT) {
            if (resultCode == Activity.RESULT_CANCELED) {
                finish();
            }
        }
        super.onActivityResult(requestCode, resultCode, data);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String permissions[], @NonNull int[] grantResults) {
        if (requestCode == PERMISSION_REQUEST_COARSE_LOCATION) {
            if (grantResults[0] != PackageManager.PERMISSION_GRANTED) {
                finish();
            }
        }
    }

    private void pointerUpdate() {
        boolean trackingChanged = updateTracking();
        View contentView = findViewById(android.R.id.content);
        if (trackingChanged) {
            if (isTracking) {
                contentView.getOverlay().add(pointer);
            } else {
                contentView.getOverlay().remove(pointer);
            }
            contentView.invalidate();
        }

        if (isTracking) {
            boolean hitTestChanged = updateHitTest();
            if (hitTestChanged) {
                pointer.setEnabled(isHitting);
                contentView.invalidate();
            }
        }
    }

    private void slam3dUpdate() {
        if (!isTracking) {
            return;
        }
        Pose arCorePose = fragment.getArSceneView().getArFrame().getAndroidSensorPose();
        poseManager.depositArCore(SystemClock.elapsedRealtime(), arCorePose);
        for (String bcnName : poseManager.getBcnNames()) {
            if (!bcnNodes.containsKey(bcnName)) {
                bcnNodes.put(bcnName, null);
                addObject(bcnName, Uri.parse("andy.sfb"));
            }
        }
        for (Map.Entry<String, Node> entry : bcnNodes.entrySet()) {
            if (entry.getValue() != null) {
                Pose pose = poseManager.getPoseToDraw(poseManager.getBcnWorldPose(entry.getKey()));
                entry.getValue().setWorldPosition(new Vector3(pose.tx(), pose.ty(), pose.tz()));
                entry.getValue().setWorldRotation(new Quaternion(pose.qx(), pose.qy(), pose.qz(), pose.qw()));
            }
        }
    }

    private boolean updateTracking() {
        Frame frame = fragment.getArSceneView().getArFrame();
        boolean wasTracking = isTracking;
        isTracking = frame != null && frame.getCamera().getTrackingState() == TrackingState.TRACKING;
        return isTracking != wasTracking;
    }

    private boolean updateHitTest() {
        if (!isTracking)
            return false;
        Frame frame = fragment.getArSceneView().getArFrame();
        android.graphics.Point pt = getScreenCenter();
        List<HitResult> hits;
        boolean wasHitting = isHitting;
        isHitting = false;
        if (frame != null) {
            hits = frame.hitTest(pt.x, pt.y);
            for (HitResult hit : hits) {
                Trackable trackable = hit.getTrackable();
                if (trackable instanceof Plane && ((Plane) trackable).isPoseInPolygon(hit.getHitPose())) {
                    isHitting = true;
                    break;
                }
            }
        }
        return wasHitting != isHitting;
    }

    private android.graphics.Point getScreenCenter() {
        View vw = findViewById(android.R.id.content);
        return new android.graphics.Point(vw.getWidth() / 2, vw.getHeight() / 2);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == R.id.action_selectDevice) {
            bluetoothManager.showDeviceSelectDialog();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private void addObject(String bcnName, Uri model) {
        ModelRenderable.builder()
                .setSource(fragment.getContext(), model)
                .build()
                .thenAccept(renderable -> addNodeToScene(bcnName, renderable))
                .exceptionally((throwable -> {
                    AlertDialog.Builder builder = new AlertDialog.Builder(this);
                    builder.setMessage(throwable.getMessage()).setTitle("Error!");
                    AlertDialog dialog = builder.create();
                    dialog.show();
                    return null;
                }));
    }

    private void addNodeToScene(String bcnName, Renderable renderable) {
        Node node = new Node();
        node.setWorldPosition(Vector3.zero());
        node.setWorldRotation(Quaternion.identity());
        node.setRenderable(renderable);
        fragment.getArSceneView().getScene().addChild(node);
        bcnNodes.put(bcnName, node);
    }

    private String generateVioFilename(Date date) {
        String dateString = new SimpleDateFormat("yyyyMMddHHmmss", java.util.Locale.getDefault()).format(date);
        return Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOCUMENTS)
                + File.separator + "arslam/" + dateString + "_vio.csv";
    }

    private String generateUwbFilename(Date date) {
        String dateString = new SimpleDateFormat("yyyyMMddHHmmss", java.util.Locale.getDefault()).format(date);
        return Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOCUMENTS)
                + File.separator + "arslam/" + dateString + "_uwb.csv";
    }

    private String generateRssiFilename(Date date) {
        String dateString = new SimpleDateFormat("yyyyMMddHHmmss", java.util.Locale.getDefault()).format(date);
        return Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOCUMENTS)
                + File.separator + "arslam/" + dateString + "_rssi.csv";
    }

    private String generateTagFilename(Date date) {
        String dateString = new SimpleDateFormat("yyyyMMddHHmmss", java.util.Locale.getDefault()).format(date);
        return Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOCUMENTS)
                + File.separator + "arslam/" + dateString + "_tag.csv";
    }

    private String generateBcnFilename(Date date) {
        String dateString = new SimpleDateFormat("yyyyMMddHHmmss", java.util.Locale.getDefault()).format(date);
        return Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOCUMENTS)
                + File.separator + "arslam/" + dateString + "_bcn.csv";
    }

    private void pressSave() {

    }
}
