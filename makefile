CC= gcc
# IROOT directory based on installed distribution tree (not archive/development tree). 
# IROOT=../..
IROOT=/usr/dalsa/GigeV

#
# Get the configured include defs file.
# (It gets installed to the distribution tree).
ifeq ($(shell if test -e archdefs.mk; then echo exists; fi), exists)
	include archdefs.mk
else
# Force an error
$(error	archdefs.mk file not found. It gets configured on installation ***)
endif

VIDEO_WRITER_PATH = ../ReadWriteMoviesWithOpenCV/

INC_PATH = -I. -I$(IROOT)/include $(INC_GENICAM) -I$(VIDEO_WRITER_PATH)/..
DEPS = $(VIDEO_WRITER_PATH)/DataManagement/VideoIO.h

                          
DEBUGFLAGS = -g 

CXX_COMPILE_OPTIONS = -c $(DEBUGFLAGS)

C_COMPILE_OPTIONS= $(DEBUGFLAGS)

LCLLIBS=  -L$(ARCHLIBDIR) -L/usr/local/lib -lGevApi -lCorW32

# # TODO: Cleanup
# OPENCV_PATHS=-I/usr/local/include/opencv -I/usr/local/include/opencv2 -L/usr/local/lib/
# OPENCV_LIBS=-lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_contrib -lopencv_legacy -lopencv_stitching

OPENCV = `pkg-config opencv --cflags --libs`

# objects to compile

OBJS += ../ReadWriteMoviesWithOpenCV/DataManagement/VideoIO.o \
        ../ReadWriteMoviesWithOpenCV/DataManagement/Pipe.o \
        dalsaGrabber.o

all: dalsa_grabber

#dalsaGrabber.o: dalsaGrabber.o
#	echo fooooo

%.o : %.c $(DEPS)
	$(CC) -I. $(INC_PATH) $(C_COMPILE_OPTIONS) $(ARCH_OPTIONS) -c $< -o $@

%.o : %.cpp $(DEPS)
	$(CC) -I. $(INC_PATH) $(CXX_COMPILE_OPTIONS) $(ARCH_OPTIONS) -c $< -o $@ -std=c++11


dalsa_grabber : $(OBJS) 
	$(CC) -g $(ARCH_LINK_OPTIONS) -std=c++11 -o dalsaGrabber $(OBJS) $(OPENCV) $(LCLLIBS) $(GENICAM_LIBS) -L$(ARCHLIBDIR) -lstdc++ -lm -lboost_system -lboost_program_options -lboost_thread

clean:
	rm *.o dalsaGrabber 
