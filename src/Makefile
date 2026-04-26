# Thin wrapper — delegates librebound build to the top-level unified Makefile.
# Needed for compatibility with the rebound/examples/ Makefiles.
include Makefile.defs

.PHONY: all librebound clean

all librebound:
	$(MAKE) -C .. librebound

clean:
	$(MAKE) -C .. clean
	-$(RM) *.$(OBJFILEEXT)
