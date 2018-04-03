#ifndef __DALSACAMERA_CPP__
#define __DALSACAMERA_CPP__

#include <stdlib.h>
#include <stdio.h>
#include <string> 
#include <iostream>
#include <fstream>
#include <math.h>
#include <map>
#include <ctime>

using namespace std;

#include <chrono>
using namespace std::chrono;
 

#include <opencv2/opencv.hpp>
using namespace cv;
// Typically found in /opt/genicam_v3_0/library/CPP/include/GenApi/
//#include "GenApi/GenApi.h"		//!< GenApi lib definitions.

// found in /usr/dalsa/GigeV/include
#include "gevapi.h"				//!< GEV lib definitions.

#include <time.h>

#include "DalsaCamera.h"
#include "ReadWriteMoviesWithOpenCV/DataManagement/VideoIO.h"
#include "encoder.cpp"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
namespace logging = boost::log;

// How much to rescale the image by in monitor mode (TODO: option?)

milliseconds time_now()
{
	return duration_cast<milliseconds>(
    	system_clock::now().time_since_epoch()
	);
}

DalsaCamera::DalsaCamera()
{
	//TODO: hardcoded values arrr baaaad
	numBuf = 64;
	bufAddress = new PUINT8[numBuf];
	frameCount = 0;
	timestampPrevFrame = -1;

	_width = 0;
	_height = 0;
	_framerate = 0;
}

int DalsaCamera::width()
{
	return _width;
}

int DalsaCamera::height()
{
	return _height;
}

float DalsaCamera::framerate()
{
	return _framerate;
}

int DalsaCamera::isOpened()
{
	return _isOpened;
}

int DalsaCamera::open(int width, int height, float framerate)
// Suggested settings:
//		width: 2560
//		height: 2048
//		framerate: 20
{
	// TODO: Hardcoded value
	float exposureTime = 10000; //100000;

	// Set default options for the library.
	GEVLIB_CONFIG_OPTIONS options = {0};
	GevGetLibraryConfigOptions( &options);
	options.logLevel = GEV_LOG_LEVEL_NORMAL;
	GevSetLibraryConfigOptions( &options);


	// DISCOVER Cameras
	// Get all the IP addresses of attached network cards.
	int numCameras = 0;
	//TODO: genicam_cpp_demo uses the equation (MAX_NETIF * MAX_CAMERAS_PER_NETIF), with the
	//hardcoded values below
	int maxCameras = 8 * 32;

	//For storing status 
	UINT16 status;

	GEV_DEVICE_INTERFACE  pCamera[maxCameras] = {0};
	status = GevGetCameraList( pCamera, maxCameras, &numCameras);
	printf("%d camera(s) on the network\n", numCameras);

	//TODO goto fail to ensure things are closed
	if(!numCameras)
	{
		cerr << "ERROR: Could not find any cameras\n";
		return 1;
	}

	//TODO: print details of the camera chosen
	int camIndex = 0;
	status = GevOpenCamera(&pCamera[camIndex], GevExclusiveMode, &handle);

	GEV_CAMERA_OPTIONS camOptions = {0};

	// Adjust the camera interface options if desired (see the manual)
	GevGetCameraInterfaceOptions(handle, &camOptions);
	camOptions.heartbeat_timeout_ms = 90000;		// For debugging (delay camera timeout while in debugger)

	// These hardcoded values are taken from the genicam_cpp_demo
	// TODO: settings file?
	camOptions.streamFrame_timeout_ms = 1001;				// Internal timeout for frame reception.
	camOptions.streamNumFramesBuffered = 4;				// Buffer frames internally.
	camOptions.streamMemoryLimitMax = 64*1024*1024;		// Adjust packet memory buffering limit.	
	camOptions.streamPktSize = 9180;							// Adjust the GVSP packet size.
	camOptions.streamPktDelay = 10;							// Add usecs between packets to pace arrival at NIC.

	GevSetCameraInterfaceOptions( handle, &camOptions);


	//TODO goto fail
	if(status != GEVLIB_OK)
	{
		cerr << "Failed to open camera, status: " << status << "\n";
		return 1;
	}

	// Initiliaze access to GenICam features via Camera XML File
	status = GevInitGenICamXMLFeatures(handle, TRUE);				
	if (status != GEVLIB_OK)
	{
		cerr << "Failed to find xml file for camera status: " << status << "\n";
	}

	// Get the name of XML file name back (example only - in case you need it somewhere).
	//TODO: Where does max_path come from?
	char xmlFileName[MAX_PATH] = {0};
	status = GevGetGenICamXML_FileName(handle, (int)sizeof(xmlFileName), xmlFileName);
	if (status != GEVLIB_OK) 
	{
		cerr << "Failed to open xml file for camera status: %s\n" << status;
		cerr << "For File: " << xmlFileName;
	}
	
	// TODO: Log mode only?
	printf("XML stored as %s\n", xmlFileName);

	// Always disable pesky auto-brightness
	int autoBrightness = 0;
	if(GevSetFeatureValue(handle, "autoBrightnessMode", sizeof(autoBrightness), &autoBrightness))
	{
		cerr << "Failed to set autobrightness to " << autoBrightness << endl;
		return 1;
	}

	if(GevSetFeatureValue(handle, "ExposureTime", sizeof(exposureTime), &exposureTime))
	{
		cerr << "Failed to set exposureTime to " << exposureTime << endl;
		return 1;
	}

	int stat = GevSetFeatureValue(handle, "AcquisitionFrameRate", sizeof(framerate), &framerate);
	if(stat)
	{
		cerr << "Failed to set framerate to " << framerate << stat << endl;
		return 1;
	}

	// Whacking down the resolution Setting Feature Values
	if(GevSetFeatureValue(handle, "Width", sizeof(width), &width))
	{
		cerr << "Failed to set width to " << width << endl;
		return 1;
	}
	
	if(GevSetFeatureValue(handle, "Height", sizeof(height), &height))
	{
		cerr << "Failed to set height to " << height << endl;
		return 1;
	}


	// Get and display camera settings
	UINT32 format=0;
	int type;	

	// TODO: DRY this up
	// TODO: Assert the retrieved value matches the one passed in
	// TODO: Log mode only?
	float readExposed = -1;
	GevGetFeatureValue(handle, "Width", &type, sizeof(width), &width);
	GevGetFeatureValue(handle, "Height", &type, sizeof(height), &height);
	GevGetFeatureValue(handle, "AcquisitionFrameRate", &type, sizeof(framerate), &framerate);
	GevGetFeatureValue(handle, "ExposureTime", &type, sizeof(readExposed), &readExposed);

	BOOST_LOG_TRIVIAL(info) << "\tCamera Width" << width;
	BOOST_LOG_TRIVIAL(info) << "\tCamera Height" << height;
	BOOST_LOG_TRIVIAL(info) << "\tCamera Framerate" << framerate;
	BOOST_LOG_TRIVIAL(info) << "\tCamera Exposure" << readExposed;

	_width = width;
	_height = height;
	_framerate = framerate;
	//TODO: exposure

	char value_str[MAX_PATH] = {0};
	GevGetFeatureValueAsString( handle, "PixelFormat ", &type, MAX_PATH, value_str);
	BOOST_LOG_TRIVIAL(info) << "\tCamera Pixel Format " << value_str;
	
	// Setup buffers
	int size = height * width * GetPixelSizeInBytes(format);
	BOOST_LOG_TRIVIAL(debug) << "\tPixel Size in Bytes " << GetPixelSizeInBytes(format);

	int numBuffers = numBuf;
	for (int i = 0; i < numBuffers; i++)
	{
		bufAddress[i] = (PUINT8)malloc(size);
		memset(bufAddress[i], 0, size);
	}

	// Initialise Image Transfer
	// TODO: Use Sync! It is more thread safe
	status = GevInitImageTransfer(handle, SynchronousNextEmpty, numBuf, bufAddress);
	if(status != GEVLIB_OK)
	{
		cerr << "Failed to Initiliaze image transfer\n";
		return 1;
	}

	// TODO: Offload this to record/start functions?
	status = GevStartImageTransfer(handle, -1);
	if(status != GEVLIB_OK)
	{
		cerr << "Failed to start image transfer\n";
		return 1;
	}	
	GEV_BUFFER_OBJECT* img_obj = nextAcquiredImage();
	_tNextFrameMicroseconds = img_obj->timestamp_lo + periodMicroseconds();

	_isOpened = 1;

	// For framerate tracking
	tStart = time(NULL);

	return 0;
}

GEV_BUFFER_OBJECT* DalsaCamera::nextAcquiredImage(){
	GEV_BUFFER_OBJECT* imgGev = NULL;
	int status;
	
	status = GevWaitForNextImage(handle, &imgGev, 10000);

	// Check that we have received data ok, 
	// TODO: Nicer to put in next_acquired_image
	if (imgGev == NULL)
	{
		cerr << "Failed to wait for next acquired image.\n";
		cerr << "NULL Image Object.\n";
		cerr << "possibly caused by filling up GigE-V buffers";
		throw "next_acquired_image failure";
	}
	if(status != GEVLIB_OK)
	{
		cerr << "Failed to wait for next acquired image.\n";
		cerr << "GevWaitForNextImage returned " << status << "\n";
		throw "next_acquired_image failure";
	}
	if (imgGev->status !=0)
	{
		cerr << "Failed to wait for next acquired image.\n";
		cerr << "img->status = " << imgGev->status << "\n";
		//HACK: Fix me
		// throw "next_acquired_image failure";
	}
	// Check image data is actually there
	if(imgGev -> address == NULL)
	{
		cerr << "Failed to wait for next acquired image.\n";
		cerr << "img->address = NULL\n";
		throw "next_acquired_image failure";
	}

	return imgGev;
}

// Debayered image
// Handle what happens when gevapi fills provided buffers / internal camera 
//     buffer fills. Maybe reset the map and return an error code
int DalsaCamera::getNextImage(cv::Mat *img)
{
	//TODO: delete?
	frameCount++;

	UINT16 status;

	if(!isOpened())
	{
		cerr << "open camera before calling get_next_image";
		return 1;
	}
	
	// Add frames to the map until the next one is acquired
	while(_reorderingMap.find(_tNextFrameMicroseconds) == _reorderingMap.end())
	{
		GEV_BUFFER_OBJECT *nextImage = nextAcquiredImage();
		cout << "Acquriing new image " << nextImage -> timestamp_lo << endl;		
		cout << "(expecting)" << _tNextFrameMicroseconds << endl;		
		_reorderingMap[nextImage->timestamp_lo] = nextImage;
	}

		// Get the next frame
	GEV_BUFFER_OBJECT *imgGev = _reorderingMap[_tNextFrameMicroseconds];
	_reorderingMap.erase(_tNextFrameMicroseconds);

	cout << "Image acquired" << endl;

	// int dt = img_gev->timestamp_lo - timestamp_prev_frame;
	// timestamp_prev_frame = img_gev->timestamp_lo;
	//TODO: handle a reset of next frame
	_tNextFrameMicroseconds += periodMicroseconds();

	//TODO: Log mode
	printf("Acquired Image:\n");
	printf("\tFramecount: %i\n", frameCount);
	printf("\tTimestamp hi: %i\n", imgGev->timestamp_hi);
	printf("\tTimestamp low: %i\n", imgGev->timestamp_lo);
	// printf("\tTime Between Frames: %i\n", dt);
	printf("\tw: %i\n", imgGev->w);
	printf("\th: %i\n", imgGev->h);
	printf("\td: %i\n", imgGev->d);
	printf("\tformat: %i\n", imgGev->format);
	printf("\taddress: %x\n", *imgGev->address);
	printf("\timg_gev->status: %i\n", imgGev->status);
	printf("\tImage transfered: %i\n", status);
	printf("\tMap Size: %i\n", (int)_reorderingMap.size());

	time_t tEnd = time(NULL);
	float avgFramerate = (float)frameCount / ((float) ((long)tEnd-(long)tStart));
	printf("\tAvg Framerate: %.0f\n", avgFramerate);
	printf("\n");

	// Debayer the image
    cv::Mat imgCv = cv::Mat(height(), width(), CV_8UC1, imgGev->address);
    cv::Mat rgb8BitMat(height(), width(), CV_8UC3);
    cv::cvtColor(imgCv, rgb8BitMat, CV_BayerGB2RGB);

    *img = rgb8BitMat;

    // Release Image Buffer
	GevReleaseImage(handle, imgGev);
	imgCv.release();

	return 0;
}

// Duration is in seconds
int DalsaCamera::record(float duration, char filename[])
{
	// TODO: Make this configuration more accessible
	char ffmpegOptions[] = "-y -codec:v libx264 -preset veryfast";
	int numFrames = round(duration * float(_framerate));

	// Initialise video writer
	VideoEncoder writer(filename, (int)(width()), (int)(height()), _framerate);

    // Collect the frames
	for(int i=0; i<numFrames; i++)
	{
		cout << "before write frame" << endl;
		
		cv::Mat img;
		getNextImage(&img);

		// Write the current frame to the mp4 file
		milliseconds renderStart = time_now();
		if (!writer.writeFrame(img))
		{
			fprintf( stderr, "Could not write frame\n" );
			return -2;
		}

		milliseconds renderEnd = time_now();
		cout << "Loop Time ms: " << std::to_string((renderEnd - renderStart).count()) << endl;
	}

	writer.close();

}

// In Microseconds
int DalsaCamera::periodMicroseconds()
{
	return round(1.0/_framerate*1000000.0);
}



//TODO: check that the camera is open
int DalsaCamera::close() 
{
	UINT16 status;

	//TODO: GOTO Fail
	// Must close everything in order, otherwise things may hang. 
	// (1) Camera
	// (2) Gev API
	// (3) Sockets

	GevAbortImageTransfer(handle);
	status = GevFreeImageTransfer(handle);

	printf("Closing camera...\n");
	GevCloseCamera(&handle);

	// Close down the API.
	printf("Uninitialising API...\n");
	GevApiUninitialize();

	// Close socket API
	// must close API even on error	
	printf("Closing socket API...\n");
	_CloseSocketAPI ();	

	_isOpened = 0;
	return status;
}

#endif /* __DALSACAMERA_CPP__ */
