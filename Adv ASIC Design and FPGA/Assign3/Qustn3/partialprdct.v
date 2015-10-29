`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    08:37:34 05/06/2013 
// Design Name: 
// Module Name:    partialprdct 
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
module partialprdct(input [5:0]A,B,output [5:0] p1,p2,p3,p4,p5,p6
    );
assign p1 = A&{6{B[0]}},
       p2 = A&{6{B[1]}},
		 p3 = A&{6{B[2]}},
		 p4 = A&{6{B[3]}},
		 p5 = A&{6{B[4]}},
		 p6 = A&{6{B[5]}};

endmodule
