`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    22:10:28 06/11/2013 
// Design Name: 
// Module Name:    clkDevider 
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
module clkDevider(input clkin,rst,output reg clkout);
wire tick;
ModmCountr Counter(
    .clk(clkin),.reset(rst),
    .max_tick(tick),
     .q()
   );
	always @(posedge clkin, posedge rst)
	if(rst)
	    clkout <= 0;
	else if(tick)
	     clkout <= ~clkout ;
endmodule
