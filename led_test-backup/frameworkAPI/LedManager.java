package android.os;
import android.content.Context;
import android.os.Binder;
import android.os.Bundle;
import android.os.Parcelable;
import android.os.ParcelFileDescriptor;
import android.os.Process;
import android.os.RemoteException;
import android.os.Handler;
import android.os.Message;
import android.os.ServiceManager;
import android.util.Log;
import android.os.ILedService;
 
public class LedManager{
 
	private static final String TAG="LedManager";
	private ILedService mLedService;
	private Context mContext;
 
	public LedManager(Context context, ILedService service){
			mContext = context;
			mLedService = service;
	}
 
	public boolean LedOn(){
		try{
			return mLedService.setOn();
		}catch(RemoteException e){
			Log.e(TAG,"RemoteException in LedManager Ledon",e);
			return false;
		}
	}
 
	public boolean LedOff(){
		try{
			return mLedService.setOff();
		}catch(RemoteException e){
			Log.e(TAG,"RemoteException in LedManager Ledoff",e);
			return false;
		}
	}
}
