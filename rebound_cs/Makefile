# pycs/Makefile — build CS shared library for Python ctypes bindings
#
# Targets:
#   make          — build _cs.so / _cs.dll
#   make install  — copy to sibling pycs/ package dir
#   make clean    — remove build artifacts
#
# Requirements:
#   Windows : MSVC (run vcvars64.bat first) + nmake
#   Linux   : gcc + make
#   macOS   : clang + make

SRCDIR = ../src
CSDIR  = ../cs
PYDIR  = pycs

# All REBOUND core sources (OpenGL/MPI/server files are #ifdef-guarded, safe to compile)
REB_SRCS = \
	binarydiff.c boundary.c collision.c communication_mpi.c derivatives.c \
	display.c fmemopen.c frequency_analysis.c glad.c gravity.c input.c \
	integrator.c integrator_bs.c integrator_eos.c integrator_ias15.c \
	integrator_janus.c integrator_leapfrog.c integrator_mercurius.c \
	integrator_saba.c integrator_sei.c integrator_trace.c \
	integrator_whfast.c integrator_whfast512.c output.c particle.c \
	rebound.c rotations.c server.c simulationarchive.c tools.c \
	transformations.c tree.c

# All CS extension sources
CS_SRCS = \
	cs_simulation.c cs_gr.c cs_radiation.c cs_harmonics.c cs_tides.c \
	cs_solarmass.c

# -----------------------------------------------------------------------
# Platform detection
# -----------------------------------------------------------------------
UNAME := $(shell uname 2>/dev/null || echo Windows)

ifeq ($(OS), Windows_NT)
# MSVC
CC       = cl
CFLAGS   = /nologo /LD /I. /I$(SRCDIR) /D_GNU_SOURCE /DINLINE=__inline /Ox /fp:precise
LDFLAGS  =
OUT      = $(PYDIR)/_cs.dll
RM       = del /q 2>nul
else ifeq ($(UNAME), Darwin)
# macOS / clang
CC       = cc
CFLAGS   = -shared -fPIC -std=c99 -I. -I$(SRCDIR) -D_GNU_SOURCE -O2 -Wall -g
LDFLAGS  = -lm
OUT      = $(PYDIR)/_cs.dylib
RM       = rm -f
else
# Linux / gcc
CC       = gcc
CFLAGS   = -shared -fPIC -std=c99 -I. -I$(SRCDIR) -D_GNU_SOURCE -O2 -Wall -g
LDFLAGS  = -lm -lrt
OUT      = $(PYDIR)/_cs.so
RM       = rm -f
endif

OBJDIR  = build
REB_OBJ = $(REB_SRCS:%.c=$(OBJDIR)/%.o)
CS_OBJ  = $(CS_SRCS:%.c=$(OBJDIR)/%.o)

# -----------------------------------------------------------------------
# Targets
# -----------------------------------------------------------------------
.PHONY: all install clean

all: $(OUT)

install: $(OUT)
	@echo Installing to $(PYDIR)/
	cp $(OUT) $(PYDIR)/

$(OUT): $(REB_OBJ) $(CS_OBJ)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
	@echo "Built: $@"

# -----------------------------------------------------------------------
# Compile rules
# -----------------------------------------------------------------------
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(CSDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# -----------------------------------------------------------------------
# Clean
# -----------------------------------------------------------------------
clean:
	$(RM) $(REB_OBJ) $(CS_OBJ)
	$(RM) $(OUT)
	$(RM) $(OBJDIR)/*.o
	-rmdir $(OBJDIR) 2>/dev/null || true
