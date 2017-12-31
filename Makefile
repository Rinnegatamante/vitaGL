TARGET		:= libvitaGL
SOURCES		:= source
			
INCLUDES	:= include

LIBS = -lvita2d -lSceLibKernel_stub -lScePvf_stub \
	-lSceAppMgr_stub -lm -lSceAppUtil_stub -lScePgf_stub \
	-ljpeg -lfreetype -lc -lSceCommonDialog_stub -lpng16 -lz \
	-lSceGxm_stub -lSceDisplay_stub -lSceSysmodule_stub \

CFILES   := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.c))
CPPFILES   := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.cpp))
BINFILES := $(foreach dir,$(DATA), $(wildcard $(dir)/*.bin))
OBJS     := $(addsuffix .o,$(BINFILES)) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o) 

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CXX      = $(PREFIX)-g++
CFLAGS  = -fno-lto -g -Wl,-q -O3
CXXFLAGS  = $(CFLAGS) -fno-exceptions -std=gnu++11 -fpermissive
ASFLAGS = $(CFLAGS)

all: $(TARGET).a

$(TARGET).a: $(OBJS)
	$(AR) -rc $@ $^

clean:
	@rm -rf $(TARGET).a $(TARGET).elf $(OBJS)
	make -C samples/sample1 clean
	make -C samples/sample2 clean
	make -C samples/sample3 clean
	
install: $(TARGET).a
	@mkdir -p $(VITASDK)/$(PREFIX)/lib/
	cp $(TARGET).a $(VITASDK)/$(PREFIX)/lib/
	@mkdir -p $(VITASDK)/$(PREFIX)/include/
	cp source/vitaGL.h $(VITASDK)/$(PREFIX)/include/
	
samples: $(TARGET).a
	make -C samples/sample1
	cp "samples/sample1/vitaGL-Sample001.vpk" .
	make -C samples/sample2
	cp "samples/sample2/vitaGL-Sample002.vpk" .
	make -C samples/sample3
	cp "samples/sample3/vitaGL-Sample003.vpk" .