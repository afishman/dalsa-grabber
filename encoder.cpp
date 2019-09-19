/*
    A simple wrapper around the videoIO ffmpeg piper with a SPSC work queue
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

#include "videoIO/VideoIO.h"
#include "encoder.h"

/* Prepare for frame encoding */ 
VideoEncoder::VideoEncoder(char filename[], int width, int height, int framerate, int crf, bool debug=false)
{
    writeCount = 0;
    _debug = debug;

    // Setup ffmpeg subprocess
    char ffmpegOptions[] = FFMPEG_OPTIONS;
    _writer.DebugMode = true;
    _writer.Create(filename, width, height, framerate, (std::string(ffmpegOptions)+" -crf "+std::to_string(crf)).c_str());

    // Nothing more to add to queue flag
    _done = false;

    _encoderThread = new boost::thread(boost::bind(&VideoEncoder::ffmpegWorker, this));
    _displayThread = new boost::thread(boost::bind(&VideoEncoder::displayFrame, this));

    // Monitor Window
    // TODO: Include option to disable this
    namedWindow(WINDOW_NAME, WINDOW_AUTOSIZE);
}


/* ffmpeg worker thread */ 
void VideoEncoder::ffmpegWorker(void)
{
    while (!_done) 
    {
        // Sleep while the queue is empty
        std::unique_lock<std::mutex> lk(cv_m);
        cv.wait(lk);

        writeQueue();
    }
    writeQueue();
}

/* pipe all accumulated frames onto ffmpeg */ 
void VideoEncoder::writeQueue()
{
    cv::Mat img;
    while (_queue.pop(img))
    {
        // Pass to ffmpeg
        // TODO: handle status
        int status = _writer.WriteFrame(img);
        logFrame();
        img.release();
    }
}

/* Log */ 
// TODO: use boost logging framework
void VideoEncoder::logFrame()
{
    writeCount++;

    if(!_debug)
    {
        return;
    }

    cout << "writeQueue() # " << writeCount << endl;
    cout << "\tencoder queue size: " << _queue.read_available() << endl;
}

/* Display Thread */ 
void VideoEncoder::displayFrame(void)
{
    while(!_done)
    {
        // Wait for next image
        std::unique_lock<std::mutex> lk(cv_m);
        cv.wait(lk);

        // Lock the frame
        std::lock_guard<std::mutex> lock(display_m);

        // Display
        if(!displayImg.empty()) imshow(WINDOW_NAME, displayImg);
        int key = waitKey(1);

        // Release Image
        displayImg.release();
    }
}


/* Add an image to the encoder queue */ 
int VideoEncoder::writeFrame(cv::Mat img)
{
    while (!_queue.push(img));

    // Pass to display
    if(display_m.try_lock()) 
    {
        try {cv::resize(img, displayImg, cv::Size(), DISPLAY_SCALE, DISPLAY_SCALE);}
        catch(...){}

        display_m.unlock();
    }

    // Wake up the threads
    cv.notify_all();

    return 0;
}

/* Hangs until the encoder queue is processed */
int VideoEncoder::close()
{
    _done = true;
    _encoderThread->join();
    _displayThread->join();

    return 0;
}


#endif /* __ENCODER_CPP__ */