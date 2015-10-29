there are two ways to give instruction to RISC processor
1. a simple machine program is stored in instruction memory.
 it runs by default.

2. to take 16 bit external instruction through switches make swt_en = 1 
   then instruction will be read from [15:0]Swt_Inst.

After Reading the complete instruction memory processor will stop 
to start it from 0 adress you have to reset