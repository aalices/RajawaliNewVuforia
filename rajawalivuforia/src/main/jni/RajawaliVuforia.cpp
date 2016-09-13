#include <jni.h>
#include <android/log.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <Vuforia/Vuforia.h>
#include <Vuforia/CameraDevice.h>
#include <Vuforia/Renderer.h>
#include <Vuforia/VideoBackgroundConfig.h>
#include <Vuforia/Trackable.h>
#include <Vuforia/TrackableResult.h>
#include <Vuforia/Tool.h>
#include <Vuforia/Tracker.h>
#include <Vuforia/TrackerManager.h>
#include <Vuforia/ObjectTracker.h>
#include <Vuforia/CameraCalibration.h>
#include <Vuforia/UpdateCallback.h>
#include <Vuforia/DataSet.h>

#include "Utils.h"

#ifdef __cplusplus
extern "C" {
#endif

unsigned int screenWidth = 0;
unsigned int screenHeight = 0;
unsigned int videoWidth = 0;
unsigned int videoHeight = 0;
unsigned int maximumSimultaneousImageTargets = 1;

bool isActivityInPortraitMode = false;
bool activateDataSet = false;
bool activateExtendedTracking = false;
bool isExtendedTrackingActivated = false;

Vuforia::DataSet* dataSetToActivate = NULL;

Vuforia::CameraDevice::CAMERA_DIRECTION currentCamera;

Vuforia::Matrix44F projectionMatrix;

//New global vars for Cloud Reco
bool scanningMode = false;
static const size_t CONTENT_MAX = 256;
char lastTargetId[CONTENT_MAX];
char targetMetadata[CONTENT_MAX];

static const char* kAccessKey = NULL;
static const char* kSecretKey = NULL;

class ImageTargets_UpdateCallback: public Vuforia::UpdateCallback {
	virtual void Vuforia_onUpdate(Vuforia::State&) {

		if (dataSetToActivate != NULL) {
			Vuforia::TrackerManager& trackerManager =
					Vuforia::TrackerManager::getInstance();
			Vuforia::ObjectTracker* ObjectTracker =
					static_cast<Vuforia::ObjectTracker*>(trackerManager.getTracker(
							Vuforia::ObjectTracker::getClassType()));
			if (ObjectTracker == 0) {
				LOG("Failed to activate data set.");
				return;
			}
			ObjectTracker->activateDataSet(dataSetToActivate);

			if(isExtendedTrackingActivated)
			{
			    LOG("Activate extended tracking.");
				for (int tIdx = 0; tIdx < dataSetToActivate->getNumTrackables(); tIdx++)
				{
					Vuforia::Trackable* trackable = dataSetToActivate->getTrackable(tIdx);
					trackable->startExtendedTracking();
				}
			}

			dataSetToActivate = NULL;
		} else if(activateExtendedTracking == true && isExtendedTrackingActivated == false) {
			LOG("Activating extended tracking!");
			activateExtendedTracking = false;

			Vuforia::TrackerManager& trackerManager = Vuforia::TrackerManager::getInstance();
		    Vuforia::ObjectTracker* ObjectTracker = static_cast<Vuforia::ObjectTracker*>(
		          trackerManager.getTracker(Vuforia::ObjectTracker::getClassType()));

		    Vuforia::DataSet* currentDataSet = ObjectTracker->getActiveDataSet();
		    if (ObjectTracker == 0 || currentDataSet == 0)
		    	return;
			//LOG("NUM TRACKABLES %d", currentDataSet->getNumTrackables());
		    for (int tIdx = 0; tIdx < currentDataSet->getNumTrackables(); tIdx++)
		    {
		    	LOG("TRACKABLE %d", tIdx);
		        Vuforia::Trackable* trackable = currentDataSet->getTrackable(tIdx);
		        if(!trackable->startExtendedTracking()) {
		        	LOG("Couldn't start extended tracking");
		        } else {
		        	LOG("Successfully started extended tracking");
		        }
		    }

		    isExtendedTrackingActivated = true;
		}

		if (scanningMode) {
			Vuforia::TrackerManager& trackerManager =
					Vuforia::TrackerManager::getInstance();
			Vuforia::ObjectTracker* ObjectTracker =
					static_cast<Vuforia::ObjectTracker*>(trackerManager.getTracker(
							Vuforia::ObjectTracker::getClassType()));

		}
	}
};

ImageTargets_UpdateCallback updateCallback;

JNIEXPORT void JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_setActivityPortraitMode(JNIEnv *,
		jobject, jboolean isPortrait) {
	isActivityInPortraitMode = isPortrait;
}

JNIEXPORT int JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_initTracker(JNIEnv *env,
		jobject object, jint trackerType) {
LOG("Java_com_vuforia_samples_ImageTargets_ImageTargets_initTrackerox");

// Initialize the marker tracker:
Vuforia::TrackerManager& trackerManager = Vuforia::TrackerManager::getInstance();
//Vuforia::setHint(Vuforia::HINT_MAX_SIMULTANEOUS_IMAGE_TARGETS, 2);

Vuforia::Tracker* tracker = trackerManager.initTracker(
        Vuforia::ObjectTracker::getClassType());
if (tracker == NULL) {
LOG("Failed to initialize ObjectTracker.");
return 0;
}

LOG("Successfully initialized ObjectTracker.");


return 1;
}

JNIEXPORT int JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_createFrameMarker(JNIEnv* env,
		jobject object, jint markerId, jstring markerName, jfloat width,
		jfloat height) {

}

JNIEXPORT int JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_createImageMarker(JNIEnv* env,
		jobject object, jstring dataSetFile) {
	Vuforia::TrackerManager& trackerManager = Vuforia::TrackerManager::getInstance();
	Vuforia::ObjectTracker* ObjectTracker =
			static_cast<Vuforia::ObjectTracker*>(trackerManager.getTracker(
					Vuforia::ObjectTracker::getClassType()));

	if (ObjectTracker == NULL) {
		LOG("Failed to load tracking data set because the ObjectTracker has not"
		" been initialized.");
		return 0;
	}

	Vuforia::DataSet* dataSet = ObjectTracker->createDataSet();
	if (dataSet == 0) {
		LOG("Failed to create a new tracking data.");
		return 0;
	}

	// Load the data sets:
	const char *nativeString = env->GetStringUTFChars(dataSetFile, NULL);
	if (!dataSet->load(nativeString, Vuforia::DataSet::STORAGE_APPRESOURCE)) {
		LOG("Failed to load data set.");
		env->ReleaseStringUTFChars(dataSetFile, nativeString);
		return 0;
	}
	env->ReleaseStringUTFChars(dataSetFile, nativeString);

	// Activate the data set:
	if (!ObjectTracker->activateDataSet(dataSet)) {
		LOG("Failed to activate data set.");
		return 0;
	}

	LOG("Successfully loaded and activated data set.");

	return 1;
}

JNIEXPORT void JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_deinitTracker(JNIEnv *, jobject) {
	LOG("Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_deinitTracker");

	// Deinit the marker tracker, this will destroy all created frame markers:
	Vuforia::TrackerManager& trackerManager = Vuforia::TrackerManager::getInstance();
	if (trackerManager.getTracker(Vuforia::ObjectTracker::getClassType()) != NULL)
		trackerManager.deinitTracker(Vuforia::ObjectTracker::getClassType());
}

JNIEXPORT void JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaRenderer_renderFrame(JNIEnv* env,
		jobject object, jint frameBufferId, jint frameBufferTextureId) {

	jclass ownerClass = env->GetObjectClass(object);

	// Clear color and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Get the state from Vuforia and mark the beginning of a rendering section
	Vuforia::State state = Vuforia::Renderer::getInstance().begin();

	glBindFramebuffer(GL_FRAMEBUFFER, (int)frameBufferId);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, (int)frameBufferTextureId, 0);
	// Explicitly render the Video Background
	Vuforia::Renderer::getInstance().drawVideoBackground();
	jfloatArray modelViewMatrixOut = env->NewFloatArray(16);

    if (state.getNumTrackableResults() == 0) {
        jmethodID noFrameMarkersFoundMethod = env->GetMethodID(ownerClass,
                "noFrameMarkersFound", "()V");
        env->CallVoidMethod(object, noFrameMarkersFoundMethod);
        }

	// Did we find any trackables this frame?
	for (int tIdx = 0; tIdx < state.getNumTrackableResults(); tIdx++) {
		// Get the trackable:
		const Vuforia::TrackableResult* trackableResult = state.getTrackableResult(
				tIdx);
		const Vuforia::Trackable& trackable = trackableResult->getTrackable();

		Vuforia::Matrix44F modelViewMatrix = Vuforia::Tool::convertPose2GLMatrix(
				trackableResult->getPose());
        Vuforia::Matrix44F modelViewProjection;

        Utils::translatePoseMatrix(0.0f, 0.0f, 3.0f,
                                         &modelViewMatrix.data[0]);
        Utils::scalePoseMatrix(3.0f, 3.0f, 3.0f, &modelViewMatrix.data[0]);
        Utils::multiplyMatrix(&projectionMatrix.data[0], &modelViewMatrix.data[0],
                                                &modelViewProjection.data[0]);

		if (isActivityInPortraitMode)
			Utils::rotatePoseMatrix(90.0f, 0, 1.0f, 0,
					&modelViewMatrix.data[0]);
		//Utils::rotatePoseMatrix(-90.0f, 1.0f, 0, 0, &modelViewMatrix.data[0]);

        jmethodID foundImageMarkerMethod = env->GetMethodID(ownerClass,
                "foundImageMarker", "(Ljava/lang/String;[F)V");
        env->SetFloatArrayRegion(modelViewMatrixOut, 0, 16,
                modelViewProjection.data);
        const char* trackableName = trackable.getName();
        jstring trackableNameJava = env->NewStringUTF(trackableName);
        env->CallVoidMethod(object, foundImageMarkerMethod,
					trackableNameJava, modelViewMatrixOut);

	}
	env->DeleteLocalRef(modelViewMatrixOut);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	Vuforia::Renderer::getInstance().end();
}

// Initialize State Variables for Cloud Reco
void initStateVariables() {
	lastTargetId[0] = '\0';
	scanningMode = false;
}

void configureVideoBackground() {
	// Get the default video mode:
	Vuforia::CameraDevice& cameraDevice = Vuforia::CameraDevice::getInstance();
	Vuforia::VideoMode videoMode = cameraDevice.getVideoMode(
			Vuforia::CameraDevice::MODE_DEFAULT);

	// Configure the video background
	Vuforia::VideoBackgroundConfig config;
	config.mEnabled = true;
	//config.mSynchronous = true;
	config.mPosition.data[0] = 0.0f;
	config.mPosition.data[1] = 0.0f;

	if (isActivityInPortraitMode) {
		LOG("configureVideoBackground PORTRAIT");
		config.mSize.data[0] = videoMode.mHeight
				* (screenHeight / (float) videoMode.mWidth);
		config.mSize.data[1] = screenHeight;

		if (config.mSize.data[0] < screenWidth) {
			LOG(
					"Correcting rendering background size to handle missmatch between screen and video aspect ratios.");
			config.mSize.data[0] = screenWidth;
			config.mSize.data[1] = screenWidth
					* (videoMode.mWidth / (float) videoMode.mHeight);
		}
	} else {
		LOG("configureVideoBackground LANDSCAPE");
		config.mSize.data[0] = screenWidth;
		config.mSize.data[1] = videoMode.mHeight
				* (screenWidth / (float) videoMode.mWidth);

		if (config.mSize.data[1] < screenHeight) {
			LOG(
					"Correcting rendering background size to handle missmatch between screen and video aspect ratios.");
			config.mSize.data[0] = screenHeight
					* (videoMode.mWidth / (float) videoMode.mHeight);
			config.mSize.data[1] = screenHeight;
		}
	}

	videoWidth = config.mSize.data[0];
	videoHeight = config.mSize.data[1];

	LOG(
			"Configure Video Background : Video (%d,%d), Screen (%d,%d), mSize (%d,%d)", videoMode.mWidth, videoMode.mHeight, screenWidth, screenHeight, config.mSize.data[0], config.mSize.data[1]);

	// Set the config:
	Vuforia::Renderer::getInstance().setVideoBackgroundConfig(config);
}

JNIEXPORT void JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_initApplicationNative(JNIEnv* env,
		jobject obj, jint width, jint height) {
	LOG("Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_initApplicationNative");
	//Vuforia::setHint(Vuforia::HINT_MAX_SIMULTANEOUS_IMAGE_TARGETS, maximumSimultaneousImageTargets);
	Vuforia::registerCallback(&updateCallback);
	// Store screen dimensions
	screenWidth = width;
	screenHeight = height;
}

JNIEXPORT void JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_deinitApplicationNative(
		JNIEnv* env, jobject obj) {
	isExtendedTrackingActivated = false;
	LOG(
			"Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_deinitApplicationNative");
}

JNIEXPORT void JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_startCamera(JNIEnv *env,
		jobject object, jint camera) {
	LOG("Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_startCamera");

	// Select the camera to open, set this to Vuforia::CameraDevice::CAMERA_FRONT
	// to activate the front camera instead.
    currentCamera = static_cast<Vuforia::CameraDevice::CAMERA_DIRECTION> (camera);

    // Initialize the camera:
    if (!Vuforia::CameraDevice::getInstance().init(currentCamera))
        return;

	// Configure the video background
	configureVideoBackground();

	// Select the default mode:
	if (!Vuforia::CameraDevice::getInstance().selectVideoMode(
			Vuforia::CameraDevice::MODE_DEFAULT))
		return;

	// Start the camera:
	if (!Vuforia::CameraDevice::getInstance().start())
		return;

	// Start the tracker:
	Vuforia::TrackerManager& trackerManager = Vuforia::TrackerManager::getInstance();

	Vuforia::ObjectTracker* ObjectTracker =
			static_cast<Vuforia::ObjectTracker*>(trackerManager.getTracker(
					Vuforia::ObjectTracker::getClassType()));
	if (ObjectTracker != 0)
		ObjectTracker->start();

	// Start cloud based recognition if we are in scanning mode:
	//if (scanningMode) {
	//	Vuforia::TargetFinder* targetFinder = ObjectTracker->getTargetFinder();
	//	assert(targetFinder != 0);

	//	targetFinder->startRecognition();
	//}
}

JNIEXPORT jfloat JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaRenderer_getFOV(JNIEnv *env,
		jobject object) {
	const Vuforia::CameraCalibration& cameraCalibration =
			Vuforia::CameraDevice::getInstance().getCameraCalibration();
	Vuforia::Vec2F size = cameraCalibration.getSize();
	Vuforia::Vec2F focalLength = cameraCalibration.getFocalLength();
	float fovRadians = 2 * atan(0.5f * size.data[1] / focalLength.data[1]);
	float fovDegrees = fovRadians * 180.0f / M_PI;

	return fovDegrees;
}

JNIEXPORT jint JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaRenderer_getVideoWidth(JNIEnv *env,
		jobject object) {
	return videoWidth;
}

JNIEXPORT jint JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaRenderer_getVideoHeight(JNIEnv *env,
		jobject object) {
	return videoHeight;
}

JNIEXPORT void JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_stopCamera(JNIEnv *, jobject) {
	LOG("Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_stopCamera");

	// Stop the tracker:
	Vuforia::TrackerManager& trackerManager = Vuforia::TrackerManager::getInstance();

	Vuforia::ObjectTracker* ObjectTracker =
			static_cast<Vuforia::ObjectTracker*>(trackerManager.getTracker(
					Vuforia::ObjectTracker::getClassType()));
	if (ObjectTracker != 0)
		ObjectTracker->stop();

	Vuforia::CameraDevice::getInstance().stop();

	// Stop Cloud Reco:
	//Vuforia::TargetFinder* targetFinder = ObjectTracker->getTargetFinder();
	//assert(targetFinder != 0);

	//targetFinder->stop();

	// Clears the trackables
	//targetFinder->clearTrackables();

	Vuforia::CameraDevice::getInstance().deinit();

	initStateVariables();
}

JNIEXPORT void JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_setProjectionMatrix(JNIEnv *env,
		jobject object) {
	LOG("Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_setProjectionMatrix");

	// Cache the projection matrix:
	const Vuforia::CameraCalibration& cameraCalibration =
			Vuforia::CameraDevice::getInstance().getCameraCalibration();
	projectionMatrix = Vuforia::Tool::getProjectionGL(cameraCalibration, 2.0f,
			2500.0f);
}

JNIEXPORT jboolean JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_autofocus(JNIEnv*, jobject) {
	return Vuforia::CameraDevice::getInstance().setFocusMode(
			Vuforia::CameraDevice::FOCUS_MODE_TRIGGERAUTO) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_setFocusMode(JNIEnv*, jobject,
		jint mode) {
	int VuforiaFocusMode;

	switch ((int) mode) {
	case 0:
		VuforiaFocusMode = Vuforia::CameraDevice::FOCUS_MODE_NORMAL;
		break;

	case 1:
		VuforiaFocusMode = Vuforia::CameraDevice::FOCUS_MODE_CONTINUOUSAUTO;
		break;

	case 2:
		VuforiaFocusMode = Vuforia::CameraDevice::FOCUS_MODE_INFINITY;
		break;

	case 3:
		VuforiaFocusMode = Vuforia::CameraDevice::FOCUS_MODE_MACRO;
		break;

	default:
		return JNI_FALSE;
	}

	return Vuforia::CameraDevice::getInstance().setFocusMode(VuforiaFocusMode) ?
			JNI_TRUE : JNI_FALSE;
}

JNIEXPORT int JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_destroyTrackerData(JNIEnv *env,
		jobject object) {
	LOG("Java_org_rajawali3d_vuforia_RajawaliVuforiaRenderer_destroyTrackerData");

	Vuforia::TrackerManager& trackerManager = Vuforia::TrackerManager::getInstance();
	Vuforia::ObjectTracker* ObjectTracker =
			static_cast<Vuforia::ObjectTracker*>(trackerManager.getTracker(
					Vuforia::ObjectTracker::getClassType()));
	if (ObjectTracker == NULL) {
		return 0;
	}

	for (int tIdx = 0; tIdx < ObjectTracker->getActiveDataSetCount(); tIdx++) {
		Vuforia::DataSet* dataSet = ObjectTracker->getActiveDataSet(tIdx);
		ObjectTracker->deactivateDataSet(dataSet);
		ObjectTracker->destroyDataSet(dataSet);
	}

	return 1;
}

JNIEXPORT void JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_activateAndStartExtendedTracking(JNIEnv*, jobject) {
	activateExtendedTracking = true;
}

JNIEXPORT jboolean JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_startExtendedTracking(JNIEnv*, jobject)
{
    Vuforia::TrackerManager& trackerManager = Vuforia::TrackerManager::getInstance();
    Vuforia::ObjectTracker* ObjectTracker = static_cast<Vuforia::ObjectTracker*>(
          trackerManager.getTracker(Vuforia::ObjectTracker::getClassType()));

    Vuforia::DataSet* currentDataSet = ObjectTracker->getActiveDataSet();
    if (ObjectTracker == 0 || currentDataSet == 0)
    	return JNI_FALSE;

    for (int tIdx = 0; tIdx < currentDataSet->getNumTrackables(); tIdx++)
    {
        Vuforia::Trackable* trackable = currentDataSet->getTrackable(tIdx);
        if(!trackable->startExtendedTracking())
        	return JNI_FALSE;
    }

    isExtendedTrackingActivated = true;
    return JNI_TRUE;
}


JNIEXPORT jboolean JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_stopExtendedTracking(JNIEnv*, jobject)
{
    Vuforia::TrackerManager& trackerManager = Vuforia::TrackerManager::getInstance();
    Vuforia::ObjectTracker* ObjectTracker = static_cast<Vuforia::ObjectTracker*>(
          trackerManager.getTracker(Vuforia::ObjectTracker::getClassType()));

    Vuforia::DataSet* currentDataSet = ObjectTracker->getActiveDataSet();
    if (ObjectTracker == 0 || currentDataSet == 0)
    	return JNI_FALSE;

    for (int tIdx = 0; tIdx < currentDataSet->getNumTrackables(); tIdx++)
    {
    	Vuforia::Trackable* trackable = currentDataSet->getTrackable(tIdx);
        if(!trackable->stopExtendedTracking())
        	return JNI_FALSE;
    }

    isExtendedTrackingActivated = false;
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaRenderer_initRendering(JNIEnv* env,
		jobject obj) {
	LOG("Java_org_rajawali3d_vuforia_RajawaliVuforiaRenderer_initRendering");
}

JNIEXPORT void JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaRenderer_updateRendering(JNIEnv* env,
		jobject obj, jint width, jint height) {
	LOG("Java_org_rajawali3d_vuforia_RajawaliVuforiaRenderer_updateRendering");

	// Update screen dimensions
	screenWidth = width;
	screenHeight = height;

	// Reconfigure the video background
	configureVideoBackground();
}

JNIEXPORT int JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_initCloudReco(JNIEnv *, jobject) {
	LOG(
			"Java_com_qualcomm_VuforiaSamples_ImageTargets_ImageTargets_initCloudReco");

	Vuforia::TrackerManager& trackerManager = Vuforia::TrackerManager::getInstance();
	Vuforia::ObjectTracker* ObjectTracker =
			static_cast<Vuforia::ObjectTracker*>(trackerManager.getTracker(
					Vuforia::ObjectTracker::getClassType()));

	assert(ObjectTracker != NULL);

	//Get the TargetFinder:
	//Vuforia::TargetFinder* targetFinder = ObjectTracker->getTargetFinder();
	//assert(targetFinder != NULL);

	// Start initialization:
	//if (targetFinder->startInit(kAccessKey, kSecretKey)) {
	//	targetFinder->waitUntilInitFinished();
	//}

	//int resultCode = targetFinder->getInitState();
	//if (resultCode != Vuforia::TargetFinder::INIT_SUCCESS) {
	//	LOG("Failed to initialize target finder.");
	//	return resultCode;
	//}

	// Use the following calls if you would like to customize the color of the UI
	// targetFinder->setUIScanlineColor(1.0, 0.0, 0.0);
	// targetFinder->setUIPointColor(0.0, 0.0, 1.0);

	return 0;
}

JNIEXPORT int JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_deinitCloudReco(JNIEnv *,
		jobject) {

	// Get the image tracker:
	Vuforia::TrackerManager& trackerManager = Vuforia::TrackerManager::getInstance();
	Vuforia::ObjectTracker* ObjectTracker =
			static_cast<Vuforia::ObjectTracker*>(trackerManager.getTracker(
					Vuforia::ObjectTracker::getClassType()));

	if (ObjectTracker == NULL) {
		LOG(
				"Failed to deinit CloudReco as the ObjectTracker was not initialized.");
		return 0;
	}

	// Deinitialize Cloud Reco:
	//Vuforia::TargetFinder* finder = ObjectTracker->getTargetFinder();
	//finder->deinit();

	return 1;
}

JNIEXPORT void JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_enterScanningModeNative(JNIEnv*,
		jobject) {
	Vuforia::TrackerManager& trackerManager = Vuforia::TrackerManager::getInstance();
	Vuforia::ObjectTracker* ObjectTracker =
			static_cast<Vuforia::ObjectTracker*>(trackerManager.getTracker(
					Vuforia::ObjectTracker::getClassType()));

	assert(ObjectTracker != 0);

	//Vuforia::TargetFinder* targetFinder = ObjectTracker->getTargetFinder();
	//assert(targetFinder != 0);

	// Start Cloud Reco
	//targetFinder->startRecognition();

	// Clear all trackables created previously:
	//targetFinder->clearTrackables();

	scanningMode = true;
}

JNIEXPORT bool JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_getScanningModeNative(JNIEnv*,
		jobject) {
	return scanningMode;
}

JNIEXPORT void JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_setCloudRecoDatabase(JNIEnv* env,
		jobject, jstring AccessKey, jstring SecretKey) {
	const jbyte* argvv = (jbyte*) env->GetStringUTFChars(AccessKey, NULL);
	kAccessKey = (char *) argvv;
	argvv = (jbyte*) env->GetStringUTFChars(SecretKey, NULL);
	kSecretKey = (char *) argvv;
}

JNIEXPORT void JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_setMaxSimultaneousImageTargets(JNIEnv* env,
        jobject obj, jint maxSimImageTargets) {
    maximumSimultaneousImageTargets = (int)maxSimImageTargets;
}

JNIEXPORT jstring JNICALL
Java_org_rajawali3d_vuforia_RajawaliVuforiaActivity_getMetadataNative(JNIEnv* env,
		jobject) {
//	char *buf = (char*)malloc(CONTENT_MAX);
//	strcpy(buf, targetMetadata);
	jstring jstrBuf = env->NewStringUTF(targetMetadata);

	return jstrBuf;
}

#ifdef __cplusplus
}
#endif
