nrfjprog -f NRF52 --eraseall
nrfjprog  -f NRF52 --program  "burn_file.hex" --verify
nrfjprog -f NRF52 --reset