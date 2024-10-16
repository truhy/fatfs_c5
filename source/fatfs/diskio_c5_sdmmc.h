/*
	Created on: 12 Oct 2024
	Author: Truong Hy
*/

#ifndef DISKIO_C5_SDMMC_H
#define DISKIO_C5_SDMMC_H

#include "alt_sdmmc.h"
#include "socal/alt_sdmmc.h"
#include "socal/hps.h"
#include "socal/socal.h"

// Get the cardinfo (Altera HWLIB structure) that was stored when the card initialise was called
ALT_SDMMC_CARD_INFO_t sdmmc_get_init_alt_cardinfo(void);

// Get current sector block size.  This code is duplicated here but made global, due to HWLIB being too protective with a local static version
static __inline uint16_t sdmmc_get_block_size(void){
    uint32_t blksiz_register = alt_read_word(ALT_SDMMC_BLKSIZ_ADDR);
    return ALT_SDMMC_BLKSIZ_BLOCK_SIZE_GET(blksiz_register);
}

#endif
