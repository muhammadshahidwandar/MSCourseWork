`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    07:56:48 06/02/2013 
// Design Name: 
// Module Name:    rom 
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
module rom ( input [7:0] addr,output reg [15:0] dout );
always @ (addr)
case (addr)
8'b0000_0000: dout = 16'b0111_0000_0001_0111; ///addi r1,r0,#7;
8'b0000_0001: dout = 16'b1000_0001_0010_0010; //Subi r2,r1,#2 
8'b0000_0010: dout = 16'b1010_0000_0010_0000; //store [r0],r2
8'b0000_0011: dout = 16'b1001_0000_0011_0000; //load  r3 [r0]
8'b0000_0100: dout = 16'b0001_0001_0010_0100; //add r4,r1,r2
8'b0000_0101: dout = 16'b0010_0100_0010_0101;  //Sub r5,r4,r2
8'b0000_0110: dout = 16'b1100_0000_0101_0001;  //stori [1],$r5;
8'b0000_0111: dout = 16'b1011_0000_0110_0001;  //loadi r6,[1];
8'b0000_1000: dout = 16'b0101_0100_0111_0011;  //SHL r7, r4,#3 
8'b0000_1001: dout = 16'b0110_0100_1000_0010;  //SHR r8,r4,#2
8'b0000_1010: dout = 16'b0011_0100_0001_1001;  //AND R9, R4, R1;
8'b0000_1011: dout = 16'b0100_0100_0010_1010;  //OR R10, R4, R2;
8'b0000_1100: dout = 16'b1101_0110_0101_0111;  //Bre Jump R10, R4, R2;
8'b0000_1101: dout = 16'b0000_0000_0000_0000;  //Halt
default:      dout = 16'h0000;
endcase
endmodule
