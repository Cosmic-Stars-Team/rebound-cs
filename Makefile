# This Makefile compiles the shared dynamic library librebound.so
include src/Makefile.defs

librebound:
	$(MAKE) -C src
	@$(LINKORCOPYLIBREBOUNDMAIN)
	@echo "To compile the example problems, go to a subdirectory of examples/ and execute make there."

libcs:
	$(MAKE) -C cs

test: libcs
	$(MAKE) -C cs test

demo: libcs
	$(MAKE) -C cs demo

.PHONY: pythoncopy
pythoncopy:
	-cp librebound.so `python -c "import rebound; print(rebound.__libpath__)"`

all: librebound libcs pythoncopy

clean:
	$(MAKE) -C src clean
	$(MAKE) -C cs clean
