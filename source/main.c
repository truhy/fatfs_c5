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
	Target : ARM Cortex-A9 on the DE10-Nano development board (Intel Cyclone V SoC
	         FPGA)
	Type   : C

	A standalone C program demonstrating use of the FatFs module library to write
	and read a plain text file with a SD card containing a FAT/exFAT filesystem.
	
	Intel HWLIB is used for the low-level SD/MMC card controller functions.
*/

#include "tru_config.h"
#include "tru_logger.h"
#include "alt_sdmmc.h"
#include "diskio_c5_sdmmc.h"
#include "diskio.h"
#include <stdio.h>
#include <string.h>

#ifdef SEMIHOSTING
	extern void initialise_monitor_handles(void);  // Reference function header from the external Semihosting library
#endif

const TCHAR drvno[] = _T("0");  // Logical drive number
const TCHAR rootpath[] = _T("");  // Root directory
const TCHAR sampletext[] = _T("Hello, World! FatFs example");
const TCHAR filename[] = _T("fatfs_example.txt");

// Use Altera HWLIB functions to display some card information
void disp_cardinfo(void){
	ALT_STATUS_CODE alt_status;

	printf("Card info:\n");
	ALT_SDMMC_CARD_INFO_t cardinfo = sdmmc_get_init_alt_cardinfo();
	printf("Card type: ");
	switch(cardinfo.card_type){
		case ALT_SDMMC_CARD_TYPE_MMC: printf("MMC (MultiMedia Card)\n"); break;
		case ALT_SDMMC_CARD_TYPE_SD: printf("SD (Secure Digital Memory Card)\n"); break;
		case ALT_SDMMC_CARD_TYPE_SDIOIO: printf("SDIOIO (Secure Digital Input Output)\n"); break;
		case ALT_SDMMC_CARD_TYPE_SDIOCOMBO: printf("SDIOCOMBO (Secure Digital Input Output Combo)\n"); break;
		case ALT_SDMMC_CARD_TYPE_SDHC: printf("SDHC (Secure Digital High Capacity)\n"); break;
		case ALT_SDMMC_CARD_TYPE_CEATA: printf("CEATA (Serial ATA interface based on the MultiMediaCard standard)\n"); break;
		default: printf("Unknown\n");
	}
	printf("Max speed: %lu bps\n", cardinfo.xfer_speed);
	printf("Max read block size: %lu byte(s)\n", cardinfo.max_r_blkln);
	printf("Max write block size: %lu byte(s)\n", cardinfo.max_w_blkln);
	printf("Controller supports 1 bit bus width: %s\n", (cardinfo.scr_bus_widths & ALT_SDMMC_BUS_WIDTH_1) ? "Yes" : "No");
	printf("Controller supports 4 bit bus width: %s\n", (cardinfo.scr_bus_widths & ALT_SDMMC_BUS_WIDTH_4) ? "Yes" : "No");
	printf("Controller supports 8 bit bus width: %s\n", (cardinfo.scr_bus_widths & ALT_SDMMC_BUS_WIDTH_8) ? "Yes" : "No");
	printf("Controller supports 50MHz high speed: %s\n", cardinfo.high_speed ? "Yes" : "No");
	printf("Total blocks: %llu\n", (uint64_t)cardinfo.blk_number_high << 32U | cardinfo.blk_number_low);
	printf("Capacity: %llu MB\n", (uint64_t)cardinfo.blk_number_low * cardinfo.max_w_blkln / (1024UL * 1024UL));

	printf("\nCard current configuration:\n");
	ALT_SDMMC_CARD_MISC_t card_misc_cfg;
	alt_status = alt_sdmmc_card_misc_get(&card_misc_cfg);
	if(alt_status == ALT_E_SUCCESS){
		printf("Block size: %lu byte(s)\n", card_misc_cfg.block_size);
		printf("Bus width: %lu bit\n", card_misc_cfg.card_width);
		printf("Data timeout: %lu\n", card_misc_cfg.data_timeout);
		printf("Debounce count: %lu\n", card_misc_cfg.debounce_count);
		printf("Response timeout: %lu\n", card_misc_cfg.response_timeout);
	}
	uint32_t speed = alt_sdmmc_card_speed_get();
	printf("Speed: %lu bps\n", speed);
}

void disp_dir(const TCHAR *path){
	DIR fdir;

	if(f_opendir(&fdir, path) == FR_OK){
		// Iterate files
		do{
			FILINFO finfo;

			// Read info of all files (non-recursive) inside the directory
			if(f_readdir(&fdir, &finfo) != FR_OK) break;
			if(finfo.fname[0] == 0) break;

			printf("%.10lu %04u-%02u-%02u %02u:%02u:%02u %c%c%c%c%c %s\n",
				finfo.fsize,
				(finfo.fdate >> 9U) + 1980U,
				finfo.fdate >> 5U & 0xfU,
				finfo.fdate & 0xfU,
				finfo.ftime >> 11U,
				finfo.ftime >> 5U & 0x3fU,
				finfo.ftime & 0x1fU,
				((finfo.fattrib & AM_DIR) ? 'D' : '-'),
				((finfo.fattrib & AM_RDO) ? 'R' : '-'),
				((finfo.fattrib & AM_ARC) ? 'A' : '-'),
				((finfo.fattrib & AM_SYS) ? 'S' : '-'),
				((finfo.fattrib & AM_HID) ? 'H' : '-'),
				finfo.fname
			);
		}while(1);
	}else{
		printf("Error: Could not open directory\n");
	}
}

FRESULT write_textfile(const TCHAR *filename, const TCHAR *text, UINT len){
	FIL fp;
	FRESULT fres;
	UINT bw;

	fres = f_open(&fp, filename, FA_WRITE | FA_OPEN_ALWAYS | FA_CREATE_ALWAYS);
	if(fres != FR_OK){
		printf("Error: Could not open file\n");
		return fres;
	}

	fres = f_write(&fp, text, len, &bw);
	if(fres == FR_OK){
		printf("Write: Wrote '%s'\n", text);
	}else{
		printf("Error: Could not write to file\n");
	}

	fres = f_close(&fp);
	if(fres != FR_OK){
		printf("Error: Could not close file\n");
	}

	return fres;
}

void read_textfile(const TCHAR *filename){
	FIL fp;
	TCHAR line[255];

	if(f_open(&fp, filename, FA_READ) != FR_OK){
		printf("Error: Could not open file\n");
		return;
	}

	TCHAR* rres = f_gets(line, sizeof(line), &fp);
	if(rres != 0){
		printf("Read : File contains '%s'\n", line);
	}else{
		printf("Error: Could not read from file\n");
	}

	if(f_close(&fp) != FR_OK){
		printf("Error: Could not close file\n");
	}
}

void run_demo(void){
	// Initialise FatFs library for physical drive 0
	if(disk_initialize(0) != STA_NOINIT){
		disp_cardinfo();

		// Mount drive
		FATFS fatfs;
		if(f_mount(&fatfs, drvno, 1) != FR_OK){
			printf("Error: Could not mount drive");
			return;
		}

		// List files
		printf("\nFiles in the root directory:\n");
		disp_dir(rootpath);

		// Write and then read file
		printf("\nWrite and read file test (%s)\n", filename);
		write_textfile(filename, sampletext, strlen(sampletext));
		read_textfile(filename);

		// Unmount drive
		f_unmount(drvno);
	}else{
		printf("Error: Could not initialise SD/MMC controller\n");
	}
}

int main(int argc, char *const argv[]){
	#ifdef SEMIHOSTING
		initialise_monitor_handles();  // Initialise Semihosting
	#endif

		run_demo();

#if(TRU_EXIT_TO_UBOOT == 1U)
	tru_hps_uart_ll_wait_empty((TRU_TARGET_TYPE *)TRU_HPS_UART0_BASE);  // Before returning to U-Boot, we will wait for the UART to empty out
#endif

	return 0xa9;
}
