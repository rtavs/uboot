
include $(TOPDIR)/config.mk

all:
ifdef CONFIG_AML_SMP
	./bin2code smp_code $(SRCTREE)/$(CPUDIR)/$(SOC)/firmware/smp.bin $(SRCTREE)/$(CPUDIR)/$(SOC)/firmware/smp.dat
endif
	$(MAKE) -f Makefile.gcc
	$(MAKE) -f Makefile.ddr_init


ifeq ($(CONFIG_CMD_RUNARC),y)
	$(MAKE) -f Makefile.rbt
endif

