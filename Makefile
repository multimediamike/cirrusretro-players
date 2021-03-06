.DEFAULT_GOAL:=all

ifeq (,$(BUILD_THREADS))
    JOBS:=
else
    JOBS=-j$(BUILD_THREADS)
endif

all:
	make -f Makefile-gme GME_PLAYER=ay $(JOBS)
	make -f Makefile-gme GME_PLAYER=gbs $(JOBS)
	make -f Makefile-gme GME_PLAYER=gym $(JOBS)
	make -f Makefile-gme GME_PLAYER=hes $(JOBS)
	make -f Makefile-gme GME_PLAYER=kss $(JOBS)
	make -f Makefile-gme GME_PLAYER=nsf $(JOBS)
	make -f Makefile-gme GME_PLAYER=sap $(JOBS)
	make -f Makefile-gme GME_PLAYER=spc $(JOBS)
	make -f Makefile-gme GME_PLAYER=vgm $(JOBS)
	make -f Makefile-vio2sf $(JOBS)
	make -f Makefile-aodsf $(JOBS)
	make -f Makefile-aossf $(JOBS)

clean:
	rm -rf objects
