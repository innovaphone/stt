include stt/apps/apps.mak

APP_OBJS += $(OUTDIR)/obj/stt.o
$(OUTDIR)/obj/stt.o: stt/stt.cpp $(OUTDIR)/stt/stt.png $(OUTDIR)/stt/apps/innovaphone-stt.png

$(OUTDIR)/stt/stt.png: stt/stt.png
	copy stt\stt.png $(OUTDIR)\stt\stt.png

$(OUTDIR)/stt/apps/innovaphone-stt.png: stt/apps/innovaphone-stt.png
	copy stt\apps\innovaphone-stt.png $(OUTDIR)\stt\apps\innovaphone-stt.png

APP_OBJS += $(OUTDIR)/obj/rccapi.o
$(OUTDIR)/obj/rccapi.o: stt/rccapi.cpp