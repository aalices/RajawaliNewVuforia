package org.rajawali3d.vuforia;

import android.app.ActionBar;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.ViewGroup;
import android.view.WindowManager;

import com.vuforia.Vuforia;

import org.rajawali3d.renderer.RajawaliRenderer;
import org.rajawali3d.surface.IRajawaliSurface;
import org.rajawali3d.surface.RajawaliSurfaceView;
import org.rajawali3d.util.RajLog;

public abstract class RajawaliVuforiaActivity extends Activity {
	protected static int TRACKER_TYPE_IMAGE = 0;
	protected static int TRACKER_TYPE_MARKER = 1;
		
    private static final int APPSTATUS_UNINITED         = -1;
    private static final int APPSTATUS_INIT_APP         = 0;
    private static final int APPSTATUS_INIT_Vuforia        = 1;
    private static final int APPSTATUS_INIT_APP_AR      = 2;
    private static final int APPSTATUS_INIT_TRACKER     = 3;
    private static final int APPSTATUS_INIT_CLOUDRECO 	= 4;
    private static final int APPSTATUS_INITED           = 5;
    private static final int APPSTATUS_CAMERA_STOPPED   = 6;
    private static final int APPSTATUS_CAMERA_RUNNING   = 7;
    private static final int FOCUS_MODE_NORMAL = 0;
    private static final int FOCUS_MODE_CONTINUOUS_AUTO = 1;

 // These codes match the ones defined in TargetFinder.h for Cloud Reco service
    static final int INIT_SUCCESS = 2;
    static final int INIT_ERROR_NO_NETWORK_CONNECTION = -1;
    static final int INIT_ERROR_SERVICE_NOT_AVAILABLE = -2;
    static final int UPDATE_ERROR_AUTHORIZATION_FAILED = -1;
    static final int UPDATE_ERROR_PROJECT_SUSPENDED = -2;
    static final int UPDATE_ERROR_NO_NETWORK_CONNECTION = -3;
    static final int UPDATE_ERROR_SERVICE_NOT_AVAILABLE = -4;
    static final int UPDATE_ERROR_BAD_FRAME_QUALITY = -5;
    static final int UPDATE_ERROR_UPDATE_SDK = -6;
    static final int UPDATE_ERROR_TIMESTAMP_OUT_OF_RANGE = -7;
    static final int UPDATE_ERROR_REQUEST_TIMEOUT = -8;
    
	private static final String NATIVE_LIB_VUFORIA = "Vuforia";
	private static final String NATIVE_LIB_RAJAWALI_VUFORIA = "RajawaliVuforia";

    private RajawaliRenderer mRenderer;
    private RajawaliSurfaceView mSurfaceView;
    private int mScreenWidth = 0;
    private int mScreenHeight = 0;
    private int mAppStatus = APPSTATUS_UNINITED;
    private int mFocusMode;
    private int mScreenOrientation = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
    private InitVuforiaTask mInitVuforiaTask;
    private Object mShutdownLock = new Object();
    private boolean mUseCloudRecognition = false;
    private InitCloudRecoTask mInitCloudRecoTask;
    private boolean mRajawaliIsInitialized;
    
	static
	{
		loadLibrary(NATIVE_LIB_VUFORIA);
		loadLibrary(NATIVE_LIB_RAJAWALI_VUFORIA);
	}

	 /** An async task to initialize cloud-based recognition asynchronously. */
    private class InitCloudRecoTask extends AsyncTask<Void, Integer, Boolean>
    {
        // Initialize with invalid value
        private int mInitResult = -1;

        protected Boolean doInBackground(Void... params)
        {
            // Prevent the onDestroy() method to overlap:
            synchronized (mShutdownLock)
            {
                // Init cloud-based recognition:
                mInitResult = initCloudReco();
                return mInitResult == INIT_SUCCESS;
            }
        }


        protected void onPostExecute(Boolean result)
        {
            RajLog.d("InitCloudRecoTask::onPostExecute: execution "
                    + (result ? "successful" : "failed"));

            if (result)
            {
                // Done loading the tracker, update application status:
                updateApplicationStatus(APPSTATUS_INITED);
             }
            else
            {
                updateApplicationStatus(APPSTATUS_INITED);
               // Create dialog box for display error:
                AlertDialog dialogError = new AlertDialog.Builder(
                        RajawaliVuforiaActivity.this).create();
 
                dialogError.setButton
                (
                    DialogInterface.BUTTON_POSITIVE,
                    "Close",
                    new DialogInterface.OnClickListener()
                    {
                        public void onClick(DialogInterface dialog, int which)
                        {
  //                          System.exit(1);
                        }
                    }
                );


                String logMessage;

                if (mInitResult == Vuforia.INIT_DEVICE_NOT_SUPPORTED)
                {
                    logMessage = "Failed to initialize Vuforia because this " +
                        "device is not supported.";
                }
                else
                {
                    logMessage = "Failed to initialize CloudReco.";
                }

                RajLog.e("InitVuforiaTask::onPostExecute: " + logMessage +
                        " Exiting.");

                dialogError.setMessage(logMessage);
                dialogError.show();
            }
        }
    }
    
    /** An async task to initialize Vuforia asynchronously. */
    private class InitVuforiaTask extends AsyncTask<Void, Integer, Boolean>
    {
        private int mProgressValue = -1;

        protected Boolean doInBackground(Void... params)
        {
            synchronized (mShutdownLock)
            {
                Vuforia.setInitParameters(RajawaliVuforiaActivity.this, Vuforia.GL_20, "AVIvPfn/////AAAAAQjwyymGBEvLugTPCEDY3fcDnQPK+xugA9lh4VltJRh6ivUK7pCFogIA9JaQmIn4iO77VXP53dUnSR+eKZDuvZvmOaivZm+dBCrUI+lgwdGBsx9sSflrmnu8qLhYU8Tq/slhb5kIej/il7JNJSmZKy+VAd22okRB01U9UuI9J3HYXVpPplMPXNdRwcX7LhMigqueDCeGzvpXinhcn5MyFqmU6pi+zb7/pqHXLr9qekKQbnaiDmQ3uu5Udk26rEsjp2G9XHhmpfYY+UqMka45el2Q8FVFsSThVgHYjkEVEmSBSkeQyDwuUR3dkZ/T3DAD6dOBUAhrSzipwq1+yICZy/Gud94eIu95kxgirgCD9Qdb");

                do
                {
                    mProgressValue = Vuforia.init();
                    publishProgress(mProgressValue);
                } while (!isCancelled() && mProgressValue >= 0
                         && mProgressValue < 100);

                return (mProgressValue > 0);
            }
        }

        protected void onProgressUpdate(Integer... values)
        {
        }

        protected void onPostExecute(Boolean result)
        {
            if (result)
            {
                RajLog.d("InitVuforiaTask::onPostExecute: Vuforia " +
                              "initialization successful");

                updateApplicationStatus(APPSTATUS_INIT_TRACKER);
            }
            else
            {
                AlertDialog dialogError = new AlertDialog.Builder(
                    RajawaliVuforiaActivity.this
                ).create();

                dialogError.setButton
                (
                    DialogInterface.BUTTON_POSITIVE,
                    "Close",
                    new DialogInterface.OnClickListener()
                    {
                        public void onClick(DialogInterface dialog, int which)
                        {
                            System.exit(1);
                        }
                    }
                );

                String logMessage;

                if (mProgressValue == Vuforia.INIT_DEVICE_NOT_SUPPORTED)
                {
                    logMessage = "Failed to initialize Vuforia because this " +
                        "device is not supported.";
                }
                else
                {
                    logMessage = "Failed to initialize Vuforia.";
                }

                RajLog.e("InitVuforiaTask::onPostExecute: " + logMessage +
                                " Exiting.");

                dialogError.setMessage(logMessage);
                dialogError.show();
            }
        }
    }
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		storeScreenDimensions();
	}
	
	protected void startVuforia()
	{
		updateApplicationStatus(APPSTATUS_INIT_APP);
	}

	protected void onResume()
    {
        super.onResume();
        Vuforia.onResume();

        if (mAppStatus == APPSTATUS_CAMERA_STOPPED)
        {
            updateApplicationStatus(APPSTATUS_CAMERA_RUNNING);
        }
    };
	
    public void onConfigurationChanged(Configuration config)
    {
        super.onConfigurationChanged(config);

        storeScreenDimensions();

        if (Vuforia.isInitialized() && (mAppStatus == APPSTATUS_CAMERA_RUNNING))
            setProjectionMatrix();
    }
    
    protected void onPause()
    {
        super.onPause();
        
        if (mAppStatus == APPSTATUS_CAMERA_RUNNING)
        {
            updateApplicationStatus(APPSTATUS_CAMERA_STOPPED);
        }
        Vuforia.onPause();
    }

    protected void onDestroy()
    {
        super.onDestroy();

        if (mInitVuforiaTask != null &&
            mInitVuforiaTask.getStatus() != InitVuforiaTask.Status.FINISHED)
        {
            mInitVuforiaTask.cancel(true);
            mInitVuforiaTask = null;
        }

        if (mInitCloudRecoTask != null
                && mInitCloudRecoTask.getStatus() != InitCloudRecoTask.Status.FINISHED)
        {
            mInitCloudRecoTask.cancel(true);
            mInitCloudRecoTask = null;
        }
        
        synchronized (mShutdownLock) {
        	destroyTrackerData();
            deinitApplicationNative();
            deinitTracker();
            deinitCloudReco();
            Vuforia.deinit();
        }

        System.gc();
    }
	
    private void storeScreenDimensions()
    {
        // Query display dimensions:
        DisplayMetrics metrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(metrics);
        mScreenWidth = metrics.widthPixels;
        mScreenHeight = metrics.heightPixels;
    }
    
    private synchronized void updateApplicationStatus(int appStatus)
    {
        if (mAppStatus == appStatus)
            return;

        mAppStatus = appStatus;

        switch (mAppStatus)
        {
            case APPSTATUS_INIT_APP:
                initApplication();
                updateApplicationStatus(APPSTATUS_INIT_Vuforia);
                break;

            case APPSTATUS_INIT_Vuforia:
                try
                {
                    mInitVuforiaTask = new InitVuforiaTask();
                    mInitVuforiaTask.execute();
                }
                catch (Exception e)
                {
                    RajLog.e("Initializing Vuforia SDK failed");
                }
                break;

            case APPSTATUS_INIT_TRACKER:
            	setupTracker();                
                break;
                
            case APPSTATUS_INIT_CLOUDRECO:
            	if(mUseCloudRecognition)
            	{
	                try
	                {
	                    mInitCloudRecoTask = new InitCloudRecoTask();
	                    mInitCloudRecoTask.execute();
	                }
	                catch (Exception e)
	                {
	                    RajLog.e("Failed to initialize CloudReco");
	                }
            	}
            	else
            	{
            		updateApplicationStatus(APPSTATUS_INITED);
            	}
                break;
                
            case APPSTATUS_INIT_APP_AR:
                initApplicationAR();
                updateApplicationStatus(APPSTATUS_INITED);
                break;

            case APPSTATUS_INITED:
                System.gc();
                updateApplicationStatus(APPSTATUS_CAMERA_RUNNING);
                break;

            case APPSTATUS_CAMERA_STOPPED:
                stopCamera();
                break;

            case APPSTATUS_CAMERA_RUNNING:
                startCamera(1);
                setProjectionMatrix();
                mFocusMode = FOCUS_MODE_CONTINUOUS_AUTO;
                if(!setFocusMode(mFocusMode))
                {
                    mFocusMode = FOCUS_MODE_NORMAL;
                    setFocusMode(mFocusMode);
                }

                if(!mRajawaliIsInitialized) {
                    initRajawali();
                    mRajawaliIsInitialized = true;
                }
                break;

            default:
                throw new RuntimeException("Invalid application state");
        }
    }
	
    private void initApplication()
    {
        setRequestedOrientation(mScreenOrientation);
        setActivityPortraitMode(
            mScreenOrientation == ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);

        storeScreenDimensions();

        getWindow().setFlags(
            WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
            WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }
    
    public void setScreenOrientation(final int screenOrientation)
    {
    	mScreenOrientation = screenOrientation;
    }
    
    public int getScreenOrientation()
    {
    	return mScreenOrientation;
    }
    
    protected void setupTracker()
    {
    	updateApplicationStatus(APPSTATUS_INIT_APP_AR);
    }
    
    protected void initApplicationAR()
    {
        RajLog.i("======init app AR width: " + mScreenWidth + " height: " + mScreenHeight);
        initApplicationNative(mScreenWidth, mScreenHeight);
    }
    
    protected void initRajawali()
    {
        if(mRenderer == null) {
            RajLog.e("initRajawali(): You need so set a renderer first.");
        }

        mSurfaceView = new RajawaliSurfaceView(this);
        mSurfaceView.setFrameRate(60.0);
        mSurfaceView.setRenderMode(IRajawaliSurface.RENDERMODE_WHEN_DIRTY);
        mSurfaceView.setSurfaceRenderer(mRenderer);

        addContentView(mSurfaceView, new ActionBar.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT));
    }

    public void setRenderer(RajawaliRenderer renderer) {
        mRenderer = renderer;
    }
    
    protected void useCloudRecognition(boolean value)
    {
    	mUseCloudRecognition = value;
    }
  
    protected native void initApplicationNative(int width, int height);
    protected native void setActivityPortraitMode(boolean isPortrait);
    protected native void setMaxSimultaneousImageTargets(int maxSimultaneousImageTargets);
    protected native void deinitApplicationNative();
    protected native void activateAndStartExtendedTracking();
    public native int initTracker(int trackerType);
    protected native int createFrameMarker(int markerId, String markerName, float width, float height);
    protected native int createImageMarker(String dataSetFile);
    public native void deinitTracker();
    private native int destroyTrackerData();
    protected native void startCamera(int camera);
    protected native void stopCamera();
    protected native void setProjectionMatrix();
    protected native boolean autofocus();
    protected native boolean setFocusMode(int mode);
    public native int initCloudReco();
    public native void setCloudRecoDatabase(String kAccessKey, String kSecretKey);
    public native void deinitCloudReco();
    public native void enterScanningModeNative();
    public native int initCloudRecoTask(); 
    public native boolean getScanningModeNative(); 
    public native String getMetadataNative(); 
    protected native boolean startExtendedTracking();
    protected native boolean stopExtendedTracking();

    /** A helper for loading native libraries stored in "libs/armeabi*". */
    public static boolean loadLibrary(String nLibName)
    {
        try
        {
            System.loadLibrary(nLibName);
            RajLog.i("Native library lib" + nLibName + ".so loaded");
            return true;
        }
        catch (UnsatisfiedLinkError ulee)
        {
            RajLog.e("The library lib" + nLibName +
                            ".so could not be loaded");
        }
        catch (SecurityException se)
        {
            RajLog.e("The library lib" + nLibName +
                            ".so was not allowed to be loaded");
        }

        return false;
    }
}
