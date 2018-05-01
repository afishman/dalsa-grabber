# dalsa-grabber
An simple recording app using the Dalsa GigE-V framework for Linux to make recording and display easy.
Defaults were tested to be appropriate for the Teledyne Dalsa XX camera.

## Installation ##

i. Install Dependencies

* Gig-EV framework for linux (tested on v2.01.0.0120)
* Opencv (tested on v2.4.9.1)
* Boost libraries (tested on v1.58)
* ffmpeg (tested on v3.1.3)

ii. Compile

  * `cd path_to_repo`
  
  * `make`

## Usage ##

dalsaGrabber is the main console app produced from the makefile that let's you  write videos and monitor. Run `dalsaGrabber --help` for options (such as sepcifying framerate / resolution etc...)

### Examples ###

Record a video for 10s at 10Hz:
`./dalsaGrabber record 10 ~/test.mp4`

Stream video to screen but don't record
`./dalsaGrabber monitor`
