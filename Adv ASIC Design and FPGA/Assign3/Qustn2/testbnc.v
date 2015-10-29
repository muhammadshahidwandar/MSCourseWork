`timescale 1ns / 1ps

////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer:
//
// Create Date:   13:16:49 05/04/2013
// Design Name:   mix16bitaddr
// Module Name:   D:/Xilinx_ISE_Pro/maju_ASIC_FPGA/assign3/mixAdderq2/testbnc.v
// Project Name:  mixAdderq2
// Target Device:  
// Tool versions:  
// Description: 
//
// Verilog Test Fixture created by ISE for module: mix16bitaddr
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
	reg [15:0] A;
	reg [15:0] B;
	reg Cin;

	// Outputs
	wire [15:0] Sum;
	wire Cout;

	// Instantiate the Unit Under Test (UUT)
	mix16bitaddr uut (
		.A(A), 
		.B(B), 
		.Cin(Cin), 
		.Sum(Sum), 
		.Cout(Cout)
	);
      integer i;
	initial begin
	  for(i=0;i<=2**17;i=i+1) //////2^17
	    begin
	        {A,B,Cin}=i;
	        #5 $monitor("A=%d, B=%d, CarryIn=%d, Carryout=%d,  Sum=%d",A,B,Cin,Cout,Sum);
	        
         end 
         $stop;end
       //initial begin
     //#1000;
  //    $stop;
  //end
endmodule

