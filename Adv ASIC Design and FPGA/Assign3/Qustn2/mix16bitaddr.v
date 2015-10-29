`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    12:56:37 05/04/2013 
// Design Name: 
// Module Name:    mix16bitaddr 
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
module mix16bitaddr(input [15:0] A,B,input Cin,output [15:0] Sum,output Cout
    );
wire c1,c2;
cond_sum CndSum(A[3:0],B[3:0],Cin,Sum[3:0],c1);
	 
 ClaA CLaA(A[7:4],B[7:4],c1,Sum[7:4],c2);
 
CSAdder CrySlctAdr(A[15:8],B[15:8],c2,Sum[15:8],Cout);
	 
endmodule
