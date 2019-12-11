sudo usermod -a -G disk portasynthinca3
sudo chmod o+rw /dev/sdc
VBoxManage internalcommands createrawvmdk -filename ./usb.vmdk -rawdisk /dev/sdc
