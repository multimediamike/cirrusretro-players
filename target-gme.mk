.SUFFIXES:

OBJDIR := objects/gme-$(GME_PLAYER)

MAKETARGET = $(MAKE) --no-print-directory -C $@ -f $(CURDIR)/Makefile-gme \
                 SRCDIR=$(CURDIR) $(MAKECMDGOALS)

.PHONY: $(OBJDIR)
$(OBJDIR):
	+@[ -d $@ ] || mkdir -p $@
	+@$(MAKETARGET)

Makefile : ;
%.mk :: ;

% :: $(OBJDIR) ; :
