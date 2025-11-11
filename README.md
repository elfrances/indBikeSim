# Introduction

indBikeSim is an app that simulates a basic FTMS Indoor Bike device. It supports the following features:

* Uses Direct Connect (DIRCON) as the communication protocol with the virtual cycling app.
* Uses the Avahi Daemon to advertise the wahoo-fitness-tnpWFTNP service on the local network.
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

For example, if the desired serial number is 123456789 and the network interface on the host where the indBikeSim app is enp2s0, you can set the "serial-number" and "mac-address" TXT records as follows:

``` bash
sed -i s"/SN/123456789/" wahoo-fitness-tnp.service
sed -i s"/MAC/`cat /sys/class/net/enp2s0/address`/" wahoo-fitness-tnp.service
```

Then you can install the service definition file as follows:

``` bash
sudo cp wahoo-fitness-tnp.service /etc/avahi/services/
sudo systemctl restart avahi-daemon
```

To ensure that the new service is being advertised correctly on the local network, you can use the avahi-browse utility:

``` bash
avahi-browse -rt _wahoo-fitness-tnp._tcp
```

The output should look like this:

```
+ enp2s0 IPv6 indBikeSim on milou                           _wahoo-fitness-tnp._tcp local
+ enp2s0 IPv4 indBikeSim on milou                           _wahoo-fitness-tnp._tcp local
+     lo IPv4 indBikeSim on milou                           _wahoo-fitness-tnp._tcp local
= enp2s0 IPv6 indBikeSim on milou                           _wahoo-fitness-tnp._tcp local
   hostname = [milou.local]
   address = [fe80::3fb4:c8a9:101f:ed8a]
   port = [36866]
   txt = ["ble-service-uuids=0x1826" "mac-address=6c:4b:90:33:b3:f1" "serial-number=123456789"]
= enp2s0 IPv4 indBikeSim on milou                           _wahoo-fitness-tnp._tcp local
   hostname = [milou.local]
   address = [192.168.0.242]
   port = [36866]
   txt = ["ble-service-uuids=0x1826" "mac-address=6c:4b:90:33:b3:f1" "serial-number=123456789"]
=     lo IPv4 indBikeSim on milou                           _wahoo-fitness-tnp._tcp local
   hostname = [milou.local]
   address = [127.0.0.1]
   port = [36866]
   txt = ["ble-service-uuids=0x1826" "mac-address=6c:4b:90:33:b3:f1" "serial-number=123456789"]
```

Alternatively, you can use the Avahi Discovery app to check that the service is properly configured. In the example below the service was advertised by the Ubuntu laptop "ideapad" over its WiFi interface wlp3s0:

![Avahi Discovery app](./assets/Avahi-Discovery.png)
