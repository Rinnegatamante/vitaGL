TARGET          := libvitaGL
SOURCES         := source source/utils

CFILES   := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.c))
CPPFILES := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.cpp))
OBJS     := $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)

SAMPLES     := $(foreach dir,$(wildcard samples/*), $(dir).smp)
SAMPLES_CLR := $(foreach dir,$(wildcard samples/*), $(dir).smpc)

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CXX     = $(PREFIX)-g++
AR      = $(PREFIX)-gcc-ar
CFLAGS  = -g -Wl,-q -O3 -ffast-math -mtune=cortex-a9 -mfpu=neon -Wno-incompatible-pointer-types
ASFLAGS = $(CFLAGS)

ifeq ($(SOFTFP_ABI),1)
CFLAGS += -mfloat-abi=softfp -DHAVE_SOFTFP_ABI
endif

ifeq ($(NO_DEBUG),1)
CFLAGS += -DSKIP_ERROR_HANDLING
endif

ifeq ($(NO_TEX_COMBINER),1)
CFLAGS += -DDISABLE_TEXTURE_COMBINER
endif

ifeq ($(HAVE_SHARK_LOG),1)
CFLAGS += -DHAVE_SHARK_LOG
endif

ifeq ($(HAVE_CUSTOM_HEAP),1)
CFLAGS += -DHAVE_CUSTOM_HEAP
endif

ifeq ($(HAVE_UNFLIPPED_FBOS),1)
CFLAGS += -DHAVE_UNFLIPPED_FBOS
endif

ifeq ($(DRAW_SPEEDHACK),1)
CFLAGS += -DDRAW_SPEEDHACK
endif

ifeq ($(MATH_SPEEDHACK),1)
CFLAGS += -DMATH_SPEEDHACK
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

ifeq ($(SINGLE_THREADED_GC),1)
CFLAGS += -DHAVE_SINGLE_THREADED_GC
endif

ifeq ($(CIRCULAR_VERTEX_POOL),1)
CFLAGS += -DHAVE_CIRCULAR_VERTEX_POOL
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

ifeq ($(HAVE_DEBUGGER),2)
CFLAGS += -DHAVE_DEVKIT -DHAVE_RAZOR -DHAVE_RAZOR_INTERFACE -DHAVE_DEBUG_INTERFACE -DHAVE_LIGHT_RAZOR
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

ifeq ($(SAMPLERS_SPEEDHACK),1)
CFLAGS += -DSAMPLERS_SPEEDHACK
endif

ifeq ($(HAVE_HIGH_FFP_TEXUNITS),1)
CFLAGS += -DHAVE_HIGH_FFP_TEXUNITS
endif

ifeq ($(HAVE_DISPLAY_LISTS),1)
CFLAGS += -DHAVE_DLISTS
endif

ifeq ($(HAVE_PTHREAD),1)
CFLAGS += -DHAVE_PTHREAD
endif

CXXFLAGS  = $(CFLAGS) -fno-exceptions -std=gnu++11 -Wno-write-strings

all: $(TARGET).a

$(TARGET).a: $(OBJS)
	$(AR) -rc $@ $^
	
%.smpc:
	@make -C $(@:.smpc=) clean
	
%.smp:
	@make -C $(@:.smp=)
	ls -1 $(@:.smp=)/*.vpk | xargs -L1 -I{} cp {} .
	
clean: $(SAMPLES_CLR)
	@rm -rf $(TARGET).a $(TARGET).elf $(OBJS)
	
install: $(TARGET).a
	@mkdir -p $(VITASDK)/$(PREFIX)/lib/
	cp $(TARGET).a $(VITASDK)/$(PREFIX)/lib/
	@mkdir -p $(VITASDK)/$(PREFIX)/include/
	cp source/vitaGL.h $(VITASDK)/$(PREFIX)/include/
	
samples: $(SAMPLES)
