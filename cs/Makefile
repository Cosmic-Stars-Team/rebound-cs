# Makefile for Cosmic Stars (cs/) physics extension layer
#
# Targets:
#   make          - build static library libcs.a / cs.lib
#   make test     - build and run test suite
#   make demo     - build and run physics demos
#   make clean    - remove .o and library files

include ../src/Makefile.defs

# --- paths ---
ROOT   = ..
SRCDIR = $(ROOT)/src
TSTDIR = $(ROOT)/build

# --- CS layer sources ---
CS_SRC  = cs_simulation.c cs_gr.c cs_radiation.c cs_solarmass.c
CS_OBJ  = $(CS_SRC:.c=.$(OBJFILEEXT))
CS_INC  = cs.h cs_simulation.h cs_gr.h cs_radiation.h cs_solarmass.h

# --- static library name ---
ifeq ($(OS), Windows_NT)
    LIBCS   = cs.lib
    AR_CMD  = lib /nologo /OUT:$(LIBCS)
else
    LIBCS   = libcs.a
    AR_CMD  = ar rcs $(LIBCS)
endif

# --- compiler flags ---
CINCL = -I$(ROOT) -I$(SRCDIR)

# ====================================================================
all: $(LIBCS)

%.$(OBJFILEEXT): %.c $(CS_INC)
	@echo "  CC  $<"
	$(CC) -c $(OPT) $(PREDEF) $(CINCL) $<

$(LIBCS): $(CS_OBJ)
	@echo "  AR  $(LIBCS)"
	$(AR_CMD) $(CS_OBJ)

# ====================================================================
# test suite (source in build/test_cs.c)
# ====================================================================
TST_SRC  = $(TSTDIR)/test_cs.c
TST_OBJ  = test_cs.$(OBJFILEEXT)
ifeq ($(OS), Windows_NT)
    TST_EXE = $(TSTDIR)/test_cs.exe
else
    TST_EXE = $(TSTDIR)/test_cs
endif

# compile ALL of src/ so we can link (no pre-built librebound needed)
SRC_ALL  = $(wildcard $(SRCDIR)/*.c)
SRC_OBJ  = $(notdir $(SRC_ALL:.c=.$(OBJFILEEXT)))

# rule: compile src/*.c → current dir
%.$(OBJFILEEXT): $(SRCDIR)/%.c
	@echo "  CC  $<"
	$(CC) -c $(OPT) $(PREDEF) $(CINCL) $<

$(TST_OBJ): $(TST_SRC)
	@echo "  CC  $<"
	$(CC) -c $(OPT) $(PREDEF) $(CINCL) $<

test: $(SRC_OBJ) $(CS_OBJ) $(TST_OBJ)
	@echo "  LINK  $(TST_EXE)"
ifeq ($(OS), Windows_NT)
	$(CC) /nologo $(SRC_OBJ) $(CS_OBJ) $(TST_OBJ) /Fe:$(TST_EXE)
else
	$(CC) $(OPT) $(SRC_OBJ) $(CS_OBJ) $(TST_OBJ) $(LIB) -o $(TST_EXE)
endif
	@echo "  RUN"
	cd $(TSTDIR) && ./$(notdir $(TST_EXE))

# ====================================================================
# physics demos
# ====================================================================
DEMO_SRC = $(TSTDIR)/demo_physics.c
DEMO_OBJ = demo_physics.$(OBJFILEEXT)
ifeq ($(OS), Windows_NT)
    DEMO_EXE = $(TSTDIR)/demo_physics.exe
else
    DEMO_EXE = $(TSTDIR)/demo_physics
endif

$(DEMO_OBJ): $(DEMO_SRC)
	@echo "  CC  $<"
	$(CC) -c $(OPT) $(PREDEF) $(CINCL) $<

demo: $(SRC_OBJ) $(CS_OBJ) $(DEMO_OBJ)
	@echo "  LINK  $(DEMO_EXE)"
ifeq ($(OS), Windows_NT)
	$(CC) /nologo $(SRC_OBJ) $(CS_OBJ) $(DEMO_OBJ) /Fe:$(DEMO_EXE)
else
	$(CC) $(OPT) $(SRC_OBJ) $(CS_OBJ) $(DEMO_OBJ) $(LIB) -o $(DEMO_EXE)
endif
	@echo "  RUN"
	cd $(TSTDIR) && ./$(notdir $(DEMO_EXE))

# ====================================================================
DEMO_MASS_SRC = $(TSTDIR)/demo_mass.c
DEMO_MASS_OBJ = demo_mass.$(OBJFILEEXT)
ifeq ($(OS), Windows_NT)
    DEMO_MASS_EXE = $(TSTDIR)/demo_mass.exe
else
    DEMO_MASS_EXE = $(TSTDIR)/demo_mass
endif

$(DEMO_MASS_OBJ): $(DEMO_MASS_SRC)
	@echo "  CC  $<"
	$(CC) -c $(OPT) $(PREDEF) $(CINCL) $<

demo_mass: $(SRC_OBJ) $(CS_OBJ) $(DEMO_MASS_OBJ)
	@echo "  LINK  $(DEMO_MASS_EXE)"
ifeq ($(OS), Windows_NT)
	$(CC) /nologo $(SRC_OBJ) $(CS_OBJ) $(DEMO_MASS_OBJ) /Fe:$(DEMO_MASS_EXE)
else
	$(CC) $(OPT) $(SRC_OBJ) $(CS_OBJ) $(DEMO_MASS_OBJ) $(LIB) -o $(DEMO_MASS_EXE)
endif
	@echo "  RUN"
	cd $(TSTDIR) && ./$(notdir $(DEMO_MASS_EXE))

# ====================================================================
clean:
	@echo "Cleaning cs/ build artifacts..."
	@-$(RM) *.$(OBJFILEEXT)
	@-$(RM) $(LIBCS)
	@-$(RM) *.exe
	@-$(RM) test_cs demo_physics demo_mass
	@echo "Cleaning build/ executables..."
	@-$(RM) $(TSTDIR)/*.exe
	@-$(RM) $(TSTDIR)/test_cs
	@-$(RM) $(TSTDIR)/demo_physics
	@-$(RM) $(TSTDIR)/demo_mass
