package com.example.arslam;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanRecord;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.Context;
import android.os.Handler;
import android.os.ParcelUuid;
import android.support.v7.app.AlertDialog;
import android.util.Log;
import android.util.Pair;
import android.widget.ArrayAdapter;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;

public class BluetoothSystemManager {

    public static abstract class RangeCallback {
        abstract public void onBleConnect(String deviceName);
        abstract public void onUwbRange(String bcnName, float range, float quality);
        abstract public void onBleRssi(String address, int rssi);
    }

    private List<BluetoothDevice> discoveredDevices = new ArrayList<>();
    private ArrayAdapter<String> discoveredDeviceNames;
    private BluetoothLeScanner leScanner;
    private ScanSettings scanSettings;
    private List<ScanFilter> scanFilters;
    private Handler scanHandler;
    private BluetoothGatt deviceGatt;
    private RangeCallback rangeCallback;
    private AlertDialog selectDeviceDialog;

//    private Timer timer;
//    private static final long RANGE_PERIOD = 1000L;
    private static final long SCAN_PERIOD = 10000L;

    private static final ParcelUuid NETWORK_NODE_UUID = ParcelUuid.fromString("680c21d9-c946-4c1f-9c11-baa1c21329e7");
    private static final UUID LOCATION_DATA_UUID = UUID.fromString("003bbdf2-c634-4b3d-ab56-7ec889b89a37");
    private static final UUID CLIENT_CHARACTERISTIC_CONFIG = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");

    private static final String LOG_TAG = "BluetoothSystemManager";

    public BluetoothSystemManager(Context context, RangeCallback rangeCallback) {
        scanHandler = new Handler();
        this.rangeCallback = rangeCallback;

        discoveredDeviceNames = new ArrayAdapter<>(context, android.R.layout.select_dialog_singlechoice);
        discoveredDeviceNames.add("Use phone RSSI");
        discoveredDevices.add(null);
        selectDeviceDialog = new AlertDialog.Builder(context)
                .setTitle("Detected Devices:")
                .setCancelable(false)
                .setAdapter(discoveredDeviceNames, (dialog, which) -> {
                    if (which >= 0) {
                        scanLeDevice(false);
                        BluetoothDevice selectedDevice = discoveredDevices.get(which);
                        if (deviceGatt != null) {
                            deviceGatt.close();
                        }
                        if (selectedDevice != null) {
                            Log.i(LOG_TAG, "Connecting to " + selectedDevice.getName());
                            scanForRssi(false);
                            deviceGatt = selectedDevice.connectGatt(context, true, gattCallback);
                        } else {
                            Log.i(LOG_TAG, "Using phone RSSI");
                            deviceGatt = null;
                            scanForRssi(true);
                        }
                    }
                })
                .setNegativeButton("Cancel", null)
                .create();
    }

    public void resume(BluetoothAdapter adapter) {
        leScanner = adapter.getBluetoothLeScanner();
        scanSettings = new ScanSettings.Builder()
                .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
                .build();
        scanFilters = new ArrayList<>();
    }

    public void pause() {
        if (leScanner != null) {
            scanLeDevice(false);
            scanForRssi(false);
        }
    }

    public void destroy() {
        if (deviceGatt != null) {
            deviceGatt.close();
            deviceGatt = null;
        }
        if (leScanner != null) {
            scanLeDevice(false);
            scanForRssi(false);
        }
    }

    public void showDeviceSelectDialog() {
        selectDeviceDialog.show();
        if (leScanner != null)
            scanLeDevice(true);
    }

    private void scanLeDevice(boolean enable) {
        if (enable) {
            scanHandler.postDelayed(() -> leScanner.stopScan(leScanForConnectionCallback), SCAN_PERIOD);
            leScanner.startScan(scanFilters, scanSettings, leScanForConnectionCallback);
        } else {
            leScanner.stopScan(leScanForConnectionCallback);
        }
    }

    private void scanForRssi(boolean enable) {
        if (enable) {
            leScanner.startScan(scanFilters, scanSettings, leScanForRssiCallback);
        } else {
            leScanner.stopScan(leScanForRssiCallback);
        }
    }

    private ScanCallback leScanForConnectionCallback = new ScanCallback() {
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            addScannedDevice(result);
        }

        @Override
        public void onBatchScanResults(List<ScanResult> results) {
            for (ScanResult result : results) {
                addScannedDevice(result);
            }
        }

        @Override
        public void onScanFailed(int errorCode) {
            Log.e(LOG_TAG, "Scan Failed. Error Code: " + errorCode);
        }
    };

    private ScanCallback leScanForRssiCallback = new ScanCallback() {
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            handleScanResult(result);
        }

        @Override
        public void onBatchScanResults(List<ScanResult> results) {
            for (ScanResult result : results) {
                handleScanResult(result);
            }
        }

        @Override
        public void onScanFailed(int errorCode) {
            Log.e(LOG_TAG, "Scan Failed. Error Code: " + errorCode);
        }

        private void handleScanResult(ScanResult result) {
            rangeCallback.onBleRssi(result.getDevice().getAddress(), result.getRssi());
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
                    rangeCallback.onBleConnect(gatt.getDevice().getName());
//                    gatt.requestMtu(512);
                    gatt.discoverServices();
                    break;
                case BluetoothProfile.STATE_DISCONNECTED:
                    Log.e(LOG_TAG, "STATE_DISCONNECTED");
//                    if (timer != null) {
//                        timer.cancel();
//                        timer = null;
//                    }
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
//                        timer = new Timer();
//                        timer.schedule(new TimerTask() {
//                            @Override
//                            public void run() {
//                                gatt.readCharacteristic(characteristic);
//                            }
//                        }, 0, RANGE_PERIOD);
                        gatt.setCharacteristicNotification(characteristic, true);
                        BluetoothGattDescriptor descriptor = characteristic.getDescriptor(CLIENT_CHARACTERISTIC_CONFIG);
                        descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
                        gatt.writeDescriptor(descriptor);
                    }
                }
            }
        }

//        @Override
//        public void onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
//            HashMap<String, Pair<Float, Float>> ranges = decodeCharacteristic(characteristic);
//            Log.i(LOG_TAG, "got ranges: " + ranges.toString() + " " + characteristic.getProperties());
//            for (Map.Entry<String, Pair<Float, Float>> range : ranges.entrySet()) {
//                rangeCallback.onUwbRange(range.getKey(), range.getValue().first, range.getValue().second);
//            }
//        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
            HashMap<String, Pair<Float, Float>> ranges = decodeCharacteristic(characteristic);
            Log.i(LOG_TAG, "got ranges: " + ranges.toString() + " " + characteristic.getProperties());
            for (Map.Entry<String, Pair<Float, Float>> range : ranges.entrySet()) {
                rangeCallback.onUwbRange(range.getKey(), range.getValue().first, range.getValue().second);
            }
        }
    };

    private HashMap<String, Pair<Float, Float>> decodeCharacteristic(BluetoothGattCharacteristic characteristic) {
        HashMap<String, Pair<Float, Float>> ranges = new HashMap<>();
        int offset = 0;
//        Log.i(LOG_TAG, "total bytes " + characteristic.getValue().length);
        Integer type = characteristic.getIntValue(BluetoothGattCharacteristic.FORMAT_UINT8, offset);
        if (type == null) {
            return ranges;
        }
        offset += 1;
        if (type == 2) {
            offset += 13;
//            Log.i(LOG_TAG, "position type");
        } else if (type != 1) {
            return ranges;
        } else {
//            Log.i(LOG_TAG, "range type");
        }
        Integer distanceCount = characteristic.getIntValue(BluetoothGattCharacteristic.FORMAT_UINT8, offset);
        if (distanceCount == null) {
            return ranges;
        }
        Log.i(LOG_TAG, distanceCount.toString());
        offset += 1;
        for (int i = 0; i < distanceCount; ++i) {
            Integer nodeId = characteristic.getIntValue(BluetoothGattCharacteristic.FORMAT_UINT16, offset);
            if (nodeId == null) {
//                Log.i(LOG_TAG, "null nodeId");
            }
            offset += 2;
            Integer range = characteristic.getIntValue(BluetoothGattCharacteristic.FORMAT_SINT32, offset);
            if (range == null) {
//                Log.i(LOG_TAG, "null range");
            }
            offset += 4;
            Integer quality = characteristic.getIntValue(BluetoothGattCharacteristic.FORMAT_SINT8, offset);
            if (quality == null) {
//                Log.i(LOG_TAG, "null quality");
            }
            offset += 1;
            if (nodeId == null || range == null || quality == null) {
                return ranges;
            }
            ranges.put(nodeId.toString(), new Pair<>(range / 1000.0f, quality / 100.0f));
        }
        return ranges;
    }
}
