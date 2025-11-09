# indBikeSim

indBikeSim is an app that simulates a basic FTMS Indoor Bike over DIRCON.

```
sudo apt-get install avahi-utils avahi-discover
sudo cp wahoo-fitness-tnp.service /etc/avahi/services/
sudo systemctl restart avahi-daemon
avahi-browse -rt _wahoo-fitness-tnp._tcp
avahi-browse -rt _wahoo-fitness-tnp._tcp
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
