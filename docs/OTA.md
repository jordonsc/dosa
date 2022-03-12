Over-The-Air Updates
====================
Some DOSA application support updating their firmware via OTA updates. OTA updates are achieved by:

* Storing the application binary on GCP Cloud Storage
* Storing a DOSA version number on Cloud Storage
* Transmitting a UDP packet asking devices to check for OTA updates
* OTA-aware devices will check then the version number, if it is different, it will download and apply the new binary.

To make an application OTA-aware, you should extend the `dosa::OtaApplication` class instead of the `dosa::App` class.

Uploading a new binary
----------------------
Use your GCP user credentials to authenticate with GCP -

    gcloud auth login
    
When you request the DOSA script to do an OTA upload, it will validate you have bucket access first.

    ./dosa ota (APPLICATION) (DOSA VERSION)
    
eg:

    # Deploy DOSA version 25 of the sonar application
    gcloud auth login
    ./dosa ota sonar 25 
    
