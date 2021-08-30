Troubleshooting
===============

USB Port Access
---------------
When uploading, I get error:

> TOUCH: error during reset: opening port at 1200bps: Permission denied

You need to be in the `dialout` user group: 

    sudo usermod -a -G dialout $USER
    
> You will require a logout/restart to take effect.
