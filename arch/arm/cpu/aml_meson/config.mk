CROSS_COMPILE=arm-none-eabi-
ARM_CPU=cortex-a9
PLATFORM_CPPFLAGS += $(call cc-option,-mcpu=cortex-a9  -ffixed-r8 -mno-long-calls  -Wall -fPIC )
#USE_PRIVATE_LIBGCC=yes
ifeq ($(CONFIG_AML_MESON_8),y) #m8 or m8baby or m8m2
	TEXT_BASE=0x10000000
	UCL_TEXT_BASE=0x10800000
else #not m8 series
	TEXT_BASE=0x8F800000
	UCL_TEXT_BASE=0x8F000000
endif
#$(warning $(PLATFORM_CPPFLAGS))
