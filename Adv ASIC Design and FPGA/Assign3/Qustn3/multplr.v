`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    16:30:25 05/05/2013 
// Design Name: 
// Module Name:    multplr 
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
module multplr(input [5:0]X,Y, output reg [11:0] prdct );

wire [5:0] p1,p2,p3,p4,p5,p6;
partialprdct ppdct(X,Y,p1,p2,p3,p4,p5,p6);
/////////////////L0ha1s(lavel zero half adder 1 sum)
reg L0ha1s,L0ha1c,L0fa1s,L0fa1c,L0fa2s,L0fa2c,L0fa3s,L0fa3c,L0fa4s,L0fa4c,L0ha2s,L0ha2c;	 

/////////////////L1ha1s(lavel 1 half adder 1 sum)
reg L1ha1s,L1ha1c,L1fa1s,L1fa1c,L1fa2s,L1fa2c,L1fa3s,L1fa3c,L1fa4s,L1fa4c,L1ha2s,L1ha2c;

/////////////////L2ha1s(lavel 2 half adder 1 sum)
reg L2ha1s,L2ha1c,L2fa1s,L2fa1c,L2fa2s,L2fa2c,L2fa3s,L2fa3c,L2fa4s,L2fa4c,L2fa5s,L2fa5c;

/////////////////L3ha1s(lavel 3 half adder 1 sum)
reg L3ha1s,L3ha1c,L3ha2s,L3ha2c,L3fa1s,L3fa1c,L3fa2s,L3fa2c,L3fa3s,L3fa3c,L3ha3s,L3ha3c,L3ha4s,L3ha4c,L3ha5s,L3ha5c;

////////////////////ripple carry signals//////////
reg c1,c2,c3,c4,c5,c6,c7,c8;	 

///////////////////////Half Adders and Full Adders Layers//////////////////////////	 
////////////////////LAYER 0 adders///////////////////
always @* begin
hfaddr(p1[1],p2[0],L0ha1s,L0ha1c);

fuladdr( p1[2],p2[1],p3[0],L0fa1s,L0fa1c);
fuladdr( p1[3],p2[2],p3[1],L0fa2s,L0fa2c);
fuladdr( p1[4],p2[3],p3[2],L0fa3s,L0fa3c);
fuladdr(p1[5],p2[4],p3[3],L0fa4s,L0fa4c);

hfaddr(p2[5],p3[4],L0ha2s,L0ha2c);
////////////////////LAYER 1 adders///////////////////
hfaddr(p4[1],p5[0], L1ha1s,L1ha1c);

fuladdr( p4[2],p5[1],p6[0],L1fa1s,L1fa1c);
fuladdr( p4[3],p5[2],p6[1],L1fa2s,L1fa2c);
fuladdr( p4[4],p5[3],p6[2],L1fa3s,L1fa3c);
fuladdr( p4[5],p5[4],p6[3],L1fa4s,L1fa4c);

hfaddr(p5[5],p6[4],L1ha2s,L1ha2c);
////////////////////LAYER 2 adders///////////////////
hfaddr(L0ha1c,L0fa1s, L2ha1s,L2ha1c);

fuladdr( L0fa1c,L0fa2s,p4[0],L2fa1s,L2fa1c);
fuladdr( L0fa2c,L1ha1s,L0fa3s,L2fa2s,L2fa2c);
fuladdr( L0fa3c,L1fa1s,L0fa4s,L2fa3s,L2fa3c);
fuladdr( L0fa4c,L1fa2s,L0ha2s,L2fa4s,L2fa4c);
fuladdr( L0ha2c,L1fa3s,p3[5],L2fa5s,L2fa5c);

////////////////////LAYER 3 adders///////////////////
hfaddr(L2ha1c,L2fa1s, L3ha1s,L3ha1c);
hfaddr(L2fa1c,L2fa2s, L3ha2s,L3ha2c);

fuladdr( L2fa2c,L1ha1c,L2fa3s,L3fa1s,L3fa1c);
fuladdr( L2fa3c,L1fa1c,L2fa4s,L3fa2s,L3fa2c);
fuladdr( L2fa4c,L1fa2c,L2fa5s,L3fa3s,L3fa3c);

hfaddr(L2fa5c,L1fa4s, L3ha3s,L3ha3c);
hfaddr(L1fa4c,L1ha2s, L3ha4s,L3ha4c);
hfaddr(L1ha2c,p6[5], L3ha5s,L3ha5c);
//////Finally Carry propagate Adder//////////////////
 prdct[0] = p1[0];
 prdct[1] = L0ha1s;
 prdct[2] = L2ha1s;
 prdct[3] = L3ha1s;
 hfaddr(L3ha1c,L3ha2s,prdct[4],c1);
 
 fuladdr( c1,L3ha2c,L3fa1s,prdct[5],c2);
 fuladdr( c2,L3fa1c,L3fa2s,prdct[6],c3);
 fuladdr( c3,L3fa2c,L3fa3s,prdct[7],c4);
 fuladdr( c4,L3fa3c,L3ha3s,prdct[8],c5);
 fuladdr( c5,L3ha3c,L3ha4s,prdct[9],c6);
 fuladdr( c6,L3ha4c,L3ha5s,prdct[10],c7);
 hfaddr( c7,L3ha5c,prdct[11],c8);
end
/////////////////////half adder task////////////////////////	 
task hfaddr(input a,b,output x,y);
begin
       x=a^b;
       y=a&b;
		 end
endtask

/////////////////////Full Adder adder task////////////////////////
task fuladdr(input a,b,c,output x,y);
reg w1;
begin

       w1 =(a^b);
       x = w1^c;
       y =(w1&c)|(a&b);
end
endtask	 
	 
	 

endmodule








