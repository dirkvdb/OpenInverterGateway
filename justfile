build-home:
    -FIRMWARE_BUILD_FLAGS="-DFIRMWARE_HOME" platformio run --environment ShineWifiX
    cp .pio/build/ShineWifiX/firmware.bin growfw-home.bin

build-garage:
    -FIRMWARE_BUILD_FLAGS="-DFIRMWARE_GARAGE" platformio run --environment ShineWifiX
    cp .pio/build/ShineWifiX/firmware.bin growfw-garage.bin

clean:
    -platformio run -t clean
