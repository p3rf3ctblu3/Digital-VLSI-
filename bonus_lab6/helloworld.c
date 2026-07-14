#include <stdio.h>
#include "platform.h"
#include <math.h>
#include <string.h>
#include "xil_printf.h"
#include "xparameters.h"
#include "xparameters_ps.h"
#include "xaxidma.h"
#include "xtime_l.h"
#include <unistd.h>
#include <stdint.h>
#include "xil_io.h"
#include "xil_types.h"
#include <stdint.h>

#define TX_DMA_ID                 XPAR_PS2PL_DMA_DEVICE_ID
#define TX_DMA_MM2S_LENGTH_ADDR  (XPAR_PS2PL_DMA_BASEADDR + 0x28)

#define RX_DMA_ID                 XPAR_PL2PS_DMA_DEVICE_ID
#define RX_DMA_S2MM_LENGTH_ADDR  (XPAR_PL2PS_DMA_BASEADDR + 0x58)

#define TX_BUFFER (XPAR_DDR_MEM_BASEADDR + 0x08000000)
#define RX_BUFFER (XPAR_DDR_MEM_BASEADDR + 0x10000000)

#define REG_IO_0 XPAR_RECONFIGURABLE_DEBAY_0_S00_AXI_BASEADDR

// FPGA outputs 4 bytes per pixel (for RGB and flags)
#define WORDS_PER_PIXEL 1
#define BYTES_PER_PIXEL 4  // 3 bytes for RBG, 1 byte 0x00

/* User application global variables & defines */
XAxiDma AxiDmaTx;
XAxiDma AxiDmaRx;

static uint8_t r[1024*1024];
static uint8_t g[1024*1024];
static uint8_t b[1024*1024];

static uint8_t* txBuffer = (uint8_t*) TX_BUFFER;
static uint32_t* rxBuffer = (uint32_t*) RX_BUFFER;
int status;
XAxiDma_Config *CfgPtr;

uint8_t input_raw[] = {
#include "1024x1024.txt"
};

int init_tx_dma()
{
	XAxiDma_Config *CfgPtr;

	CfgPtr = XAxiDma_LookupConfig(XPAR_PS2PL_DMA_DEVICE_ID);
	if (!CfgPtr) {
		xil_printf("TX-DMA: Lookup failed\r\n");
		return XST_FAILURE;
	}

	status = XAxiDma_CfgInitialize(&AxiDmaTx, CfgPtr);
	if (status != XST_SUCCESS) {
		xil_printf("TX-DMA: Initialization failed\r\n");
		return XST_FAILURE;
	}

	if (XAxiDma_HasSg(&AxiDmaTx)) {
		xil_printf("TX-DMA: Scatter-Gather mode not supported\r\n");
		return XST_FAILURE;
	}

	XAxiDma_IntrDisable(&AxiDmaTx, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);

	xil_printf("TX-DMA initialized successfully (PS -> PL)\r\n");
	return XST_SUCCESS;
}

int init_rx_dma()
{
	XAxiDma_Config *CfgPtr;

	CfgPtr = XAxiDma_LookupConfig(XPAR_PL2PS_DMA_DEVICE_ID);

	 if (!CfgPtr) {
	        xil_printf("RX-DMA: Lookup failed\r\n");
	        return XST_FAILURE;
	    }

	 status = XAxiDma_CfgInitialize(&AxiDmaRx, CfgPtr);
	     if (status != XST_SUCCESS) {
	         xil_printf("RX-DMA: Initialization failed\r\n");
	         return XST_FAILURE;
	     }

	if (XAxiDma_HasSg(&AxiDmaRx)) {
		xil_printf("RX-DMA: Scatter-Gather mode not supported\r\n");
		return XST_FAILURE;
	}

	XAxiDma_IntrDisable(&AxiDmaRx, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);

	xil_printf("RX-DMA initialized successfully (PL -> PS)\r\n");
	return XST_SUCCESS;
}

static inline int get_pixel(const uint8_t *bayer, int dimension, int x, int y) {
    if (x < 0 || x >= dimension || y < 0 || y >= dimension)
        return 0;
    return bayer[y * dimension + x];
}

void demosaic_bayer_gbrg(uint8_t *bayer, uint8_t *r, uint8_t *g, uint8_t *b, int dimension) {
    for (int y = 0; y < dimension; y++) {
        for (int x = 0; x < dimension; x++) {
            // Fixed index calculation
            int idx = y * dimension + x;
            uint8_t R = 0, G = 0, B = 0;

            int evenRow = (y % 2) == 0;
            int evenCol = (x % 2) == 0;

            if (!evenRow && !evenCol) { // Green pixel (in a Blue/Green row)
                G = get_pixel(bayer, dimension, x, y);
                R = (get_pixel(bayer, dimension, x-1, y) + get_pixel(bayer, dimension, x+1, y)) / 2;
                B = (get_pixel(bayer, dimension, x, y-1) + get_pixel(bayer, dimension, x, y+1)) / 2;
            }
            else if (evenRow && evenCol) { // Green pixel (in a Green/Red row)
                G = get_pixel(bayer, dimension, x, y);
                R = (get_pixel(bayer, dimension, x, y-1) + get_pixel(bayer, dimension, x, y+1)) / 2;
                B = (get_pixel(bayer, dimension, x-1, y) + get_pixel(bayer, dimension, x+1, y)) / 2;
            }
            else if (!evenRow && evenCol) { // Red pixel
                R = get_pixel(bayer, dimension, x, y);
                G = (get_pixel(bayer, dimension, x, y-1) +
                     get_pixel(bayer, dimension, x+1, y) +
                     get_pixel(bayer, dimension, x, y+1) +
                     get_pixel(bayer, dimension, x-1, y)) / 4;
                B = (get_pixel(bayer, dimension, x-1, y-1) +
                     get_pixel(bayer, dimension, x+1, y-1) +
                     get_pixel(bayer, dimension, x+1, y+1) +
                     get_pixel(bayer, dimension, x-1, y+1)) / 4;
            }
            else { // Blue pixel
                B = get_pixel(bayer, dimension, x, y);
                G = (get_pixel(bayer, dimension, x, y-1) +
                     get_pixel(bayer, dimension, x+1, y) +
                     get_pixel(bayer, dimension, x, y+1) +
                     get_pixel(bayer, dimension, x-1, y)) / 4;
                R = (get_pixel(bayer, dimension, x-1, y-1) +
                     get_pixel(bayer, dimension, x+1, y-1) +
                     get_pixel(bayer, dimension, x+1, y+1) +
                     get_pixel(bayer, dimension, x-1, y+1)) / 4;
            }

            r[idx] = R;
            g[idx] = G;
            b[idx] = B;
        }
    }
}

int main()
{

	unsigned long long int  preExecCyclesFPGA = 0;
	unsigned long long int  postExecCyclesFPGA = 0;
	unsigned long long int  preExecCyclesSW = 0;
	unsigned long long int  postExecCyclesSW = 0;

	print("RECONFIGURABLE DEBAYERING FILTER\r\n");

	init_platform();
	Xil_DCacheDisable();

	// Step 1: Initialize TX-DMA Device (PS->PL)
	if (init_tx_dma() != XST_SUCCESS) {
		xil_printf("TX-DMA init failed\r\n");
		return -1;
	}

	// Step 2: Initialize RX-DMA Device (PL->PS)
	if (init_rx_dma() != XST_SUCCESS) {
		xil_printf("RX-DMA init failed\r\n");
		return -1;
	}

	xil_printf("DMA init completed.\r\n");

	// GET IMAGE DIMENSIONS
	int detected_size = sizeof(input_raw) / sizeof(input_raw[0]);

	if (detected_size == 4096) {
		xil_printf("Resolution validated: 64x64\r\n");
	} else if (detected_size == 16384) {
		xil_printf("Resolution validated: 128x128\r\n");
	} else if (detected_size == 65536) {
		xil_printf("Resolution validated: 256x256\r\n");
	} else if (detected_size == 262144) {
		xil_printf("Resolution validated: 512x512\r\n");
	} else if (detected_size == 1048576) {
		xil_printf("Resolution validated: 1024x1024\r\n");
	} else {
		xil_printf("FATAL ERROR: File size %d does not match any allowed dimensions.\r\n", detected_size);
		return -1;
	}

	uint32_t BUFFER_SIZE = (uint32_t)detected_size;

	uint32_t DIMENSION;
	switch(detected_size) {
	    case 4096:    DIMENSION = 64;   break;
	    case 16384:   DIMENSION = 128;  break;
	    case 65536:   DIMENSION = 256;  break;
	    case 262144:  DIMENSION = 512;  break;
	    case 1048576: DIMENSION = 1024; break;
	}

	for (uint32_t i = 0; i < BUFFER_SIZE; i++) {
	    txBuffer[i] = input_raw[i];
	}

	xil_printf("Starting FPGA processing via DMA...\r\n");

	// SEND IMAGE DIMENSION WITH IMAGE_DIM_VLD HIGH
	Xil_Out32(REG_IO_0, (DIMENSION & 0x7FF) | (1 << 11));
	usleep(1000);
	Xil_Out32(REG_IO_0, (DIMENSION & 0x7FF) | (0 << 11));

	xil_printf("Transfer size: %d bytes (%d pixels x %d bytes/pixel)\r\n",
		4*BUFFER_SIZE, BUFFER_SIZE, BYTES_PER_PIXEL);

	// Step 3 : Perform FPGA processing
	XTime_GetTime(&preExecCyclesFPGA);

	// 3a: Setup RX-DMA transaction
	status = XAxiDma_SimpleTransfer(&AxiDmaRx, (UINTPTR)rxBuffer, 4*BUFFER_SIZE, XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS) {
		xil_printf("RX-DMA setup failed\r\n");
		return XST_FAILURE;
	} else xil_printf("RX-DMA setup completed\r\n");

	// 3b: Setup TX-DMA transaction
	status = XAxiDma_SimpleTransfer(&AxiDmaTx, (UINTPTR)txBuffer, BUFFER_SIZE, XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS) {
		xil_printf("TX-DMA setup failed\r\n");
		return XST_FAILURE;
	} else xil_printf("TX-DMA setup completed\r\n");

	// 3c: Wait for TX-DMA & RX-DMA to finish
	xil_printf("Waiting for TX (PS->PL)...\r\n");
	while (XAxiDma_Busy(&AxiDmaTx, XAXIDMA_DMA_TO_DEVICE));

	xil_printf("TX Done. Waiting for RX (PL->PS)...\r\n");
	while (XAxiDma_Busy(&AxiDmaRx, XAXIDMA_DEVICE_TO_DMA));

	xil_printf("All DMA Done!\r\n");
	XTime_GetTime(&postExecCyclesFPGA);

	xil_printf("FPGA processing completed\r\n");

	// ============================================================================
	// DEBUG: Print first 20 u32 words and compare formats
	// ============================================================================
	xil_printf("\n========== DEBUG OUTPUT ==========\n");
	xil_printf("First 20 u32 words from FPGA (hex):\n");
	for (int i = 0; i < 20; i++) {
		xil_printf("  rxBuffer[%2d] = 0x%08X\n", i, rxBuffer[i]);
	}

	xil_printf("Starting Software processing\r\n");

	uint8_t *bayer = input_raw;
	//uint8_t r[BUFFER_SIZE], g[BUFFER_SIZE], b[BUFFER_SIZE];

	XTime_GetTime(&preExecCyclesSW);
	// Step 5: Perform SW processing - Demosaic and fill R, G, B arrays
	demosaic_bayer_gbrg(bayer, r, g, b, DIMENSION);
	XTime_GetTime(&postExecCyclesSW);

	xil_printf("Debayering completed\r\n");

	//---------------------------- VERIFICATION -------------------------------
	int correct = 0;
	int wrong = 0;
	uint32_t pixel_idx = 0;
	uint32_t rx_buffer_idx = 0;
	uint32_t total_buffer_words = BUFFER_SIZE * WORDS_PER_PIXEL;
	uint8_t R_HW, G_HW, B_HW, R_SW, G_SW, B_SW;

	xil_printf("\nStarting verification:\n");

	// Verification loop
	while (pixel_idx < BUFFER_SIZE && rx_buffer_idx < total_buffer_words) {
		R_HW = (rxBuffer[rx_buffer_idx] >> 16) & 0xFF;
		G_HW= (rxBuffer[rx_buffer_idx] >> 8) & 0xFF;
		B_HW = (rxBuffer[rx_buffer_idx] >> 0) & 0xFF;

		R_SW = r[pixel_idx];
		G_SW = g[pixel_idx];
		B_SW = b[pixel_idx];

		if (R_HW == R_SW && G_HW == G_SW && B_HW == B_SW) {
			correct++;
		} else {
			wrong++;
			if (wrong <= 10) {
				xil_printf("Mismatch at pixel %d: SW(R:%3d G:%3d B:%3d) != HW(R:%3d G:%3d B:%3d)\r\n",
						   pixel_idx, R_SW, G_SW, B_SW, R_HW, G_HW, B_HW);
			}
		}

		pixel_idx++;
		rx_buffer_idx += WORDS_PER_PIXEL;
	}

	if (wrong > 10) {
		xil_printf("(Additional %d mismatches not shown)\r\n", wrong - 10);
	}

	xil_printf("\n========== VERIFICATION RESULTS ==========\n");
	xil_printf("Total pixels processed: %d\n", pixel_idx);
	xil_printf("Correct matches: %d\n", correct);
	xil_printf("Mismatches: %d\n", wrong);

	xil_printf("Match rate: %.2f%%\n", (float)correct / (float)pixel_idx * 100.0f);
	xil_printf("==========================================\n\n");

	// Report timing
	unsigned long long int fpga_cycles = postExecCyclesFPGA - preExecCyclesFPGA;
	printf("FPGA execution cycles: %llu\n", fpga_cycles);

	unsigned long long int sw_cycles = postExecCyclesSW - preExecCyclesSW;
	printf("SW execution cycles: %llu\n", sw_cycles);

	if(fpga_cycles != 0) {
	    unsigned long long int speedup = (sw_cycles * 100) / fpga_cycles;
	    printf("Speedup (SW/FPGA): %llu.%02llu x\n", speedup / 100, speedup % 100);
	} else {
	    printf("FPGA cycles = 0 (timing error)\n");
	}

	cleanup_platform();
	return 0;
}

