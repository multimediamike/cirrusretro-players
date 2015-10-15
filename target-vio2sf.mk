.SUFFIXES:

OBJDIR := objects/vio2sf-0.16

MAKETARGET = $(MAKE) --no-print-directory -C $@ -f $(CURDIR)/Makefile-vio2sf \
                 SRCDIR=$(CURDIR) $(MAKECMDGOALS)

.PHONY: $(OBJDIR)
$(OBJDIR):
	+@[ -d $@ ] || mkdir -p $@
	+@$(MAKETARGET)

Makefile : ;
%.mk :: ;

% :: $(OBJDIR) ; :
