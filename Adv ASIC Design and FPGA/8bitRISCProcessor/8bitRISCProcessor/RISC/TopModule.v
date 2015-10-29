`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    21:59:04 06/11/2013 
// Design Name: 
// Module Name:    TopModule 
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
module TopModule(input clk,rst,input [15:0] Swt_Inst,input swt_en,
output AN0, AN1, AN2, AN3, CA, CB, CC, CD, CE, CF, CG, CDP,clktick
    );
 wire clkSec;
 
 clkDevider SecClkGen(.clkin(clk),.rst(rst),.clkout(clkSec));
assign clktick=clkSec;
wire [15:0]SevenSeg;
	 
MainSingleCycle SingleCyclUnit(.clk(clkSec),.rst(rst),.Swt_Inst(Swt_Inst),.swt_en(swt_en),.dout1(SevenSeg[15:8]),.dout2(SevenSeg[7:0])
    );
	 
SevenSegDec  SevnSegDc(.clock(clk),.DataIn(SevenSeg),.AN0(AN0), .AN1(AN1), .AN2(AN2), .AN3(AN3), .CA(CA),
 .CB(CB), .CC(CC), .CD(CD), .CE(CE), .CF(CF), .CG(CG), .CDP(CDP));

endmodule
