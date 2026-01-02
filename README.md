# ATC1441 pixlAnalyzer

Simple 2.4GHz Spectrum Analyzer based on the nRF52832 in the PixlJs allmiibo emulator

This Firmware is currently compatible with the LCD and OLED variant you can get on Aliexpress for around 10-15€ it will make it a very simple battery powered 2400 - 2487MHz Spectrum Analyzer with a Waterfall like history show with little options.

![Image](AnalyzerDemo.jpg)


This repo is made together with this explanation video:(click on it)

[![YoutubeVideo](https://img.youtube.com/vi/kgrsfGIeL9w/0.jpg)](https://www.youtube.com/watch?v=kgrsfGIeL9w)

Find them on Aliexpress as example here:

https://aliexpress.com/item/1005008726926205.html


## Version history

2025-12-06: first release

2026-01-xx (WIP):

* button debouncing
* LCD contrast adjustment made possible via menu
* long press (5 seconds) to enter sleep mode

## Flashing

You can flash this firmware fully OTA and go back to the stock Pixl.js firmware as well.
Navigate to the Pixl.js firmware settings and enter the "Firmware Update" menu
The device will reboot and show "DFU Update" now use the nRFConnect App to connect to the "Pixl DFU" device showing.
Select the correct firmware update file "PixlAnalyzerLCD.zip" or "PixlAnalyzerOLED.zip" depending on your device and it will flash and reboot to the new Firmware.

To Go back to the Pixl.js firmware you can open the menu by pressing the middle button, then navigate to "DFU" and flash the Stock firmware in the same way again.

## Compiling

Its made to be used with Windows right now some changes will be needed to make it Linux compatible.
You need to have installed:

* `make`
* `gcc-arm-none-eabi`
  * set the path to the gcc's bin folder in the `/pixlAnalyzer/firmware/sdk/components/toolchain/gcc/Makefile.posix` or ...`Makefile.windows` file, depending on your OS.
  * example for MacOS: `GNU_INSTALL_ROOT ?= /Applications/ArmGNUToolchain/14.2.rel1/arm-none-eabi/bin/`
* `nrfutil`
  * unless you have a very old version (that is, 6.x or lower), you will also need to run `nrfutil install nrf5sdk-tools` after installation of nrfutil.

## Credits

Credit goes to this repo for the codebase and pinout etc.:

https://github.com/solosky/pixl.js/

## Future Work / Ideas

See [ideas](ideas.md) for more details.
