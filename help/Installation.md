## Installation

You need to:
- Install the needed software in the PC
- Assemble and configure the usb2rf module (Connects to a USB port of the PC)
- Configure rftool to use the usb2rf module
- Upload the apropriate firmware to usb2rf

### Install the apropriate software
First of all, a linux PC is needed for the development. I don't use Windows neither MAC
and is very difficult for me to support another platform. The instructions below are valid
for Ubuntu 18.04 LTS (and Mint 19.X) and probably for Ubuntu 16.04 and the base debian release. Older releases (Ubuntu 14.04 LTS and Mint 17.X) may have a problem with "avrdude" but you can fix it by installing for example the [Ubuntu 16.04 avrdude](https://launchpad.net/ubuntu/+source/avrdude/6.2-5/+build/8794450/+files/avrdude_6.2-5_i386.deb). For other distributions you need to adapt the procedure for your environment.<br/>


Open a terminal:

```bash
# this is recommended before start
> sudo apt-get update ; sudo apt-get upgrade

# This also installs avr compiler and avr-libc
> sudo apt-get install arduino-core arduino-mk

# A programmers editor. You can use another if you like.
# The instructions here, use geany as text editor
> sudo apt-get install geany

# a serial terminal. I suggest to use this serial terminal
# before try your luck with another one.
> sudo apt-get install gtkterm

# To easily download rfboot.
> sudo apt-get install git
```

The ubuntu/debian/mint packages are old (1.0.6) but work perfectly for rfboot purposes. You
can edit the Makefile of the project and point to locally installed, newer Arduino releases.
You can do the same for newer Arduino-Makefile releases.
The auto-generated Makefile (see below) has already commented some possible options.

Now download the rfboot repository on your PC. Place it in some relatively safe place in your
home like ~/programming/

```bash
> mkdir ~/programming
> cd ~/programming
> git clone https://github.com/pkarsy/rfboot.git
```
The "rftool" utility needs to be in the PATH.
```bash
# if the ~/bin does not exist "mkdir ~/bin" and then logout and login again
> cd ~/bin
> chmod +x ~/programming/rfboot/rftool/rftool
# Do NOT copy rftool, just symlink it
> ln -s ~/programming/rfboot/rftool/rftool
```
Now if you type
```bash
> rftool
```
should give you a small usage message. This means rftool is in the PATH. As you can see the
rftool is pre-compiled. It is a statically linked executable, and it can run on any x86_64(AMD64) linux system. You can recompile it of course if you want, see [rftool README](../rftool/README.md)

Now it is time to install mCC1101, a modified (and simplified) panStamp
CC1101 library. The skeleton projects created with "rftool create ProjName"
use this library. (You can modify the code to use another CC1101 library)

Change "~/sketchbook/libraries" with your actual sketchbook folder.

```bash
> cd ~/sketchbook/libraries
> git clone https://github.com/pkarsy/mCC1101.git
```

Linux by default does not give permission (to regular users) to access the Serial ports, neither the ISP programmers. To change this for Serial port:

```bash
> sudo adduser myusername dialout
```
You need to logout and login again for the changes to take effect.<br>
For ISP programmer:
```bash
> sudo nano /etc/udev/rules.d/99-isp.rules
```

and add the lines
```sh
# USBasp
SUBSYSTEM=="usb", ATTR{product}=="USBasp", ATTR{idProduct}=="05dc", ATTRS{idVendor}=="16c0", MODE="0666"

# USBtiny
SUBSYSTEM=="usb", ATTR{idVendor}=="1781", ATTR{idProduct}=="0c9f", MODE="0666"
```

Now in the command line
```bash
> sudo service udev restart
```
You have to disconnect the module from the USB port (if connected), and reconnect for the changes to take effect.

***Now the Linux PC has all neccesary software components***
<br/>The steps above should be repeated on every PC you are going to use for development.

### usb2rf module

***WARNING: The red FTDI modules (USB-to-Serial, they probably have a fake FTDI chip) that can be found on many online shops, are very unreliable. A lot of failed uploads, and mysterious CRC errors, disappeared by using a CP2102 or Pl2303 USB-to-Serial module.<br/>
I don't have a genuine FTDI module to test it.<br>Maybe the problem is power related as the FTDI regulator powers the ProMini and the CC1101 module directly. This means that if we resist the temptation to connect FTDI<-->ProMini directly with the 6-pin headers, and connect the 5V FTDI output to the RAW proMini pin, then they may work reliably.<br/>
I didn't investigate further, as CP2102 is working perfectly.***

To build the usb2rf module you need :
- A CP2102 module ( A CP2104 or a PL2303 is also OK)
- ProMini 3.3V. Do not use a 5V ProMini. CC1101 cannot tolerate 5V.
- CC1101 module (I use D-SUN modules)
- some female-female jumper wires (2.54mm spacing).

```
+----------+
|          |         +--------+          +----------+         +--------+
| gtkterm  |  <--->  | CP2102 |  <--->   | Pro mini |  <--->  | CC1101 |
| rftool   |   USB   +--------+  Serial  +----------+   SPI   +--------+
+----------+
   PC                |                   usb2rf module                 |
                     +-------------------------------------------------+
```

Here is a photo of the materials we need
![usb2rf](files/usb2rf1.jpg)

***Serial connection (CP2102<-->ProMini)***

4 cables are required :

CP2102 | Cable COLOR | Pro Mini
---- | ----- | --------
GND  | Black |GND
5V   | Red | RAW
TX   | Yellow | RX
RX   | Green |TX

ProMini regulates the 5V from RAW pin to 3.3V. CP2102 has also an internal 3.3 regulator and the 2 modules
can communicate without the need for logic level conversion.<br/>
Notice: CP2102 has also a 3.3V output but I am not sure
that the current is enough to power ProMini+CC1101 so we don't use it. Instead we feed the far more capable ProMini's regulator (The RAW pin is the regulator input) with 5V as you can see in the connections table

***SPI connection (ProMini<-->CC1101)***

7 cables are required :

CC1101 PIN | Cable COLOR | ProMini pin
------------- | ----------- | -----------
GND | Black | GND
VCC | Red | VCC(3.3V)
CSN | Yellow | 10
SCK | Green | 13
MOSI | Blue | 11
MISO | Violet | 12
GDO0 | Gray | 2
GDO2 |  | Not Connected

CC1101 is working fine with 3.3V, so again no level conversion is needed.

Now connect the proMini pins D3 and RST (Orange cable).
See [Explanation](usb2rf-reset.md)

![usb2rf2](files/usb2rf2.jpg)

And here is the final module

![usb2rf2](files/usb2rf3.jpg)

### Assign the USB-to-Serial module, a unique serial ID
***Notice: I am not the developer of these utilities, all credit goes to them.***
<br/>
This is mandatory if you are going to have more than one CP2102
modules connected to your PC at the same time, even for completely different purposes.<br/>
https://github.com/DiUS/cp210x-cfg.git<br/>
compile the source (install libusb-1.0-0-dev first)

```sh
# Be sure there is only one CP2102 module connected to the PC
> sudo ./cp210x-cfg -S "random_serial"
# disconnect the module from USB port and connect again after a few seconds
```

- "random_serial" should be unique for every module ie 12F34, 25317, 1awd34fg etc.
- The CP2104 chip seems to have already unique ID, so this procedure is not needed and maybe in
fact do harm, as this chip is not tested by the developer.
- The Pl2303 chip cannot programmed, so if you are going to use this chip, you
have to use only one, and use other brands, if you need to connect other
USB-to-Serial modules.
- The FTDI modules also have unique serial IDs, if you manage to bypass the reliability problems as explained earlier.
- There is another linux utility doing the same job, and seems to have more options<br/>
http://cp210x-program.sourceforge.net/<br/>
but some distros might have difficulties to install the dependencies (python-usb).



### Configure rftool to use the usb2rf module
```bash
> rftool addport
```
and insert the usb2rf module. rftool detects it and saves the Serial port device
to "~/.usb2rf" file. Note that **if you omit this step, rftool will not be able to use this module, for code upload.**

Note: Sometimes the module is detected as "/dev/ttyUSB0". Another time as "/dev/ttyUSB1", and so
on. rftool instead searches the "/dev/serial/by-id" directory and always finds the correct usb2rf module.
You can have more than one usb2rf devices and rftool will use the first one referenced in "~/.usb2rf"

### Burn the appropriate firmware to usb2rf

The usb2rf module (The proMini) needs a firmware in order to do the job as a USB<-->RF bridge.
**This step will work only if you already done the "rftool addport" step above.** And of course it
needs to be done once. Ensure the module is the only one connected (if you happen to have more than one)
To upload the pre-compiled .hex
```bash
> cd usb2rf
> make sendHex
```
You may need to press proMini reset button as auto-reset does not work.

As always, you can build the sketch yourself. (See [usb2rf/README.md](../usb2rf/README.md) )

***Hardware and software setup is done !***

Continue with
[The First Project](The-First-Project.md)

