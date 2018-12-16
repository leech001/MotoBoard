package ru.nocrash.autooiler_java;

import android.bluetooth.BluetoothGattCharacteristic;
import android.os.Bundle;
import android.support.design.widget.Snackbar;
import android.util.Log;
import android.widget.Button;
import android.widget.EditText;
import android.widget.SeekBar;
import android.widget.TextView;

import com.jakewharton.rx.ReplayingShare;
import com.polidea.rxandroidble2.RxBleConnection;
import com.polidea.rxandroidble2.RxBleDevice;
import com.trello.rxlifecycle2.components.support.RxAppCompatActivity;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.UUID;

import butterknife.BindView;
import butterknife.ButterKnife;
import butterknife.OnClick;
import io.reactivex.Observable;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.subjects.PublishSubject;

import static com.trello.rxlifecycle2.android.ActivityEvent.PAUSE;

public class CharacteristicOperationExampleActivity extends RxAppCompatActivity {

    public static final String EXTRA_CHARACTERISTIC_UUID = "extra_uuid";
    public static final String EXTRA_MAC_ADDRESS = "extra_mac_address";

    @BindView(R.id.receiveText)
    TextView readText;

    @BindView(R.id.wrl_get)
    TextView wrl_get;
    @BindView(R.id.seekBarWRL)
    SeekBar seekBar_wrl;

    @BindView(R.id.ld_get)
    TextView ld_get;
    @BindView(R.id.seekBarLD)
    SeekBar seekBar_ld;

    @BindView(R.id.wrc_get)
    TextView wrc_get;

    @BindView(R.id.lc_get)
    TextView lc_get;

    @BindView(R.id.sp_get)
    TextView sp_get;

    @BindView(R.id.connect)
    Button connectButton;

    private UUID characteristicUuid;
    private PublishSubject<Boolean> disconnectTriggerSubject = PublishSubject.create();
    private Observable<RxBleConnection> connectionObservable;
    private RxBleDevice bleDevice;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_connect);
        ButterKnife.bind(this);
        String macAddress = getIntent().getStringExtra(CharacteristicOperationExampleActivity.EXTRA_MAC_ADDRESS);
        characteristicUuid = (UUID) getIntent().getSerializableExtra(EXTRA_CHARACTERISTIC_UUID);
        bleDevice = AutoOilerApplication.getRxBleClient(this).getBleDevice(macAddress);
        connectionObservable = prepareConnectionObservable();
        //noinspection ConstantConditions
        getSupportActionBar().setSubtitle(getString(R.string.mac_address, macAddress));
//        onConnectToggleClick();


        seekBar_wrl.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
//                wrl_get.setText(String.valueOf(progress));
                send_json("{\"WRL\":" + String.valueOf(progress*300) + "}");
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });

        seekBar_ld.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
//                ld_get.setText(String.valueOf(progress));
                send_json("{\"LD\":" + String.valueOf(progress*200) + "}");
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });
    }

    private void send_json(String json){
        if (isConnected()) {
            connectionObservable
                    .firstOrError()
                    .flatMap(rxBleConnection -> rxBleConnection.writeCharacteristic(characteristicUuid, putData(json)))
                    .observeOn(AndroidSchedulers.mainThread())
                    .subscribe(
                            bytes -> onWriteSuccess(),
                            this::onWriteFailure
                    );
        }
    }

    private Observable<RxBleConnection> prepareConnectionObservable() {
        return bleDevice
                .establishConnection(false)
                .takeUntil(disconnectTriggerSubject)
                .compose(bindUntilEvent(PAUSE))
                .compose(ReplayingShare.instance());
    }

    @OnClick(R.id.connect)
    public void onConnectToggleClick() {

        if (isConnected()) {
            triggerDisconnect();
        } else {
            connectionObservable
                    .flatMapSingle(RxBleConnection::discoverServices)
                    .flatMapSingle(rxBleDeviceServices -> rxBleDeviceServices.getCharacteristic(characteristicUuid))
                    .observeOn(AndroidSchedulers.mainThread())
                    .doOnSubscribe(disposable -> connectButton.setText(R.string.connecting))
                    .subscribe(
                            characteristic -> {
                                updateUI(characteristic);
                                Log.i(getClass().getSimpleName(), "Hey, connection has been established!");
                            },
                            this::onConnectionFailure,
                            this::onConnectionFinished
                    );
        }
    }

    private boolean isConnected() {
        return bleDevice.getConnectionState() == RxBleConnection.RxBleConnectionState.CONNECTED;
    }

    private void onConnectionFailure(Throwable throwable) {
        //noinspection ConstantConditions
        Snackbar.make(findViewById(R.id.main), "Ошибка соединения: " + throwable, Snackbar.LENGTH_SHORT).show();
        updateUI(null);
    }

    private void onConnectionFinished() {
        updateUI(null);
    }


    private void onWriteSuccess() {
        //noinspection ConstantConditions
        Snackbar.make(findViewById(R.id.main), "Установленно", Snackbar.LENGTH_SHORT).show();
    }

    private void onWriteFailure(Throwable throwable) {
        //noinspection ConstantConditions
        Snackbar.make(findViewById(R.id.main), "Ошибка: " + throwable, Snackbar.LENGTH_SHORT).show();
    }

    private void onNotificationReceived(byte[] bytes) {
        //noinspection ConstantConditions
//        Snackbar.make(findViewById(R.id.main), "Change: " + HexString.bytesToHex(bytes), Snackbar.LENGTH_SHORT).show();
        String jString = new String(bytes);
        try {
            JSONObject jsonObject = new JSONObject(jString);
            if (jsonObject.getInt("WRL") != 0) wrl_get.setText(String.valueOf(jsonObject.getInt("WRL")));
        } catch (JSONException e){}

        try {
            JSONObject jsonObject = new JSONObject(jString);
            if (jsonObject.getInt("LD") != 0) ld_get.setText(String.valueOf(jsonObject.getInt("LD")));
        } catch (JSONException e){}

        try {
            JSONObject jsonObject = new JSONObject(jString);
            if (jsonObject.getInt("WRC") != 0)wrc_get.setText(String.valueOf(jsonObject.getInt("WRC")));
        } catch (JSONException e){}

        try {
            JSONObject jsonObject = new JSONObject(jString);
            if (jsonObject.getInt("LC") != 0) lc_get.setText(String.valueOf(jsonObject.getInt("LC")));
        } catch (JSONException e){}

        try {
            JSONObject jsonObject = new JSONObject(jString);
            if (jsonObject.getDouble("SP") != 0) sp_get.setText(String.valueOf(jsonObject.getInt("SP")));
        } catch (JSONException e){}

        readText.setText(jString);
//        Snackbar.make(findViewById(R.id.main), "Change: " + jString, Snackbar.LENGTH_SHORT).show();
    }

    private void onNotificationSetupFailure(Throwable throwable) {
        //noinspection ConstantConditions
        Snackbar.make(findViewById(R.id.main), "Ошибка соединения: " + throwable, Snackbar.LENGTH_SHORT).show();
    }

    private void notificationHasBeenSetUp() {
        //noinspection ConstantConditions
        Snackbar.make(findViewById(R.id.main), "Успешно соединено", Snackbar.LENGTH_SHORT).show();
    }

    private void triggerDisconnect() {
        disconnectTriggerSubject.onNext(true);
    }

    /**
     * This method updates the UI to a proper state.
     *
     * @param characteristic a nullable {@link BluetoothGattCharacteristic}. If it is null then UI is assuming a disconnected state.
     */
    private void updateUI(BluetoothGattCharacteristic characteristic) {
        connectButton.setText(characteristic != null ? R.string.disconnect : R.string.connect);
//        writeButton.setEnabled(hasProperty(characteristic, BluetoothGattCharacteristic.PROPERTY_WRITE));
        if (isConnected()) {
            connectionObservable
                    .flatMap(rxBleConnection -> rxBleConnection.setupNotification(characteristicUuid))
                    .doOnNext(notificationObservable -> runOnUiThread(this::notificationHasBeenSetUp))
                    .flatMap(notificationObservable -> notificationObservable)
                    .observeOn(AndroidSchedulers.mainThread())
                    .subscribe(this::onNotificationReceived, this::onNotificationSetupFailure);
        }
    }

    private boolean hasProperty(BluetoothGattCharacteristic characteristic, int property) {
        return characteristic != null && (characteristic.getProperties() & property) > 0;
    }

    private byte[] putData( String sendString) {
        sendString = sendString + "\r\n";
        return sendString.getBytes();
    }
}
