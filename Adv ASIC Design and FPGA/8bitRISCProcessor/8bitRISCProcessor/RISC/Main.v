`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    17:15:05 06/07/2013 
// Design Name: 
// Module Name:    Main 
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
module MainSingleCycle(input clk,rst,input [15:0] Swt_Inst,input swt_en,output [7:0] dout1,dout2 
    );
wire [7:0] PCBranch_w,PCPlus1_w,PCprime_w;
wire PCSourse_w;
//////////////PC Mux////////////////////////////
mux8bit PC_MUX(.in1(PCPlus1_w),.in2(PCBranch_w),.out(PCprime_w), .sel(PCSourse_w));	

wire [7:0] PC_w; 
//////////////Program Counter//////////////////
PC_reg P_Counter(.clk(clk),.rst(rst),.pc_in(PCprime_w),.pc_out(PC_w),.Pc_pls1(PCPlus1_w));

wire [15:0] Instct_w,Instct_w1;
assign Instct_w = swt_en?Swt_Inst:Instct_w1;  /////
//////////////Instruction ROM///////////////
rom Ins_ROM(.addr(PC_w),.dout(Instct_w1));
wire [7:0] RD1_w,RD2_w;
wire [3:0] A1_w,A2_w,A3_w,AWrite_w;
wire [7:0] DataIn_w;
wire RegDst_w;
assign  A1_w = Instct_w[11:8],
        A2_w = Instct_w[7:4], 
		  A3_w = Instct_w[3:0];
		
/////////////////////////////////Write Adress Mux
mux4bit Wradd_MUX (.in1(A2_w),.in2(A3_w), .sel(RegDst_w),.out(AWrite_w)
    );	
wire RegWrite_w;
/////////////Register File//////////////
regfile Reg_File(.clk(clk),.we(RegWrite_w),.dataout1(RD1_w),.dataout2(RD2_w),.readadr1(A1_w),
.readadr2(A2_w),.writeadr(AWrite_w),.datain(DataIn_w));

wire [7:0] signExtnd_w,AluIn2_w;
wire ALUSrc_w;
assign signExtnd_w = {{4{A3_w[3]}},A3_w[3:0]};
assign  PCBranch_w = signExtnd_w+PCPlus1_w;
//////////////////////////ALU Source2 Mux
mux8bit Mux_Alu(.in1(RD2_w),.in2(signExtnd_w),.out(AluIn2_w), .sel(ALUSrc_w));

wire Alu_zero_flg_w;
wire [7:0] Alu_out_w;
wire [2:0] ALUControl_w;
////////////////ALU_Unit/////////////////////
Alu ALU_Unit(.Alu_in1(RD1_w),.Alu_in2(AluIn2_w),.Alu_sel(ALUControl_w),.Alu_zero_flg(Alu_zero_flg_w),
.Alu_out(Alu_out_w));
assign dout1 = Alu_out_w;

wire [7:0] RamDataOut_w;
wire MemWrite_w;
/////////////////////Memory /////////////////////
ram Data_memory(.clk(clk), .rw(MemWrite_w), .addr(Alu_out_w), .din(RD2_w), .dout(RamDataOut_w));

mux8bit MuxMemory(.in1(Alu_out_w),.in2(RamDataOut_w),.out(DataIn_w), .sel(MemtoReg_w));
assign dout2 = DataIn_w;
wire Branch_w;
///////////////////Control Unit////////////////////////////////
Control Control_Unit(.opcode(Instct_w[15:12]),.ALUControl(ALUControl_w),.RegWrite(RegWrite_w),.RegDst(RegDst_w),
.ALUSrc(ALUSrc_w),.Branch(Branch_w),.MemWrite(MemWrite_w),.MemtoReg(MemtoReg_w)
    );
assign PCSourse_w = Branch_w&Alu_zero_flg_w; 

endmodule
