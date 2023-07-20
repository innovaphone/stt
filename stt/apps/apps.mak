
APPWEBPATH = STT/apps

STARTHTMLS += \
	$(APPWEBPATH)/innovaphone-stt.htm

APPWEBSRC_NOZIP += \
	$(APPWEBPATH)/innovaphone-stt.png

APPWEBSRC_ZIP += \
	$(APPWEBPATH)/innovaphone-stt.js \
	$(APPWEBPATH)/innovaphone-stt.css \
	$(APPWEBPATH)/innovaphone-stttexts.js \
	$(APPWEBPATH)/innovaphone-searchbar.js \
	$(APPWEBPATH)/innovaphone-transcriptionbox.js \
	$(APPWEBPATH)/PhoneNumberLib.js

MANAGERSRC = \
	$(APPWEBPATH)/innovaphone.sttmanager.js \
	$(APPWEBPATH)/innovaphone.sttmanager.css \
	$(APPWEBPATH)/innovaphone.sttmanagertexts.js

APPWEBSRC = $(APPWEBSRC_ZIP) $(MANAGERSRC) $(APPWEBSRC_NOZIP)

$(OUTDIR)/obj/apps_start.cpp: $(STARTHTMLS) $(OUTDIR)/obj/innovaphone.manifest
		$(IP_SRC)/exe/httpfiles -k -d $(OUTDIR)/obj -t $(OUTDIR) -o $(OUTDIR)/obj/apps_start.cpp \
	-s 0 $(subst $(APPWEBPATH)/,,$(STARTHTMLS))

$(OUTDIR)/obj/apps.cpp: $(APPWEBSRC)
		$(IP_SRC)/exe/httpfiles -k -d $(APPWEBPATH) -t $(OUTDIR) -o $(OUTDIR)/obj/apps.cpp \
	-s 0,HTTP_GZIP $(subst $(APPWEBPATH)/,,$(APPWEBSRC_ZIP) $(MANAGERSRC)) \
	-s 0 $(subst $(APPWEBPATH)/,,$(APPWEBSRC_NOZIP))

APP_OBJS += $(OUTDIR)/obj/apps_start.o
$(OUTDIR)/obj/apps_start.o: $(OUTDIR)/obj/apps_start.cpp
APP_OBJS += $(OUTDIR)/obj/apps.o
$(OUTDIR)/obj/apps.o: $(OUTDIR)/obj/apps.cpp
