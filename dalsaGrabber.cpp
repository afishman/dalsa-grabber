#include <stdlib.h>
#include <stdio.h>
#include <string> 
#include <iostream>
//#include <iostream>
#include <fstream>
using namespace std;

#include <opencv2/opencv.hpp>
using namespace cv;

#include "DalsaCamera.cpp"

#include <boost/program_options.hpp>
namespace po = boost::program_options;

//TODO: program option
const float monitorScale = 0.25;


void printHelp(po::options_description desc)
{
    cout << desc << endl;
}

void speedTest(int width, int height, float framerate)
{
    DalsaCamera camera = DalsaCamera();
    camera.open(width, height, framerate);
    cv::Mat img;
    
    while(true)
    {
        if(camera.getNextImage(&img))
        {
            break;
        }

        img.release();        
    }
}

void monitor(int width, int height, float framerate)
{
    // Open Camera
    DalsaCamera camera = DalsaCamera();
    if(camera.open(width, height, framerate))
    {
        cerr << "Failed to open camera";
        return;
    }

    // Setup OpenCV display window
    char windowName[] = "Dalsa Monitor";
    namedWindow(windowName, WINDOW_AUTOSIZE );
    
    // Display frames 4 evs
    cv::Mat img;
    for(;;)
    {
        if(camera.getNextImage(&img))
        {
            break;
        }

        cv::Mat displayImg;
        cv::resize(img, displayImg, cv::Size(), monitorScale, monitorScale);


        imshow(windowName, displayImg);
        img.release();
        displayImg.release();

        int key = waitKey(1);
        if( (char) key == 'q') 
        {
            cout << "Quitting...\n";
            break; 
        } 
    }

    camera.close();
    cvDestroyWindow(windowName);
}


void record(int width, int height, float framerate, float duration, char filename[])
{
    DalsaCamera camera = DalsaCamera();
    if(!camera.open(width, height, framerate))
    {
        cerr << "Failed to open camera";
        return;
    } 
    

    camera.record(duration, filename);       
    camera.close();
}


int main(int argc, char* argv[])
{
    /*
        A Simple console app to record and test
    */


    // Global options
    // Thanks: https://stackoverflow.com/questions/15541498/how-to-implement-subcommands-using-boost-program-options
    po::options_description globalArgs("Global options");
    globalArgs.add_options()
        ("command", po::value<std::string>(), "speed-test | record <seconds> | monitor")
        ("subargs", po::value<std::vector<std::string> >(), "Sub args if required")
        ("framerate", po::value<float>()->default_value(29), "max 29fps")
        ("width", po::value<int>()->default_value(2560), "width should be an integer fraction of the max (2560)")
        ("height", po::value<int>()->default_value(1024), "height should be an integer fraction of the max (2048)")
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


    // Get chosen commands
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

    // Retrieve other camera params
    int framerate = vm["framerate"].as<float>();
    int width = vm["width"].as<int>();
    int height = vm["height"].as<int>();

    if(command == "speed-test")
    {
        speedTest(width, height, framerate);
    }
    else if(command == "monitor")
    {
        monitor(width, height, framerate);
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
            cerr << "ERROR: you need provide duration and filename with record";
            printHelp(globalArgs);
        }

        // Convert to char array
        char filename[filenameStr.length()+1];
        strcpy(filename, filenameStr.c_str());

        record(width, height, framerate, duration, filename);
    }
    else
    {
        cerr << "COMMAND UNRECOGNISED: " << command << endl;
        printHelp(globalArgs);
    }

    return 0;
}