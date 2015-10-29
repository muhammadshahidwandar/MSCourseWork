`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    07:23:47 06/04/2013 
// Design Name: 
// Module Name:    mux8bit 
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
module mux8bit(input [7:0] in1,in2,output [7:0] out, input sel
    );
assign out = sel? in2:in1;

endmodule
