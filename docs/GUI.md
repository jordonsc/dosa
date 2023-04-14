GUI for Power Grid
==================
Assumes a headless install of Raspbian 11+

See also:
 * [Kivy R-Pi Dependencies](https://kivy.org/doc/stable/installation/installation-rpi.html)
 * [Kivy Pip Installation](https://kivy.org/doc/stable/gettingstarted/installation.html#installation-canonical)
 * [Kivy Tutorials](https://kivy.org/doc/stable/tutorials/pong.html)

Install Pre-reqs
----------------
Run the script in `tools/setup-gui.sh` to install the pre-reqs and build SDL2 from source.

Backlight Control
-----------------
See:
 * [StackExchange Discussion](https://raspberrypi.stackexchange.com/questions/36774/backlight-control-for-official-raspberry-pi-7-inch-touchscreen)
 * [Python Code Control](https://github.com/linusg/rpi-backlight)

Short-and-sweet:

    sudo sh -c "echo 255 > /sys/class/backlight/10-0045/brightness"
    sudo sh -c "echo 50 > /sys/class/backlight/10-0045/brightness"
    sudo sh -c "echo 0 > /sys/class/backlight/10-0045/brightness"

UI Icons
--------
Icons should be transparent `.png` files, 85x85 px including a small margin.

The current icons have been recoloured from pure white:

* 30 deg hue
* 20% saturation
* -30% lightness

