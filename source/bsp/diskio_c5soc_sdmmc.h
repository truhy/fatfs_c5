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
*/

#ifndef DISKIO_C5SOC_SDMMC_H
#define DISKIO_C5SOC_SDMMC_H

#include "alt_sdmmc.h"
#include "socal/alt_sdmmc.h"
#include "socal/hps.h"
#include "socal/socal.h"

// Get the cardinfo (Altera HWLIB structure) that was stored when the card initialise was called
ALT_SDMMC_CARD_INFO_t sdmmc_get_init_alt_cardinfo(void);

// Get current sector block size.  This code is duplicated here but made global due to HWLIB being too protective with a local static version
static __inline uint16_t sdmmc_get_block_size(void){
    uint32_t blksiz_register = alt_read_word(ALT_SDMMC_BLKSIZ_ADDR);
    return ALT_SDMMC_BLKSIZ_BLOCK_SIZE_GET(blksiz_register);
}

#endif
