# LED Controller

## Description
Pi Pico2 W based LED strip controller for addressable LEDs.

## Features
- LED Controller
- WiFi
    - Web Interface
    - MQTT
    - Control from other pluto applications
      -  e.g. sprinkler controller indicates irrigation status while in operation
    - OTA software upgrades  

## Installation of tools on Ubuntu Linux
```
sudo apt install git build-essential cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
```
## Clone and build the code
```
git clone --recurse-submodules https://github.com/NewmanIsTheStar/led-controller.git 
cd led-controller
mkdir build
cd build
cmake ..
make
```
Upon completion of a successful build the file led-controller.uf2 should be created.  This may be loaded onto the Pico2 W in the usual manner: by dragging and dropping the file onto the Pi Pcio2 W mass storage device.

## Initial Configuration
- The Pico will initially create a WiFi network called **pluto**.  Connect to this WiFi network and then point your web browser to http://192.168.4.1
  - Note that many web browsers automatically change the URL from http:// to https:// so if it is not connecting you might need to reenter the URL.
- Set the WiFi country, network and password then hit save and reboot.  The Pico will attempt to connect to the WiFi network.  If it fails then it will fall back to AP mode and you can once again connect to the pluto network and correct your mistakes.  
- Refer to the [Wiki](https://github.com/NewmanIsTheStar/led-controller/wiki) for more details
