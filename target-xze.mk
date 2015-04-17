.SUFFIXES:

OBJDIR := objects/xz-embedded

MAKETARGET = $(MAKE) --no-print-directory -C $@ -f $(CURDIR)/Makefile-xze \
                 SRCDIR=$(CURDIR) $(MAKECMDGOALS)

.PHONY: $(OBJDIR)
$(OBJDIR):
	+@[ -d $@ ] || mkdir -p $@
	+@$(MAKETARGET)

Makefile : ;
%.mk :: ;

% :: $(OBJDIR) ; :
