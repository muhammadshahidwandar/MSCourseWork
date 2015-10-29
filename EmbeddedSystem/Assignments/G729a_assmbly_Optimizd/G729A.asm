
;Word16 add(Word16 var1,Word16 var2)
;  {
;   Word16 var_out;
;   Word32 L_somme;
;
;   L_somme = (Word32) var1 + var2;
;   var_out = sature(L_somme);
;   return(var_out);
;  }

        .global _asmADD16bit
        .text

_asmADD16bit:

;    mv.L1 A4, A6               ; 	a6 = a4 ---> d[][]
;||	mv.S1 A10, A24             ;	Save A10,A12,A14 and restore
;	mv.L1 A12, A26             ;   later
;||	mv.S1 A14, A28             ;
;	nop 2					;

	add.L1	A4,B4,A4		;

;	nop 2
;	mv.L1 A24, A10          ;	Restore A10,A12,A14 
;||	mv.S1 A26, A12          ;   
;	mv A28, A14             ;

	b b3
    nop 5                   ; return        
    
;    	.end


;void Copy(
;  Word16 x[],      /* (i)   : input vector   */
;  Word16 y[],      /* (o)   : output vector  */
;  Word16 L         /* (i)   : vector length  */
;)
;{
;   Word16 i;
;
;   for (i = 0; i < L; i++)
;     y[i] = x[i];
;
;   return;
;}

        .global _asmCopy
        .text

_asmCopy:

	mv A6, A1

COPYLOOP:
	LDH .D1   *A4++,A8 ; load ai
	NOP     4
	STH.D2 	A8 ,*B4++		;	
	SUB .S1   A1,1,A1 ; dec loop count

	[A1] B .S2 COPYLOOP ; branch to start
	NOP 5
	
	b b3
    nop 5                   ; return
  ;//////////////////////////////////RESIDU Function///////////////// 
 ; void Residu(
 ; Word16 a[],    /* (i) Q12 : prediction coefficients                     */
;  Word16 x[],    /* (i)     : speech (values x[-m..-1] are needed         */
;  Word16 y[],    /* (o)     : residual signal                             */
;  Word16 lg      /* (i)     : size of filtering                           */
;)
;{
;  Word16 i, j;
;  Word32 s;
;  /*  FUNCTION MODIFIED */

 ; for (i = lg-1; i <=0; i--)
 ; {
 ;    s=0;
 ;   //s = L_mult(x[i], a[0]);
  ;//	s = ((Word32)x[i]* (Word32)a[0])<<1;
 ;   for (j = M; j <= 0; j--)
;      //s = L_mac(s, a[j], x[i-j]);
;	  s += ((Word32)a[j] * (Word32)x[i-j])<<1;

;   // s = L_shl(s, 3);
;	s = s << 3;
 ;   //y[i] = round(s);
;	y[i] = (Word16)(( s + (Word32)0x8000 )>>16);
;  }
;  return;
;} 
    
    .global _asmResidu
    .text

_asmResidu:
           
		   MV.L1 B6,A0     ;lg i initialization
		   SUB.L1 A0, 1, A0     ; i initialization by lg-1
           MVK.S1 10, A1  ;M = 10; loop count J
           ADD.L1 A0,A4,A4
           ADD.L2 B0,B4,B4 

RESIDULOOP1:
           ZERO.L1 A8     ;sum = 8
RESIDULOOP2:
             LDH .D1 *A4--,A2 ; load a[J]
             LDH .D2 *B4++,B5 ; load x[i-j]
			 NOP 4
			 MPY .M1 A2,B5,A6 ; a[j] * x[i-j];
			 ADD .L1 A6,A8,A8 ; sum += (a[j] * x[i-j]
			 SUB .S2 B0,1,B0 ; dec loop count j--
			 [A0] B .S2 RESIDULOOP2 ; branch to star
             NOP 5
			 SHL.S1 A8, 3, A8
			 SUB.S1 A0,1,A0 ; dec loop count i-- 
			 [A0] B .S2 RESIDULOOP1 ; branch to star
			 NOP 5 
             MV A8,A4 ;result returned in A4 
             B B3 ;return addr to calling routine 
             NOP 4                              
    
    	.end


