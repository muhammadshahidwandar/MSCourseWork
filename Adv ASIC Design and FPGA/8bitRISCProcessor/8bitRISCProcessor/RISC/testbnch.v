`timescale 1ns / 1ps

////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer:
//
// Create Date:   15:24:43 06/12/2013
// Design Name:   TopModule
// Module Name:   D:/Xilinx_ISE_Pro/maju_ASIC_FPGA/SINGLECycle/main_Unit/testbnch.v
// Project Name:  main_Unit
// Target Device:  
// Tool versions:  
// Description: 
//
// Verilog Test Fixture created by ISE for module: TopModule
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
	reg clk;
	reg rst;

	// Outputs
	wire AN0;
	wire AN1;
	wire AN2;
	wire AN3;
	wire CA;
	wire CB;
	wire CC;
	wire CD;
	wire CE;
	wire CF;
	wire CG;
	wire CDP;
	wire clktick;

	// Instantiate the Unit Under Test (UUT)
	TopModule uut (
		.clk(clk), 
		.rst(rst), 
		.AN0(AN0), 
		.AN1(AN1), 
		.AN2(AN2), 
		.AN3(AN3), 
		.CA(CA), 
		.CB(CB), 
		.CC(CC), 
		.CD(CD), 
		.CE(CE), 
		.CF(CF), 
		.CG(CG), 
		.CDP(CDP), 
		.clktick(clktick)
	);
	localparam T =20;
	always begin
	clk = 0;
	#(T/2);
	clk=1;
	#(T/2);
	  end

	initial begin
		// Initialize Inputs
		rst = 1;

		// Wait 100 ns for global reset to finish
		#100;
		rst = 0;
		#600;
		rst = 1;
		#20;
		rst = 0;
		
        
		// Add stimulus here

	end
      
endmodule

