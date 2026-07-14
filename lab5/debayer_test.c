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

#define IMG_WIDTH 1024
#define IMG_HEIGHT 1024
#define BUFFER_SIZE (IMG_WIDTH * IMG_HEIGHT)

#define WIDTH  1024
#define HEIGHT 1024

// FPGA outputs 16 bytes per pixel = 4 u32 words per pixel
#define WORDS_PER_PIXEL 4
#define BYTES_PER_PIXEL 16

/* User application global variables & defines */
XAxiDma AxiDmaTx;
XAxiDma AxiDmaRx;

static u8* txBuffer = (u8*) TX_BUFFER;
static u32* rxBuffer = (u32*) RX_BUFFER;
int status;
XAxiDma_Config *CfgPtr;

static u8 r[WIDTH * HEIGHT];
static u8 g[WIDTH * HEIGHT];
static u8 b[WIDTH * HEIGHT];
static u8 input_raw[BUFFER_SIZE] = {
#include "image1.txt"
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

// Helper function to access pixels with boundary checks
static inline int get_pixel(const uint8_t *bayer, int width, int height, int x, int y) {
	if (x < 0 || x >= width || y < 0 || y >= height)
		return 0;
	return bayer[y * width + x];
}

void demosaic_bayer_gbrg(uint8_t *bayer, uint8_t *r, uint8_t *g, uint8_t *b,
                         int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            uint8_t R = 0, G = 0, B = 0;

            int evenRow = (y % 2) == 0;
            int evenCol = (x % 2) == 0;

            if (!evenRow && !evenCol) {
                G = get_pixel(bayer, width, height, x, y);
                R = (get_pixel(bayer, width, height, x-1, y) + get_pixel(bayer, width, height, x+1, y)) / 2;
                B = (get_pixel(bayer, width, height, x, y-1) + get_pixel(bayer, width, height, x, y+1)) / 2;
            }
            else if (evenRow && evenCol) {
                G = get_pixel(bayer, width, height, x, y);
                R = (get_pixel(bayer, width, height, x, y-1) + get_pixel(bayer, width, height, x, y+1)) / 2;
                B = (get_pixel(bayer, width, height, x-1, y) + get_pixel(bayer, width, height, x+1, y)) / 2;
            }
            else if (!evenRow && evenCol) {
                R = get_pixel(bayer, width, height, x, y);
                G = (get_pixel(bayer, width, height, x, y-1) +
                     get_pixel(bayer, width, height, x+1, y) +
                     get_pixel(bayer, width, height, x, y+1) +
                     get_pixel(bayer, width, height, x-1, y)) / 4;
                B = (get_pixel(bayer, width, height, x-1, y-1) +
                     get_pixel(bayer, width, height, x+1, y-1) +
                     get_pixel(bayer, width, height, x+1, y+1) +
                     get_pixel(bayer, width, height, x-1, y+1)) / 4;
            }
            else {
                B = get_pixel(bayer, width, height, x, y);
                G = (get_pixel(bayer, width, height, x, y-1) +
                     get_pixel(bayer, width, height, x+1, y) +
                     get_pixel(bayer, width, height, x, y+1) +
                     get_pixel(bayer, width, height, x-1, y)) / 4;
                R = (get_pixel(bayer, width, height, x-1, y-1) +
                     get_pixel(bayer, width, height, x+1, y-1) +
                     get_pixel(bayer, width, height, x+1, y+1) +
                     get_pixel(bayer, width, height, x-1, y+1)) / 4;
            }

            r[idx] = R;
            g[idx] = G;
            b[idx] = B;
        }
    }
}

int main()
{
	Xil_DCacheDisable();

	unsigned long long int  preExecCyclesFPGA = 0;
	unsigned long long int  postExecCyclesFPGA = 0;
	unsigned long long int  preExecCyclesSW = 0;
	unsigned long long int  postExecCyclesSW = 0;

	print("HELLO 1 - DEBAYERING FILTER (FIXED VERSION)\r\n");

	init_platform();

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

	// Input defined here
	for (u32 i = 0; i < BUFFER_SIZE; i++) {
		//input_raw[i] = rand() % 256;
	    txBuffer[i] = input_raw[i];
	}

	xil_printf("Starting FPGA processing via DMA...\r\n");
	xil_printf("Transfer size: %d bytes (%d pixels x %d bytes/pixel)\r\n",
		4*BUFFER_SIZE, BUFFER_SIZE, BYTES_PER_PIXEL);

	// Step 3 : Perform FPGA processing
	XTime_GetTime(&preExecCyclesFPGA);

	// 3a: Setup RX-DMA transaction
	status = XAxiDma_SimpleTransfer(&AxiDmaRx, (UINTPTR)rxBuffer, 4*BUFFER_SIZE, XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS) {
		xil_printf("RX-DMA setup failed\r\n");
		return XST_FAILURE;
	}

	// 3b: Setup TX-DMA transaction
	status = XAxiDma_SimpleTransfer(&AxiDmaTx, (UINTPTR)txBuffer, BUFFER_SIZE, XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS) {
		xil_printf("TX-DMA setup failed\r\n");
		return XST_FAILURE;
	}

	// 3c: Wait for TX-DMA & RX-DMA to finish
	while (XAxiDma_Busy(&AxiDmaTx, XAXIDMA_DMA_TO_DEVICE));
	while (XAxiDma_Busy(&AxiDmaRx, XAXIDMA_DEVICE_TO_DMA));
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
	int width = WIDTH, height = HEIGHT;

	XTime_GetTime(&preExecCyclesSW);
	// Step 5: Perform SW processing - Demosaic and fill R, G, B arrays
	demosaic_bayer_gbrg(bayer, r, g, b, width, height);
	XTime_GetTime(&postExecCyclesSW);

	xil_printf("Demosaicing completed\r\n");

	// ============================================================================
	// FIXED: Verification loop with correct increment (4 words per pixel)
	// ============================================================================
	int correct = 0;
	int wrong = 0;
	u32 pixel_idx = 0;
	u32 word_idx = 0;
	u32 total_words = BUFFER_SIZE * WORDS_PER_PIXEL;
	u8 r_hw, g_hw, b_hw, r_sw, g_sw, b_sw;

	xil_printf("\nStarting verification (FIXED algorithm):\n");

	// Verification loop - FIX: Use 4 words per pixel
	while (pixel_idx < BUFFER_SIZE && word_idx < total_words) {
		u32 word_r = (rxBuffer[word_idx] >> 16) & 0xFF;
		u32 word_g = (rxBuffer[word_idx] >> 8) & 0xFF;
		u32 word_b = (rxBuffer[word_idx] >> 0) & 0xFF;

		// Extract RGB values (assuming each word contains value in LSB)
		r_hw = word_r & 0xFF;
		g_hw = word_g & 0xFF;
		b_hw = word_b & 0xFF;

		// Get software reference values
		r_sw = r[pixel_idx];
		g_sw = g[pixel_idx];
		b_sw = b[pixel_idx];

		// Compare
		if (r_hw == r_sw && g_hw == g_sw && b_hw == b_sw) {
			correct++;
		} else {
			wrong++;
			// Only print first 10 mismatches to avoid flooding output
			if (wrong <= 10) {
				xil_printf("Mismatch at pixel %d: SW(R:%3d G:%3d B:%3d) != HW(R:%3d G:%3d B:%3d)\r\n",
						   pixel_idx, r_sw, g_sw, b_sw, r_hw, g_hw, b_hw);
			}
		}

		pixel_idx++;
		word_idx++;
	}

	if (wrong > 10) {
		xil_printf("(Additional %d mismatches not shown)\r\n", wrong - 10);
	}

	xil_printf("\n========== VERIFICATION RESULTS ==========\n");
	xil_printf("Total pixels processed: %d\n", pixel_idx);
	xil_printf("Correct matches: %d\n", correct);
	xil_printf("Mismatches: %d\n", wrong);

	float peepee = (float)correct / (float)pixel_idx * 100.0f;
	int AAAAA  = (int) peepee;
	int fuckThePolice = (int)(peepee * 100.0);
	xil_printf("Match rate: %d.%f%%\n", AAAAA, fuckThePolice);
	xil_printf("==========================================\n\n");

	// Report timing
	unsigned long long int fpga_cycles = postExecCyclesFPGA - preExecCyclesFPGA;
	printf("FPGA execution cycles: %llu\n", fpga_cycles);

	unsigned long long int sw_cycles = postExecCyclesSW - preExecCyclesSW;
	printf("SW execution cycles: %llu\n", sw_cycles);

	if(fpga_cycles != 0) {
		printf("Speedup (SW/FPGA): %0.2lf x\n", (double) sw_cycles / (double) fpga_cycles);
	} else {
		printf("FPGA cycles = 0 (timing error)\n");
	}

	cleanup_platform();
	return 0;
}
