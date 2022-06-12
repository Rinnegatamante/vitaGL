TITLEID     := VGLDEFRND
TARGET		:= ssao_deferred_rendering
SOURCES		:= .
			
INCLUDES	:= include

LIBS = -lvitaGL -lSceLibKernel_stub -lSceAppMgr_stub -lSceAppUtil_stub -lmathneon \
	-lc -lSceCommonDialog_stub -lm -lSceGxm_stub -lSceDisplay_stub -lSceSysmodule_stub \
	-lvitashark -lSceShaccCg_stub -lSceKernelDmacMgr_stub -lstdc++ -lSceCtrl_stub \
	-ltoloader -lSceTouch_stub -lSceShaccCgExt -ltaihen_stub

CFILES   := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.c))
CPPFILES   := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.cpp))
BINFILES := $(foreach dir,$(DATA), $(wildcard $(dir)/*.bin))
OBJS     := $(addsuffix .o,$(BINFILES)) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o) 

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CXX      = $(PREFIX)-g++
CFLAGS  = -mfp16-format=ieee -g -Wl,-q -O2 -ftree-vectorize
CXXFLAGS  = $(CFLAGS) -fno-exceptions -std=gnu++11 -fpermissive
ASFLAGS = $(CFLAGS)

all: $(TARGET).vpk

$(TARGET).vpk: eboot.bin
	vita-mksfoex -s TITLE_ID=$(TITLEID) "$(TARGET)" param.sfo
	vita-pack-vpk -s param.sfo -b eboot.bin \
		-a bunny.obj=bunny.obj \
		-a sphere.obj=sphere.obj \
		-a cube.obj=cube.obj \
		-a geometry.frag=geometry.frag \
		-a geometry.vert=geometry.vert \
		-a ssao.frag=ssao.frag \
		-a ssao.vert=ssao.vert \
		-a lighting.frag=lighting.frag \
		-a lighting.vert=lighting.bert $@

eboot.bin: $(TARGET).velf
	vita-make-fself -s $< eboot.bin	
	
%.velf: %.elf
	vita-elf-create $< $@
	
$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@
	
clean:
	@rm -rf *.velf *.elf *.vpk $(OBJS) param.sfo eboot.bin
