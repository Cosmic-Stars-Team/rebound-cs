# Cosmic Stars — unified Makefile
#
# All source files compile once into build/*.o / *.obj, then
# link into the appropriate target (shared lib, static lib, executable).
#
# Targets:
#   make (all)        — build librebound + libcs
#   make librebound   — REBOUND shared library (for rebound/examples/)
#   make libcs        — CS static library
#   make pycs         — Python ctypes shared library (rebound_cs/_cs.*)
#   make test         — build & run test suite
#   make demo         — build & run physics demos
#   make demo_mass    — build & run mass-loss demo
#   make clean        — remove build artifacts
#
# Requirements:
#   Linux/macOS  — gcc/clang + ar + make
#   Windows      — MSVC (run vcvars64.bat first) + make

include src/Makefile.defs

# -------------------------------------------------------------------
# Directories
# -------------------------------------------------------------------
SRCDIR := src
CSDIR  := cs
BLDDIR := build
PYDIR  := rebound_cs

# -------------------------------------------------------------------
# Source files — defined once, shared across all targets
# -------------------------------------------------------------------
CS_SRC := $(CSDIR)/cs_simulation.c $(CSDIR)/cs_gr.c \
          $(CSDIR)/cs_radiation.c $(CSDIR)/cs_harmonics.c \
          $(CSDIR)/cs_tides.c $(CSDIR)/cs_solarmass.c
SRC_ALL := $(wildcard $(SRCDIR)/*.c)

TST_SRC  := $(BLDDIR)/test_cs.c
DEMO_SRC := $(BLDDIR)/demo_physics.c
DMAS_SRC := $(BLDDIR)/demo_mass.c

# -------------------------------------------------------------------
# Object files — compiled once to build/
# -------------------------------------------------------------------
CS_OBJ  := $(patsubst $(CSDIR)/%.c,$(BLDDIR)/%.$(OBJFILEEXT),$(CS_SRC))
SRC_OBJ := $(patsubst $(SRCDIR)/%.c,$(BLDDIR)/%.$(OBJFILEEXT),$(SRC_ALL))
ALL_OBJ := $(SRC_OBJ) $(CS_OBJ)

TST_OBJ  := $(TST_SRC:.c=.$(OBJFILEEXT))
DEMO_OBJ := $(DEMO_SRC:.c=.$(OBJFILEEXT))
DMAS_OBJ := $(DMAS_SRC:.c=.$(OBJFILEEXT))

# -------------------------------------------------------------------
# Platform-specific output names
# -------------------------------------------------------------------
ifeq ($(OS), Windows_NT)
    LIBCS    := $(BLDDIR)/cs.lib
    LIBRE    := $(BLDDIR)/librebound.dll
    PYCS_OUT := $(PYDIR)/_cs.dll
    TST_EXE  := $(BLDDIR)/test_cs.exe
    DEMO_EXE := $(BLDDIR)/demo_physics.exe
    DMAS_EXE := $(BLDDIR)/demo_mass.exe
else ifeq ($(OS), Darwin)
    LIBCS    := $(BLDDIR)/libcs.a
    LIBRE    := $(BLDDIR)/librebound.so
    PYCS_OUT := $(PYDIR)/_cs.dylib
    TST_EXE  := $(BLDDIR)/test_cs
    DEMO_EXE := $(BLDDIR)/demo_physics
    DMAS_EXE := $(BLDDIR)/demo_mass
else
    LIBCS    := $(BLDDIR)/libcs.a
    LIBRE    := $(BLDDIR)/librebound.so
    PYCS_OUT := $(PYDIR)/_cs.so
    TST_EXE  := $(BLDDIR)/test_cs
    DEMO_EXE := $(BLDDIR)/demo_physics
    DMAS_EXE := $(BLDDIR)/demo_mass
endif

CINCL := -I. -I$(SRCDIR)

# ====================================================================
# Default target
# ====================================================================
.PHONY: all librebound libcs pycs test demo demo_mass clean

all: librebound libcs

# ====================================================================
# REBOUND shared library
# ====================================================================
# Built in build/, then linked into src/ so that the upstream
# rebound/examples/ Makefiles can find it.
librebound: $(LIBRE)

ifeq ($(OS), Windows_NT)
$(LIBRE): $(SRC_OBJ)
	@echo "  LINK $@"
	$(CC) /nologo /D_USRDLL /D_WINDLL $(SRC_OBJ) /link /DLL /OUT:$@
	@copy /y $(LIBRE) $(SRCDIR)\ > nul 2>&1
	@if exist $(BLDDIR)\librebound.lib copy /y $(BLDDIR)\librebound.lib $(SRCDIR)\ > nul 2>&1
else
$(LIBRE): $(SRC_OBJ)
	@echo "  LINK $@"
	$(CC) $(OPT) -shared $(SRC_OBJ) $(LIB) -o $@
	@ln -s -f ../$@ $(SRCDIR)/$(notdir $(LIBRE)) 2>/dev/null || true
endif

# ====================================================================
# CS static library
# ====================================================================
libcs: $(LIBCS)

ifeq ($(OS), Windows_NT)
$(LIBCS): $(CS_OBJ)
	@echo "  LIB $@"
	lib /nologo /OUT:$@ $(CS_OBJ)
else
$(LIBCS): $(CS_OBJ)
	@echo "  AR  $@"
	ar rcs $@ $(CS_OBJ)
endif

# ====================================================================
# Python ctypes shared library (monolithic: REBOUND + CS in one)
# ====================================================================
pycs: $(PYCS_OUT)

ifeq ($(OS), Windows_NT)
$(PYCS_OUT): $(ALL_OBJ)
	@echo "  LINK $@"
	$(CC) /nologo /LD $(ALL_OBJ) /Fe:$@
else
$(PYCS_OUT): $(ALL_OBJ)
	@echo "  LINK $@"
	$(CC) $(OPT) -shared $(ALL_OBJ) $(LIB) -o $@
endif

# ====================================================================
# Test suite
# ====================================================================
test: $(SRC_OBJ) $(LIBCS) $(TST_EXE)
	@echo "  RUN  $(notdir $(TST_EXE))"
	cd $(BLDDIR) && ./$(notdir $(TST_EXE))

$(TST_EXE): $(TST_OBJ) $(CS_OBJ) $(SRC_OBJ)
ifeq ($(OS), Windows_NT)
	@echo "  LINK $@"
	$(CC) /nologo $(SRC_OBJ) $(CS_OBJ) $(TST_OBJ) /Fe:$@
else
	@echo "  LINK $@"
	$(CC) $(OPT) $(SRC_OBJ) $(CS_OBJ) $(TST_OBJ) $(LIB) -o $@
endif

# ====================================================================
# Physics demos
# ====================================================================
demo: $(SRC_OBJ) $(LIBCS) $(DEMO_EXE)
	@echo "  RUN  $(notdir $(DEMO_EXE))"
	cd $(BLDDIR) && ./$(notdir $(DEMO_EXE))

$(DEMO_EXE): $(DEMO_OBJ) $(CS_OBJ) $(SRC_OBJ)
ifeq ($(OS), Windows_NT)
	@echo "  LINK $@"
	$(CC) /nologo $(SRC_OBJ) $(CS_OBJ) $(DEMO_OBJ) /Fe:$@
else
	@echo "  LINK $@"
	$(CC) $(OPT) $(SRC_OBJ) $(CS_OBJ) $(DEMO_OBJ) $(LIB) -o $@
endif

demo_mass: $(SRC_OBJ) $(LIBCS) $(DMAS_EXE)
	@echo "  RUN  $(notdir $(DMAS_EXE))"
	cd $(BLDDIR) && ./$(notdir $(DMAS_EXE))

$(DMAS_EXE): $(DMAS_OBJ) $(CS_OBJ) $(SRC_OBJ)
ifeq ($(OS), Windows_NT)
	@echo "  LINK $@"
	$(CC) /nologo $(SRC_OBJ) $(CS_OBJ) $(DMAS_OBJ) /Fe:$@
else
	@echo "  LINK $@"
	$(CC) $(OPT) $(SRC_OBJ) $(CS_OBJ) $(DMAS_OBJ) $(LIB) -o $@
endif

# ====================================================================
# Compile pattern rules — all objects land in build/
# ====================================================================
ifeq ($(OS), Windows_NT)
$(BLDDIR)/%.$(OBJFILEEXT): $(SRCDIR)/%.c
	@if not exist "$(BLDDIR)" mkdir "$(BLDDIR)"
	@echo "  CC  $<"
	$(CC) -c $(OPT) $(PREDEF) $(CINCL) $< -Fo$@

$(BLDDIR)/%.$(OBJFILEEXT): $(CSDIR)/%.c
	@if not exist "$(BLDDIR)" mkdir "$(BLDDIR)"
	@echo "  CC  $<"
	$(CC) -c $(OPT) $(PREDEF) $(CINCL) $< -Fo$@

$(BLDDIR)/%.$(OBJFILEEXT): $(BLDDIR)/%.c
	@echo "  CC  $<"
	$(CC) -c $(OPT) $(PREDEF) $(CINCL) $< -Fo$@
else
$(BLDDIR)/%.$(OBJFILEEXT): $(SRCDIR)/%.c
	@mkdir -p $(BLDDIR)
	@echo "  CC  $<"
	$(CC) -c $(OPT) $(PREDEF) $(CINCL) -o $@ $<

$(BLDDIR)/%.$(OBJFILEEXT): $(CSDIR)/%.c
	@mkdir -p $(BLDDIR)
	@echo "  CC  $<"
	$(CC) -c $(OPT) $(PREDEF) $(CINCL) -o $@ $<

$(BLDDIR)/%.$(OBJFILEEXT): $(BLDDIR)/%.c
	@echo "  CC  $<"
	$(CC) -c $(OPT) $(PREDEF) $(CINCL) -o $@ $<
endif

# ====================================================================
# Clean
# ====================================================================
clean:
	@echo "Cleaning build artifacts..."
	-$(RM) $(BLDDIR)/*.$(OBJFILEEXT)
	-$(RM) $(LIBCS) $(LIBRE) $(TST_EXE) $(DEMO_EXE) $(DMAS_EXE)
	-$(RM) $(BLDDIR)/*.lib $(BLDDIR)/*.exp
	-$(RM) $(PYCS_OUT)
	-$(RM) $(PYDIR)/_cs.*
ifeq ($(OS), Windows_NT)
	-$(RM) $(SRCDIR)\$(notdir $(LIBRE)) $(SRCDIR)\librebound.lib 2>nul
else
	-$(RM) $(SRCDIR)/$(notdir $(LIBRE))
endif
