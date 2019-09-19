# dalsa-grabber
A simple recording app using the Dalsa GigE-V framework for Linux to make recording and display easy.
Defaults were tested to be appropriate for the Teledyne Dalsa Genie TS-C2500 camera.

## Installation ##

1. Install Dependencies

* GigE-V framework for linux (tested on v2.01.0.0120)
* ffmpeg (tested on v3.1.3)
* Opencv (tested on v2.4.9.1) - on Ubuntu install with `sudo apt-get install libopencv-dev`
* Boost libraries (tested on v1.58) - on Ubuntu install with `sudo apt-get install libboost-all-dev`

2. Compile

  * `cd /PATH_TO_REPO/`
  
  * `make`

## Usage ##

dalsaGrabber is the main console app produced from the makefile that let's you  write videos and monitor. Run `dalsaGrabber --help` for options (such as sepcifying framerate / resolution etc...)

Note: It is *strongly* reccomended the GigE-V network config script to improve network performance:
* `sudo /PATH_TO_GIGE-V_FRAMEWORK/DALSA/GigeV/bin/gev_nettweak ETHERNET_DEVICE_NAME`

(Hint: Available network devices can be listed with `ifconfig`)

### Examples ###

Record a video for 10s
`./dalsaGrabber record 10 ~/test.mp4`

Heads-up display
`./dalsaGrabber monitor`

Take an image
`./dalsaGrabber snapshot ~/test.jpg`
