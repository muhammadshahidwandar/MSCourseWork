`timescale 1ns / 1ps
module testbnch;

	// Inputs
	reg [5:0] X;
	reg [5:0] Y;

	// Outputs
	wire [11:0] prdct;

	// Instantiate the Unit Under Test (UUT)
	multplr uut (
		.X(X), 
		.Y(Y), 
		.prdct(prdct)
	);
	integer i;
	initial begin
       for(i=0;i<2**10;i=i+1) begin
         X = {1'b0,i[4:0]};      ///// to make unsigned multiplication using * operator sign bit should be 0
         Y = {1'b0,i[9:5]};
         #5 if(prdct!=X*Y)
           $display("Error has occurred during calculation, x=%d, y=%d, prdct=%d",X,Y,prdct);
          end
        $stop;

	end
      
endmodule