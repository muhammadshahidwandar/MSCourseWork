
/*!
 ************************************************************************
 * \file vlc.h
 *
 * \brief
 *    header for (CA)VLC coding functions
 *
 * \author
 *    Karsten Suehring
 *
 ************************************************************************
 */

#ifndef _VLC_H_
#define _VLC_H_
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int se_v (char *tracestring, Bitstream *bitstream,h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int ue_v (char *tracestring, Bitstream *bitstream, h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int u_1 (char *tracestring, Bitstream *bitstream,h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int u_v (int LenInBits, char *tracestring, Bitstream *bitstream,h264_decoder* dec_params);

// UVLC mapping
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void linfo_ue(int len, int info, int *value1, int *dummy, h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void linfo_se(int len, int info, int *value1, int *dummy, h264_decoder* dec_params);

/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void linfo_cbp_intra(int len,int info,int *cbp, int *dummy,h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void linfo_cbp_inter(int len,int info,int *cbp, int *dummy, h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void linfo_levrun_inter(int len,int info,int *level,int *irun,h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void linfo_levrun_c2x2(int len,int info,int *level,int *irun, h264_decoder* dec_params);

/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int  readSyntaxElement_VLC (SyntaxElement *sym, Bitstream *currStream);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int readSyntaxElement_UVLC(SyntaxElement *sym, DataPartition *dP);
int  readSyntaxElement_Intra4x4PredictionMode(SyntaxElement *sym,ImageParameters *img, InputParameters *inp,DataPartition *dp);

int  GetVLCSymbol (byte buffer[],int totbitoffset,int *info, int bytecount);
int  GetVLCSymbol_IntraMode (byte buffer[],int totbitoffset,int *info, int bytecount);

int readSyntaxElement_FLC(SyntaxElement *sym, Bitstream *currStream);
__inline int readSyntaxElement_NumCoeffTrailingOnes(SyntaxElement *sym,  DataPartition *dP);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
__inline int readSyntaxElement_NumCoeffTrailingOnesChromaDC(SyntaxElement *sym,  DataPartition *dP, h264_decoder* dec_params);
__inline int readSyntaxElement_Level_VLC0(SyntaxElement *sym, DataPartition *dP);
__inline int readSyntaxElement_Level_VLCN(SyntaxElement *sym, int vlc, DataPartition *dP);
__inline int readSyntaxElement_TotalZeros(SyntaxElement *sym,  DataPartition *dP);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
__inline int readSyntaxElement_TotalZerosChromaDC(SyntaxElement *sym,  DataPartition *dP,h264_decoder* dec_params);
__inline int readSyntaxElement_Run(SyntaxElement *sym,  DataPartition *dP);
int GetBits (byte buffer[],int totbitoffset,int *info, int bytecount, int numbits);
int ShowBits (byte buffer[],int totbitoffset,int bytecount, int numbits);
int more_rbsp_data (byte buffer[],int totbitoffset,int bytecount);


void readCoeff4x4_CAVLC_AC ( ImageParameters* img, int block_type, 
							 int mb_nnz ,
							 int levarr[16], int runarr[16], int *number_coefficients );
#endif

