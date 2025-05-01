make clean && make
sudo rmmod mt7902_skeleton                    # önceki sürüm
sudo insmod mt7902_skeleton.ko
dmesg -t | grep mt7902_skeleton

