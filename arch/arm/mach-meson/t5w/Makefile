obj-y	:= sdio.o
obj-y   += bl31_apis.o
obj-y	+= watchdog.o
obj-$(CONFIG_AML_HDMITX20) += hdmitx20/
obj-y	+= timer.o
obj-y	+= mailbox.o
obj-y	+= stick_mem.o
#obj-y   += gate_init.o
#obj-y   += power_cal.o
#obj-y   += power_domain.o
obj-y   += cpu_id.o
obj-y   += pwrc.o
obj-y	+= pinctrl_init.o

obj-$(CONFIG_CMD_AML_MTEST) += core.o
obj-y += usb.o
obj-$(CONFIG_CMD_HDMIRX) += hdmirx/
obj-$(CONFIG_AML_TSENSOR) += tsensor.o
