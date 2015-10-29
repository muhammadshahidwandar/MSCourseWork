`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    20:35:24 06/11/2013 
// Design Name: 
// Module Name:    SevenSeg 
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
module SevenSegDec (clock,DataIn, AN0, AN1, AN2, AN3, CA, CB, CC, CD, CE, CF, CG, CDP);

input [15:0] DataIn;  // = 16'h9ABC;
//USED FOR SEVEN SEG
input clock;

output AN0, AN1, AN2, AN3, CA, CB, CC, CD, CE, CF, CG, CDP;

reg [7:0] cathodedata; //cathode data
reg [3:0] anodedata; //anode data
reg [2:0] digit = 1;
reg [3:0] data;
reg setdp;
reg [19:0] counter = 0;

assign CA = cathodedata [7];
assign CB = cathodedata [6];
assign CC = cathodedata [5];
assign CD = cathodedata [4];
assign CE = cathodedata [3];
assign CF = cathodedata [2];
assign CG = cathodedata [1];
assign CDP = cathodedata [0];
assign AN3 = anodedata [3];
assign AN2 = anodedata [2];
assign AN1 = anodedata [1];
assign AN0 = anodedata [0];
//USED FOR SEVEN SEG

    //Multiplexing
    //Board Clock: 50MHz
    //p = t*f
    //t = 16ms
    //p = 16ms * 50*10^6 = 800,000 cycles
    //200,000 cycles for each digit
    //Refreshed every 16ms (~60Hz)

always@(negedge clock)
begin
    if (digit == 1)
        begin
            if (counter == 200_000)
                begin
                    digit = 2;
                end
            else
                begin
                counter = counter + 1;
                data = DataIn[15:12];
                end
        end
    else if (digit == 2)
        begin
            if (counter == 400_000)
                begin
                    digit = 3;
                end
            else
                begin
                    counter = counter + 1;
                    data = DataIn[11:8];
                end
        end
    else if (digit == 3)
        begin
            if (counter == 600_000)
                begin
                    digit = 4;
                end
            else
                begin
                    counter = counter + 1;
                    data = DataIn[7:4];
                end
        end
    else if (digit == 4)
        begin
            if (counter == 800_000)
                begin
                    digit = 1;
                    counter = 0;
                end 
            else
                begin
                    counter = counter + 1;
                    data = DataIn[3:0];
                end
        end 
end


always @ (*)
begin

    case (data)
        4'd0: cathodedata = 8'b00000011; //0
        4'd1: cathodedata = 8'b10011111; //1
        4'd2: cathodedata = 8'b00100101; //2
        4'd3: cathodedata = 8'b00001101; //3
        4'd4: cathodedata = 8'b10011001; //4
        4'd5: cathodedata = 8'b01001001; //5
        4'd6: cathodedata = 8'b01000001; //6
        4'd7: cathodedata = 8'b00011111; //7
        4'd8: cathodedata = 8'b00000001; //8
        4'd9: cathodedata = 8'b00001001; //9
        4'd10: cathodedata = 8'b00010001; //A
        4'd11: cathodedata = 8'b11000001; //B
        4'd12: cathodedata = 8'b01100011; //C
        4'd13: cathodedata = 8'b10000101; //D
        4'd14: cathodedata = 8'b00100001; //E
        4'd15: cathodedata = 8'b01110001; //F
        default: cathodedata = 8'b11111111; //default all off
    endcase

    if (setdp == 1) //decimal point
        cathodedata = cathodedata & 8'hFE;

    case(digit)
        0: anodedata = 4'b1111; //all OFF
        4: anodedata = 4'b1110; //AN0
        3: anodedata = 4'b1101; //AN1
        2: anodedata = 4'b1011; //AN2
        1: anodedata = 4'b0111; //AN3
        default:
        anodedata = 4'b1111; //all OFF
    endcase

end 
endmodule