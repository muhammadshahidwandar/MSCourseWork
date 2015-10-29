`timescale 1ns / 1ps

////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer:
//
// Create Date:   18:36:01 05/30/2013
// Design Name:   fsm_mulplier
// Module Name:   D:/Xilinx_ISE_Pro/maju_ASIC_FPGA/assign4/shardmulty/testbn.v
// Project Name:  shardmulty
// Target Device:  
// Tool versions:  
// Description: 
//
// Verilog Test Fixture created by ISE for module: fsm_mulplier
//
// Dependencies:
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
////////////////////////////////////////////////////////////////////////////////

module testbn;

	// Inputs
	reg clk;
	reg rst;
	reg in_en;
	reg [7:0] in_a;
	reg [7:0] in_b;

	// Outputs
	wire [15:0] prdct;
	wire out_en;

	// Instantiate the Unit Under Test (UUT)
	fsm_mulplier uut (
		.clk(clk), 
		.rst(rst), 
		.in_en(in_en), 
		.a(in_a), 
		.b(in_b), 
		.prdct(prdct), 
		.out_en(out_en)
	);
localparam T =20;

always begin
   clk = 0;
	#(T/2);
	clk = 1;
	#(T/2);
	      end
	initial begin
		// Initialize Inputs
		rst = 1;
		in_en = 0;
		in_a = 0;
		in_b = 0;

		// Wait 100 ns for global reset to finish
		#70;
        rst = 0;
		#10
		in_en = 1'b1;
		in_a  = 8'b00000101 ;
		in_b  = 8'b00000101 ;
		#20
		in_en = 1'b0;
		// Add stimulus here

	end
      
endmodule

