package com.android.server.led;
 
import android.util.Config;
import android.util.Log;
import android.content.Context;
import android.os.Binder;
import android.os.Bundle;
import android.os.RemoteException;
import android.os.IBinder;
import android.os.ILedService;
 
public final class LedService extends ILedService.Stub{
 
	private Context mContext;
 
	public LedService(Context context){
		super();
		mContext = context;
		Log.i("LedService","Go to get Led Stub");
		init_native();
	}
	
	public boolean setOn(){
		Log.i("LedService","Led On");
		return led_on(0);
	}
 
	public boolean setOff(){
		Log.i("LedService","Led Off");
		return led_off(1);
	}
 
	private static native boolean init_native(); //和jni里面的对应
	private static native boolean led_on(int led);
	private static native boolean led_off(int led);
}

