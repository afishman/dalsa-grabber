/*
* MIT License
* 
* Copyright (c) 2019 Aaron Fishman
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string> 
#include <iostream>
//#include <iostream>
#include <fstream>
using namespace std;

#include <opencv2/opencv.hpp>
using namespace cv;

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include "dalsaCamera.cpp"

// TODO: include option for scale
#define MONITOR_SCALE 1
#define WINDOW_NAME "Dalsa Monitor"

// Global camera reference for graceful termination
DalsaCamera *DALSA_CAMERA = NULL;

/* For graceful failing */
void onExit()
{
    if(DALSA_CAMERA != NULL)
    {
        DALSA_CAMERA->close();
    }

    cvDestroyAllWindows();  
}

/* Help message */
void printHelp(po::options_description desc)
{
    cout << desc << endl;
}

/* Test the speed of frame acquisition. Grab and release frames without any rendering. */
void speedTest(DalsaCamera *camera)
{
    cv::Mat img;    
    while(true)
    {
        if(camera->getNextImage(&img))
        {
            break;
        }

        img.release();        
    }
}

/* Live heads-up display of the dalsa */
void monitor(DalsaCamera *camera)
{
    cv::Mat img;

    // Setup OpenCV display window
    namedWindow(WINDOW_NAME, WINDOW_NORMAL);
 
    // Use first image to setup window
    if(camera->getNextImage(&img))
    {
        camera->close();
        cvDestroyWindow(WINDOW_NAME);
        return;
    }
    cv::Mat displayImg;
    cv::resize(img, displayImg, cv::Size(), MONITOR_SCALE, MONITOR_SCALE);
    cv::resizeWindow(WINDOW_NAME, displayImg.cols, displayImg.rows);

    // Display loop
    for(;;)
    {
        // Grab frame
        if(camera->getNextImage(&img))
        {
            break;
        }

        // Display
        cv::Mat displayImg;
        cv::resize(img, displayImg, cv::Size(), MONITOR_SCALE, MONITOR_SCALE);

        imshow(WINDOW_NAME, displayImg);
        img.release();
        displayImg.release(); 

        // User input
        int key = waitKey(1);
        if( (char) key == 'q') 
        {
            cout << "Quitting...\n";
            break; 
        } 
    }

    // Cleanup
    camera->close();
    cvDestroyWindow(WINDOW_NAME);
}

/* Record video */
void record(DalsaCamera *camera, float duration, int crf, char filename[])
{
    camera->record(duration, crf, filename);       
    camera->close();
}

/* Take a snapshot */
void snapshot(DalsaCamera *camera, char filename[])
{
    cout << "taking a snapshot to " << filename << endl;
    camera->snapshot(filename);
    camera->close();
}

/* A Simple console app to record and monitor using a Teledyne Dalsa GigE-V camera */
int main(int argc, char* argv[])
{   
    // Global options
    // Thanks: https://stackoverflow.com/questions/15541498/how-to-implement-subcommands-using-boost-program-options
    po::options_description globalArgs("Global options");
    bool debug = false; // flag
    globalArgs.add_options()
        ("command", po::value<std::string>(), "monitor | snapshot <filename> | record <seconds> <filename> | speed-test")
        ("subargs", po::value<std::vector<std::string> >(), "Sub args if required")
        ("framerate", po::value<float>()->default_value(29), "max 29fps")
        ("width", po::value<int>()->default_value(2560), "width should be an integer fraction of the max (2560)")
        ("height", po::value<int>()->default_value(1024), "height should be an integer fraction of the max (2048)")
        ("exposure", po::value<float>()->default_value(10000), "exposure in microseconds, must be less than the framerate")
        ("crf", po::value<int>()->default_value(15), "encoding quality (0 for lossless), see ffmpeg H.264 docs for more details https://trac.ffmpeg.org/wiki/Encode/H.264#crf")
        ("debug", po::bool_switch(&debug), "verbose logging for debugging purposes")
    ;

    // Mode + subargs
    po::positional_options_description pos;
    pos
    .add("command", 1)
    .add("subargs", -1);

    // Parse args
    po::parsed_options parsed = po::command_line_parser(argc, argv)
    .options(globalArgs)
    .positional(pos)
    .allow_unregistered()
    .run();

    po::variables_map vm;
    po::store(parsed, vm);
    po::notify(vm); 


    // Determine chosen command
    // TODO: surely a better way than exception try-catch, but notify doesn't seem to be throw an exception when it's not defined
    string command;
    try 
    {
        command = vm["command"].as<std::string>();
    }
    catch (...) //TODO: Lazy exception handling, check for type
    {
        printHelp(globalArgs);
        return 0;
    }

    // Retrieve camera params
    int framerate = vm["framerate"].as<float>();
    int width = vm["width"].as<int>();
    int height = vm["height"].as<int>();
    float exposure = vm["exposure"].as<float>();
    int crf = vm["crf"].as<int>();

    // Open Camera
    DALSA_CAMERA = new DalsaCamera(debug);
    std::atexit(onExit); // For Graceful failing
    
    if(DALSA_CAMERA->open(width, height, framerate, exposure))
    {
        cerr << "Failed to open camera\n";
        return 0;
    }

    // Run command
    // For graceful failing, run the sigint handler on exception

    if(command == "speed-test")
    {
        speedTest(DALSA_CAMERA);
    }
    else if(command == "monitor")
    {
        monitor(DALSA_CAMERA);
    }
    else if(command == "record")
    {
        // Record Options
        po::options_description record_desc("record options");
        record_desc.add_options()
            ("duration", po::value<float>(), "Record duration in seconds")
            ("filename", po::value<std::string>(), "filename (currently only .mp4)")
        ;

        // Collect all the unrecognized options from the first pass. This will include the
        // (positional) command name, so we need to erase that.
        std::vector<std::string> opts = po::collect_unrecognized(parsed.options, po::include_positional);
        opts.erase(opts.begin());

        // Record positional args
        po::positional_options_description record_pos_args;
        record_pos_args
        .add("duration", 1)
        .add("filename", 1);

        // Parse again...
        po::store(po::command_line_parser(opts).options(record_desc).positional(record_pos_args).run(), vm);

        float duration;
        std::string filenameStr;
        try
        {
            duration = vm["duration"].as<float>();
            filenameStr = vm["filename"].as<std::string>();
        }
        catch(...) //TODO: Lazy exception handling, check for type
        {
            cerr << "ERROR: you need provide duration and filename with record option";
            printHelp(globalArgs);
        }

        // Convert to char array
        char filename[filenameStr.length()+1];
        strcpy(filename, filenameStr.c_str());

        record(DALSA_CAMERA, duration, crf, filename);
    }
    else if(command == "snapshot")
    {
        // Record Options
        po::options_description snap_desc("snapshot options");
        snap_desc.add_options()("filename", po::value<std::string>(), "filename (currently only .mp4)");

        // Collect all the unrecognized options from the first pass. This will include the
        // (positional) command name, so we need to erase that.
        std::vector<std::string> opts = po::collect_unrecognized(parsed.options, po::include_positional);
        opts.erase(opts.begin());

        // Record positional args
        po::positional_options_description snap_pos_args;
        snap_pos_args.add("filename", 1);

        // Parse again...
        po::store(po::command_line_parser(opts).options(snap_desc).positional(snap_pos_args).run(), vm);

        std::string filenameStr;
        try
        {
            filenameStr = vm["filename"].as<std::string>();
        }
        catch(...) //TODO: Lazy exception handling, check for type
        {
            cerr << "ERROR: you need provide filename with snapshot option";
            printHelp(globalArgs);
        }

        // Convert to char array
        char filename[filenameStr.length()+1];
        strcpy(filename, filenameStr.c_str());

        snapshot(DALSA_CAMERA, filename);            
    }
    else
    {
        cerr << "COMMAND UNRECOGNISED: " << command << endl;
        printHelp(globalArgs);
    }


    return 0;
}


