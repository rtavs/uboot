

obj-y			+= timer.o
obj-y			+= common.o

ifdef CONFIG_SPL_BUILD
obj-y += board-spl.o
else
obj-y += board.o
endif

obj-$(CONFIG_MACH_M8B)	+= clock_m8b.o
