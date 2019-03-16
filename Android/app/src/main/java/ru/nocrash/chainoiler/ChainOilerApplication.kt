package ru.nocrash.chainoiler

import android.app.Application
import com.polidea.rxandroidble2.RxBleClient
import com.polidea.rxandroidble2.internal.RxBleLog

class ChainOilerApplication : Application() {

    companion object {
        lateinit var rxBleClient: RxBleClient
            private set
    }

    override fun onCreate() {
        super.onCreate()
        rxBleClient = RxBleClient.create(this)
        RxBleClient.setLogLevel(RxBleLog.VERBOSE)
    }
}