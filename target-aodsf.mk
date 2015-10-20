.SUFFIXES:

OBJDIR := objects/aodsf

MAKETARGET = $(MAKE) --no-print-directory -C $@ -f $(CURDIR)/Makefile-aodsf \
                 SRCDIR=$(CURDIR) $(MAKECMDGOALS)

.PHONY: $(OBJDIR)
$(OBJDIR):
	+@[ -d $@ ] || mkdir -p $@
	+@$(MAKETARGET)

Makefile : ;
%.mk :: ;

% :: $(OBJDIR) ; :
