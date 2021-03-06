# Copyright (c) 2016      Bryce Adelstein-Lelbach aka wash
# Copyright (c) 2000-2016 Paul Ullrich 
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying 
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Set compiler flags and preprocessor defines.

###############################################################################
# Configuration-independent configuration.

CXXFLAGS+= -std=c++11

ifndef TEMPESTREMAPDIR
  $(error TEMPESTREMAPDIR is not defined)
endif

# Add the source directories to the include path
CXXFLAGS+= -I$(TEMPESTREMAPDIR)/src/base

###############################################################################
# Configuration-dependent configuration.

ifeq ($(OPT),TRUE)
  # NDEBUG disables assertions, among other things.
  CXXFLAGS+= -O3 -DNDEBUG 
  F90FLAGS+= -O3
else
  CXXFLAGS+= -O0
  F90FLAGS+= -O0 
endif

ifeq ($(DEBUG),TRUE)
  # Frame pointers give us more meaningful stack traces in OPT+DEBUG builds.
  CXXFLAGS+= -ggdb -fno-omit-frame-pointer
  F90FLAGS+= -g
endif

ifeq ($(PARALLEL),MPIOMP)
  CXXFLAGS+= -DTEMPEST_MPIOMP 
  CXX= $(MPICXX)
  F90= $(MPIF90)
else ifeq ($(PARALLEL),NONE)
else
  $(error mk/config.make does not properly define PARALLEL)
endif

ifeq ($(NETCDF),TRUE)
  CXXFLAGS+=  -DTEMPEST_NETCDF $(NETCDF_CXXFLAGS)
  LIBRARIES+= $(NETCDF_LIBRARIES)
  LDFLAGS+=   $(NETCDF_LDFLAGS)
endif

ifeq ($(LAPACK_INTERFACE),ESSL)
  CXXFLAGS+= -DTEMPEST_LAPACK_ESSL_INTERFACE
else ifeq ($(LAPACK_INTERFACE),ACML)
  CXXFLAGS+= -DTEMPEST_LAPACK_ACML_INTERFACE
else ifeq ($(LAPACK_INTERFACE),FORTRAN)
  CXXFLAGS+= -DTEMPEST_LAPACK_FORTRAN_INTERFACE
else
  $(error mk/system/$(SYSTEM_MAKEFILE) does not properly define LAPACK_INTERFACE)
endif
  
CXXFLAGS+=  $(LAPACK_CXXFLAGS)
LIBRARIES+= $(LAPACK_LIBRARIES)
LDFLAGS+=   $(LAPACK_LDFLAGS)

# DO NOT DELETE
