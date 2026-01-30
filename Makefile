obj-m += system_freeze.o

all: modules monitor

modules:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

monitor: system_freeze_monitor.c
	gcc -o system_freeze_monitor system_freeze_monitor.c -lpthread

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f system_freeze_monitor
