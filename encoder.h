/*
    A simple wrapper around the videoIO ffmpeg piper with a SPSC work queue
    Thanks: http://www.boost.org/doc/libs/1_53_0/doc/html/lockfree/examples.html
*/
#include <boost/thread/thread.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/atomic.hpp>

#include <opencv2/opencv.hpp>
using namespace cv;

#include <mutex>
#include <condition_variable>

#include <ctime>
using namespace std;

#include <chrono>
using namespace std::chrono;

// TODO: offload these to runtime settings file
#define QUEUE_CAPACITY 64
#define DISPLAY_SCALE 0.25
#define FFMPEG_OPTIONS "-y -codec:v libx264 -preset ultrafast"
#define WINDOW_NAME "Dalsa Monitor"

class Encoder 
{
    public:
        Encoder(char*, int, int, int, int, bool);
        int writeFrame(cv::Mat img);
        int close();

    private:
        bool _debug;
        int writeCount;
        VideoIO _writer;
        boost::atomic<bool> _done;
        boost::thread* _encoderThread;
        boost::thread* _displayThread;
        boost::lockfree::spsc_queue<cv::Mat, boost::lockfree::capacity<QUEUE_CAPACITY>> _queue;

        // This mutex is used for synchronise communication with ffmpeg
        std::condition_variable cv;
        std::mutex cv_m;
        std::mutex display_m;

        void ffmpegWorker(void); // ffmpeg worker thread

        void writeQueue(); // pipe all accumulated frames onto ffmpeg

        void logFrame();

        // Display Thread 
        cv::Mat displayImg;  
        void displayFrame(void);
};
