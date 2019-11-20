# gosund-sp111-homekit
ESP-OPEN-RTOS firmware for Gosund SP111 outlet with Apple HomeKit support


This project uses the Apple HomeKit accessory server library [ESP-HomeKit](https://github.com/maximkulkin/esp-homekit) from [@MaximKulkin](https://github.com/maximkulkin) for [ESP-OPEN-RTOS](https://github.com/SuperHouse/esp-open-rtos). And it uses the OTA update system [Life-Cycle-Manager (LCM)](https://github.com/HomeACcessoryKid/life-cycle-manager) from [@HomeACessoryKid](https://github.com/HomeACcessoryKid).

Although already forbidden by the sources and subsequent licensing, it is not allowed to use or distribute this software for a commercial purpose.

## Instructions
### Hardware preparation
[Serial Connection](https://github.com/arendst/Tasmota/wiki/BlitzWolf-SHP6)

### SDK setup
[esp-homekit build instructions](https://github.com/maximkulkin/esp-homekit-demo/wiki/Build-instructions-ESP8266)  


### Clone esp-open-rtos SDK
```bash
git clone --recursive https://github.com/SuperHouse/esp-open-rtos.git
```

### Setup environment variables
```bash
export SDK_PATH=`pwd`/esp-open-rtos;
export ESPPORT=/dev/cu.usbserial-1410;
export FLASH_SIZE=8;
export HOMEKIT_SPI_FLASH_BASE_ADDR=0x7a000;
export HOMEKIT_DEBUG=1;
```

### Build firmware
```bash
docker-run esp-rtos make -C . ESPPORT=/dev/cu.usbserial-1410 FLASH_SIZE=8 HOMEKIT_SPI_FLASH_BASE_ADDR=0x8c000 HOMEKIT_DEBUG=1 clean
docker-run esp-rtos make -C . ESPPORT=/dev/cu.usbserial-1410 FLASH_SIZE=8 HOMEKIT_SPI_FLASH_BASE_ADDR=0x8c000 HOMEKIT_DEBUG=1 all
```

### Flash firmware
Erase flash:  
```bash
esptool.py -p /dev/cu.usbserial-1410 --baud 115200 erase_flash;
```

Flash firmware:  
```bash
esptool.py -p /dev/cu.usbserial-A50285BI --baud 115200 write_flash -fs 8m -fm dout -ff 40m \
        0x0 firmware_prebuilt/rboot.bin 0x1000 firmware_prebuilt/blank_config.bin 0x2000 ./firmware/gosund_sp111.bin
```

### Debug
```bash
screen /dev/cu.usbserial-1410 115200 –L
```
