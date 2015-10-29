`timescale 1ns / 1ps

module testbnc;

	// Inputs
	reg [3:0] X;
	reg [3:0] Y;
	reg Cin;

	// Outputs
	wire [3:0] Sum;
	wire C_out;

	// Instantiate the Unit Under Test (UUT)
	cond_sum uut (
		.X(X), 
		.Y(Y), 
		.Cin(Cin), 
		.Sum(Sum), 
		.C_out(C_out)
	);
	integer i;

	initial begin
		for(i=0;i<=2**8;i=i+1)begin
		{X,Y,Cin}=i;
		#5 $display("X=%d, Y=%d, Cin=%d, Carryout=%d, SUM=%d",X,Y,Cin,C_out,Sum);
		end
		
	end
      
endmodule

