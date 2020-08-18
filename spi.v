// SPI receiver module, written by Kerry Wang, Summer 2020

module SPI_receive
(
	input wire SCLK, SS, MOSI,
	output wire MISO,
	output wire [31:0] byteRx
);
	reg out;
	reg [31:0] sr; // internal shift register used for receiving
	
	always @ (posedge SCLK)
	begin
		if (SS == 1'b0) // if chip is selected and clock is pulled high
		begin
			out = sr[31]; // catch outgoing bit before it shifts off the edge
			sr[31:1] = sr[30:0]; // shift everything over by one
			sr[0] = MOSI; // get bit from master
		end
	end

	assign MISO = SS ? 1'bz : out; //Set MISO to high-z when chip is not selected; send outgoing bit when chip is selected
	assign byteRx = SS ? sr : 8'bx; //Set output to be unknown while transmission is not finished
endmodule
