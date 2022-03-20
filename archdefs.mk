#-----------------------------------------------------------------------------
# archdefs.mk               						
#
# Description: 
#       Architecture specific definitions for compiling and linking
#		programs using GigE-V Framework API and GenICam libraries.
#		For i86/x86_64  
#
#-----------------------------------------------------------------------------#
# Architecture-dependent definitions.
# (GenICam libraries have different paths and names depending on the architecture)
#
# Architecture specific environment defs for self-hosted environment 
#
# Reproduced from PATH_TO_GIGE-V_FRAMEWORK/DALSA/GigeV/examples/genicam_c_demo  
ifndef ARCH
  ARCH := $(shell uname -m | sed -e s/i.86/i386/ -e s/x86_64/x86_64/ -e s/armv5.*/armv5/ -e s/armv6.*/armv6/ -e s/armv7.*/armv7/ -e s/aarch64/aarch64/)
endif

ifeq  ($(ARCH), x86_64)
	ARCHNAME=x86_64
	ARCH_GENICAM_BIN=Linux64_x64
	ARCH_OPTIONS= -Dx86_64 -D_REENTRANT
	ARCH_GCCVER=421
else
ifeq  ($(ARCH), i386)
	ARCHNAME=i386
	ARCH_GENICAM_BIN=Linux32_i86
	ARCH_OPTIONS= -D__i386__ -D_REENTRANT
	ARCH_GCCVER=421
else
ifeq  ($(ARCH), aarch64)
	# Very Old
	ARCHNAME=armv8-a
	ARCH_GENICAM_BIN=Linux64_ARM
	ARCH_OPTIONS= -D__arm__ -D_REENTRANT
	ARCH_GCCVER=54
else
# Not supported
$(error Architecture $(ARCH) not configured for this installation.)
endif
endif
endif

ifeq  ($(ARCH), aarch64)
ARCHLIBDIR=/usr/lib/aarch64-linux-gnu
ARCH_LINK_OPTIONS=
else
ARCHLIBDIR=/usr/lib
endif

#
# Arch dependent GenICam library specification
#
GENICAM_PATH_VERSION=v3_0
GENICAM_PATH:=$(GENICAM_ROOT_V3_0)
INC_GENICAM=-I$(GENICAM_PATH)/library/CPP/include
GENICAM_LIBS=-L$(GENICAM_PATH)/bin/$(ARCH_GENICAM_BIN)\
					-lGenApi_gcc$(ARCH_GCCVER)_$(GENICAM_PATH_VERSION)\
					-lGCBase_gcc$(ARCH_GCCVER)_$(GENICAM_PATH_VERSION)			
					
