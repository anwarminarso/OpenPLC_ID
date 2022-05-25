# OpenPLC
Repository ini adalah extension/add-on untuk *[OpenPLC V3](https://github.com/thiagoralves/OpenPLC_v3)* dan berikut aplikasi supportnya *[OpenPLC Editor](https://github.com/thiagoralves/OpenPLC_Editor)*


## ESP32+ OpenPLC
Software/Firmware ini adalah PLC untuk board ESP32 dengan OpenPLC compatible, dan memiliki fitur tambahan:
1. Embedded Web GUI (dengan Authrorization)
2. Soft Start/Stop PLC
3. Live Monitoring (Digital Input, Digital Output, Analog Input, Analog Output) 
4. WiFi Configuration (Station Mode (Enable/Disable) atau Access Point Mode)
5. Modbus Configuration (Modbus RTU dengan Serial2 atau Bluetooth, Modbus TCP, Enable/Disable)
6. MQTT Configuration (Enable/disable)
7. OTA (over-the-air), Update software/firmware melalui WebGUI
8. Security (Change Password, Username)
9. Reset Config
10. Reboot system


`ESP32+ menggunakan custom partisi dengan format (1.76MB Application, 1.76MB OTA dan 460KB SPIFF), maka sebelum menggunakan upload melalui OpenPLC Editor sebaiknya flashing dahulu ESP32nya dengan ESP32+ OpenPLC Slave, agar partisi nya sesuai`

### Cara Upload
1. Ekstrak .zip file ke folder, misal C:\ESP32PlusBin.
2. Hubungkan ESP32 ke komputer dan pastikan mendapat portnya, misal COM6
3. Buka command prompt
4. Upload file binary ke ESP32 dengan melakukan perintah
```
esptool.exe --chip esp32 --port "COM6" --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x1000 "C:\ESP32PlusBin\ESP32_Plus_Slave.bootloader.bin" 0x8000 "C:\ESP32PlusBin\ESP32_Plus_Slave.partitions.bin" 0xe000 "C:\ESP32PlusBin\boot_app0.bin" 0x10000 "C:\ESP32PlusBin\ESP32_Plus_Slave.bin"
```

Untuk upload file SPIFF dapat dilakukan dengan perintah :
```
esptool.exe --chip esp32 --port "COM6" --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 3604480 "C:\ESP32PlusBin\ESP32_Plus_Slave.spiffs.bin"
```

### MQTT
Direction dari sisi ESP32
| Function                  | Direction   | Topic                | Sample Message                        | Description                                             |
| ------------------------- | ----------- | -------------------- | ------------------------------ |-------------------------------------------------------- |
| Telemetry                 | Publish     | {ROOT_TOPIC}/tele    | `{"DiscreteInputs":[0,0,0,0,0,0,0],"InputRegisters":[4544,4096,8512,9216],"Coils":[0,0,0,0,0,0,0],"HoldingRegisters":[0,0]}` |
| Result                    | Publish     | {ROOT_TOPIC}/result  |                                | ESP32 akan publish topic ini, jika ada perintah command |
| Write Coil                | Subscribe   | {ROOT_TOPIC}/command | `{ cmd: 5, reg: 0, val: 0 }`     | cmd 5 adalah fix, reg = coil register, val adalah value (0 s/d 1) |
| Write Single Register     | Subscribe   | {ROOT_TOPIC}/command | `{ cmd: 6, reg: 0, val: 12345 }` | cmd 6 adalah fix, reg = holding register, val adalah value (0 s/d 65535) |
| Write Multiple Registers  | Subscribe   | {ROOT_TOPIC}/command | `{ cmd: 16, regs: [ 0, 1, 2 ], vals: [ 1111, 2222, 3333 ] }` | cmd 16 adalah fix, regs adalah array  register dan value adalah array value, panjang array register dan value harus sama |


### Fungsi esptool untuk maintenance:
1. Erase Flash
```
esptool.exe --chip esp32 --port COM6 erase_flash
```
2. Jika terjadi error ketika flashing dan ESP32 tidak dapat diflash ulang, error message seperti : `A fatal error occurred: MD5 of file does not match data in flash!`
```
esptool.exe --port COM6 write_flash_status --non-volatile 0
```
