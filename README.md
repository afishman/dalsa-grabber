# dalsa-grabber
An simple recording app using the Dalsa GigE-V framework for Linux to make recording and display easy.
Defaults were tested to be appropriate for the Teledyne Dalsa XX camera.

## Installation ##

i. Install Dependencies

* Gig-EV framework for linux (tested on XX)
* Opencv (tested on XX)
* Boost libraries (tested on XX)
* ffmpeg (tested on XX)

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
