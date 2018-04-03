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
		float _framerate;
		GEV_BUFFER_OBJECT* nextAcquiredImage();

		std::map<uint, GEV_BUFFER_OBJECT*> _reorderingMap;
		uint _tNextFrameMicroseconds;
		
		void logImg(GEV_BUFFER_OBJECT*);


 	public:
		DalsaCamera(bool debugMode);
		int width();
		int height();
		float framerate();
		int isOpened();
		int open(int width, int height, float framerate);
		int getNextImage(cv::Mat *img);
		int close();
		int periodMicroseconds();
		int record(float duration, char filename[]);
		bool debug;
};