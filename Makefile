TARGET          := libvitaGL
SOURCES         := source
SHADERS         := shaders

LIBS = -lSceLibKernel_stub -lSceAppMgr_stub -lm -lSceAppUtil_stub \
	-lc -lSceCommonDialog_stub -lSceGxm_stub -lSceDisplay_stub -lSceSysmodule_stub \

CFILES   := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.c))
CGFILES  := $(foreach dir,$(SHADERS), $(wildcard $(dir)/*.cg))
OBJS     := $(addsuffix .o,$(CGFILES)) $(CFILES:.c=.o)

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CFLAGS  = -fno-lto -g -Wl,-q -O3 -ffat-lto-objects
ASFLAGS = $(CFLAGS)

all: $(TARGET).a

$(TARGET).a: $(OBJS)
	$(AR) -rc $@ $^

%_f.cg.o:
	psp2cgc -profile sce_fp_psp2 $(@:_f.cg.o=_f.cg)
	$(PREFIX)-objcopy -I binary -O elf32-littlearm -B arm $(@:_f.cg.o=_f_cg.gxp) $@
	@rm -rf $(@:_f.cg.o=_f_cg.gxp)
	
%_v.cg.o:
	psp2cgc -profile sce_vp_psp2 $(@:_v.cg.o=_v.cg)
	$(PREFIX)-objcopy -I binary -O elf32-littlearm -B arm $(@:_v.cg.o=_v_cg.gxp) $@
	@rm -rf $(@:_v.cg.o=_v_cg.gxp)
	
clean:
	@rm -rf $(TARGET).a $(TARGET).elf $(OBJS)
	@make -C samples/sample1 clean
	@make -C samples/sample2 clean
	@make -C samples/sample3 clean
	
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