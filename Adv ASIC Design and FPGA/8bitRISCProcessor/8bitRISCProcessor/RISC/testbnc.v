`timescale 1ns / 1ps

////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer:
//
// Create Date:   11:14:56 06/09/2013
// Design Name:   Main
// Module Name:   D:/Xilinx_ISE_Pro/maju_ASIC_FPGA/Singl_Complt/main_Unit/testbnc.v
// Project Name:  main_Unit
// Target Device:  
// Tool versions:  
// Description: 
//
// Verilog Test Fixture created by ISE for module: Main
//
// Dependencies:
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
////////////////////////////////////////////////////////////////////////////////

module testbnc;

	// Inputs
	reg clk;
	reg rst;

	// Outputs
	wire [7:0] dout;

	// Instantiate the Unit Under Test (UUT)
	Main uut (
		.clk(clk), 
		.rst(rst), 
		.dout(dout)
	);
localparam T = 20;
always begin
clk = 0;
#(T/2);
clk = 1;
#(T/2);  end
	initial begin
		// Initialize Inputs
		
		rst = 1;

		// Wait 100 ns for global reset to finish
		#100;
		rst =0;
        
		// Add stimulus here

	end
      
endmodule

