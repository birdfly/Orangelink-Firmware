nrfjprog -f nrf52 --eraseall
nrfjprog -f nrf52  --program  "burn_file_without_boot.hex" --verify
nrfjprog -f nrf52 --reset