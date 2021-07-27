/**
 *@file rf69.c
 *@author Ribin Huang (you@domain.com)
 *@brief 
 *@version 1.0
 *@date 2021-01-05
 *
 *Copyright (c) 2019 - 2020 Fractal Auto Technology Co.,Ltd.
 *All right reserved.
 *
 *This program is free software; you can redistribute it and/or modify
 *it under the terms of the GNU General Public License version 2 as
 *published by the Free Software Foundation.
 *
 */
#include <string.h>
 
#include "rf69.h"
#include "rf69_regisers.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "nrf_drv_spi.h"

#define RF69_FSTEP 	61.03515625

#define TAG	"RFM"

static const nrf_drv_spi_t spiInst = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);
static nrf_drv_spi_config_t spiCfg = 
{                                                            \
    .sck_pin      = SPI_SCLK_PIN,                            \
    .mosi_pin     = SPI_MOSI_PIN,                            \
    .miso_pin     = SPI_MISO_PIN,                            \
    .ss_pin       = NRF_DRV_SPI_PIN_NOT_USED,                \
    .irq_priority = SPI_DEFAULT_CONFIG_IRQ_PRIORITY,         \
    .orc          = 0xFF,                                    \
    .frequency    = NRF_DRV_SPI_FREQ_4M,                     \
    .mode         = NRF_DRV_SPI_MODE_0,                      \
    .bit_order    = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST,         \
};

static eRf69Mode_t freq433DevMode = RF69_MODE_NONE;
static eRf69Mode_t freq916n868DevMode = RF69_MODE_NONE;

static uint8_t freq916CfgTbl[][2] =
{
	/* 0x01 */ { REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY },
	/* 0x02 */ { REG_DATAMODUL, RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_OOK | RF_DATAMODUL_MODULATIONSHAPING_00 },
	/* 0x03 */ { REG_BITRATEMSB, RF_BITRATEMSB_16384},
	/* 0x04 */ { REG_BITRATELSB, RF_BITRATELSB_16384},
	///* 0x05 */ { REG_FDEVMSB, RF_FDEVMSB_26370},//default: 5KHz, (FDEV + BitRate / 2 <= 500KHz)
	///* 0x06 */ { REG_FDEVLSB, RF_FDEVLSB_26370},
	/* 0x07 */ { REG_FRFMSB, (uint8_t)(RF_FRFMSB_916) },
	/* 0x08 */ { REG_FRFMID, (uint8_t)(RF_FRFMID_916) },
	/* 0x09 */ { REG_FRFLSB, (uint8_t)(RF_FRFLSB_916) },
	// looks like PA1 and PA2 are not implemented on RFM69W, hence the max output power is 13dBm
	// +17dBm and +20dBm are possible on RFM69HW
	// +13dBm formula: Pout = -18 + OutputPower (with PA0 or PA1**)
	// +17dBm formula: Pout = -14 + OutputPower (with PA1 and PA2)**
	// +20dBm formula: Pout = -11 + OutputPower (with PA1 and PA2)** and high power PA settings (section 3.3.7 in datasheet)
	///* 0x11 */ { REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | RF_PALEVEL_OUTPUTPOWER_11111},
	///* 0x13 */ { REG_OCP, RF_OCP_ON | RF_OCP_TRIM_95 },//over current protection (default is 95mA)
	//* 0x18 */ { REG_LNA, RF_LNA_ZIN_200 | RF_LNA_LOWPOWER_OFF | RF_LNA_CURRENTGAIN | RF_LNA_GAINSELECT_MAXMINUS48 },
	// RXBW defaults are { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_24 | RF_RXBW_EXP_5} (RxBw: 10.4KHz)
	/* 0x19 */ { REG_RXBW, RF_RXBW_DCCFREQ_000 | RF_RXBW_MANT_20 | RF_RXBW_EXP_0 },//(BitRate < 2 * RxBw)
	//for BR-19200: /* 0x19 */ { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_24 | RF_RXBW_EXP_3 },
	/* 0x25 */ { REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_00 },//DIO0 is the only IRQ we're using
	/* 0x26 */ { REG_DIOMAPPING2, RF_DIOMAPPING2_CLKOUT_OFF },//DIO5 ClkOut disable for power saving
	/* 0x28 */ { REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN },//writing to this bit ensures that the FIFO & status flags are reset
	/* 0x29 */ { REG_RSSITHRESH, 228 },//must be set to dBm = (-Sensitivity / 2), default is 0xE4 = 228 so -114dBm
	/* 0x2C */ { REG_PREAMBLEMSB, RF_PREAMBLESIZE_MSB_VALUE },//default 3 preamble bytes 0xAAAAAA
	/* 0x2D */ { REG_PREAMBLELSB, RF_PREAMBLESIZE_LSB_VALUE },//default 3 preamble bytes 0xAAAAAA
	/* 0x2E */ { REG_SYNCCONFIG, RF_SYNC_ON | RF_SYNC_FIFOFILL_AUTO | RF_SYNC_SIZE_4 | RF_SYNC_TOL_0 },
	/* 0x2F */ { REG_SYNCVALUE1, 0xFF },
	/* 0x30 */ { REG_SYNCVALUE2, 0x00 },
	/* 0x31 */ { REG_SYNCVALUE3, 0xFF },
	/* 0x32 */ { REG_SYNCVALUE4, 0x00 }, 
	/* 0x37 */ { REG_PACKETCONFIG1, RF_PACKET1_FORMAT_FIXED | RF_PACKET1_DCFREE_OFF | RF_PACKET1_CRC_OFF | RF_PACKET1_CRCAUTOCLEAR_OFF | RF_PACKET1_ADRSFILTERING_OFF },
	/* 0x38 */ { REG_PAYLOADLENGTH, 0xFF },//in variable length mode: the max frame size, not used in TX
	///* 0x39 */ { REG_NODEADRS, nodeID },//turned off because we're not using address filtering
	/* 0x3C */ { REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY | RF_FIFOTHRESH_VALUE },//TX on FIFO not empty
	/* 0x3D */ { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_NONE | RF_PACKET2_AUTORXRESTART_OFF | RF_PACKET2_AES_OFF },//RXRESTARTDELAY must match transmitter PA ramp-down time (bitrate dependent)
	//for BR-19200: /* 0x3D */ { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_NONE | RF_PACKET2_AUTORXRESTART_ON | RF_PACKET2_AES_OFF },//RXRESTARTDELAY must match transmitter PA ramp-down time (bitrate dependent)
	///* 0x58 */ { REG_TESTLNA, RF_TESTLNA_HIGH_SENSITIVITY },//run DAGC continuously in RX mode for Fading Margin Improvement, recommended default for AfcLowBetaOn=0
	/* 0x6F */ { REG_TESTDAGC, RF_DAGC_IMPROVED_LOWBETA0 },//run DAGC continuously in RX mode for Fading Margin Improvement, recommended default for AfcLowBetaOn=0
	{255, 0}
};

static uint8_t freq868CfgTbl[][2] =
{
	/* 0x01 */ { REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY },
	/* 0x02 */ { REG_DATAMODUL, RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_OOK | RF_DATAMODUL_MODULATIONSHAPING_00 },
	/* 0x03 */ { REG_BITRATEMSB, RF_BITRATEMSB_16384},
	/* 0x04 */ { REG_BITRATELSB, RF_BITRATELSB_16384},
	///* 0x05 */ { REG_FDEVMSB, RF_FDEVMSB_26370},//default: 5KHz, (FDEV + BitRate / 2 <= 500KHz)
	///* 0x06 */ { REG_FDEVLSB, RF_FDEVLSB_26370},
	/* 0x07 */ { REG_FRFMSB, (uint8_t)(RF_FRFMSB_868) },
	/* 0x08 */ { REG_FRFMID, (uint8_t)(RF_FRFMID_868) },
	/* 0x09 */ { REG_FRFLSB, (uint8_t)(RF_FRFLSB_868) },
	// looks like PA1 and PA2 are not implemented on RFM69W, hence the max output power is 13dBm
	// +17dBm and +20dBm are possible on RFM69HW
	// +13dBm formula: Pout = -18 + OutputPower (with PA0 or PA1**)
	// +17dBm formula: Pout = -14 + OutputPower (with PA1 and PA2)**
	// +20dBm formula: Pout = -11 + OutputPower (with PA1 and PA2)** and high power PA settings (section 3.3.7 in datasheet)
	///* 0x11 */ { REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | RF_PALEVEL_OUTPUTPOWER_11111},
	///* 0x13 */ { REG_OCP, RF_OCP_ON | RF_OCP_TRIM_95 }, // over current protection (default is 95mA)
	///* 0x18 */ { REG_LNA, RF_LNA_ZIN_200 | RF_LNA_LOWPOWER_OFF | RF_LNA_CURRENTGAIN | RF_LNA_GAINSELECT_MAXMINUS48 },
	// RXBW defaults are { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_24 | RF_RXBW_EXP_5} (RxBw: 10.4KHz)
	/* 0x19 */ { REG_RXBW, RF_RXBW_DCCFREQ_000 | RF_RXBW_MANT_16 | RF_RXBW_EXP_0},//RF_RXBW_EXP_0 }, // (BitRate < 2 * RxBw)
	///* 0x1B */ { REG_OOKPEAK, RF_OOKPEAK_THRESHTYPE_FIXED },
	///* 0x1D */ { REG_OOKFIX, RF_OOKFIX_FIXEDTHRESH_VALUE },
	//for BR-19200: /* 0x19 */ { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_24 | RF_RXBW_EXP_3 },
	/* 0x25 */ { REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_00 },//DIO0 is the only IRQ we're using
	/* 0x26 */ { REG_DIOMAPPING2, RF_DIOMAPPING2_CLKOUT_OFF },//DIO5 ClkOut disable for power saving
	/* 0x28 */ { REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN },//writing to this bit ensures that the FIFO & status flags are reset
	/* 0x29 */ { REG_RSSITHRESH, 228 },//must be set to dBm = (-Sensitivity / 2), default is 0xE4 = 228 so -114dBm
	/* 0x2C */ { REG_PREAMBLEMSB, RF_PREAMBLESIZE_MSB_VALUE },//default 3 preamble bytes 0xAAAAAA
	/* 0x2D */ { REG_PREAMBLELSB, RF_PREAMBLESIZE_LSB_VALUE },//default 3 preamble bytes 0xAAAAAA
	/* 0x2E */ { REG_SYNCCONFIG, RF_SYNC_ON | RF_SYNC_FIFOFILL_AUTO | RF_SYNC_SIZE_4 | RF_SYNC_TOL_0 },
	/* 0x2F */ { REG_SYNCVALUE1, 0xFF },
	/* 0x30 */ { REG_SYNCVALUE2, 0x00 },
	/* 0x31 */ { REG_SYNCVALUE3, 0xFF },
	/* 0x32 */ { REG_SYNCVALUE4, 0x00 }, 
	/* 0x37 */ { REG_PACKETCONFIG1, RF_PACKET1_FORMAT_FIXED | RF_PACKET1_DCFREE_OFF | RF_PACKET1_CRC_OFF | RF_PACKET1_CRCAUTOCLEAR_OFF | RF_PACKET1_ADRSFILTERING_OFF },
	/* 0x38 */ { REG_PAYLOADLENGTH, 0xFF },//in variable length mode: the max frame size, not used in TX
	///* 0x39 */ { REG_NODEADRS, nodeID },//turned off because we're not using address filtering
	/* 0x3C */ { REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY | RF_FIFOTHRESH_VALUE },//TX on FIFO not empty
	/* 0x3D */ { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_NONE | RF_PACKET2_AUTORXRESTART_OFF | RF_PACKET2_AES_OFF },//RXRESTARTDELAY must match transmitter PA ramp-down time (bitrate dependent)
	//for BR-19200: /* 0x3D */ { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_NONE | RF_PACKET2_AUTORXRESTART_ON | RF_PACKET2_AES_OFF }, // RXRESTARTDELAY must match transmitter PA ramp-down time (bitrate dependent)
	///* 0x58 */ { REG_TESTLNA, RF_TESTLNA_HIGH_SENSITIVITY },
	/* 0x6F */ { REG_TESTDAGC, RF_DAGC_IMPROVED_LOWBETA0 },//run DAGC continuously in RX mode for Fading Margin Improvement, recommended default for AfcLowBetaOn=0
	{255, 0}
};

static uint8_t freq433CfgTbl[][2] =
{
	/* 0x01 */ { REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY },
	/* 0x02 */ { REG_DATAMODUL, RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_FSK | RF_DATAMODUL_MODULATIONSHAPING_00 }, // no shaping
	/* 0x03 */ { REG_BITRATEMSB, RF_BITRATEMSB_40625},
	/* 0x04 */ { REG_BITRATELSB, RF_BITRATELSB_40625},
	/* 0x05 */ { REG_FDEVMSB, RF_FDEVMSB_26370},//default: 5KHz, (FDEV + BitRate / 2 <= 500KHz)
	/* 0x06 */ { REG_FDEVLSB, RF_FDEVLSB_26370},
	/* 0x07 */ { REG_FRFMSB, (uint8_t)(RF_FRFMSB_434) },
	/* 0x08 */ { REG_FRFMID, (uint8_t)(RF_FRFMID_434) },
	/* 0x09 */ { REG_FRFLSB, (uint8_t)(RF_FRFLSB_434) },
	// looks like PA1 and PA2 are not implemented on RFM69W, hence the max output power is 13dBm
	// +17dBm and +20dBm are possible on RFM69HW
	// +13dBm formula: Pout = -18 + OutputPower (with PA0 or PA1**)
	// +17dBm formula: Pout = -14 + OutputPower (with PA1 and PA2)**
	// +20dBm formula: Pout = -11 + OutputPower (with PA1 and PA2)** and high power PA settings (section 3.3.7 in datasheet)
	///* 0x11 */ { REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | RF_PALEVEL_OUTPUTPOWER_11111},
	///* 0x13 */ { REG_OCP, RF_OCP_ON | RF_OCP_TRIM_95 }, // over current protection (default is 95mA)
	///* 0x18 */ { REG_LNA, RF_LNA_ZIN_200 | RF_LNA_LOWPOWER_OFF | RF_LNA_CURRENTGAIN | RF_LNA_GAINSELECT_MAXMINUS48 },
	// RXBW defaults are { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_24 | RF_RXBW_EXP_5} (RxBw: 10.4KHz)
	/* 0x19 */ { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_20 | RF_RXBW_EXP_2 },//(BitRate < 2 * RxBw)
	//for BR-19200: /* 0x19 */ { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_24 | RF_RXBW_EXP_3 },
	/* 0x25 */ { REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_00 },//DIO0 is the only IRQ we're using
	/* 0x26 */ { REG_DIOMAPPING2, RF_DIOMAPPING2_CLKOUT_OFF },//DIO5 ClkOut disable for power saving
	/* 0x28 */ { REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN },//writing to this bit ensures that the FIFO & status flags are reset
	/* 0x29 */ { REG_RSSITHRESH, 228 },//must be set to dBm = (-Sensitivity / 2), default is 0xE4 = 228 so -114dBm
	/* 0x2C */ { REG_PREAMBLEMSB, RF_PREAMBLESIZE_MSB_VALUE },//default 3 preamble bytes 0xAAAAAA
	/* 0x2D */ { REG_PREAMBLELSB, RF_PREAMBLESIZE_LSB_VALUE },//default 3 preamble bytes 0xAAAAAA
	/* 0x2E */ { REG_SYNCCONFIG, RF_SYNC_ON | RF_SYNC_FIFOFILL_AUTO | RF_SYNC_SIZE_4 | RF_SYNC_TOL_0 },
	/* 0x2F */ { REG_SYNCVALUE1, 0x66 },
	/* 0x30 */ { REG_SYNCVALUE2, 0x65 },
	/* 0x31 */ { REG_SYNCVALUE3, 0xA5 },
	/* 0x32 */ { REG_SYNCVALUE4, 0x5A }, 
	/* 0x37 */ { REG_PACKETCONFIG1, RF_PACKET1_FORMAT_FIXED | RF_PACKET1_DCFREE_OFF | RF_PACKET1_CRC_OFF | RF_PACKET1_CRCAUTOCLEAR_OFF | RF_PACKET1_ADRSFILTERING_OFF },
	/* 0x38 */ { REG_PAYLOADLENGTH, 0XFF },//in variable length mode: the max frame size, not used in TX
	///* 0x39 */ { REG_NODEADRS, nodeID },//turned off because we're not using address filtering
	/* 0x3C */ { REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY | RF_FIFOTHRESH_VALUE },//TX on FIFO not empty
	/* 0x3D */ { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_NONE | RF_PACKET2_AUTORXRESTART_OFF | RF_PACKET2_AES_OFF },//RXRESTARTDELAY must match transmitter PA ramp-down time (bitrate dependent)
	//for BR-19200: /* 0x3D */ { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_NONE | RF_PACKET2_AUTORXRESTART_ON | RF_PACKET2_AES_OFF },//RXRESTARTDELAY must match transmitter PA ramp-down time (bitrate dependent)
	/* 0x6F */ { REG_TESTDAGC, RF_DAGC_IMPROVED_LOWBETA0 },//run DAGC continuously in RX mode for Fading Margin Improvement, recommended default for AfcLowBetaOn=0
	{255, 0}
};

static void spi_select(eRf69Dev_t dev)
{
    nrf_drv_spi_init(&spiInst, &spiCfg, NULL, NULL);

	switch(dev)
	{
		case RF69_DEV_FREQ433:
			nrf_gpio_cfg_output(SPI_NSS_0_PIN);
			nrf_gpio_pin_clear(SPI_NSS_0_PIN);
			break;

		case RF69_DEV_FREQ916N868:
			nrf_gpio_cfg_output(SPI_NSS_1_PIN);
			nrf_gpio_pin_clear(SPI_NSS_1_PIN);
			break;

		default:
			break;
	}
}

static void spi_unselect(eRf69Dev_t dev)
{
	switch(dev)
	{
		case RF69_DEV_FREQ433:
			nrf_gpio_cfg_default(SPI_NSS_0_PIN);  
			break;

		case RF69_DEV_FREQ916N868:
			nrf_gpio_cfg_default(SPI_NSS_1_PIN);  
			break;

		default:
			break;
	}
	
	nrf_drv_spi_uninit(&spiInst);
	nrf_gpio_cfg_default(SPI_SCLK_PIN);  
	nrf_gpio_cfg_default(SPI_MISO_PIN);  
	nrf_gpio_cfg_default(SPI_MOSI_PIN);  
}

static uint8_t spi_read_reg(eRf69Dev_t dev, uint8_t addr)
{
	uint8_t rxData;
	uint8_t dummy = 0x01;
	
	spi_select(dev);
    nrf_drv_spi_transfer(&spiInst, &addr, 1, NULL, 0);
    nrf_drv_spi_transfer(&spiInst, &dummy, 1, &rxData, 1);
	spi_unselect(dev);
	
	return rxData;
}

static void spi_write_reg(eRf69Dev_t dev, uint8_t addr, uint8_t value)
{
	uint8_t txData[2];

	txData[0] = addr | 0x80;
	txData[1] = value;
	spi_select(dev);
    nrf_drv_spi_transfer(&spiInst, txData, 2, NULL, 0);
	spi_unselect(dev);
}

static void spi_write_burst(eRf69Dev_t dev, uint8_t addr, const uint8_t *pData, uint16_t cnt)
{
	uint8_t txData[255];

	txData[0] = addr | 0x80;
	memcpy(txData + 1, pData, cnt);
	spi_select(dev);
	nrf_drv_spi_transfer(&spiInst, txData, cnt + 1, NULL, 0);
	spi_unselect(dev);
}

void Rf69_SetMode(eRf69Dev_t dev, eRf69Mode_t newMode)
{
	eRf69Mode_t *pOldMode;
		
	pOldMode = (dev == RF69_DEV_FREQ433) ? (&freq433DevMode) : (&freq916n868DevMode);
	
	if(newMode == *pOldMode)
	{
		return;
	}

	switch(newMode) 
	{
		case RF69_MODE_TX:
			spi_write_reg(dev, REG_OPMODE, (spi_read_reg(dev, REG_OPMODE) & 0xE3) | RF_OPMODE_TRANSMITTER);
			break;
			
		case RF69_MODE_RX:
			spi_write_reg(dev, REG_OPMODE, (spi_read_reg(dev, REG_OPMODE) & 0xE3) | RF_OPMODE_RECEIVER);
			break;
			
		case RF69_MODE_SYNTH:
			spi_write_reg(dev, REG_OPMODE, (spi_read_reg(dev, REG_OPMODE) & 0xE3) | RF_OPMODE_SYNTHESIZER);
			break;
			
		case RF69_MODE_STANDBY:
			spi_write_reg(dev, REG_OPMODE, (spi_read_reg(dev, REG_OPMODE) & 0xE3) | RF_OPMODE_STANDBY);
			break;
			
		case RF69_MODE_SLEEP:
			spi_write_reg(dev, REG_OPMODE, (spi_read_reg(dev, REG_OPMODE) & 0xE3) | RF_OPMODE_SLEEP);
			break;
			
		default:
			return;
	}
	
	//we are using packet mode, so this check is not really needed,
	//but waiting for mode ready is necessary when going from sleep because 
	//the FIFO may not be immediately available from previous mode
	while ((spi_read_reg(dev, REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00);//wait for ModeReady
	*pOldMode = newMode;
}


/*return the frequency (in Hz)*/
uint32_t Rf69_GetFreq(eRf69Dev_t dev)
{
	return RF69_FSTEP * (((uint32_t) spi_read_reg(dev, REG_FRFMSB) << 16)
		+ ((uint16_t) spi_read_reg(dev, REG_FRFMID) << 8) + spi_read_reg(dev, REG_FRFLSB));
}

/*set the frequency (in Hz)*/
void Rf69_SetFreq(eRf69Dev_t dev, uint32_t freqHz)
{
	eRf69Mode_t oldMode;
		
	oldMode = (dev == RF69_DEV_FREQ433) ? freq433DevMode : freq916n868DevMode;
	
	if (oldMode == RF69_MODE_TX) 
	{
		Rf69_SetMode(dev, RF69_MODE_RX);
	}
	
	freqHz /= RF69_FSTEP;//divide down by FSTEP to get FRF
	spi_write_reg(dev, REG_FRFMSB, freqHz >> 16);
	spi_write_reg(dev, REG_FRFMID, freqHz >> 8);
	spi_write_reg(dev, REG_FRFLSB, freqHz);
	
	if (oldMode == RF69_MODE_RX) 
	{
		Rf69_SetMode(dev, RF69_MODE_SYNTH);
	}
	
	Rf69_SetMode(dev, oldMode);
}

/*
set *transmit/TX* output power: 0=min, 31=max
this results in a "weaker" transmitted signal, and directly results in a lower RSSI at the receiver
the power configurations are explained in the SX1231H datasheet (Table 10 on p21; RegPaLevel p66): http://www.semtech.com/images/datasheet/sx1231h.pdf
valid powerLevel parameter values are 0-31 and result in a directly proportional effect on the output/transmission power
this function implements 2 modes as follows:
- for RFM69W the range is from 0-31 [-18dBm to 13dBm] (PA0 only on RFIO pin)
- for RFM69HW the range is from 0-31 [5dBm to 20dBm]  (PA1 & PA2 on PA_BOOST pin & high Power PA settings - see section 3.3.7 in datasheet, p22)
*/
void Rf69_SetPowerLevel(eRf69Dev_t dev, uint8_t powerLevel)
{
	uint8_t powerLevelTmp;
		
	powerLevelTmp = (powerLevel > 31 ? 31 : powerLevel);
	spi_write_reg(dev, REG_PALEVEL, (spi_read_reg(dev, REG_PALEVEL) & 0xE0) | powerLevelTmp);
}

/*get the received signal strength indicator (RSSI)*/
int16_t Rf69_ReadRssi(eRf69Dev_t dev, bool forceTrigger) 
{
	int16_t rssi = 0;
	
	if(forceTrigger)
	{
		//RSSI trigger not needed if DAGC is in continuous mode
		spi_write_reg(dev, REG_RSSICONFIG, RF_RSSI_START);
		while((spi_read_reg(dev, REG_RSSICONFIG) & RF_RSSI_DONE) == 0x00);//wait for RSSI_Ready
	}
	
	rssi = -spi_read_reg(dev, REG_RSSIVALUE);
	rssi >>= 1;
	
	return rssi;
}

bool Rf69_IsFifoEmpty(eRf69Dev_t dev) 
{
	return (spi_read_reg(dev, REG_IRQFLAGS2) & RF_IRQFLAGS2_FIFONOTEMPTY) == 0;
}

bool Rf69_IsFifoFull(eRf69Dev_t dev) 
{
	return (spi_read_reg(dev, REG_IRQFLAGS2) & RF_IRQFLAGS2_FIFOFULL);
}

bool Rf69_IsFifoOverThreshold(eRf69Dev_t dev) 
{
	return (spi_read_reg(dev, REG_IRQFLAGS2) & RF_IRQFLAGS2_FIFOLEVEL);
}

void Rf69_ClearFifo(eRf69Dev_t dev) 
{
	spi_write_reg(dev, REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN);
}

void Rf69_XmitByte(eRf69Dev_t dev, uint8_t data) 
{
	spi_write_reg(dev, REG_FIFO, data);
}

void Rf69_XmitBuf(eRf69Dev_t dev, const uint8_t* pData, int len) 
{
	spi_write_burst(dev, REG_FIFO, pData, len);
}

bool Rf69_PacketSeen(eRf69Dev_t dev) 
{
	return (spi_read_reg(dev, REG_IRQFLAGS1) & RF_IRQFLAGS1_SYNCADDRESSMATCH);
}

uint8_t Rf69_RcvByte(eRf69Dev_t dev) 
{
	return spi_read_reg(dev, REG_FIFO);
}

void Rf69_SetSeqOnOff(eRf69Dev_t dev, bool onOff)
{
	uint8_t tmp;
	
	tmp = spi_read_reg(dev, REG_OPMODE);
	
	if(onOff)
	{
		tmp = tmp & 0x7F;
	}
	else
	{
		tmp = tmp | 0x80;
	}
	
	spi_write_reg(dev, REG_OPMODE, tmp);
}

void Rf69_SetPayloadLen(eRf69Dev_t dev, uint8_t len)
{ 
	uint8_t tmp;
	
	tmp = spi_read_reg(dev, REG_PACKETCONFIG1);
	tmp &= 0x7f;
	spi_write_reg(dev, REG_PACKETCONFIG1, tmp);
	spi_write_reg(dev, REG_PAYLOADLENGTH, len);
}

void Rf69_SetSyncOnOff(eRf69Dev_t dev, bool onOff) 
{
	uint8_t tmp;
	
	tmp = spi_read_reg(dev, REG_SYNCCONFIG);
	
	if(onOff)
	{
		tmp |= 0x80;
	}
	else
	{
		tmp &= 0x7F;
	}
	
	spi_write_reg(dev, REG_SYNCCONFIG, tmp);
}

void Rf69_SetPreambleSize(eRf69Dev_t dev, uint16_t size) 
{
	spi_write_reg(dev, REG_PREAMBLEMSB, (uint8_t)(size >> 8));
	spi_write_reg(dev, REG_PREAMBLELSB, (uint8_t)(size & 0xff));
}

void Rf69_SetUnlimitedLenPkt(eRf69Dev_t dev) 
{
	uint8_t tmp;
	
	tmp = spi_read_reg(dev, REG_PACKETCONFIG1);
	tmp &= 0x7f;
	spi_write_reg(dev, REG_PACKETCONFIG1, tmp);
	spi_write_reg(dev, REG_PAYLOADLENGTH, 0);
}

void Rf69_SetOokBw250khz(eRf69Dev_t dev)
{
	spi_write_reg(dev, REG_RXBW, RF_RXBW_DCCFREQ_000 | RF_RXBW_MANT_16 | RF_RXBW_EXP_0); 
}

void Rf69_SetOokBw200khz(eRf69Dev_t dev)
{
	spi_write_reg(dev, REG_RXBW, RF_RXBW_DCCFREQ_000 | RF_RXBW_MANT_20 | RF_RXBW_EXP_0);
}

void Rf69_SetDioMapping(eRf69Dev_t dev)
{
	spi_write_reg(dev, REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_00);//DIO0 is "Packet Sent"
}

void Rf69_DevParaCfg(eRf69Dev_t dev, eRf69Freq_t freq)
{    
	uint8_t (*pCfgTbl)[2];
	uint8_t i;
	
	switch(freq)
	{
		case RF69_FREQ_916:
			pCfgTbl = freq916CfgTbl;
			freq916n868DevMode = RF69_MODE_STANDBY;
			break;
			
		case RF69_FREQ_433:
			pCfgTbl = freq433CfgTbl;
			freq433DevMode = RF69_MODE_STANDBY;
			break;
			
		case RF69_FREQ_868:
			pCfgTbl = freq868CfgTbl;
			freq916n868DevMode = RF69_MODE_STANDBY;
			break;
			
		default:
			break;
	}
	
	for(i = 0; pCfgTbl[i][0] != 255; i++)
	{
		spi_write_reg(dev, pCfgTbl[i][0], pCfgTbl[i][1]);
	}	
	Rf69_SetMode(dev, RF69_MODE_SLEEP);
}


