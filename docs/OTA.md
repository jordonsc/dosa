Over-The-Air Updates
====================
Some DOSA application support updating their firmware via OTA updates. OTA updates are achieved by:

* Storing the application binary on GCP Cloud Storage
* Storing a DOSA version number on Cloud Storage
* Transmitting a UDP packet asking devices to check for OTA updates
* OTA-aware devices will check then the version number, if it is different, it will download and apply the new binary.

To make an application OTA-aware, you should extend the `dosa::OtaApplication` class instead of the `dosa::App` class.

DOSA Version
------------
The DOSA version is compiled into the binary using a define. This is found in `lib/common/src/const.h` and should be
updated with every major DOSA release. It is particularly important this value is updated when deploying new OTA assets
else the devices will not detect a new OTA package.

The DOSA version can be overridden with an OS-level define:

    export DOSA_VERSION=123

New builds, the CLI and the OTA version used will all now use version '123'. 

Uploading a new binary
----------------------
Use your GCP user credentials to authenticate with GCP -

    gcloud auth login
    
When you request the DOSA script to do an OTA upload, it will validate you have bucket access first.

    ./dosa ota (APPLICATION)
    
eg:

    # Deploy the current version of DOSA 'sonar' application
    gcloud auth login
    ./dosa ota sonar
    
    # Override the DOSA version, compile and deploy 'sonar' OTA assets
    export DOSA_VERSION=123
    ./dosa ota sonar
    