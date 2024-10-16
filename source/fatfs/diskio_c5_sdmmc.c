/*
	MIT License

	Copyright (c) 2024 Truong Hy

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.

	Version: 20241006
	Target : ARM Cortex-A9 integrated on Intel Cyclone V SoCFPGA
	Type   : Bare-metal C

	Provides the required Cyclone V SoCFPGA vendor functions for FatFs.

	Limitations:

	I did not write an override for the get_fattime() function because the
	Cyclone V SoCFPGA does not have an RTC, instead we simply set the parameter
	FF_FS_NORTC = 1 and with a constant date in the config file ffconf.h.

	Only the char type string is supported;	it is very difficult to support
	wchar_t string, UNICODE string, etc.
*/

#include "tru_logger.h"
#include "alt_sdmmc.h"
#include "socal/alt_sdmmc.h"
#include "socal/hps.h"
#include "socal/socal.h"
#include "diskio_c5_sdmmc.h"
#include "diskio.h"

#define SDMAXTIMEOUT_COUNT 1000000UL

static volatile DSTATUS stat = STA_NOINIT;  // Physical drive status
static BYTE cardtype;
static ALT_SDMMC_CARD_INFO_t cardinfo;

// ================
// Helper functions
// ================

ALT_SDMMC_CARD_INFO_t sdmmc_get_init_alt_cardinfo(void){
	return cardinfo;
}

// 1 = idle, else busy
static int sdmmc_is_idle(void){
	uint32_t mmc_state = ALT_SDMMC_STAT_CMD_FSM_STATES_GET(alt_read_word(ALT_SDMMC_STAT_ADDR));
	uint32_t dma_state = ALT_SDMMC_IDSTS_FSM_GET(alt_read_word(ALT_SDMMC_IDSTS_ADDR));
	if((mmc_state != 0) || (dma_state != 0)){
		return 0;  // Busy
	}

	return 1;  // Idle
}

// 0 = timeout, else OK
static int sdmmc_wait_ready(void){
	uint32_t timeout = SDMAXTIMEOUT_COUNT;
	while(!sdmmc_is_idle() && --timeout);
	return timeout ? 1 : 0;
}

static int sdmmc_set_max_speed(void){
	ALT_STATUS_CODE alt_status;

	if(cardinfo.high_speed){
		alt_status = alt_sdmmc_card_speed_set(&cardinfo, cardinfo.xfer_speed);
		if(alt_status != ALT_E_SUCCESS) return 0;
	}

	return 1;
}

static int sdmmc_set_max_bus_width(void){
	ALT_STATUS_CODE alt_status;

	if(cardinfo.scr_bus_widths & ALT_SDMMC_BUS_WIDTH_8){  // Support 8 bits?
		alt_status = alt_sdmmc_card_bus_width_set(&cardinfo, ALT_SDMMC_BUS_WIDTH_8);
		if(alt_status != ALT_E_SUCCESS) return 0;
	}else if(cardinfo.scr_bus_widths & ALT_SDMMC_BUS_WIDTH_4){  // Support 4 bits?
		alt_status = alt_sdmmc_card_bus_width_set(&cardinfo, ALT_SDMMC_BUS_WIDTH_4);
		if(alt_status != ALT_E_SUCCESS) return 0;
	}

	return 1;
}

static int sdmmc_set_512_block(void){
	ALT_STATUS_CODE alt_status = alt_sdmmc_card_block_size_set(512);
	if(alt_status != ALT_E_SUCCESS) return 0;
	return 1;
}

// =========================================
// Implement FatFs vendor portable functions
// =========================================

DSTATUS disk_initialize(
	BYTE drv  // Physical drive number
){
	ALT_STATUS_CODE alt_status;
	
	if(drv) return STA_NOINIT;  // Only drive 0 supported
	
	// Initialize sdmmc controller
	cardtype = 0;
	stat = STA_NOINIT;
	alt_status = alt_sdmmc_init();
	if(alt_status == ALT_E_SUCCESS){
		stat &= ~STA_NOINIT;  // Clear STA_NOINIT flag
	}else{
		LOG("Error: disk_initialize(), could not init sdmmc controller\n");
		return stat;
	}

	// Check if a card is inserted in the slot
	if(!alt_sdmmc_card_is_detected()){
		LOG( "Error: disk_initialize(), no card detected\n" );
		stat = STA_NOINIT;
		return stat;
	}

	alt_status = alt_sdmmc_card_identify(&cardinfo);
	if(alt_status == ALT_E_SUCCESS){
		switch(cardinfo.card_type){
			case ALT_SDMMC_CARD_TYPE_MMC:       cardtype = CT_MMC; break;
			case ALT_SDMMC_CARD_TYPE_SD:        cardtype = CT_SDC; break;
			case ALT_SDMMC_CARD_TYPE_SDIOIO:    cardtype = CT_BLOCK; break;
			case ALT_SDMMC_CARD_TYPE_SDIOCOMBO: cardtype = CT_BLOCK; break;
			case ALT_SDMMC_CARD_TYPE_SDHC:      cardtype = CT_SDC; break;
			case ALT_SDMMC_CARD_TYPE_CEATA:     cardtype = CT_BLOCK; break;
			default: cardtype = 0;
		}
		if(!cardtype){
			stat = STA_NOINIT;
			return stat;
		}
	}else{
		LOG( "Error: disk_initialize(), could not get card type\n" );
		stat = STA_NOINIT;
		return stat;
	}

	// Optional: set higher speed (error is ignored)
	if(!sdmmc_set_max_speed()){
		//LOG( "Error: disk_initialize(), could not set higher speed\n" );
	}

	// Optional: set maximum bus width (error is ignored)
	if(!sdmmc_set_max_bus_width()){
		//LOG( "Error: disk_initialize(), could not set max bus width\n" );
	}

	// Optional: set 512 byte block size (error is ignored)
	if(!sdmmc_set_512_block()){
		//LOG( "Error: disk_initialize(), could not set 512 byte block size\n" );
	}

	return stat;
}

DSTATUS disk_status(
	BYTE drv  // Physical drive number
){
	if(drv) return STA_NOINIT;  // Only drive 0 supported
	return stat;
}

DRESULT disk_read (
	BYTE drv,     // Physical drive number
	BYTE *buff,   // Pointer to the data buffer to store read data
	LBA_t sector, // Start sector number (LBA)
	UINT count    // Number of sectors to read (1..128)
){
	ALT_STATUS_CODE alt_status;
	
	// Error check
	if(drv || !count) return RES_PARERR;  // Check parameter
	if (stat & STA_NOINIT) return RES_NOTRDY;  // Check if drive is ready
	
	// Read
	uint16_t block_size = sdmmc_get_block_size();
	void *sectbyte = (void *)(sector * block_size);  // Convert sector block (LBA) to sector byte offset
	alt_status =
		alt_sdmmc_read(
			&cardinfo,
			(void *)buff,
			sectbyte,
			count * block_size
		);
	if(alt_status == ALT_E_SUCCESS){
		return RES_OK;
	}
	
	return RES_ERROR;
}

DRESULT disk_write (
	BYTE drv,         // Physical drive number
	const BYTE *buff, // Pointer to the data to be written
	LBA_t sector,     // Start sector number (LBA)
	UINT count        // Sector count (1..128)
){
	ALT_STATUS_CODE alt_status;
	
	// Error check
	if(drv || !count) return RES_PARERR;  // Check parameter
	if(stat & STA_NOINIT) return RES_NOTRDY;  // Check if drive is ready
	if(stat & STA_PROTECT) return RES_WRPRT;	// Check write protect
	
	// Write
	uint16_t block_size = sdmmc_get_block_size();
	void *sectbyte = (void *)(sector * block_size);  // Convert sector block (LBA) to sector byte offset
	alt_status =
		alt_sdmmc_write(
			&cardinfo,
			sectbyte,
			(void *)buff,
			count * block_size
		);
	if(alt_status == ALT_E_SUCCESS){
		return RES_OK;
	}
	
	return RES_ERROR;
}

DRESULT disk_ioctl (
	BYTE drv,  // Physical drive nmuber
	BYTE cmd,  // Control code
	void *buff // Buffer to send/receive control data
){
	DRESULT res = RES_ERROR;
	
	if(drv) return RES_PARERR;  // Check parameter
	if(stat & STA_NOINIT) return RES_NOTRDY;  // Check if drive is ready
	
	switch(cmd){
		case CTRL_SYNC:  // Wait for end of internal write process of the drive
			if(sdmmc_wait_ready()) res = RES_OK;
			break;
		case GET_SECTOR_COUNT:  // Get drive capacity in unit of sector (DWORD)
			*(LBA_t*)buff = cardinfo.blk_number_low;
			res = RES_OK;
			break;
		case GET_BLOCK_SIZE:  // Get block size sector (DWORD)
			*(DWORD*)buff = sdmmc_get_block_size();
			res = RES_OK;
			break;
		case MMC_GET_TYPE:  // Get MMC/SDC type (BYTE)
			*(BYTE*)buff = cardtype;
			res = RES_OK;
			break;
		default:
			res = RES_PARERR;
	}
	
	return res;
}
