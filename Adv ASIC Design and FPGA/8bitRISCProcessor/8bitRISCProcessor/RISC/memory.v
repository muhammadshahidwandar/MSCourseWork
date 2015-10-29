`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    07:35:57 06/02/2013 
// Design Name: 
// Module Name:    memory 
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
module ram (input clk, rw,input [7:0] addr,input [7:0] din, output [7:0] dout);
reg [7:0] mem [0:255];
always @ (posedge clk)begin
if (rw) 
mem [addr] <= din;  end
assign dout = mem[addr];
endmodule
