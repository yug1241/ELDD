
obj-m = q1.o

modules:
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C /home/sunbeam/Desktop/linux/ M=`pwd` modules
	
copy:
	scp q1.ko debian@192.168.7.2:/home/debian/yug	
clean:
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C /home/sunbeam/Desktop/linux/ M=`pwd` clean		
	
	
	

.PHONY: modules clean

