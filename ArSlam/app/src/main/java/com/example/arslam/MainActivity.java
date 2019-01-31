package com.example.arslam;

import android.Manifest;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanRecord;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.ParcelUuid;
import android.os.SystemClock;
import android.support.annotation.NonNull;
import android.support.design.widget.FloatingActionButton;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.Toast;

import com.google.ar.core.Anchor;
import com.google.ar.core.Frame;
import com.google.ar.core.HitResult;
import com.google.ar.core.Plane;
import com.google.ar.core.Pose;
import com.google.ar.core.Trackable;
import com.google.ar.core.TrackingState;
import com.google.ar.sceneform.AnchorNode;
import com.google.ar.sceneform.rendering.ModelRenderable;
import com.google.ar.sceneform.rendering.Renderable;
import com.google.ar.sceneform.ux.ArFragment;
import com.google.ar.sceneform.ux.TransformableNode;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;

public class MainActivity extends AppCompatActivity {

    private ArFragment fragment;
    private PointerDrawable pointer = new PointerDrawable();
    private boolean isTracking;
    private boolean isHitting;

    private AlertDialog selectDeviceDialog;
    private List<BluetoothDevice> discoveredDevices = new ArrayList<>();
    private ArrayAdapter<String> discoveredDeviceNames;
    private BluetoothAdapter bluetoothAdapter;
    private BluetoothLeScanner leScanner;
    private ScanSettings scanSettings;
    private List<ScanFilter> scanFilters;
    private Handler scanHandler;
    private BluetoothGatt deviceGatt;

    private Slam3dJni slam3d;
    private ArrayList<Slam3dJni.TagLocation> tagLocations = new ArrayList<>();

    private static final ParcelUuid NETWORK_NODE_UUID = ParcelUuid.fromString("680c21d9-c946-4c1f-9c11-baa1c21329e7");
    private static final UUID LOCATION_DATA_UUID = UUID.fromString("003bbdf2-c634-4b3d-ab56-7ec889b89a37");
    private static final long SCAN_PERIOD = 10000L;
    private static final int REQUEST_ENABLE_BT = 1;
    private static final int PERMISSION_REQUEST_COARSE_LOCATION = 2;
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

        initializeGallery();
        slam3d = new Slam3dJni();

        scanHandler = new Handler();
        if (!getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)) {
            Toast.makeText(this, "BLE Not Supported", Toast.LENGTH_LONG).show();
            finish();
        }
        final BluetoothManager bluetoothManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        bluetoothAdapter = bluetoothManager.getAdapter();

        requestPermissions(new String[]{Manifest.permission.ACCESS_COARSE_LOCATION}, PERMISSION_REQUEST_COARSE_LOCATION);

        discoveredDeviceNames = new ArrayAdapter<>(this, android.R.layout.select_dialog_singlechoice);
        selectDeviceDialog = new AlertDialog.Builder(this)
                .setTitle("Detected Devices:")
                .setCancelable(false)
                .setAdapter(discoveredDeviceNames, (dialog, which) -> {
                    if (which >= 0) {
                        BluetoothDevice selectedDevice = discoveredDevices.get(which);
                        if (selectedDevice != null) {
                            if (deviceGatt != null) {
                                deviceGatt.close();
                            }
                            Log.i(LOG_TAG, "Connecting to " + selectedDevice.getName());
                            scanLeDevice(false);
                            deviceGatt = selectedDevice.connectGatt(MainActivity.this, false, gattCallback);
                        }
                    }
                })
                .setNegativeButton("Cancel", null)
                .create();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (bluetoothAdapter == null || !bluetoothAdapter.isEnabled()) {
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
        } else {
            leScanner = bluetoothAdapter.getBluetoothLeScanner();
            scanSettings = new ScanSettings.Builder()
                    .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
                    .build();
            scanFilters = new ArrayList<>();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (bluetoothAdapter != null && bluetoothAdapter.isEnabled()) {
            scanLeDevice(false);
        }
    }

    @Override
    protected void onDestroy() {
        if (deviceGatt != null) {
            deviceGatt.close();
            deviceGatt = null;
        }
        slam3d.free();
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

    private void scanLeDevice(boolean enable) {
        if (enable) {
            scanHandler.postDelayed(() -> leScanner.stopScan(leScanCallback), SCAN_PERIOD);
            leScanner.startScan(scanFilters, scanSettings, leScanCallback);
        } else {
            leScanner.stopScan(leScanCallback);
        }
    }

    private ScanCallback leScanCallback = new ScanCallback() {
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
//            Log.i(LOG_TAG, "callbackType: " + String.valueOf(callbackType));
//            Log.i(LOG_TAG, "result: " + result.toString());
            addScannedDevice(result);
        }

        @Override
        public void onBatchScanResults(List<ScanResult> results) {
            for (ScanResult result : results) {
//                Log.i(LOG_TAG, "ScanResult - Results: " + result.toString());
                addScannedDevice(result);
            }
        }

        @Override
        public void onScanFailed(int errorCode) {
            Log.e(LOG_TAG, "Scan Failed. Error Code: " + errorCode);
        }
    };

    private void addScannedDevice(ScanResult result) {
        BluetoothDevice device = result.getDevice();
        if (discoveredDevices.contains(device)) {
            return;
        }
        if (device == null || device.getName() == null) {
            return;
        }
        ScanRecord scanRecord = result.getScanRecord();
        if (scanRecord == null) {
            return;
        }
        Map<ParcelUuid, byte[]> serviceData = scanRecord.getServiceData();
        if (serviceData == null || !serviceData.containsKey(NETWORK_NODE_UUID)) {
            return;
        }
        discoveredDevices.add(device);
        discoveredDeviceNames.add(device.getName());
        ((ArrayAdapter) selectDeviceDialog.getListView().getAdapter()).notifyDataSetChanged();
    }

    private final BluetoothGattCallback gattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            Log.i(LOG_TAG, "onConnectionStateChange: Status: " + status);
            switch (newState) {
                case BluetoothProfile.STATE_CONNECTED:
                    Log.i(LOG_TAG, "STATE_CONNECTED");
                    gatt.discoverServices();
                    break;
                case BluetoothProfile.STATE_DISCONNECTED:
                    Log.e(LOG_TAG, "STATE_DISCONNECTED");
                    break;
                default:
                    Log.e(LOG_TAG, "STATE_OTHER");
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            List<BluetoothGattService> services = gatt.getServices();
            if (services == null) {
                return;
            }
            for (BluetoothGattService service : services) {
                if (service == null) {
                    continue;
                }
                List<BluetoothGattCharacteristic> characteristics = service.getCharacteristics();
                if (characteristics == null) {
                    continue;
                }
                for (BluetoothGattCharacteristic characteristic : characteristics) {
                    if (characteristic == null) {
                        continue;
                    }
                    if (characteristic.getUuid().equals(LOCATION_DATA_UUID)) {
                        deviceGatt.readCharacteristic(characteristic);
                    }
                }
            }
        }

        @Override
        public void onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
            Log.i(LOG_TAG, "onCharacteristicRead: " + decodeCharacteristic(characteristic));
        }
    };

    private HashMap<String, Float> decodeCharacteristic(BluetoothGattCharacteristic characteristic) {
        HashMap<String, Float> ranges = new HashMap<>();
        if (characteristic.getValue().length == 0) {
            return ranges;
        }
        int offset = 0;
        Integer type = characteristic.getIntValue(BluetoothGattCharacteristic.FORMAT_UINT8, offset);
        offset += 1;
        if (type == 2) {
            offset += 13;
        } else if (type != 1) {
            return ranges;
        }
        Integer distanceCount = characteristic.getIntValue(BluetoothGattCharacteristic.FORMAT_UINT8, offset);
        offset += 1;
        for (int i = 0; i < distanceCount; ++i) {
            Integer nodeId = characteristic.getIntValue(BluetoothGattCharacteristic.FORMAT_UINT16, offset);
            offset += 2;
            Integer range = characteristic.getIntValue(BluetoothGattCharacteristic.FORMAT_SINT32, offset);
            offset += 4;
            offset += 1; // quality factor
            ranges.put(nodeId.toString(), range / 1000.0f);
        }
        return ranges;
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
        Pose pose = fragment.getArSceneView().getArFrame().getAndroidSensorPose();
        slam3d.depositVio(SystemClock.elapsedRealtime() / 1000.0, pose.tx(), pose.ty(), pose.tz());
        tagLocations.add(new Slam3dJni.TagLocation(slam3d.tagLocation));
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
            selectDeviceDialog.show();
            scanLeDevice(true);
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private void initializeGallery() {
        LinearLayout gallery = findViewById(R.id.gallery_layout);

        ImageView andy = new ImageView(this);
        andy.setImageResource(R.drawable.droid_thumb);
        andy.setContentDescription("andy");
        andy.setOnClickListener(view -> {addObject(Uri.parse("andy.sfb"));});
        gallery.addView(andy);

        ImageView cabin = new ImageView(this);
        cabin.setImageResource(R.drawable.cabin_thumb);
        cabin.setContentDescription("cabin");
        cabin.setOnClickListener(view -> {addObject(Uri.parse("Cabin.sfb"));});
        gallery.addView(cabin);

        ImageView house = new ImageView(this);
        house.setImageResource(R.drawable.house_thumb);
        house.setContentDescription("house");
        house.setOnClickListener(view -> {addObject(Uri.parse("House.sfb"));});
        gallery.addView(house);

        ImageView igloo = new ImageView(this);
        igloo.setImageResource(R.drawable.igloo_thumb);
        igloo.setContentDescription("igloo");
        igloo.setOnClickListener(view -> {addObject(Uri.parse("igloo.sfb"));});
        gallery.addView(igloo);
    }

    private void addObject(Uri model) {
        Frame frame = fragment.getArSceneView().getArFrame();
        android.graphics.Point pt = getScreenCenter();
        List<HitResult> hits;
        if (frame != null && isTracking) {
            hits = frame.hitTest(pt.x, pt.y);
            for (HitResult hit : hits) {
                Trackable trackable = hit.getTrackable();
                if (trackable instanceof Plane && ((Plane) trackable).isPoseInPolygon(hit.getHitPose())) {
                    placeObject(fragment, hit.createAnchor(), model);
                    break;
                }
            }
        }
    }

    private void placeObject(ArFragment fragment, Anchor anchor, Uri model) {
        ModelRenderable.builder()
                .setSource(fragment.getContext(), model)
                .build()
                .thenAccept(renderable -> addNodeToScene(fragment, anchor, renderable))
                .exceptionally((throwable -> {
                    AlertDialog.Builder builder = new AlertDialog.Builder(this);
                    builder.setMessage(throwable.getMessage()).setTitle("Error!");
                    AlertDialog dialog = builder.create();
                    dialog.show();
                    return null;
                }));
    }

    private void addNodeToScene(ArFragment fragment, Anchor anchor, Renderable renderable) {
        AnchorNode anchorNode = new AnchorNode(anchor);
        TransformableNode node = new TransformableNode(fragment.getTransformationSystem());
        node.setRenderable(renderable);
        node.setParent(anchorNode);
        fragment.getArSceneView().getScene().addChild(anchorNode);
        node.select();
    }

    private String generateTagFilename() {
        String date = new SimpleDateFormat("yyyyMMddHHmmss", java.util.Locale.getDefault()).format(new Date());
        return Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOCUMENTS)
                + File.separator + "arslam/" + date + "_tag.csv";
    }

    private String generateBcnFilename() {
        String date = new SimpleDateFormat("yyyyMMddHHmmss", java.util.Locale.getDefault()).format(new Date());
        return Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOCUMENTS)
                + File.separator + "arslam/" + date + "_bcn.csv";
    }

    private void saveTraceToDisk(String filename) throws IOException {
        File out = new File(filename);
        if (!out.getParentFile().exists()) {
            out.getParentFile().mkdirs();
        }
        try (FileOutputStream stream = new FileOutputStream(out)) {
            for (Slam3dJni.TagLocation loc : tagLocations) {
                stream.write((loc.toString() + "\n").getBytes());
            }
        }
        tagLocations.clear();
    }

    private void saveBcnToDisk(String filename) throws IOException {
        File out = new File(filename);
        if (!out.getParentFile().exists()) {
            out.getParentFile().mkdirs();
        }
        try (FileOutputStream stream = new FileOutputStream(out)) {
            for (Map.Entry<String, Slam3dJni.BcnLocation> entry : slam3d.bcnLocations.entrySet()) {
                stream.write((entry.getKey() + "," + entry.getValue().toString() + "\n").getBytes());
            }
        }
    }

    private void pressSave() {
        final String tagFilename = generateTagFilename();
        final String bcnFilename = generateBcnFilename();
        try {
            saveTraceToDisk(tagFilename);
            saveBcnToDisk(bcnFilename);
        } catch (IOException e) {
            Toast toast = Toast.makeText(MainActivity.this, e.toString(), Toast.LENGTH_LONG);
            toast.show();
            return;
        }
        Toast toast = Toast.makeText(MainActivity.this, "Saved trace", Toast.LENGTH_LONG);
        toast.show();
    }
}
