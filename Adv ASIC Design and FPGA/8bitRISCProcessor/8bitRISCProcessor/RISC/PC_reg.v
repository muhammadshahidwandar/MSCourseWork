`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    07:10:55 06/04/2013 
// Design Name: 
// Module Name:    PC_reg 
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
module PC_reg(input clk,rst,input [7:0] pc_in,output [7:0] pc_out,output [7:0] Pc_pls1
    );
	 reg [7:0]pcReg;
	 assign pc_out = pcReg;
	always @(posedge clk,posedge rst)
	if(rst)
	   pcReg <= 8'b0;
	else 
      pcReg <= pc_in;
		
assign Pc_pls1 = pcReg+1;
endmodule
