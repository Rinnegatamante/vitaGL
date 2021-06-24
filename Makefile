TARGET          := libvitaGL
SOURCES         := source source/utils
SHADERS         := shaders

CFILES   := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.c))
CPPFILES := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.cpp))
CGFILES  := $(foreach dir,$(SHADERS), $(wildcard $(dir)/*.cg))
HEADERS  := $(CGFILES:.cg=.h)
OBJS     := $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CXX     = $(PREFIX)-g++
AR      = $(PREFIX)-gcc-ar
CFLAGS  = -g -Wl,-q -O3 -ffast-math -mtune=cortex-a9 -mfpu=neon
ASFLAGS = $(CFLAGS)

ifeq ($(SOFTFP_ABI),1)
CFLAGS += -mfloat-abi=softfp -DHAVE_SOFTFP_ABI
endif

ifeq ($(NO_DEBUG),1)
CFLAGS += -DSKIP_ERROR_HANDLING
endif

ifeq ($(HAVE_SHARK_LOG),1)
CFLAGS += -DHAVE_SHARK_LOG
endif

ifeq ($(HAVE_UNFLIPPED_FBOS),1)
CFLAGS += -DHAVE_UNFLIPPED_FBOS
endif

ifeq ($(DRAW_SPEEDHACK),1)
CFLAGS += -DDRAW_SPEEDHACK
endif

ifeq ($(SHADER_COMPILER_SPEEDHACK),1)
CFLAGS += -DSHADER_COMPILER_SPEEDHACK
endif

ifeq ($(SHARED_RENDERTARGETS),1)
CFLAGS += -DHAVE_SHARED_RENDERTARGETS
endif

ifeq ($(UNPURE_TEXTURES),1)
CFLAGS += -DHAVE_UNPURE_TEXTURES
endif

ifeq ($(PHYCONT_ON_DEMAND),1)
CFLAGS += -DPHYCONT_ON_DEMAND
endif

ifeq ($(CIRCULAR_VERTEX_POOL),1)
CFLAGS += -DHAVE_CIRCULAR_VERTEX_POOL
endif

ifeq ($(SAMPLER_UNIFORMS),1)
CFLAGS += -DHAVE_SAMPLERS_AS_UNIFORMS
endif

ifeq ($(LOG_ERRORS),1)
CFLAGS += -DLOG_ERRORS
endif

ifeq ($(LOG_ERRORS),2)
CFLAGS += -DLOG_ERRORS -DFILE_LOG
endif

ifeq ($(HAVE_DEBUGGER),1)
CFLAGS += -DHAVE_DEBUG_INTERFACE
endif

ifeq ($(HAVE_RAZOR),1)
CFLAGS += -DHAVE_RAZOR
endif

ifeq ($(HAVE_RAZOR),2)
CFLAGS += -DHAVE_RAZOR -DHAVE_RAZOR_INTERFACE -DHAVE_DEBUG_INTERFACE
endif

ifeq ($(HAVE_DEVKIT),1)
CFLAGS += -DHAVE_DEVKIT -DHAVE_RAZOR
endif

ifeq ($(HAVE_DEVKIT),2)
CFLAGS += -DHAVE_DEVKIT -DHAVE_RAZOR -DHAVE_RAZOR_INTERFACE -DHAVE_DEBUG_INTERFACE
endif

CXXFLAGS  = $(CFLAGS) -fno-exceptions -std=gnu++11 -Wno-write-strings

all: $(TARGET).a

$(TARGET).a: $(OBJS)
	$(AR) -rc $@ $^

%_f.h:
	psp2cgc -profile sce_fp_psp2 $(@:_f.h=_f.cg) -Wperf -o $(@:_f.h=_f.gxp)
	bin2c $(@:_f.h=_f.gxp) source/shaders/$(notdir $(@)) $(notdir $(@:_f.h=_f))
	@rm -rf $(@:_f.h=_f.gxp)
	
%_v.h:
	psp2cgc -profile sce_vp_psp2 $(@:_v.h=_v.cg) -Wperf -o $(@:_v.h=_v.gxp)
	bin2c $(@:_v.h=_v.gxp) source/shaders/$(notdir $(@:_v.h=_v.h)) $(notdir $(@:_v.h=_v))
	@rm -rf $(@:_v.h=_v.gxp)

shaders: $(HEADERS)
	
clean:
	@rm -rf $(TARGET).a $(TARGET).elf $(OBJS)
	@make -C samples/sample1 clean
	@make -C samples/sample2 clean
	@make -C samples/sample3 clean
	@make -C samples/sample4 clean
	@make -C samples/sample5 clean
	@make -C samples/sample6 clean
	@make -C samples/sample7 clean
	@make -C samples/sample8 clean
	@make -C samples/sample9 clean
	@make -C samples/sample10 clean
	
install: $(TARGET).a
	@mkdir -p $(VITASDK)/$(PREFIX)/lib/
	cp $(TARGET).a $(VITASDK)/$(PREFIX)/lib/
	@mkdir -p $(VITASDK)/$(PREFIX)/include/
	cp source/vitaGL.h $(VITASDK)/$(PREFIX)/include/
	
samples: $(TARGET).a
	@make -C samples/sample1
	cp "samples/sample1/vitaGL-Sample001.vpk" .
	@make -C samples/sample2
	cp "samples/sample2/vitaGL-Sample002.vpk" .
	@make -C samples/sample3
	cp "samples/sample3/vitaGL-Sample003.vpk" .
	@make -C samples/sample4
	cp "samples/sample4/vitaGL-Sample004.vpk" .
	@make -C samples/sample5
	cp "samples/sample5/vitaGL-Sample005.vpk" .
	@make -C samples/sample6
	cp "samples/sample6/vitaGL-Sample006.vpk" .
	@make -C samples/sample7
	cp "samples/sample7/vitaGL-Sample007.vpk" .
	@make -C samples/sample8
	cp "samples/sample8/vitaGL-Sample008.vpk" .
	@make -C samples/sample9
	cp "samples/sample9/vitaGL-Sample009.vpk" .
	@make -C samples/sample10
	cp "samples/sample10/vitaGL-Sample010.vpk" .
