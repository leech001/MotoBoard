package ru.nocrash.chainoiler

import android.bluetooth.BluetoothGattCharacteristic
import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.util.Log
import android.widget.SeekBar
import androidx.appcompat.app.AppCompatActivity
import com.jakewharton.rx.ReplayingShare
import com.polidea.rxandroidble2.RxBleConnection
import com.polidea.rxandroidble2.RxBleDevice
import io.reactivex.Observable
import io.reactivex.android.schedulers.AndroidSchedulers
import io.reactivex.disposables.CompositeDisposable
import io.reactivex.subjects.PublishSubject
import kotlinx.android.synthetic.main.activity_characteristic_operation.*
import org.json.JSONException
import org.json.JSONObject
import ru.nocrash.chainoiler.util.isConnected
import ru.nocrash.chainoiler.util.showSnackbarShort
import java.util.*

private const val EXTRA_MAC_ADDRESS = "extra_mac_address"
private const val EXTRA_CHARACTERISTIC_UUID = "extra_uuid"

class CharacteristicOperationActivity : AppCompatActivity() {

    companion object {
        fun newInstance(context: Context, macAddress: String, uuid: UUID) =
            Intent(context, CharacteristicOperationActivity::class.java).apply {
                putExtra(EXTRA_MAC_ADDRESS, macAddress)
                putExtra(EXTRA_CHARACTERISTIC_UUID, uuid)
            }
    }

    private lateinit var characteristicUuid: UUID

    private val disconnectTriggerSubject = PublishSubject.create<Unit>()

    private lateinit var connectionObservable: Observable<RxBleConnection>

    private val connectionDisposable = CompositeDisposable()

    private lateinit var bleDevice: RxBleDevice


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_characteristic_operation)

        val macAddress = intent.getStringExtra(EXTRA_MAC_ADDRESS)
        characteristicUuid = intent.getSerializableExtra(EXTRA_CHARACTERISTIC_UUID) as UUID
        bleDevice = ChainOilerApplication.rxBleClient.getBleDevice(macAddress)
        connectionObservable = prepareConnectionObservable()
        supportActionBar!!.subtitle = getString(R.string.mac_address, macAddress)

        connect.setOnClickListener { onConnectToggleClick() }

        seekBarWRL.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {

            override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {

                sendParam("{\"WRLB\":" + (progress * 300).toString() + "}")
            }

            override fun onStartTrackingTouch(seekBar: SeekBar) {

            }

            override fun onStopTrackingTouch(seekBar: SeekBar) {

            }
        })

        seekBarLD.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {

            override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {

                sendParam("{\"LD\":" + (progress * 200).toString() + "}")
            }

            override fun onStartTrackingTouch(seekBar: SeekBar) {

            }

            override fun onStopTrackingTouch(seekBar: SeekBar) {

            }
        })
    }


    private fun prepareConnectionObservable(): Observable<RxBleConnection> =
        bleDevice
            .establishConnection(false)
            .takeUntil(disconnectTriggerSubject)
            .compose(ReplayingShare.instance())

    private fun onConnectToggleClick() {
        if (bleDevice.isConnected) {
            triggerDisconnect()
        } else {
            connectionObservable
                .flatMapSingle { it.discoverServices() }
                .flatMapSingle { it.getCharacteristic(characteristicUuid) }
                .observeOn(AndroidSchedulers.mainThread())
                .doOnSubscribe { connect.setText(R.string.connecting) }
                .subscribe(
                    { characteristic ->
                        updateUI(characteristic)
                        Log.i(javaClass.simpleName, "Соединение успешно установлено")
                    },
                    { onConnectionFailure(it) },
                    { updateUI(null) }
                )
                .let { connectionDisposable.add(it) }
        }
    }

    private fun onNotifyClick() {
        if (bleDevice.isConnected) {
            connectionObservable
                .flatMap { it.setupNotification(characteristicUuid) }
                .doOnNext { runOnUiThread { notificationHasBeenSetUp() } }
                // we have to flatmap in order to get the actual notification observable
                // out of the enclosing observable, which only performed notification setup
                .flatMap { it }
                .observeOn(AndroidSchedulers.mainThread())
                .subscribe({ onNotificationReceived(it) }, { onNotificationSetupFailure(it) })
                .let { connectionDisposable.add(it) }
        }
    }


    private fun sendParam(json: String) {
        if (bleDevice.isConnected) {
            connectionObservable
                .firstOrError()
                .flatMap { it.writeCharacteristic(characteristicUuid, putData(json)) }
                .observeOn(AndroidSchedulers.mainThread())
                .subscribe({ onWriteSuccess() }, { onWriteFailure(it) })
                .let { connectionDisposable.add(it) }
        }
    }


    private fun putData(sendString: String): ByteArray {
        val param = sendString + "\r\n"
        return param.toByteArray()
    }


    private fun onConnectionFailure(throwable: Throwable) {
        showSnackbarShort("Ошибка соединения: $throwable")
        updateUI(null)
    }


    private fun onWriteSuccess() = showSnackbarShort("Успешно установленно")

    private fun onWriteFailure(throwable: Throwable) = showSnackbarShort("Ошибка установки: $throwable")

    private fun onNotificationReceived(bytes: ByteArray) {

        val jString = String(bytes)

        try {
            val jsonObject = JSONObject(jString)
            if (jsonObject.getInt("WRL") != 0) wrl_get.text = jsonObject.getInt("WRL").toString()
        } catch (e: JSONException) {
        }


        try {
            val jsonObject = JSONObject(jString)
            if (jsonObject.getInt("LD") != 0) ld_get.text = jsonObject.getInt("LD").toString()
        } catch (e: JSONException) {
        }


        try {
            val jsonObject = JSONObject(jString)
            if (jsonObject.getInt("WRC") != 0) wrc_get.text = jsonObject.getInt("WRC").toString()
        } catch (e: JSONException) {
        }


        try {
            val jsonObject = JSONObject(jString)
            if (jsonObject.getInt("LC") != 0) lc_get.text = jsonObject.getInt("LC").toString()
        } catch (e: JSONException) {
        }


        try {
            val jsonObject = JSONObject(jString)
            if (jsonObject.getDouble("SP") != 0.0) sp_get.text = jsonObject.getInt("SP").toString()
        } catch (e: JSONException) {
        }

        receiveText.text = jString

//        showSnackbarShort(jString)

    }

    private fun onNotificationSetupFailure(throwable: Throwable) =
        showSnackbarShort("Ошибка уведомлений: $throwable")

    private fun notificationHasBeenSetUp() = showSnackbarShort("Уведомления включены")

    private fun triggerDisconnect() = disconnectTriggerSubject.onNext(Unit)


    private fun updateUI(characteristic: BluetoothGattCharacteristic?) {
        if (characteristic == null) {
            connect.setText(R.string.button_connect)
        } else {
            connect.setText(R.string.button_disconnect)

            onNotifyClick()
        }
    }

    override fun onPause() {
        super.onPause()
        connectionDisposable.clear()
    }


}

