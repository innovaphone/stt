#----------------------------------------------------------------------------
# stt.mak
# copyright (c) innovaphone 2015
#----------------------------------------------------------------------------

OUT = stt

include common/build/build.mak

include sdk/platform/sdk-defs.mak

include web1/appwebsocket/appwebsocket.mak
include web1/config/config.mak
include web1/fonts/fonts.mak
include web1/lib1/lib1.mak
include web1/ui1.lib/ui1.lib.mak
include web1/ui1.svg/ui1.svg.mak
include web1/ui1.switch/ui1.switch.mak
include web1/ui1.scrolling/ui1.scrolling.mak
include web1/ui1.autocompleteinput/ui1.autocompleteinput.mak

-include stt/submodules.mak
include stt/stt.mak

APP_OBJS += $(OUTDIR)/obj/stt-main.o
$(OUTDIR)/obj/stt-main.o: stt-main.cpp force

include sdk/platform/sdk.mak
