# OpenPLC_ID



## ESP32+
1. Ekstrak .zip file ke folder, misal C:\ESP32PlusBin.
2. Hubungkan ESP32 ke komputer dan pastikan mendapat portnya, misal COM6
3. Buka command prompt
4. Upload file binary ke ESP32 dengan melakukan perintah
```
esptool.exe --chip esp32 --port "COM6" --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x1000 "C:\ESP32PlusBin\ESP32_Plus_Slave.bootloader.bin" 0x8000 "C:\ESP32PlusBin\ESP32_Plus_Slave.partitions.bin" 0xe000 "C:\ESP32PlusBin\boot_app0.bin" 0x10000 "C:\ESP32PlusBin\ESP32_Plus_Slave.bin"
```

Untuk upload file SPIFF dapat dilakukan dengan perintah :
```
esptool.exe --chip esp32 --port "COM6" --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 3604480 "D:\ESP32PlusBin\ESP32_Plus_Slave.spiffs.bin"
```
