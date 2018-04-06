/*
    A simple wrapper around the ffmpeg piper with a SPSC work queue
    Thanks: http://www.boost.org/doc/libs/1_53_0/doc/html/lockfree/examples.html
*/

#ifndef __ENCODER_CPP__
#define __ENCODER_CPP__

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
 

#include "ReadWriteMoviesWithOpenCV/DataManagement/VideoIO.h"

// TODO: offload these to run/compiler-time settings file? 
#define QUEUE_CAPACITY 64
#define DISPLAY_SCALE 0.25
#define FFMPEG_OPTIONS "-y -crf 17 -codec:v libx264 -preset ultrafast"
#define WINDOW_NAME "Dalsa Monitor"

class VideoEncoder 
{
    private:
        bool _debug;
        int writeCount;
        VideoIO _writer;
        boost::atomic<bool> _done;
        boost::thread* _encoderThread;
        boost::lockfree::spsc_queue<cv::Mat, boost::lockfree::capacity<QUEUE_CAPACITY>> _queue;

        // This mutex is used for three purposes:
            // 1) to synchronize accesses to i
            // 2) to synchronize accesses to std::cerr
            // 3) for the condition variable cv
        std::condition_variable cv;
        std::mutex cv_m; 

        void ffmpegWorker(void)
        {
            // TODO: CPU hungry thread?
            while (!_done) 
            {
                writeQueue();

                // Sleep while the queue is empty
                std::unique_lock<std::mutex> lk(cv_m);
                cv.wait(lk);
            }
            writeQueue();
        }

        void writeQueue()
        {
            cv::Mat img;
            while (_queue.pop(img))
            {
                // Pass to ffmpeg
                // TODO: handle status
                int status = _writer.WriteFrame(img);

                // Pass to display
                if(!_displaying)
                {
                    cv::resize(img, displayImg, cv::Size(), DISPLAY_SCALE, DISPLAY_SCALE);
                    
                    // TODO: Remove from heap at some point?
                    boost::thread* displayThread = new boost::thread(boost::bind(&VideoEncoder::displayFrame, this));
                }

                logFrame();
                img.release();
            }
        }

        void logFrame()
        {
            writeCount++;

            if(!_debug)
            {
                return;
            }

            cout << "writeQueue() # " << writeCount << endl;
            cout << "\tencoder queue size: " << _queue.read_available() << endl;
        }

        // TODO: Quit on 'q'
        cv::Mat displayImg;
        // TODO: Use a lock
        bool _displaying = false;
        void displayFrame(void)
        {
            _displaying = true;

            if(!displayImg.empty())
            {
                imshow(WINDOW_NAME, displayImg);
            }

            int key = waitKey(1);
            if((char) key == 'q') 
            {
                cout << "Quitting display...\n";
            }

            displayImg.release();
            _displaying = false;
        }

    public:
        VideoEncoder(char*, int, int, int, bool);

        //TODO feedback if something has failed
        int writeFrame(cv::Mat img)
        {
            // TODO: A while loop missing here?
            _queue.push(img);

            cv.notify_all();
            return 0;
        }

        // Hangs until all frames have been passed to ffmpeg
        int close()
        {
            _done = true;
            _encoderThread->join();
        }
};

VideoEncoder::VideoEncoder(char filename[], int width, int height, int framerate, bool debug=false)
{
    writeCount = 0;
    _debug = debug;

    // TODO: an option for this
    char ffmpegOptions[] = FFMPEG_OPTIONS;
    _writer.DebugMode = true;
    _writer.Create(filename, width, height, framerate, ffmpegOptions);

    // Start thread
    _done = false;

    // TODO: Delete after join?
    _encoderThread = new boost::thread(boost::bind(&VideoEncoder::ffmpegWorker, this));

    // Monitor Window
    // TODO: An option to disable this
    namedWindow(WINDOW_NAME, WINDOW_AUTOSIZE );
}

#endif /* __ENCODER_CPP__ */