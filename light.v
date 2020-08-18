// SPI receiver module, written by Kerry Wang, Summer 2020
// Receives 32-bit long integer via SPI and displays on the DE0's 4 7-segment displays
// Each digit has 7 sections plus the decimal point, enough to display one byte
module light
(
	// These names are provided by the DE0 template and are already set to the right pins, a different development board will need to change the pin assignments
	inout [31:28] GPIO0_D,
	output reg [9:0] LEDG,
	output reg [6:0] HEX0_D, HEX1_D, HEX2_D, HEX3_D,
	output reg HEX0_DP, HEX1_DP, HEX2_DP, HEX3_DP,
	output [31:0] byteRx
);
	wire SCLK, SS, MISO, MOSI;
	
	SPI_receive spi( // wire this module to the SPI module
		.SCLK  (SCLK),
		.SS (SS),
		.MISO (MISO),
		.MOSI (MOSI),
		.byteRx (byteRx)
	);
	
	always @ (posedge SS) // when SPI is finished transmitting, show the received long
	begin
			{HEX3_D, HEX3_DP, HEX2_D, HEX2_DP, HEX1_D, HEX1_DP, HEX0_D, HEX0_DP} <= ~byteRx;
	end
	
	assign GPIO0_D[31:28] = {1'bz, MISO, 2'bzz}; // wire this module to GPIO
	assign SCLK = GPIO0_D[31];
	assign MOSI = GPIO0_D[29];
	assign SS = GPIO0_D[28];
endmodule
