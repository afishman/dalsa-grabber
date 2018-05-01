/**
 * @file VideoIO.h
 * @ingroup DataManagement
 * @author Dominique Vaufreydaz, Grenoble Alpes University, Inria
 * @copyright All right reserved.
 */

#ifndef __VIDEO_IO_H__
#define __VIDEO_IO_H__

#include <stdio.h>
#include <inttypes.h>

// Include Pipe class
#include "Pipe.h"

// Include OpenCv stuff
#include <opencv2/video/tracking.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/background_segm.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/core/mat.hpp>

/**
 * @class VideoIO VideoIO.cpp VideoIO.h
 * @brief Read/Write Video for OpenCV. Work under Linux/Windows/MacOSX.
 *
 * This class is used to pipe data from and to an external program
 * (usually ffmpeg) in order to read/write almost all type of video using OpenCV.
 * Idea is you can get more video support (format, codecs choices and parameters, ...)
 * than in usual OpenCV video support.
 * @author Dominique Vaufreydaz, Grenoble Alpes University, Inria
 */
class VideoIO : public Pipe
{
private:
	// Support for avconv orf vlc later will be added later
	static const char * VideoProgram;		/*!< @brief Program name to use for wideo reading, current value="ffmpeg " */
	static const char * ParamProgram;		/*!< @brief Define program to gain information about video, current value="ffprobe "  */

protected:
	/** @brief Init or re-init a VideoIO object. LogLevel and ShowInfos are not altered by Init().
	 */
	void Init();

public:

	/** @brief constructor. Set LogLevel and ShowInfos and call Init().
	 */
	VideoIO();

	/** @brief Virtual destructor, always.
	 */
	virtual ~VideoIO();

	/** @brief Create a new Video file, i.e. call and pipe video data to an external program (usually ffmpeg).
	 *
	 * @param Filename [in] The new video file name.
	 * @param eWidth [in] The width of the video stream.
	 * @param eHeight [in] The Height of the video stream.
	 * @param eFps [in] Video frame rate (default=30.0 fps).
	 * @param EncodingParams [in] Parameters for the encoding subprocess, like codec, bandwith, etc. (default="").
	 * @return true if the subprocess has been launch successfully
	 */
	bool Create( const char * Filename, int eWidth, int eHeight, double eFps = 30.0, const char * EncodingParams = "" );

	/** @brief Open an existing video file, i.e. call and pipe video data from an external program (usually ffmpeg).
	 *
	 * @param Filename [in] The video file name. This call will rely on information passed (eWidth, eHeight, eFps) to read the file.
	 * @param eWidth [in] The width of the input video stream.
	 * @param eHeight [in] The Height of the video stream.
	 * @param eFps [in] Video frame rate (default=30.0 fps).
	 * @param DecodingParams [in] Parameters for the decoding subprocess, like codec, bandwith, etc. (default="").
	 * @return true if the subprocess has been launch successfully
	 */
	bool Open( const char * Filename, int eWidth, int eHeight, double eFps = 30.0, const char * DecodingParams = "" );

	/** @brief Open an existing video file, i.e. call and pipe video data from an external program (usually ffmpeg).
	 *         A first call will be made (using ffprobe) to retrieve information about the video stream (Width, Height, Fps).
	 *		   If you want to read a specific stream, add mandatory information in DecodingParams.
	 *
	 * @param Filename [in] The video file name (can be an URL).
	 * @param DecodingParams [in] Parameters for the decoding subprocess, like codec, bandwith, etc. (default="").
	 * @return true if the subprocess has been launch successfully
	 */
	bool Open( const char * Filename, const char * DecodingParams = "" );


	/** @brief Read one frame from an input video stream and fill it in a cv::Mat in BGR.
	 *         The image will be Width x Height in BGR (24 bits).
	 *
	 * @param VideoImg [in,out] The cv::Mat to fill with the next frame.
	 * @return true if the image has been read and succefully push in VideoImg.
	 */
	bool ReadFrame( cv::Mat& VideoImg ); 

	/** @brief Read one frame from an input video stream and fill it in a cv::Mat in BGR.
	 *         The image will be Width x Height in BGR (24 bits).
	 *
	 * @param VideoImg [in,out] The cv::Mat to fill with the next frame.
	 */
	VideoIO& operator>>( cv::Mat& VideoImg )
	{
		if ( ReadFrame( VideoImg ) == false )
		{
			throw "Unable to read videoframe";
		}
		return *this;
	}

	/** @brief Write one frame to an output video stream. If the image is not in BGR
	 *         or have not the right size, a transcoded/rescaled copy of this image
	 *		   will be created.
	 *
	 * @param VideoImg [in] The cv::Mat to write as the next frame.
	 * @return true if the image has been write succefully.
	 */
	VideoIO& operator<<( const cv::Mat& VideoImg )
	{
		if ( WriteFrame( VideoImg ) == false )
		{
			throw "Unable to write videoframe";
		}
		return *this;
	}

	/** @brief Write one frame to an output video stream. If the image is not in BGR
	 *         or have not the right size, a transcoded/rescaled copy of this image
	 *		   will be created.
	 *
	 * @param VideoImg [in] The cv::Mat to write as the next frame.
	 */
	bool WriteFrame( const cv::Mat& VideoImg );

	/** @brief Close the current stream. It can take time for the encoder to finish its job.
	 */
	void Close();

	/** @enum VideoIO::ShowFlags
	 *  @brief Enum flags to select trace from encoding/decoding program
	*/
	enum ShowFlags {
		SHOW_NOTHING = 0,	/*!< @brief Default value, nothing */
		SHOW_BANNER=1,		/*!< @brief Show program banner if any */
		SHOW_STATS=2		/*!< @brief Show program encoding/decoding stats if any */
	};

public:
	// As I do not like accessors when it is not mandatory...
	int Width;				/*!< @brief Width of the video */
	int Height;				/*!< @brief Height of the video */
	double Fps;				/*!< @brief Video framerate */

	// Debugging stuff
	short int LogLevel;		/*!< @brief Define loglevel of program. Show errors only by default (LogLevel=16) */
	short ShowInfos;		/*!< @brief Memory of information to show while encoding/decoding */

protected:
	// Buffer to work
	char * InternalBuffer;	/*!< @brief Buffer for everything : read lines, read frames, etc. */
	size_t SizeOfBuffer;	/*!< @brief Actual size of InternalBuffer */

	/** @brief Allocate or extend InternalBuffer
	 *
	 * @param NewBufferSize [in] New size of the InternalBuffer.
	 */
	void Allocate(size_t NewBufferSize);	
};





#endif	// __VIDEO_IO_H__

