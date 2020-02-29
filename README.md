# Welcome
![logo](https://github.com/portasynthinca3/neutron/blob/master/gfx/logo.png "logo")\
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/3db2b0cbdd72413a944b0a5004dc3dd8)](https://app.codacy.com/manual/portasynthinca3/neutron?utm_source=github.com&utm_medium=referral&utm_content=portasynthinca3/neutron&utm_campaign=Badge_Grade_Dashboard)
# The purpose of the project
Neutron is the result of me trying to get into the low-level stuff.
This project is **not** intended to do some crazy stuff like replacing the giants like Windows, Linux and macOS. That's just not possible. However, it doesn't mean i don't have an end goal. **The end goal is to write a simple 64-bit UEFI-powered OS that can run 3rd party applications and its GUI system, while requiring as little disk space, RAM and processor resources as possible.** Oh, and also, I just want to have fun :)
# The list of people involved in this project
*  Me, Andrey Antonenko, the creator, maintainer and programmer
*  Sasha Kulichkov, graphical designer and tester
# Requirements
In theory, it should work on any modern system. Here they are:
*  CPU: x86-64 architecture
*  System: UEFI class 2 or 3
*  RAM: 64 MB
# Screenshots
They are in the `screens` directory
# Building
1.  You need to install the `x86_64-w64-mingw32-gcc` compiler. You can get it from the Arch User Repository or
some other place. Example:\
`$ pamac build mingw-w64-gcc-base`
2.  You need to have python version 3 installed.
3.  Run the\
`$ python3 builder.py`\
command inside the directory that contains the project. The ISO file will be inside the `build` directory.