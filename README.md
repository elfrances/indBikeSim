# Introduction

indBikeSim is an app that simulates a basic FTMS Indoor Bike device. It supports the following features:

* Uses Direct Connect (DIRCON) as the communication protocol with the virtual cycling app.
* Uses the Avahi Daemon to advertise the WFTNP service on the local network.
* The FTMS Indoor Bike Data notifications include cadence, heart rate, power, and speed.
* The cadence, heart rate, power, and speed values can be specified via command-line arguments or obtained from the trackpoints in a FIT file.

# Building the app

The app is written in C and can be built simply by running make:

``` bash
make
```

# Installing the app

indBikeSim uses the Avahi Daemon to advertise the WFTNP service on the local network. That's how a DIRCON-compatible virtual cycling app (e.g. FulGaz, Zwift) can discover and connect to it.

The supplied file wahoo-fitness-tnp.service is a template to be used by the Avahi Daemon to advertise the WFTNP service.  The service definition includes a TXT record that specifies the Serial Number of the indoor bike device, and the MAC address of its network interface.  You need to set the dummy SN and MAC values in the template file to their correct values, before copying the file to the Avahi system folder.

For example, if the desired serial number is 123456789 and the network interface is enp2s0, you can set the "serial-number" and "mac-address" TXT records as follows:

``` bash
sed -i s"/SN/123456789/" wahoo-fitness-tnp.service
sed -i s"/MAC/`cat /sys/class/net/enp2s0/address`/" wahoo-fitness-tnp.service
```

Then you can install the service definition file as follows:

``` bash
sudo cp wahoo-fitness-tnp.service /etc/avahi/services/
sudo systemctl restart avahi-daemon
```

To ensure the new service is being advertised on the local network, you can use the avahi-browse utility:

``` bash
sudo apt-get install avahi-utils avahi-discover
avahi-browse -rt _wahoo-fitness-tnp._tcp
```

The output should look like this:

```
+ wlp3s0 IPv6 Wahoo Fitness DIRCON Device on ideapad        _wahoo-fitness-tnp._tcp local
+ wlp3s0 IPv4 Wahoo Fitness DIRCON Device on ideapad        _wahoo-fitness-tnp._tcp local
+     lo IPv4 Wahoo Fitness DIRCON Device on ideapad        _wahoo-fitness-tnp._tcp local
= wlp3s0 IPv6 Wahoo Fitness DIRCON Device on ideapad        _wahoo-fitness-tnp._tcp local
   hostname = [ideapad.local]
   address = [fd14:d8de:554a:1:5246:8c89:8e0e:295b]
   port = [36866]
   txt = ["ble-services-uuids=0x1826" "mac-address=00-11-22-33-44-55" "serial-number=123456789"]
= wlp3s0 IPv4 Wahoo Fitness DIRCON Device on ideapad        _wahoo-fitness-tnp._tcp local
   hostname = [ideapad.local]
   address = [192.168.4.47]
   port = [36866]
   txt = ["ble-services-uuids=0x1826" "mac-address=00-11-22-33-44-55" "serial-number=123456789"]
=     lo IPv4 Wahoo Fitness DIRCON Device on ideapad        _wahoo-fitness-tnp._tcp local
   hostname = [ideapad.local]
   address = [127.0.0.1]
   port = [36866]
   txt = ["ble-services-uuids=0x1826" "mac-address=00-11-22-33-44-55" "serial-number=123456789"]
```
