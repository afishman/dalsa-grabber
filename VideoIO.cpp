/**
 * @file VideoIO.cpp
 * @ingroup DataManagement
 * @author Dominique Vaufreydaz, Grenoble Alpes University, Inria
 * @copyright All right reserved.
 */

#include "VideoIO.h"

#if defined WIN32 || defined WIN64
	#pragma warning(disable : 4996) // disable warning about non secure printf and scanf function
#endif

// Support for avconv or vlc later will be added later
const char * VideoIO::VideoProgram = "ffmpeg ";		/*!< @brief Program name to use for wideo reading */
const char * VideoIO::ParamProgram = "ffprobe ";		/*!< @brief Define program to gain information about video */

/** @brief Init or re-init a VideoIO object
 */
void VideoIO::Init()
{
	Width = Height = -1;
	Fps = 0.0;
	Mode = UNK_MODE;
}

/** @brief Allocate or extend InternalBuffer
 *
 * @param NewBufferSize [in] New size of the InternalBuffer.
 */
void VideoIO::Allocate(size_t NewBufferSize)
{
	// Do we have enougth space?
	if ( NewBufferSize <= SizeOfBuffer )
	{
		// Yes, nothing to do
		return;
	}

	// Free buffer if any
	if ( InternalBuffer != nullptr )
	{
		delete InternalBuffer;
	}

	InternalBuffer = new char[NewBufferSize+1];		// +1 for pending 0, always
	SizeOfBuffer = NewBufferSize;
}

/** @brief constructor. Set LogLevel and ShowInfos and call Init().
 */
VideoIO::VideoIO() :
	LogLevel(16),				// Show errors only by default
#ifdef USE_OLD_FFMPEG
	ShowInfos(SHOW_BANNER),		// Show banner
#else
	ShowInfos(SHOW_NOTHING),	// Do not show banner or stats
#endif
	InternalBuffer(nullptr),	// Empty buffer
	SizeOfBuffer(0)
{
	// Allocate buffer
	Allocate( 5*1024*1024 );	// 5 MiB at the beginning

	Init();
}

/** @brief Virtual destructor, always.
 */
VideoIO::~VideoIO()
{
	Close();

	if ( InternalBuffer != nullptr )
	{
		delete InternalBuffer;
	}
}

/** @brief Create a new Video file, i.e. call and pipe video data to an external program (usually ffmpeg).
	*
	* @param Filename [in] The new video file name.
	* @param eWidth [in] The width of the video stream.
	* @param eHeight [in] The Height of the video stream.
	* @param eFps [in] Video frame rate (default=30.0 fps).
	* @param EncodingParams [in] Parameters for the encoding subprocess, like codec, bandwith, etc. (default="").
	* @return true if the subprocess has been launch successfully
	*/
bool VideoIO::Create( const char * Filename, int eWidth, int eHeight, double eFps /* = 30.0 */, const char * EncodingParams /* = "" */ )
{
	// Close if already opened
	Close();

	// Set geometry of the video stream
	Width = eWidth;
	Height = eHeight;
	Fps = eFps;

	// Generate ffmpeg pipe command
	sprintf( InternalBuffer, "%s %s %s -loglevel %hu -f rawvideo -vcodec rawvideo -pix_fmt bgr24 -video_size %dx%d -r %.3lf -i - %s -an %s",
		VideoProgram, (ShowInfos & SHOW_BANNER) ? "" : "-hide_banner", 
		(ShowInfos & SHOW_STATS)  ? "" : "-nostats", LogLevel, Width, Height, Fps, EncodingParams, Filename );

	if ( DebugMode == true )
	{
		fprintf( stderr, "%s\n", InternalBuffer );
	}
		
	// Open pipe and launch executable
	return Pipe::Open( InternalBuffer, Pipe::WRITE_MODE );
}

	/** @brief Open an existing video file, i.e. call and pipe video data from an external program (usually ffmpeg).
	 *         This call will rely on information provided (eWidth, eHeight, eFps) to read the file.
	 *
	 * @param Filename [in] The video file name (be an URL).
	 * @param eWidth [in] The width of the input video stream.
	 * @param eHeight [in] The Height of the video stream.
	 * @param eFps [in] Video frame rate (default=30.0 fps).
	 * @param DecodingParams [in] Parameters for the decoding subprocess, like codec, bandwith, etc. (default="").
	 * @return true if the subprocess has been launch successfully
	 */
bool VideoIO::Open( const char * Filename, int eWidth, int eHeight, double eFps /* = 30.0 */, const char * DecodingParams /* = "" */ )
{
	// Close if already opened
	Close();

	// Set geometry of the video stream
	Width = eWidth;
	Height = eHeight;
	Fps = eFps;

	// Generate ffmpeg piping command
	sprintf( InternalBuffer, "%s %s %s -loglevel %hu %s -i %s -f rawvideo -vcodec rawvideo -pix_fmt bgr24 -an -video_size %dx%d -",
		VideoProgram, (ShowInfos & SHOW_BANNER) ? "" : "-hide_banner", 
		(ShowInfos & SHOW_STATS)  ? "" : "-nostats", LogLevel, DecodingParams, Filename, eWidth, eHeight );

	if ( DebugMode == true )
	{
		fprintf( stderr, "%s\n", InternalBuffer );
	}

	// Open pipe and launch executable
	return Pipe::Open( InternalBuffer, Pipe::READ_MODE );
}

/** @brief Open an existing video file, i.e. call and pipe video data from an external program (usually ffmpeg).
 *         A first call will be made (using ffprobe) to retrieve information about the video stream (Width, Height, Fps).
 *		   If you want to read a specific stream, add mandatory information in DecodingParams.
 *
 * @param Filename [in] The video file name (can be an URL).
 * @param DecodingParams [in] Parameters for the decoding subprocess, like codec, bandwith, etc. (default="").
 * @return true if the subprocess has been launch successfully
 */
bool VideoIO::Open( const char * Filename, const char * DecodingParams /* = "" */ )
{
	// Close if already opened
	Close();

	// We must find geometry of the input video, use ffprobe!
	sprintf( InternalBuffer, "%s %s -loglevel %hu -show_entries stream=width,height,r_frame_rate %s",
		ParamProgram, (ShowInfos & SHOW_BANNER) ? "" : "-hide_banner", LogLevel, Filename );

	if ( DebugMode == true )
	{
		fprintf( stderr, "%s\n", InternalBuffer );
	}

	// Try to launch ffprobe
	if ( Pipe::Open( InternalBuffer, Pipe::READ_MODE ) == false )
	{
		// ffprobe not installed ?
		return false;
	}

	int lWidth = -1;
	int lHeight = -1;
	double lFps = 0.0;
	// TODO : too specific, improve reading but ffmpeg json exports are not so good (even readable)
	while( fgets(InternalBuffer, (int)(SizeOfBuffer-1), InternalFile) != NULL )
	{
		if ( Width == -1 && *InternalBuffer == 'w' ) { sscanf( InternalBuffer, "width=%d", &lWidth ); continue; }
		if ( Height == -1 && *InternalBuffer == 'h' ) { sscanf( InternalBuffer, "height=%d", &lHeight );  continue; }
		if ( Fps == 0.0 && *InternalBuffer == 'r' )
		{
			int Num = 1, Den = 1;
			if ( sscanf( InternalBuffer, "r_frame_rate=%d/%d", &Num, &Den ) == 2 )
			{
				lFps = (double)Num/(double)Den;
				// We asked it last, we can break
				break;
			}
		}
	}

	// close ffprobe pipe
	Pipe::Close();

	if ( lWidth == -1 || lHeight == -1 )	// Here we do not care about Fps value, can not retrieve information about the file
	{
		Init();
		return false;
	}

	// Trace video info (if mandatory)
	if ( DebugMode == true )
	{
		fprintf( stderr, "Video format: %dx%d %.3lffps\n", lWidth, lHeight, lFps );
	}
	
	// Open the video file providing all information
	return Open( Filename, lWidth, lHeight, lFps, DecodingParams );
}

/** @brief Read one frame from an input video stream and fill it in a cv::Mat in BGR.
 *         The image will be Width x Height in BGR (24 bits).
 *
 * @param VideoImg [in,out] The cv::Mat to fill with the next frame.
 * @return true if the image has been read and succefully push in VideoImg.
 */
bool VideoIO::ReadFrame( cv::Mat& VideoImg )
{
	if ( InternalFile == nullptr || Mode != READ_MODE )
	{
		return false;
	}

	// Allocate memory for the frame
	int lsize = Width*Height*3;	// 3 because BGR24
	Allocate(lsize);			// Enlage (or not) buffer size

	// try to read a full frame
	if ( fread( InternalBuffer, lsize, 1, InternalFile ) != 1 )
	{
		// If we can not read a full frame
		return false;
	}

	// Creating and then copying the data was the only way that works...
	// TODO: check if we can not do better for performance reasons
	cv::Mat TmpMat( Height, Width, CV_8UC3, InternalBuffer );
		
	// Copy the Matrix to the VideoImg
	VideoImg = TmpMat;

	// Ok, we have a new frame
	return true;
}

/** @brief Write one frame to an output video stream. If the image is not in BGR
 *         or have not the right size, a transcoded/rescaled copy of this image
 *		   will be created.
 *
 * @param VideoImg [in] The cv::Mat to write as the next frame.
 * @return true if the image has been write succefully.
 */
bool VideoIO::WriteFrame( const cv::Mat& VideoImg )
{
	if ( InternalFile == nullptr || Mode != WRITE_MODE )
	{
		return false;
	}

	// To prevent modifying while converting
	cv::Mat* ToWrite = (cv::Mat*)&VideoImg;
	bool Unallocate = false;

	// Shall we change it to BGR24 (CV_8UC3)?
	if ( VideoImg.type() != CV_8UC3 )
	{
		ToWrite = new cv::Mat( VideoImg.size(), CV_8UC3 );
		Unallocate = true;

		VideoImg.convertTo( *ToWrite, CV_8UC3 );
	}

	// Shall we rescale it?
	if ( ToWrite->cols != Width || ToWrite->rows != Height )
	{
		// Rescale it
		cv::Mat MatForFinal( Height, Width, CV_8UC3 );
		cv::resize( *ToWrite, MatForFinal, MatForFinal.size(), 0, 0 );
		MatForFinal.copyTo( *ToWrite );
	}

	// Writing could be done in one call if the buffer is continuous
	if ( ToWrite->isContinuous() )
	{
		// Get buffer
		unsigned char * lBufLine = (unsigned char *)ToWrite->ptr<unsigned char>(0);
		if ( fwrite( lBufLine, Width*Height*3, 1, InternalFile ) != 1 )	// 3 => BGR24
		{
			// TODO: Throw an exception if i != 0, i.e. we have a part of image in the pipe...
			if ( Unallocate == true )
			{
				delete ToWrite;
			}

			// Close the pipe if there was a problem
			Close();
			return false;
		}
	}
	else
	{
		// Ok, here we will read line by line to write them
		int line_size = ToWrite->cols * 3;	// 3 => BGR24
		for( int i =0; i < ToWrite->rows; i++ )
		{
			// get local pointer to the line
			unsigned char * lBufLine = (unsigned char *)ToWrite->ptr<unsigned char>(i);
			if ( fwrite( lBufLine, line_size, 1, InternalFile ) != 1 )
			{
				// TODO: Throw an exception if i != 0, i.e. we have a part of image in the pipe...
				if ( Unallocate == true )
				{
					delete ToWrite;
				}

				// Close the pipe if there was a problem
				Close();
				return false;
			}
		}
	}

	// Here, everything went fine
	return true;
}

/** @brief Close the current stream. It can take time for the encoder to finish its job.
 */
void VideoIO::Close()
{
	Pipe::Close();
	Init();
}