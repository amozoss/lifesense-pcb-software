lifesense-pcb-software
======================
Software to run on [lifesense-pcb](https://github.com/amozoss/lifesense-pcb)

### Installation 

The program depends on the libraries in the 0101E0012 release of Energia. It can be downloaded [here](http://energia.nu/download/).

In order to transmit data, make sure to update the `char ssid[]` and `char pass[]` with your access point information (the cc3000 does not connect to ad-hoc networks). 

Update `const String token` with the token created on [lifesense](https://github.com/amozoss/lifesense) website. 

Note: The [lifesense-node](https://github.com/amozoss/lifesense-node) server must be running in order for the transmitter to connect.
