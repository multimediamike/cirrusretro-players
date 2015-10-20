.SUFFIXES:

OBJDIR := objects/aossf

MAKETARGET = $(MAKE) --no-print-directory -C $@ -f $(CURDIR)/Makefile-aossf \
                 SRCDIR=$(CURDIR) $(MAKECMDGOALS)

.PHONY: $(OBJDIR)
$(OBJDIR):
	+@[ -d $@ ] || mkdir -p $@
	+@$(MAKETARGET)

Makefile : ;
%.mk :: ;

% :: $(OBJDIR) ; :
