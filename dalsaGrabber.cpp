#include <stdlib.h>
#include <stdio.h>
#include <string> 
#include <iostream>
//#include <iostream>
#include <fstream>
using namespace std;

#include <opencv2/opencv.hpp>
using namespace cv;

#include "dalsaCamera.cpp"

#include <boost/program_options.hpp>
namespace po = boost::program_options;

//TODO: program option?
#define MONITOR_SCALE 0.25
#define WINDOW_NAME "Dalsa Monitor"

// For Graceful failing
DalsaCamera *DALSA_CAMERA = NULL;
void sigintHandler(int s)
{
    if(DALSA_CAMERA != NULL)
    {
        DALSA_CAMERA->close();
    }

    cvDestroyAllWindows();
    exit(1); 
}

// Boost Program Options help
void printHelp(po::options_description desc)
{
    cout << desc << endl;
}

// Test the speed of your camera
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

// Monitor without record
void monitor(DalsaCamera *camera)
{
    // Setup OpenCV display window
    namedWindow(WINDOW_NAME, WINDOW_AUTOSIZE );
    
    // Display frames 4 evs
    cv::Mat img;
    for(;;)
    {
        if(camera->getNextImage(&img))
        {
            break;
        }

        cv::Mat displayImg;
        cv::resize(img, displayImg, cv::Size(), MONITOR_SCALE, MONITOR_SCALE);


        imshow(WINDOW_NAME, displayImg);
        img.release();
        displayImg.release();

        int key = waitKey(1);
        if( (char) key == 'q') 
        {
            cout << "Quitting...\n";
            break; 
        } 
    }

    camera->close();
    cvDestroyWindow(WINDOW_NAME);
}

// Record some video
void record(DalsaCamera *camera, float duration, char filename[])
{
    camera->record(duration, filename);       
    camera->close();
}

int main(int argc, char* argv[])
{
    /*
        A Simple console app to record and monitor using a Teledyne Dalsa GigE-V camera
    */

    // For graceful failing with a crtl+c interrupt 
    signal(SIGINT, sigintHandler);

    // Global options
    // Thanks: https://stackoverflow.com/questions/15541498/how-to-implement-subcommands-using-boost-program-options
    // TODO: debug option
    po::options_description globalArgs("Global options");
    bool debug = false; // flag
    globalArgs.add_options()
        ("command", po::value<std::string>(), "record <seconds> <filename> | monitor | speed-test")
        ("subargs", po::value<std::vector<std::string> >(), "Sub args if required")
        ("framerate", po::value<float>()->default_value(29), "max 29fps")
        ("width", po::value<int>()->default_value(2560), "width should be an integer fraction of the max (2560)")
        ("height", po::value<int>()->default_value(1024), "height should be an integer fraction of the max (2048)")
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
    // TODO: surely a better way, 
    // but notify doesn't seem to be throwing an exception when command is not defined
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

    // Open Camera
    DALSA_CAMERA = new DalsaCamera(debug);
    if(DALSA_CAMERA->open(width, height, framerate))
    {
        cerr << "Failed to open camera";
        return 0;
    }

    // Run command
    // For graceful failing, run the sigint handler on exception
    try
    {
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

            record(DALSA_CAMERA, duration, filename);
        }
        else
        {
            cerr << "COMMAND UNRECOGNISED: " << command << endl;
            printHelp(globalArgs);
        }
    }
    catch(...)
    {
        sigintHandler(0);
    }


    return 0;
}