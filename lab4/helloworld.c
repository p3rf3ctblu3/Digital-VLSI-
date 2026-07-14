#include <stdio.h>
#include <stdint.h>
#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xil_io.h"

#define FIR_BASE XPAR_FIR_AXI_0_S00_AXI_BASEADDR
#define REG_IO_0 FIR_BASE // slv_reg0: [valid_in, x, rst]
#define REG_IO_1 FIR_BASE + 0x4 // slv_reg1: [y_out, valid_out]

int main() {

    uint8_t x_input[] = {
        208, 231, 32, 233, 161, 24, 71, 140, 245, 247,
        40, 248//, 245, 124, 204, 36, 107, 234, 202, 245,
        //0, 0, 0, 0, 0, 0, 0, 0
    };

    int num_samples = sizeof(x_input) / sizeof(x_input[0]);

    init_platform();

    Xil_Out32(REG_IO_0, (1 << 9));
    Xil_Out32(REG_IO_0, (0 << 9));

    uint8_t x;
    xil_printf("Starting FIR processing...\r\n");

    for (int sample = 0; sample < num_samples; sample++) {

        x = x_input[sample];

    	// edge detection in axi prevents from giving fir the same input twice (valid_in lasts one clk cycle)
        Xil_Out32(REG_IO_0, (1 << 8) | (x & 0xFF));
        Xil_Out32(REG_IO_0, (0 << 8) | (x & 0xFF));

        uint32_t y_raw;
        do {
        	y_raw = Xil_In32(REG_IO_1);
        } while (!(y_raw & (1 << 19)));

        Xil_Out32(REG_IO_0, (0 << 9));

        int32_t y = y_raw & 0x7FFFF;
        xil_printf("y[%d]: %d\r\n", sample, y);
    }

    xil_printf("Finished FIR processing.\r\n");
    cleanup_platform();
    return 0;
}
