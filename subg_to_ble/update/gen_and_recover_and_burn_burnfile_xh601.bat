::generate settings page for current image: app.hex (NRF52 --no-backup)  (settings version: 2)
::merge app and softdevice
::merge app softdevice,bootloader and settings
::merge whole

nrfutil settings generate --family NRF52810 --application app.hex --application-version 3 --bootloader-version 2 --bl-settings-version 2 settings.hex
mergehex.exe  --merge  app.hex s112_nrf52_6.1.1_softdevice.hex --output sd_and_app.hex
mergehex.exe  --merge  sd_and_app.hex boot_xh601.hex --output sd_and_app_and_boot.hex 
mergehex.exe  --merge  sd_and_app_and_boot.hex settings.hex --output burn_file.hex 

::recover first before eraseall(if readback protection is enable, eraseall will failuire)
nrfjprog -f NRF52 --recover
nrfjprog -f NRF52 --eraseall
nrfjprog  -f NRF52 --program  "burn_file.hex" --verify
nrfjprog -f NRF52 --reset

del settings.hex
del sd_and_app.hex
del sd_and_app_and_boot.hex 
