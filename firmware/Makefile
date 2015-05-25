
include $(TOPDIR)/config.mk

$(warning $(PLATFORM_CPPFLAGS))

PLATFORM_CPPFLAGS+=-DCONFIG_AMLROM_SPL

fwobj	:= $(OBJTREE)/firmware/

LDFLAGS	= -Bstatic -T $(fwobj)spl.lds  $(PLATFORM_LDFLAGS)


CFLAGS	:=$(CFLAGS:-marm=-mthumb)
CFLAGS	:=$(CFLAGS:-fPIC=)
CFLAGS	:=$(CFLAGS:-ffixed-r8=)
CFLAGS	+=  -DCONFIG_AML_ROMBOOT_SPL  -MD -Wno-unused-function -mno-long-calls
AFLAGS	+= -DCONFIG_AMLROM_SPL -DTEXT_BASE=$(TEXT_BASE) -MD


SOBJS	 = start.o magic.o

COBJS	 = spl.o

OBJS	:= $(addprefix $(obj),$(SOBJS) $(COBJS))
__OBJS	:= $(SOBJS) $(COBJS)
LNDIR	:= $(fwobj)

CFLAGS += -I$(SRCTREE)/firmware

origin_LDS=$(abspath firmware.lds)
target_LDS=$(abspath $(obj)spl.lds)

$(warning origin_LDS=$(origin_LDS))
$(warning target_LDS=$(target_LDS))
$(warning CFLAGS=$(CFLAGS))




ALL	= $(fwobj)firmware.out $(fwobj)firmware.bin

all:	$(obj).depend $(ALL)

$(fwobj)firmware.bin :	$(fwobj)firmware.out
	$(OBJCOPY) ${OBJCFLAGS} -O binary $< $@

$(fwobj)firmware.out: $(OBJS) $(obj)spl.o $(obj)eabi_compat.o $(target_LDS)
	cd $(LNDIR) && $(LD) $(LDFLAGS) $(filter %.o,$^) $(PLATFORM_LIBGCC) \
		-Map $(@:%=%.map) --cref -o $@

$(target_LDS):	$(origin_LDS)
	$(CPP) $(CPPFLAGS)   $(LDPPFLAGS) -std=c99 -D__ASSEMBLY__ -P - <$^ >$@


#########################################################################

$(obj)%.o:	$(obj)%.S
	$(CC) $(AFLAGS) -c -o $@ $<

$(obj)%.o:	$(obj)$.c
	$(CC) $(CFLAGS) -c -o $@ $<

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend



#########################################################################
