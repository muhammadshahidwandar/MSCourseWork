`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    09:31:25 06/09/2013 
// Design Name: 
// Module Name:    mux4bit 
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
module mux4bit(input [3:0] in1, in2, input sel, output [3:0] out
    );	 
assign out = sel? in2:in1;
endmodule
