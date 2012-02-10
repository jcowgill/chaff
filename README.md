Chaff OS
========
This is some crazy OS project I've been working on, on and off for some time now.

This is a 32-bit Unix-like OS which some day may become POSIX compliant. Currently it has many of the core parts including a memory manager, scheduler, process and signal management and general interrupts and exceptions.

The main parts of the **kernel** still to implement are

* I/O system
* Loadable Drivers
* [ELF](http://en.wikipedia.org/wiki/Executable_and_Linkable_Format) Loader
* System Calls (most calls just call other parts of the kernel)

Compiling
---------
Compiling Chaff requires:

* unix environment
* gcc (for elf-i386)
* binutils (for elf-i386)
* make
* nasm
* doxygen (only needed to generate documentation)

For Linux, everything except nasm should already be installed on most distributions.

For Windows, you need a unix environment to compile the code in. If you've installed [msysgit](http://code.google.com/p/msysgit/) (which I highly reccomend you do), you can use the "Git Bash" provided. All the other files nessesary for Windows can be found in the windows_tools.zip download. Extract this somewhere and put the bin folder in your system path - you may also need to set the environment variable CPATH to the include directory.

Once you've done all this, go to the chaff source and type

    make

This should build the chaff image and place it in the file bin/chaff.elf

To build the documentation, you need to install doxygen and type

	doxygen
	
This should create the html documentation of all the chaff functions in doc/html 

Creating an Image
-----------------
Before debugging, you need to create a bootable image containing GRUB and the chaff kernel. To do this, download grub.img.zip from the downloads section. This is a floppy image containing a blank version of GRUB which can boot Chaff. You then need to mount the image as a volume.

On Linux, create a new directory to mount the image to and then type (as root):

    mount -t vfat -o loop grub.img <new directory>

On Windows, there are various tools to mount images. My favourite is ImDisk and the installer is included in the windows_tools file. Open ImDisk and mount the grub.img file as a floppy disk to get a new drive.

After mounting the image, copy the generated chaff.elf file to the root of the image.

Debugging
---------
Now that you have a bootable image, you can start debugging it. Currently there are no debugging tools in the kernel itself so it cannot be debugged on real hardware but it can be debugged using an emulator. You first need qemu (included in windows_tool) which contains a remote gdb server that allows you to debug the kernel from another program. When you are ready run qemu like this:

    qemu -s -fda <image>

The -s allows remote debugging. The image can either be the grub.img file or an actual drive (eg **A:** for Windows or **/dev/fd0** for Linux)

You also need a GUI which can interface with GDB - I generally use [Eclipse](http://www.eclipse.org/) (download the C version). If you are using Eclipse, open the Chaff project (use import projects to workspace if you don't have it) and create a new remote debug configuration. It should connect to a gdb server on port 1234 on localhost. If you are not using eclipse, you may have to specify the symbol file as the chaff.elf file.

Once connected, you should be able to debug the kernel and set breakpoints etc in Eclipse.
