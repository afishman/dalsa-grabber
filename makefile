# Compiler and linker settings
CC=gcc
IROOT=/usr/dalsa/GigeV
VIDEO_WRITER_PATH = ./videoIO

# Architecture specific definitions (reproduced from PATH_TO_GIGE-V_FRAMEWORK/DALSA/GigeV/examples/genicam_c_demos)
ifeq ($(shell if test -e archdefs.mk; then echo exists; fi), exists)
	include archdefs.mk
else
	$(error	archdefs.mk file not found. It gets configured on installation ***)
endif

# Dependancy paths
INC_PATH = -I. -I$(IROOT)/include $(INC_GENICAM) -I$(VIDEO_WRITER_PATH)/..
DEPS = $(VIDEO_WRITER_PATH)/VideoIO.h                       
LCLLIBS=  -L$(ARCHLIBDIR) -L/usr/local/lib -lGevApi -lCorW32
OPENCV = `pkg-config opencv --cflags --libs`

# objects to compile
OBJS += ./videoIO/VideoIO.o \
        ./videoIO/Pipe.o \
        dalsaGrabber.o

DEBUGFLAGS = -g 
CXX_COMPILE_OPTIONS = -c $(DEBUGFLAGS)
C_COMPILE_OPTIONS= $(DEBUGFLAGS)

# Targets
all: dalsaGrabber

%.o : %.c $(DEPS)
	$(CC) -I. $(INC_PATH) $(C_COMPILE_OPTIONS) $(ARCH_OPTIONS) -c $< -o $@

%.o : %.cpp $(DEPS)
	$(CC) -I. $(INC_PATH) $(CXX_COMPILE_OPTIONS) $(ARCH_OPTIONS) -c $< -o $@ -std=c++11


dalsaGrabber : $(OBJS) 
	$(CC) -g $(ARCH_LINK_OPTIONS) -std=c++11 -o dalsaGrabber $(OBJS) $(OPENCV) $(LCLLIBS) $(GENICAM_LIBS) -L$(ARCHLIBDIR) -lstdc++ -lm -lboost_system -lboost_program_options -lboost_thread

clean:
	rm *.o dalsaGrabber 
