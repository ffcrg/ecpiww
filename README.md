ecpiww
======

A wireless EnergyCam setup with a Raspberry Pi to monitor the energy usage

FAST EnergyCam: The quick and inexpensive way to turn your conventional meter into a smart metering device 
(http://www.fastforward.ag/eng/index_eng.html)


Hardware:
  - Raspberry Pi
  - FAST EnergyCam RF and FAST USB Communication Interface to configure the Energycam prior to installation
	(available at http://umwelt-messtechnik.de/index.php?cat=c91_Smart-Grid-Komponenten-Smart-Grid-Komponenten.html)
  - wireless M-Bus USB Stick (2 manufacturers are supported)
  	- IMST IM871A-USB Stick ( available at http://www.tekmodul.de/index.php?id=shop-wireless_m-bus_oms_module or http://webshop.imst.de/funkmodule/im871a-usb-wireless-mbus-usb-adapter-868-mhz.html)
  	- AMBER Wireless M-Bus USB Adapter (http://amber-wireless.de/406-1-AMB8465-M.html)
  	
  - EDIMAX EW-7811UN Wireless USB Adapter


Software:
  - dygraph (https://github.com/danvk/dygraphs) 

Features:
 - The application shows you all received wireless M-Bus packages. 
 - You can add meters that are watched. The received values of these are written into csv files and presented on the webserver.
 - The energy usage is shown as an interactive, zoomable chart of time on a website.
 - install.txt describes how to configure the raspberry and compile the sources


Trademarks

Raspberry Pi and the Raspberry Pi logo are registered trademarks of the Raspberry Pi Foundation (http://www.raspberrypi.org/)

 



