`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    10:59:52 05/04/2013 
// Design Name: 
// Module Name:    ClaA 
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
module ClaA(input [3:0] A,B,input Cin,output [3:0] Sum,output Cout
    );
wire p0,g0,p1,g1,p2,g2,p3,g3;
wire c1,c2,c3,c4;
assign p0= A[0]^B[0],
       g0=A[0]&B[0], p1= A[1]^B[1], g1=A[1]&B[1], p2= A[2]^B[2], g2=A[2]&B[2], p3= A[3]^B[3], g3=A[3]&B[3];
assign c1= g0|(p0&Cin),
       c2= g1|(p1&g0)|(p1&p0&Cin),
		 c3= g2|(p2&g1)|(p2&p1&g0)|(p2&p1&p0&Cin),
		 c4= g3|(p3&g2)|(p3&p2&g1)|(p3&p2&p1&g0)|(p3&p2&p1&p0&Cin);
	//////////outputs////////////////////////
assign Cout = c4,
       Sum  = {p3^c3,p2^c2,p1^c1,p0^Cin};
endmodule
