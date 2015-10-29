`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    12:16:39 05/04/2013 
// Design Name: 
// Module Name:    CSAdder 
// Project Name: 
// Target Devices: 
// Tool versions: 
// Description: 
//
// Dependencies: 
//
// Revision: 
// Revision 0.01 - File Created
// Additional Comments: 
//
//////////////////////////////////////////////////////////////////////////////////
module CSAdder(input [7:0] A,B,input Cin,output [7:0]Sum,output Cout
    );
wire [3:0]s01,s02,s11,s12;
wire c01,c02,c11,c12;
wire C_intr;


assign {c01,s01}= A[3:0]+B[3:0]+1'b0,
       {c02,s02}= A[3:0]+B[3:0]+1'b1,
		 {c11,s11}= A[7:4]+B[7:4]+1'b0,
		 {c12,s12}= A[7:4]+B[7:4]+1'b1;
assign C_intr=Cin?c02:c01,
       Cout  =C_intr?c12:c11;
assign Sum[3:0]=Cin?s02:s01,
       Sum[7:4]=C_intr?s12:s11;
endmodule
