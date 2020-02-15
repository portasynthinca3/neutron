# Welcome!
![logo](https://github.com/portasynthinca3/neutron/blob/master/gfx/logo.png "logo")\
*Neutron logo*\
**So, you somehow got to this page.** I really appreciate it, thank you!
# What's the purpose?
Neutron is the result of me trying to get into the low-level stuff.
This project is **not** intended to do some crazy stuff like replacing the giants like Windows, Linux and macOS. That's just not possible. However, it doesn't mean i don't have an end goal. **The end goal is to write a simple 64-bit UEFI-powered OS that can run 3rd party applications and its GUI system, while requiring as little disk space, RAM and processor resources as possible.** Oh, and also, I just want to have fun :)
# Who is involved?
* Me, Andrey Antonenko, the creator, maintainer and programmer
* Sasha Kulichkov, booted this on his PC several times; created the logo
# Where does it work?
In theory, it should work on any modern system. Here are the minimal requirements:
* CPU: x86-64 architecture
* System: UEFI class 2 or 3
* RAM: 64 MB
# Show me the screenshots!
They are in the `screens` directory
# Okay, what do I need to be aware of?
Keep in mind that the system is unstable. I am **not** responsible for **any** kind of damage.
However, please tell me if you have found a bug, especially if it's dangerous enough to do any damage. That would be very nice!
# Building
1. You need to install the `x86_64-w64-mingw32-gcc` compiler. You can get it from the Arch User Repository or
some other place. I personally have used the\
`pamac build mingw-w64-gcc-base`\
command to do this.
2. You need to have python version 3 installed.
3. Run the\
`python3 builder.py`\
command inside the directory that contains the project. The ISO file will be inside the `build` directory.