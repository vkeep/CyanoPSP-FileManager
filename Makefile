TARGET = CyanogenMod
OBJS = main.o mp3player.o
	   
ifeq ($(CONFIG_620), 1)
CFLAGS += -DCONFIG_620=1
PSP_FW_VERSION = 620
endif

ifeq ($(CONFIG_635), 1)
CFLAGS += -DCONFIG_635=1
PSP_FW_VERSION = 635
endif

ifeq ($(CONFIG_639), 1)
CFLAGS += -DCONFIG_639=1
PSP_FW_VERSION = 639
endif

ifeq ($(CONFIG_660), 1)
CFLAGS += -DCONFIG_660=1
PSP_FW_VERSION = 660
endif

INCDIR = 
CFLAGS = -G4 -Wall -O2 -g -G0
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS) -c 

LIBDIR =
LDFLAGS =
STDLIBS= -losl -lpng -lz \
         -lpsphprm -lpspsdk -lpspctrl -lpspumd -lpsprtc -lpsppower -lpspgu -lpspgum  -lpspaudiolib -lpspaudio -lpsphttp -lpspssl -lpspwlan \
         -lpspnet_adhocmatching -lpspnet_adhoc -lpspnet_adhocctl -lm -ljpeg -lpspvram -lpsputility -lpspkubridge -lpspsystemctrl_user  -lpspreg \
		 -lpspusb -lpspusbstor -lpspusbdevice -lpspmp3 -lmad -lpspaudiocodec -lpspsystemctrl_kernel -lpspvshctrl -lpsprtc_driver -lpspreg_driver
		 
LIBS=$(STDLIBS)$(YOURLIBS)

PSP_LARGE_MEMORY = 1
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = CyanoPSP File Manager
PSP_EBOOT_ICON = ICON0.png
DATE = $(shell date +%Y%m%d)

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak