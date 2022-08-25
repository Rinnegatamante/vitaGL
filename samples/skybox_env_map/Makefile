TITLEID     := VGLSKYBOX
TARGET		:= skybox_env_map
SOURCES		:= .
			
INCLUDES	:= include

LIBS = -lvitaGL -lSceLibKernel_stub -lSceAppMgr_stub -lSceAppUtil_stub -lmathneon \
	-lc -lSceCommonDialog_stub -lm -lSceGxm_stub -lSceDisplay_stub -lSceSysmodule_stub \
	-lzip -lz -lvitashark -lSceShaccCgExt -ltaihen_stub -lSceShaccCg_stub \
	-lSceKernelDmacMgr_stub -lSceCtrl_stub -ltoloader 

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
		-a bunny.obj=bunny.obj \
		-a cube.obj=cube.obj \
		-a skybox.frag=skybox.frag \
		-a skybox.vert=skybox.vert \
		-a skybox/back.jpg=skybox/back.jpg \
		-a skybox/bottom.jpg=skybox/bottom.jpg \
		-a skybox/front.jpg=skybox/front.jpg \
		-a skybox/left.jpg=skybox/left.jpg \
		-a skybox/right.jpg=skybox/right.jpg \
		-a skybox/top.jpg=skybox/top.jpg \
		-a mirror.vert=mirror.vert \
		-a mirror.frag=mirror.frag $@

eboot.bin: $(TARGET).velf
	vita-make-fself -s $< eboot.bin	
	
%.velf: %.elf
	vita-elf-create $< $@
	
$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@
	
clean:
	@rm -rf *.velf *.elf *.vpk $(OBJS) param.sfo eboot.bin
