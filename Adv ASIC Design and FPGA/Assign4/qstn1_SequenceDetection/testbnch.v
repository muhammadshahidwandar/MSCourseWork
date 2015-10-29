`timescale 1ns / 1ps

////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer:
//
// Create Date:   11:35:27 05/30/2013
// Design Name:   sqncdetctr
// Module Name:   D:/Xilinx_ISE_Pro/maju_ASIC_FPGA/assign4/seqncdetect/testbnch.v
// Project Name:  seqncdetect
// Target Device:  
// Tool versions:  
// Description: 
//
// Verilog Test Fixture created by ISE for module: sqncdetctr
//
// Dependencies:
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
////////////////////////////////////////////////////////////////////////////////

module testbnch;

	// Inputs
	reg in;
	reg clk;
	reg rst;

	// Outputs
	wire detctd;

	// Instantiate the Unit Under Test (UUT)
	sqncdetctr uut (
		.in(in), 
		.clk(clk), 
		.rst(rst), 
		.detctd(detctd)
	);
	localparam T=20;
	
	always begin
	clk=0;
	#(T/2);
	clk=1;
	#(T/2);
	end

	initial begin
		// Initialize Inputs
		in = 0;
		clk = 0;
		rst = 1;
		#40;
		 rst = 0;

		// Wait 100 ns for global reset to finish
		#20;
		in = 1;
		#20;
		 in = 0;
		#20;
		  in = 1;
		#20;
		   in = 1;
		#20;
		   in = 1;
		#20;
		   in = 1; 
      #20;
		 in = 0;
		#20;
		  in = 1;
		#20;
		   in = 1;
		#20;
		   in = 1;
		#20;
		   in = 1;  			
		// Add stimulus here

	end
      
endmodule

