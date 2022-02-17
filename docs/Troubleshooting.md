Troubleshooting
===============

USB Port Access
---------------
When uploading, I get error:

> TOUCH: error during reset: opening port at 1200bps: Permission denied

You need to be in the `dialout` user group: 

    sudo usermod -a -G dialout $USER
    
> You will require a logout/restart to take effect.


Compiling `monitor` fails with "no module 'serial'"
---------------------------------------------------
While the `monitor` application uses Arduino to compile an install, under the hood it's using Python 2.7 to do the 
work. You will require the `pyserial` module installed for Python 2.

> NB: Python 2.7 is now end-of-life, and Python2-pip is no longer included in Ubuntu repos.

For Ubuntu 20, consider:

    curl https://bootstrap.pypa.io/pip/2.7/get-pip.py --output get-pip.py
    sudo python2 get-pip.py
    python2 -m pip install pyserial
