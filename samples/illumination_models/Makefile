TITLEID     := VGLILLMDL
TARGET		:= illumination_models
SOURCES		:= .
			
INCLUDES	:= include

LIBS = -lvitaGL -lSceLibKernel_stub -lSceAppMgr_stub -lSceAppUtil_stub -lmathneon \
	-lc -lSceCommonDialog_stub -lm -lSceGxm_stub -lSceDisplay_stub -lSceSysmodule_stub \
	-lvitashark -lSceShaccCg_stub -lSceKernelDmacMgr_stub -lstdc++ -lSceCtrl_stub

CFILES   := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.c))
CPPFILES   := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.cpp))
BINFILES := $(foreach dir,$(DATA), $(wildcard $(dir)/*.bin))
OBJS     := $(addsuffix .o,$(BINFILES)) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o) 

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CXX      = $(PREFIX)-g++
CFLAGS  = -g -Wl,-q -O2 -ftree-vectorize
CXXFLAGS  = $(CFLAGS) -fno-exceptions -std=gnu++11 -fpermissive
ASFLAGS = $(CFLAGS)

all: $(TARGET).vpk

$(TARGET).vpk: eboot.bin
	vita-mksfoex -s TITLE_ID=$(TITLEID) "$(TARGET)" param.sfo
	vita-pack-vpk -s param.sfo -b eboot.bin \
		-a cube.obj=cube.obj \
		-a sphere.obj=sphere.obj \
		-a bunny.obj=bunny.obj \
		-a plane.obj=plane.obj \
		-a phong.frag=phong.frag \
		-a phong.vert=phong.vert \
		-a blinn.frag=blinn.frag \
		-a blinn.vert=blinn.vert \
		-a lambertian.frag=lambertian.frag \
		-a lambertian.vert=lambertian.vert \
		-a fdg.frag=fdg.frag \
		-a fdg.vert=fdg.vert $@

eboot.bin: $(TARGET).velf
	vita-make-fself -s $< eboot.bin	
	
%.velf: %.elf
	vita-elf-create $< $@
	
$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@
	
clean:
	@rm -rf *.velf *.elf *.vpk $(OBJS) param.sfo eboot.bin
