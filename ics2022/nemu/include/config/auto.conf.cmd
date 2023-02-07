deps_config := \
	src/device/Kconfig \
	src/memory/Kconfig \
	/home/faii/Desktop/pa/ics2022/nemu/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
