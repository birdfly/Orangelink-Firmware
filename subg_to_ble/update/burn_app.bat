nrfjprog -f nrf52 --eraseall
nrfjprog -f nrf52  --program  "app.hex" --verify
nrfjprog -f nrf52 --reset