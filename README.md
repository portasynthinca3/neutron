# Welcome!
![logo](https://github.com/portasynthinca3/neutron/blob/master/gfx/logo_cut.png "logo")\
*Neutron logo*\
**So, you somehow got to this page.** I really appreciate it, thank you!
# What's the purpose?
Neutron is the result of me trying to get into the low-level stuff. That means, writing in assembly. This is quite a new
experience for me as the lowest i've ever got was writing code in C++ for the AVR micro series.\
This project is **not** intended to do some crazy stuff like replacing the giants like Windows, Linux and macOS. No-no, that's just not possible. However, it doesn't mean i don't have an end goal. **The end goal is to write a simple OS that can run in 32-bit protected mode, complete with GUI and 3rd party applications, while requiring as little disk space, RAM and processor resources as possible**
# Who is involved?
* Me, Andrey Antonenko, the creator, maintainer and programmer
* Sasha Kulichkov, booted this on his PC several times; created the logo
# List of things that are done
* Booting
* Loading second stage loader
* Loading kernel
* Hi-res 24bpp kernel graphics
# List of things that are in development right now
* USB (EHCI) support
* GUI
# What parts does this consist of?
1. First stage loader, called Muon-1
2. Second stage loader, called Muon-2
3. The monolithic kernel. So monolithic in fact, that all the system processes, even the graphics, are integrated. It's called Quark.
4. The font converter. Written in C#
5. The custom builder. Written in Python.
# Where does it work?
In theory, it should work on any modern system. Here are the minimal requirements:
* CPU: Pentium I or later
* RAM: 8 MB
* Buses: PCI, ATA
* An internal empty hard drive to boot from. You may be able to boot from external USB sticks, disk drives, floppies, etc., but
as the kernel only has ATA support right now, it woudn't be able to read any data from any drives except ATA drives or SATA
drives in ATA compatibility mode (can almost certainly be enabled in the BIOS)
* Legacy BIOS system or an UEFI system with legacy emulation mode
* A video card supporting VESA 1.0 standard (integrated CPU graphics should also work)\
\
Also, there are some detected problems:
* Some very modern systems only have UEFI support, totally eliminating the legacy BIOS standard. Neutron can't be booted on those systems.
# Show me the screenshots!
Okay, there you go!\
![boot1](https://github.com/portasynthinca3/neutron/blob/master/screens/boot_1.png "boot1")\
*Boot screen 1*\
![boot2](https://github.com/portasynthinca3/neutron/blob/master/screens/boot_2.png "boot2")\
*Boot screen 2*\
![desktop](https://github.com/portasynthinca3/neutron/blob/master/screens/desktop.png "desktop")\
*Desktop*
# Okay, what do I need to be aware of?
Keep in mind that the system is unstable. I am **not** responsible for **any** kind of damage.
However, please tell me if you have found a bug, especially if it's dangerous enough to do any damage. That would be very nice!
# Can I join the team?
Sorry, I don't think so. I want to do this project entirely on my own, at least yet.
