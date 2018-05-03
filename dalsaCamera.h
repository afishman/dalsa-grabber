#include "gevapi.h"	
#include <map>

#include <ctime>

class DalsaCamera 
{
	private:
		//TODO: Design patterns
		GEV_CAMERA_HANDLE handle = NULL;
		int numBuf;
		PUINT8 *bufAddress;

		// State of the camera
		int _isOpened;

		// Time of last frame
		int timestampPrevFrame;

		int frameCount;
		time_t tStart;

		int _width, _height;
		float _framerate, _exposure;

		GEV_BUFFER_OBJECT* nextAcquiredImage();

		std::map<uint64_t, GEV_BUFFER_OBJECT*> _reorderingMap;
		uint64_t _tNextFrameMicroseconds;

		void logImg(GEV_BUFFER_OBJECT*);
		void logCamera();


 	public:
		DalsaCamera(bool debugMode);
		int width();
		int height();
		float framerate();
		int isOpened();
		int open(int width, int height, float framerate, float exposureTime);
		int getNextImage(cv::Mat *img);
		int close();
		int periodMicroseconds();
		int record(float duration, int crf, char filename[]);
		int snapshot(char filename[]);
		bool debug;

		static uint64_t combineTimestamps(uint32_t low, uint32_t high);
};