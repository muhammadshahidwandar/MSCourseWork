      
/*!
************************************************************************
* \file vlc.c
*
* \brief
*    VLC support functions
*
* \author
*    Main contributors (see contributors.h for copyright, address and affiliation details)
*    - Inge Lille-Langy               <inge.lille-langoy@telenor.com>
*    - Detlev Marpe                    <marpe@hhi.de>
*    - Gabi Blaettermann               <blaetter@hhi.de>
************************************************************************
*/
//#include "contributors.h"

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#include "global.h"
#include "vlc.h"
//#include "elements.h"

/************************************************************
*	Function added to replace (any-variable)%8
*  / return
*			mod by 8 of an integer
*						<saad.shams@inforient.com>
************************************************************/
///??? replace with low cost macro
/*
__inline int mod8(int x)
{
	return (x&7);//(x-((x>>3)<<3));
}
*/

#define mod8(x) ((x&7))

// A little trick to avoid those horrible #if TRACE all over the source code
#if TRACE
#define SYMTRACESTRING(s) strncpy(sym->tracestring,s,TRACESTRING_SIZE)
#else
#define SYMTRACESTRING(s) // do nothing
#endif

/*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*-----------------------------------------------------------------------*/			
//extern void tracebits(const char *trace_str,  int len,  int info,int value1,  h264_decoder*dec_params);
extern void tracebits(const char *trace_str,  int len,  int info,int value1);


//int UsedBits; /*Changed by Saad Bin Shams [Removing Global Variables]*/

// Note that all NA values are filled with 0

//! for the linfo_levrun_inter routine
/*
const byte NTAB1[4][8][2] =
{
	{{1,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
	{{1,1},{1,2},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
	{{2,0},{1,3},{1,4},{1,5},{0,0},{0,0},{0,0},{0,0}},
	{{3,0},{2,1},{2,2},{1,6},{1,7},{1,8},{1,9},{4,0}},
};
const byte LEVRUN1[16]=
{
	4,2,2,1,1,1,1,1,1,1,0,0,0,0,0,0,
};


const byte NTAB2[4][8][2] =
{
	{{1,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
	{{1,1},{2,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
	{{1,2},{3,0},{4,0},{5,0},{0,0},{0,0},{0,0},{0,0}},
	{{1,3},{1,4},{2,1},{3,1},{6,0},{7,0},{8,0},{9,0}},
};

//! for the linfo_levrun__c2x2 routine
const byte LEVRUN3[4] =
{
	2,1,0,0
};
const byte NTAB3[2][2][2] =
{
	{{1,0},{0,0}},
	{{2,0},{1,1}},
};*/


static __inline unsigned int norm_x(unsigned int x)
{
        x |= (x >> 1);
        x |= (x >> 2);
        x |= (x >> 4);
        x |= (x >> 8);
        x |= (x >> 16);

		x -= ((x >> 1) & 0x55555555);
        x  = (((x >> 2) & 0x33333333) + (x & 0x33333333));
        x  = (((x >> 4) + x) & 0x0f0f0f0f);
        x += (x >> 8);
        x += (x >> 16);
        x  = x & 0x0000003f;

        //return(32 - x);
		return (x-1);
}


/*! 
*************************************************************************************
* \brief
*    ue_v, reads an ue(v) syntax element, the length in bits is stored in 
*    the global UsedBits variable
*
* \param tracestring
*    the string for the trace file
*
* \param bitstream
*    the stream to be read from
*
* \return
*    the value of the coded syntax element
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
*************************************************************************************
*/

int ue_v (char *tracestring, Bitstream *bitstream,   h264_decoder* dec_params)
{
	SyntaxElement symbol, *sym=&symbol;
	 
	assert (bitstream->streamBuffer != NULL);
	sym->type = SE_HEADER;
	sym->mapping = linfo_ue;   // Mapping rule
	SYMTRACESTRING(tracestring);
	readSyntaxElement_VLC (sym, bitstream);
	dec_params->UsedBits+=sym->len;
	return sym->value1;
}


/*! 
*************************************************************************************
* \brief
*    ue_v, reads an se(v) syntax element, the length in bits is stored in 
*    the global UsedBits variable
*
* \param tracestring
*    the string for the trace file
*
* \param bitstream
*    the stream to be read from
*
* \return
*    the value of the coded syntax element
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
*************************************************************************************
*/


int se_v (char *tracestring, Bitstream *bitstream,  h264_decoder* dec_params)
{
	SyntaxElement symbol, *sym=&symbol;
	
	assert (bitstream->streamBuffer != NULL);
	sym->type = SE_HEADER;
	sym->mapping = linfo_se;   // Mapping rule: signed integer
	SYMTRACESTRING(tracestring);
	readSyntaxElement_VLC (sym, bitstream);
	dec_params->UsedBits+=sym->len;
	return sym->value1;
}


/*! 
*************************************************************************************
* \brief
*    ue_v, reads an u(v) syntax element, the length in bits is stored in 
*    the global UsedBits variable
*
* \param LenInBits
*    length of the syntax element
*
* \param tracestring
*    the string for the trace file
*
* \param bitstream
*    the stream to be read from
*
* \return
*    the value of the coded syntax element
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
*************************************************************************************
*/
int u_v (int LenInBits, char*tracestring, Bitstream *bitstream,  h264_decoder* dec_params)
{
	SyntaxElement symbol, *sym=&symbol;
	
	assert (bitstream->streamBuffer != NULL);
	sym->type = SE_HEADER;
	sym->mapping = linfo_ue;   // Mapping rule
	sym->len = LenInBits;
	SYMTRACESTRING(tracestring);
	readSyntaxElement_FLC (sym, bitstream);
	dec_params->UsedBits+=sym->len;
	return sym->inf;
};


/*! 
*************************************************************************************
* \brief
*    ue_v, reads an u(1) syntax element, the length in bits is stored in 
*    the global UsedBits variable
*
* \param tracestring
*    the string for the trace file
*
* \param bitstream
*    the stream to be read from
*
* \return
*    the value of the coded syntax element
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
*************************************************************************************
*/
int u_1 (char *tracestring, Bitstream *bitstream,  h264_decoder* dec_params)
{
	return u_v (1, tracestring, bitstream,dec_params);
}

/*!
************************************************************************
* \brief
*    mapping rule for ue(v) syntax elements
* \par Input:
*    lenght and info
* \par Output:
*    number in the code table
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/


void linfo_ue(int len, int info, int *value1, int *dummy,   h264_decoder* dec_params)
{
	//*value1 = (int)pow(2,(len>>1))+info-1; 
	*value1 = (int)( 0x01<<(len>>1) )+info-1;
}

/*!
************************************************************************
* \brief
*    mapping rule for se(v) syntax elements
* \par Input:
*    lenght and info
* \par Output:
*    signed mvd
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/

void linfo_se(int len,  int info, int *value1, int *dummy,   h264_decoder* dec_params)
{
	int n;
	n = (int)(0x01<<(len>>1))+info-1;
	*value1 = (n+1)>>1;
	if((n & 0x01)==0)                           // lsb is signed bit
		*value1 = -*value1;
}

/*!
************************************************************************
* \par Input:
*    length and info
* \par Output:
*    cbp (intra)
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
void linfo_cbp_intra(int len,int info,int *cbp, int *dummy,   h264_decoder* dec_params)
{
	//extern const byte NCBP[2][48][2];
	extern const byte NCBP[48][2];
	int cbp_idx;
	
	linfo_ue(len,info,&cbp_idx,dummy, dec_params);
	//*cbp=NCBP[1][cbp_idx][0];
	*cbp=NCBP[cbp_idx][0];
}

/*!
************************************************************************
* \par Input:
*    length and info
* \par Output:
*    cbp (inter)
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
void linfo_cbp_inter(int len,int info,int *cbp, int *dummy,   h264_decoder* dec_params)
{
	//extern const byte NCBP[2][48][2];
	extern const byte NCBP[48][2];
	int cbp_idx;
	
	linfo_ue(len,info,&cbp_idx,dummy, dec_params);
	//*cbp=NCBP[1][cbp_idx][1];
	*cbp=NCBP[cbp_idx][1];
}


const unsigned int FLC_table[] =
{0x00000000,0x80000000,0xc0000000,0xe0000000,0xf0000000,0xf8000000,0xfc000000,0xfe000000,0xff000000,0xff800000,0xffc00000,
 0xffe00000,0xfff00000,0xfff80000,0xfffc0000,0xfffe0000,0xffff0000,0xffff8000,0xffffc000,0xffffe000,0xfffff000,0xfffff800,
 0xfffffc00,0xfffffe00,0xffffff00,0xffffff80,0xffffffc0,0xffffffe0,0xfffffff0
};

static const char LogTable256[] = 
{
  0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};

/*!
************************************************************************
* \brief
*    read next UVLC codeword from UVLC-partition and
*    map it to the corresponding syntax element
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/

///??? limited to handel 25 bit VLCs only

int readSyntaxElement_VLC(SyntaxElement *sym, Bitstream *currStream)
{
	unsigned int frame_bitoffset = currStream->frame_bitoffset;
	byte *buf = currStream->streamBuffer;
	
	unsigned int r = 0; // r will be lg(v)
	unsigned int streamWord =0;
	int map_var =0;
	
	
	unsigned int r1 = 0; // r will be lg(v)
	register unsigned int t, tt; // temporaries
	unsigned int bitoffset,byteoffset;



	byteoffset= frame_bitoffset>>3;	
	bitoffset = (frame_bitoffset&7);

	//streamWord = (( (unsigned int)buf[byteoffset])<<8 ) | (((unsigned int)buf[byteoffset+1])) ;
	streamWord = ((((unsigned int)buf[byteoffset]) << 24) | (((unsigned int)buf[byteoffset+1]) << 16) | (((unsigned int)buf[byteoffset+2]) << 8) | ((unsigned int)buf[byteoffset+3]) );
	streamWord = ((streamWord<<bitoffset));								



	
	if(!(streamWord & 0x80000000))
	{		

		if (tt = streamWord >> 16)
		{
			r = (t = streamWord >> 24) ? 24 + LogTable256[t] : 16 + LogTable256[tt & 0xFF];
		}
		else 
		{
			r = (t = streamWord >> 8) ? 8 + LogTable256[t] : LogTable256[streamWord];
		}
		
		///??? use xtensas NSAU instruction for direcr calculation of index of 1 in 32 bits
		r = 31 - r ;





		
		streamWord = streamWord << (r+1);
		sym->inf = (streamWord & FLC_table[r])>>(32-r);
				
		currStream->frame_bitoffset += sym->len = (r<<1)+1;           // return absolute offset in bit from start of frame
	}
	else
	{
		sym->inf = 0;
		currStream->frame_bitoffset += sym->len = 1;           // return absolute offset in bit from start of frame
	}

	///??? check for byte overflow in VLC
	//if (sym->len == -1)
	//	return -1;

	map_var = (int)(0x01<<(sym->len>>1))+sym->inf -1;
	
	if(sym->mapping == linfo_se)
	{		
		int n  = map_var;
		map_var = (n+1)>>1;
		if((n & 0x01) == 0)
		map_var = - map_var;

		sym->value1 = map_var;
		return 1;

	}

	if(sym->mapping == linfo_ue)
	{
		sym->value1 = map_var;
		return 1;
	}
	
	
	
	if(sym->mapping == linfo_cbp_inter)
	{ 
			
		extern const byte NCBP[48][2];
		
		map_var=NCBP[map_var][1];
			
	}else if(sym->mapping == linfo_cbp_intra)
	{ 
		extern const byte NCBP[48][2];
		
		map_var=NCBP[map_var][0];

	}
	
	
	sym->value1 = map_var;
	
	//sym->mapping(sym->len,sym->inf,&(sym->value1),&(sym->value2),dec_params);
	
	
#if TRACE
//	tracebits(sym->tracestring, sym->len, sym->inf, sym->value1,dec_params);
	tracebits(sym->tracestring, sym->len, sym->inf, sym->value1);
#endif
	
	return 1;
}

/*!
************************************************************************
* \brief
*    read next VLC codeword for 4x4 Intra Prediction Mode and
*    map it to the corresponding Intra Prediction Direction
************************************************************************
*/
int readSyntaxElement_Intra4x4PredictionMode(SyntaxElement *sym, ImageParameters *img, InputParameters *inp, DataPartition *dP)
{
	Bitstream   *currStream            = dP->bitstream;
	int         frame_bitoffset        = currStream->frame_bitoffset;
	byte        *buf                   = currStream->streamBuffer;
	int         BitstreamLengthInBytes = currStream->bitstream_length;
	
	sym->len = GetVLCSymbol_IntraMode (buf, frame_bitoffset, &(sym->inf), BitstreamLengthInBytes);
	
	if (sym->len == -1)
		return -1;
	
	currStream->frame_bitoffset += sym->len;
	sym->value1                  = sym->len == 1 ? -1 : sym->inf;
	
#if TRACE
	tracebits2(sym->tracestring, sym->len, sym->value1);
#endif
	
	return 1;
}

int GetVLCSymbol_IntraMode (byte buffer[],int totbitoffset,int *info, int bytecount)
{
	
	register int inf;
	long byteoffset;      // byte from start of buffer
	int bitoffset;      // bit from start of byte
	int ctr_bit=0;      // control bit for current bit posision
	int bitcounter=1;
	int len;
	int info_bit;
	
	byteoffset = totbitoffset>>3;
	bitoffset=7-mod8(totbitoffset);
	ctr_bit    = (buffer[byteoffset] & (0x01<<bitoffset));   // set up control bit
	len        = 1;
	
	//First bit
	if (ctr_bit)
	{
		*info = 0;
		return bitcounter;
	}
	else
		len=4;
	
	// make infoword
	inf=0;                          // shortest possible code is 1, then info is always 0
	for(info_bit=0;(info_bit<(len-1)); info_bit++)
	{
		bitcounter++;
		bitoffset-=1;
		if (bitoffset<0)
		{                 // finished with current byte ?
			bitoffset=bitoffset+8;
			byteoffset++;
		}
		if (byteoffset > bytecount)
		{
			return -1;
		}
		inf=(inf<<1);
		if(buffer[byteoffset] & (0x01<<(bitoffset)))
			inf |=1;
	}
	
	*info = inf;
	return bitcounter;           // return absolute offset in bit from start of frame
}


/*!
************************************************************************
* \brief
*    test if bit buffer contains only stop bit
*
* \param buffer
*    buffer containing VLC-coded data bits
* \param totbitoffset
*    bit offset from start of partition
* \param bytecount
*    buffer length
* \return
*    true if more bits available
************************************************************************
*/
int more_rbsp_data (byte buffer[],int totbitoffset,int bytecount)
{
	
	long byteoffset;      // byte from start of buffer
	int bitoffset;      // bit from start of byte
	int ctr_bit=0;      // control bit for current bit posision
	
	int cnt=0;
	
	
	byteoffset= totbitoffset>>3;
	bitoffset=7-mod8(totbitoffset);
	
	assert (byteoffset<bytecount);
	
	// there is more until we're in the last byte
	if (byteoffset<(bytecount-1)) return TRUE;
	
	// read one bit
	ctr_bit = (buffer[byteoffset] & (0x01<<bitoffset));
	
	// a stop bit has to be one
	if (ctr_bit==0) return TRUE;
	
	bitoffset--;
	
	while (bitoffset>=0)
	{
		ctr_bit = (buffer[byteoffset] & (0x01<<bitoffset));   // set up control bit
		if (ctr_bit>0) cnt++;
		bitoffset--;
	}
	
	return (0!=cnt);
	
}


/*!
************************************************************************
* \brief
*    Check if there are symbols for the next MB
************************************************************************
*/
int uvlc_startcode_follows(int dummy, h264_decoder* dec_params)
{
	ImageParameters* img = dec_params->img;
	//int dp_Nr = assignSE2partition[PAR_DP_1][SE_MBTYPE];
	//int dp_Nr = 0;
	DataPartition *dP = &(img->currentSlice->partArr[0]);
	Bitstream   *currStream = dP->bitstream;
	byte *buf = currStream->streamBuffer;		

	//KS: new function test for End of Buffer
	return (!(more_rbsp_data(buf, currStream->frame_bitoffset,currStream->bitstream_length)));
}






extern void tracebits2(const char *trace_str,  int len,  int info) ;


//#if 0
/*!
************************************************************************
* \brief
*    code from bitstream (2d tables)
/***********************************************************************
*	- Removed the function ShowBits from the code
*
*	 "Muhammad Tahir Awan" <tahir.awan@inforient.com>,
*	 "Umair Razzak" <umair.razzak@inforient.com>
****************************
*	 Changes till 9-12-2005
***********************************************************************
*/

int code_from_bitstream_2d(SyntaxElement *sym,  
                           DataPartition *dP,
                           int *lentab,					// 4x17 matrix
                           int *codtab,					// 4x17 matrix
                           int tabwidth,
                           int tabheight,
                           int *code)
{
	Bitstream   *currStream = dP->bitstream;
	int frame_bitoffset = currStream->frame_bitoffset;
	byte *buf = currStream->streamBuffer;
	int BitstreamLengthInBytes = currStream->bitstream_length;
	
	int i,j;
	int len, cod;

	int inf = 0;
		

	int shift_factor;
	unsigned int streamData,bitoffset,byteoffset;



	byteoffset= frame_bitoffset>>3;	
	bitoffset = (frame_bitoffset&7);

	//streamWord = (( (unsigned int)buf[byteoffset])<<8 ) | (((unsigned int)buf[byteoffset+1])) ;
	streamData = ((((unsigned int)buf[byteoffset]) << 24) | (((unsigned int)buf[byteoffset+1]) << 16) | (((unsigned int)buf[byteoffset+2]) << 8) | ((unsigned int)buf[byteoffset+3]) );
	streamData = ((streamData<<bitoffset) >> 16);								


	//tab_offset = 0;
	// this VLC decoding method is not optimized for speed
	for (j = 0; j < tabheight; j++) 
	{
		for (i = 0; i < tabwidth; i++)
		{
			len = lentab[i];			// lentab[4][17]
			
			if(len)
			{
				cod = codtab[i];			// codtab[4][17]
				
				//mask = (0x01 << numbits)-1;
				shift_factor = (16 - len);
				inf = ((streamData) >> shift_factor);			

				//if ((ShowBits(buf, frame_bitoffset, BitstreamLengthInBytes, len) == cod))
				if ( inf == cod )
				{
					sym->value1 = i;		// numcoeff's
					sym->value2 = j;		// Trailing One's
					currStream->frame_bitoffset += len; // move bitstream pointer
					sym->len = len;
					goto found_code;
				}
			}
		}
		lentab += tabwidth;
		codtab += tabwidth;
	}
	
	return -1;  // failed to find code
	
found_code:
	
	*code = cod;
	
	return 0;
}
//#endif 
/*!
************************************************************************
* \brief
*    read FLC codeword from UVLC-partition 
************************************************************************
*/

//#if 0

///??? this can handle only 25-bit FLC

int readSyntaxElement_FLC(SyntaxElement *sym, Bitstream *currStream)
{
	unsigned int frame_bitoffset = currStream->frame_bitoffset;
	byte *buf = currStream->streamBuffer;
	unsigned int BitstreamLengthInBytes = currStream->bitstream_length;
	unsigned int streamWord,bitoffset,byteoffset;
	int numbits = sym->len;


	byteoffset= frame_bitoffset>>3;	
	bitoffset = (frame_bitoffset&7);

	//streamWord = (( (unsigned int)buf[byteoffset])<<8 ) | (((unsigned int)buf[byteoffset+1])) ;
	streamWord = ((((unsigned int)buf[byteoffset]) << 24) | (((unsigned int)buf[byteoffset+1]) << 16) | (((unsigned int)buf[byteoffset+2]) << 8) | ((unsigned int)buf[byteoffset+3]) );
	streamWord = ((streamWord<<bitoffset));	

	
	sym->value1 = sym->inf = (streamWord & FLC_table[numbits])>>(32-numbits);//>>(bitoffset);
		
	currStream->frame_bitoffset += numbits; // move bitstream pointer

	
#if TRACE
	tracebits2(sym->tracestring, sym->len, sym->inf);
#endif
	
	return 1;
}
//#endif



const unsigned char BitReverseTable256[] = 
{
  0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0, 
  0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 
  0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4, 
  0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC, 
  0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 
  0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
  0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6, 
  0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
  0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
  0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9, 
  0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
  0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
  0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3, 
  0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
  0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7, 
  0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

const int Mod37BitPosition[] = // maps a bit value mod 37 to its position
{
  32, 0, 1, 26, 2, 23, 27, 0, 3, 16, 24, 30, 28, 11, 0, 13, 4,
  7, 17, 0, 25, 22, 31, 15, 29, 10, 12, 6, 0, 21, 14, 9, 5,
  20, 8, 19, 18
};
// get the suffix bits
const unsigned char first_vlc0_base[]=
{
	0,0,0,3,3,3,3,3,3,7,7,7,7,3,0
};
// number of coeffiecients,trailing ones,length
const unsigned char vlc0_coeffones[]=
{
 0,0,1,/**/1,1,2,/**/2,2,3,/**/2,1,6,3,3,5,1,0,6,3,3,5,/**/5,3,7,4,3,6,3,2,7,4,3,6,/**/
 6,3,8,3,1,8,4,2,8,2,0,8,/**/7,3,9,4,1,9,5,2,9,3,0,9,/**/8,3,10,5,1,10,6,2,10,4,0,10,/**/
 9,3,11,6,1,11,7,2,11,5,0,11,/**/8,0,13,10,3,13,8,1,13,7,1,13,9,2,13,8,2,13,7,0,13,6,0,13,/**/
 12,3,14,11,3,14,10,1,14,9,1,14,11,2,14,10,2,14,10,0,14,9,0,14,/**/14,3,15,13,3,15,12,1,15,
 11,1,15,13,2,15,12,2,15,12,0,15,11,0,15,/**/16,3,16,15,3,16,15,1,16,14,1,16,15,2,16,14,2,16,
 14,0,16,13,0,16,/**/16,0,16,16,1,16,16,2,16,15,0,16,/**/13,1,15
};
// index to point in vlc0_coeffones table
const unsigned char main_offset_vlc0[]=
{
	0,3,6,9,21,33,45,57,69,81,105,129,153,177,189
};
const unsigned char vlc0_suffix_length[]=
{
	0,0,0,2,2,2,2,2,2,3,3,3,3,2,0
};

// Data Format : code = [b11-b7][b6-b5][b4-b0]  ~  [codelength][Trailing Ones][Total Coeff's]
const unsigned short vlc0_code_tab[15][8]=
{
	{128,0,0,0,0,0,0,0},							
	{289,0,0,0,0,0,0,0},							
	{450,0,0,0,0,0,0,0},							
	{802,769,739,739,0,0,0,0},				
	{997,963,868,868,0,0,0,0},				
	{1126,1092,1059,1026,0,0,0,0},				
	{1255,1221,1188,1155,0,0,0,0},				
	{1384,1350,1317,1284,0,0,0,0},				
	{1513,1479,1446,1413,0,0,0,0},				
	{1672,1737,1704,1671,1770,1736,1703,1670},
	{1900,1867,1834,1802,1899,1866,1833,1801},
	{2030,1997,1964,1932,2029,1996,1963,1931},
	{2160,2127,2095,2062,2159,2126,2094,2061},
	{2064,2128,2096,2063,0   ,0   ,0   ,0   },
	{1965,0   ,0   ,0   ,0   ,0   ,0   ,0   }		
};
const unsigned char first_vlc1_base[]=
{
	1,3,7,3,3,3,3,7,7,7,7,3,0

};

const unsigned char vlc1_coeffones[]=
{
 1,1,2,0,0,2,/**/4,3,4,2,2,3,3,3,4,2,2,3,/**/6,3,6,5,3,5,3,1,6,2,1,5,3,2,6,5,3,5,1,0,6,2,1,5,
 /**/7,3,6,4,1,6,4,2,6,2,0,6,/**/8,3,7,5,1,7,5,2,7,3,0,7,/**/5,0,8,6,1,8,6,2,8,4,0,8,/**/9,3,9,7,1,9,
 7,2,9,6,0,9,/**/11,3,11,10,3,11,9,1,11,8,1,11,9,2,11,8,2,11,8,0,11,7,0,11,/**/11,0,12,12,3,12,11,1,12,10,1,12,11,2,12,
 10,2,12,10,0,12,9,0,12,/**/14,3,13,13,3,13,13,1,13,12,1,13,13,2,13,12,2,13,13,0,13,12,0,13,/**/15,1,14,14,2,13,15,2,14,
 14,0,13,15,0,14,14,2,13,14,1,14,14,0,13,/**/16,3,14,16,1,14,16,2,14,16,0,14,/**/15,3,13/**/
};

const unsigned char main_offset_vlc1[]=
{
	0,6,18,42,54,66,78,90,114,138,162,186,198
};

const unsigned char vlc1_suffix_length[]=
{
	1,2,3,2,2,2,2,3,3,3,3,2,0        
};

// Data Format : code = [b11-b7][b6-b5][b4-b0]  ~  [codelength][Trailing Ones][Total Coeff's]
const unsigned short vlc1_code_tab[13][8]=
{ 
	
 {289	,256  ,0	,0	    ,0	  ,0	  ,0	  ,0		}
,{612	,611  ,450	,450	,0	  ,0	  ,0  	,0			}
,{870	,835  ,803	,769	,741	,741	,674	,674	}
,{871	,836  ,804	,770	,0	  ,0	  ,0	  ,0		}
,{1000	,965  ,933	,899	,0	  ,0	  ,0	  ,0		}
,{1029	,1094 ,1062	,1028	,0	  ,0	  ,0	  ,0        }
,{1257	,1223 ,1191	,1158	,0	  ,0	  ,0	  ,0        }
,{1515	,1481 ,1449	,1416	,1514	,1480	,1448	,1415   }
,{1547	,1611 ,1579	,1546	,1644	,1610	,1578	,1545   }
,{1774	,1741 ,1709	,1677	,1773	,1740	,1708	,1676   }
,{1839	,1807 ,1871	,1838	,1742	,1742	,1678	,1678   }
,{1904	,1872 ,1840	,1808	,0	  ,0	  ,0	  ,0        }
,{1775	,0	  ,0	  ,0	,0	  ,0	  ,0	  ,0		}

};
const unsigned char first_vlc2_base[]=
{
	7,7,7,7,7,7,7,3,1,0

};

const unsigned char vlc2_coeffones[]=
{
 7,3,4,3,3,4,5,3,4,1,1,4,6,3,4,2,2,4,4,3,4,0,0,4,/**/5,1,5,3,1,5,4,1,5,3,2,5,5,2,5,8,3,5,4,2,5,2,1,5,
 /**/3,0,6,9,3,6,7,1,6,6,1,6,7,2,6,6,2,6,2,0,6,1,0,6,/**/7,0,7,10,3,7,9,2,7,8,1,7,6,0,7,8,2,7,5,0,7,4,0,7,
 /**/12,3,8,11,3,8,10,1,8,9,1,8,11,2,8,10,2,8,9,0,8,8,0,8,/**/12,0,9,13,3,9,12,1,9,11,1,9,13,2,9,12,2,9,
 11,0,9,10,0,9,/**/15,1,10,14,1,10,14,3,10,13,1,9,14,0,10,13,0,10,14,2,10,13,1,9,/**/16,1,10,15,3,10,
 15,0,10,15,2,10,/**/16,3,10,16,2,10,/**/16,0,10/**//**//**//**/
};

const unsigned char main_offset_vlc2[]=
{
	0,24,48,72,96,120,144,168,180,186
};
const unsigned char vlc2_suffix_length[]=
{
	3,3,3,3,3,3,3,2,1,0
};

// Data Format : code = [b11-b7][b6-b5][b4-b0]  ~  [codelength][Trailing Ones][Total Coeff's]
const unsigned short vlc2_code_tab[10][8]=
{ 
	{615	  ,614	  ,613	  ,612	  ,611	  ,578	  ,545	  ,512 }
 ,{677	  ,709	  ,676	  ,708	  ,675	  ,744	  ,707	  ,674 }
 ,{771	  ,839	  ,807	  ,770	  ,873	  ,838	  ,806	  ,769 }
 ,{903	  ,902	  ,969	  ,901	  ,1002	  ,968	  ,936	  ,900 }
 ,{1132	,1099	  ,1066	  ,1033	  ,1131	  ,1098	  ,1065	  ,1032}
 ,{1164	,1229	  ,1196	  ,1163	  ,1261	  ,1228	  ,1195	  ,1162}
 ,{1327	,1294	  ,1390	  ,1358	  ,1326	  ,1293	  ,1197	  ,1197}
 ,{1328	,1295	  ,1391	  ,1359	  ,0	    ,0	    ,0	    ,0   }
 ,{1392	,1360	  ,0	    ,0	    ,0	    ,0	    ,0	    ,0   }
 ,{1296	,0	    ,0	    ,0	    ,0	    ,0	    ,0	    ,0   }

};
const int lentab_NumCoeffTrailingOnesChromaDC[3][4][17] = 
{
	//YUV420
	{{ 2, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{ 0, 1, 6, 7, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
	{ 0, 0, 3, 7, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
	{ 0, 0, 0, 6, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
	//YUV422
	{{ 1, 7, 7, 9, 9,10,11,12,13, 0, 0, 0, 0, 0, 0, 0, 0},
	{ 0, 2, 7, 7, 9,10,11,12,12, 0, 0, 0, 0, 0, 0, 0, 0}, 
	{ 0, 0, 3, 7, 7, 9,10,11,12, 0, 0, 0, 0, 0, 0, 0, 0}, 
	{ 0, 0, 0, 5, 6, 7, 7,10,11, 0, 0, 0, 0, 0, 0, 0, 0}},
	//YUV444
	{{ 1, 6, 8, 9,10,11,13,13,13,14,14,15,15,16,16,16,16},
	{ 0, 2, 6, 8, 9,10,11,13,13,14,14,15,15,15,16,16,16},
	{ 0, 0, 3, 7, 8, 9,10,11,13,13,14,14,15,15,16,16,16},
	{ 0, 0, 0, 5, 6, 7, 8, 9,10,11,13,14,14,15,15,16,16}}
};

const int codtab_NumCoeffTrailingOnesChromaDC[3][4][17] = 
{
	//YUV420
	{{ 1, 7, 4, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{ 0, 1, 6, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{ 0, 0, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{ 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
	//YUV422
	{{ 1,15,14, 7, 6, 7, 7, 7, 7, 0, 0, 0, 0, 0, 0, 0, 0},
	{ 0, 1,13,12, 5, 6, 6, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0},
	{ 0, 0, 1,11,10, 4, 5, 5, 4, 0, 0, 0, 0, 0, 0, 0, 0},
	{ 0, 0, 0, 1, 1, 9, 8, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0}},
	//YUV444
	{{ 1, 5, 7, 7, 7, 7,15,11, 8,15,11,15,11,15,11, 7, 4}, 
	{ 0, 1, 4, 6, 6, 6, 6,14,10,14,10,14,10, 1,14,10, 6}, 
	{ 0, 0, 1, 5, 5, 5, 5, 5,13, 9,13, 9,13, 9,13, 9, 5}, 
	{ 0, 0, 0, 3, 3, 4, 4, 4, 4, 4,12,12, 8,12, 8,12, 8}}
	
};



const int lentab_TotalZeros[TOTRUN_NUM][16] = 
{
	
	{ 1,3,3,4,4,5,5,6,6,7,7,8,8,9,9,9},  
	{ 3,3,3,3,3,4,4,4,4,5,5,6,6,6,6},  
	{ 4,3,3,3,4,4,3,3,4,5,5,6,5,6},  
	{ 5,3,4,4,3,3,3,4,3,4,5,5,5},  
	{ 4,4,4,3,3,3,3,3,4,5,4,5},  
	{ 6,5,3,3,3,3,3,3,4,3,6},  
	{ 6,5,3,3,3,2,3,4,3,6},  
	{ 6,4,5,3,2,2,3,3,6},  
	{ 6,6,4,2,2,3,2,5},  
	{ 5,5,3,2,2,2,4},  
	{ 4,4,3,3,1,3},  
	{ 4,4,2,1,3},  
	{ 3,3,1,2},  
	{ 2,2,1},  
	{ 1,1},  
};

const int codtab_TotalZeros[TOTRUN_NUM][16] = 
{
	{1,3,2,3,2,3,2,3,2,3,2,3,2,3,2,1},
	{7,6,5,4,3,5,4,3,2,3,2,3,2,1,0},
	{5,7,6,5,4,3,4,3,2,3,2,1,1,0},
	{3,7,5,4,6,5,4,3,3,2,2,1,0},
	{5,4,3,7,6,5,4,3,2,1,1,0},
	{1,1,7,6,5,4,3,2,1,1,0},
	{1,1,5,4,3,3,2,1,1,0},
	{1,1,1,3,3,2,2,1,0},
	{1,0,1,3,2,1,1,1,},
	{1,0,1,3,2,1,1,},
	{0,1,1,2,1,3},
	{0,1,1,1,1},
	{0,1,1,1},
	{0,1,1},
	{0,1},  
};


const unsigned char totalzeros_lengths[]  = { 0,9,6,6,5,5,6,6,6,6,5,4,4,3,2,1	};

const unsigned char tatalzeros_codezero[] = {0,0,14,13,12,11,10,9,8,1,1,0,0,0,0,0};

const unsigned char totalzeros_tc1 [][2]  = {{16,16},{50,49},{68,67},{86,85},{104,103},           
											{122,121},{140,139},{158,157},{159,159}};         

const unsigned char totalzeros_tc2 [][4]  = {{51,50,49,48},{70,69,52,52},{72,72,71,71},               
								   			{90,90,89,89},{108,108,107,107},{109,109,109,109} };	 

const unsigned char totalzeros_tc3 [][4]  = {{54,51,50,49},{68,64,55,55},{72,72,69,69},				
					   						{90,90,89,89},{92,92,92,92},{107,107,107,107}};								

const unsigned char totalzeros_tc4 [][4] = {{54,53,52,49},{67,66,56,56},{73,73,71,71},	
											{90,90,80,80},{91,91,91,91}};

const unsigned char totalzeros_tc5 [][4] = {{54,53,52,51},{65,64,55,55},{72,72,66,66},
   											{74,74,74,74},{89,89,89,89},{91,91,91,91}};		

const unsigned char totalzeros_tc6 [][4] = {{53,52,51,50},{55,55,54,54},{57,57,57,57},
   											{72,72,72,72},{81,81,81,81},{96,96,96,96}};

const unsigned char totalzeros_tc7 [][4] = {{51,50,37,37},{54,54,52,52},{56,56,56,56},
										   	{71,71,71,71},{81,81,81,81},{96,96,96,96}};

const unsigned char totalzeros_tc8 [][2] = {{37,36},{54,51},{55,55},
   											{65,65},{82,82},{96,96}};

const unsigned char totalzeros_tc9 [][2] = {{36,35},{38,38},{53,53},
										   	{66,66},{87,87},{96,96}};	

const unsigned char totalzeros_tc10 [][2] = {{36,35},{37,37},{50,50},
   											{70,70},{80,80}};

const unsigned char totalzeros_tc11 [][2] = {{20,20},{51,53},{50,50},{65,65}};										   																									   																					   																					   											
										   											
const unsigned char totalzeros_tc12 [][2] = {{19,19},{34,34},{52,52},{65,65}};

const unsigned char totalzeros_tc13 [][2] = {{18,18},{35,35},{49,49}};


unsigned char CH_TOT_ZERO[3][8][2] ={
	{{3,3},{2,3},{1,2},{1,2},{0,1},{0,1},{0,1},{0,1}},
	{{2,2},{1,2},{0,1},{0,1},{0,1},{0,1},{0,1},{0,1}},
	{{1,1},{0,1},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}}
};
// only for 4:2:0 format , 
int shift_table [] = {15,14,14,13,13,13,13,13,13,13,13,13,13,13,13,13 };
int mask_table[] = {0x01,0x03,0x03,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,};
int run_table[16][8] = {{1,0,-1,-1,-1,-1,-1,-1},
						{2,1,0,0},
{3,2,1,0},
{4,3,2,2,1,1,0,0},
{5,4,3,2,1,1,0,0},
{1,2,4,3,6,5,0,0},
{0,6,5,4,3,2,1,0},
{0,6,5,4,3,2,1,0},
{0,6,5,4,3,2,1,0},
{0,6,5,4,3,2,1,0},
{0,6,5,4,3,2,1,0},
{0,6,5,4,3,2,1,0},
{0,6,5,4,3,2,1,0},
{0,6,5,4,3,2,1,0},
{0,6,5,4,3,2,1,0},
{0,6,5,4,3,2,1,0},};
					


int length_table[16][8] = {{1,1},
							{2,2,1,1},
{2,2,2,2},
{3,3,2,2,2,2,2,2},
{3,3,3,3,2,2,2,2},
{3,3,3,3,3,3,2,2},
{3,3,3,3,3,3,3,3},
{3,3,3,3,3,3,3,3},
{3,3,3,3,3,3,3,3},
{3,3,3,3,3,3,3,3},
{3,3,3,3,3,3,3,3},
{3,3,3,3,3,3,3,3},
{3,3,3,3,3,3,3,3},
{3,3,3,3,3,3,3,3},
{3,3,3,3,3,3,3,3},
{3,3,3,3,3,3,3,3},};

/*!
************************************************************************
* \brief
*  MODIFIED SHOWBITS
*  Reads bits from the bitstream buffer
*
* \param buffer
*    buffer containing VLC-coded data bits
* \param totbitoffset
*    bit offset from start of partition
* \param bytecount
*    total bytes in bitstream
* \param numbits
*    number of bits to read
*
***********************************************************************
*	- The logic has been simplified. Rather than reading a bit at a time
*	  numbits are read once.
*
*	 "Muhammad Tahir Awan" <tahir.awan@inforient.com>,
*	 "Umair Razzak" <umair.razzak@inforient.com>
****************************
*	 Changes till 9-12-2005
***********************************************************************
*/
int ShowBits (byte buffer[],int totbitoffset,int bytecount, int numbits)
{
	
	int inf=0;
	long byteoffset;      // byte from start of buffer
	int bitoffset;      // bit from start of byte
	
	int bitoffset_16;      // bit from start of byte
	//int mynumbits;
	int shift_factor;
	unsigned int streamWord , mask;
	int myinf = 0;
		
	byteoffset = totbitoffset>>3;
	bitoffset = 7-(totbitoffset&7);

	if(numbits)
	{		
		
		bitoffset_16 = bitoffset + 16;      // bit from start of byte
				
		streamWord = (((unsigned int)buffer[byteoffset])<<16) | (((unsigned int)buffer[byteoffset+1])<<8) | ((unsigned int)buffer[byteoffset+2]);
		
		mask = (0x01 << numbits)-1;
		shift_factor = (bitoffset_16 - numbits)+1;
		inf = ((streamWord) & (mask << shift_factor))>>shift_factor; 
	}
		
	return inf; // return absolute offset in bit from start of frame
}     
//#endif

int incVlc[] = {
	0,3,6,12,24,48,32768
};    // maximum vlc = 6

char trailing_ones[][8][3] = { 
	{{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},
	{{1,0,0},{-1,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},
	{{1,1,0},{-1,1,0},{1,-1,0},{-1,-1,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},
	{{1,1,1},{-1,1,1 },{1,-1,1},{-1,-1,1},{1,1,-1},{-1,1,-1},{1,-1,-1},{-1,-1,-1}}
};

unsigned int vlc_table_select[] = {0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,3};

unsigned int run_before_table[] = {0,1,2,3,4,5,6,6,6,6,6,6,6,6,6,6,6};

void readCoeff4x4_CAVLC_AC ( ImageParameters* img, int block_type, 
							 int mb_nnz ,
							 int levarr[16], int runarr[16], int *number_coefficients )
{
	int i,j;
	int mb_nr = img->current_mb_nr;
	SyntaxElement currSE;
	Slice *currSlice = img->currentSlice;
	DataPartition *dP = &(currSlice->partArr[0]);
		
	int k, code, vlcnum;
	int numcoeff, numtrailingones, numcoeff_vlc;
	int level_two_or_higher;
	int numones, totzeros, level;//, cdc=0, cac=0;
	int zerosleft, ntr, dptype = 0;
	int max_coeff_num , nnz = 0;

	/////////////////////////////////////////////////////////
	Bitstream   *currStream = dP->bitstream;
	unsigned int streamData;
	byte *buf = currStream->streamBuffer;
	unsigned int streamWord,bitoffset,byteoffset;
	int util_1,runlength;
	int tabrow , tabcol;


	mb_nr = mb_nr<<5;
	max_coeff_num = *number_coefficients;
			
	if (block_type !=1)
	{
		nnz = mb_nnz;
		{			
			int frame_bitoffset = currStream->frame_bitoffset;
			int vlcnum = vlc_table_select[nnz];
			int code;
			unsigned int czero,shift_factor;
			unsigned int idx;
			//printf("\n%d",nnz);
			if (vlcnum != 3)
			{


				
			byteoffset = frame_bitoffset>>3;			
			shift_factor = 8 - (frame_bitoffset&7) ;				
			streamData = (((((unsigned int)buf[byteoffset]) << 16) | (((unsigned int)buf[byteoffset+1]) << 8) | ((unsigned int)buf[byteoffset+2]) )>> shift_factor ) & 0xffff;

		
			czero = norm_x(streamData);


				if (vlcnum == 0)
				{
					int suffix_length,code_length,suffix,code_word, my_numcoeff,my_numtrailingones;
					
					czero = 15 - czero;			// leading zero's
					suffix_length = vlc0_suffix_length[czero];
					code_length   = czero + 1 + suffix_length;
					suffix        = (streamData >> (16-code_length)) & ((1 << suffix_length) - 1);
					code_word     = (unsigned int)vlc0_code_tab[czero][suffix];

					numcoeff      = code_word & 31;
					numtrailingones = (code_word >> 5)  & 3;
					code_length		= (code_word >> 7 ) & 31;
					currStream->frame_bitoffset += code_length;
				}
				else if (vlcnum == 1)
				{
					////////////
					int suffix_length,code_length,suffix,code_word, my_numcoeff,my_numtrailingones;
					
					czero = 15 - czero; 
					suffix_length = vlc1_suffix_length[czero];
					code_length   = czero + 1 + suffix_length;
					suffix        = (streamData >> (16-code_length)) & ((1 << suffix_length) - 1);
					code_word     = (unsigned int)vlc1_code_tab[czero][suffix];

					numcoeff      = code_word & 31;
					numtrailingones = (code_word >> 5)  & 3;
					code_length		= (code_word >> 7 ) & 31;
					currStream->frame_bitoffset += code_length;
				}
				else//vlcnum == 2
				{
					int suffix_length,code_length,suffix,code_word, my_numcoeff,my_numtrailingones;

					czero = 15 - czero; 
					suffix_length   = vlc2_suffix_length[czero];
					code_length     = czero + 1 + suffix_length;
					suffix          = (streamData >> (16-code_length)) & ((1 << suffix_length) - 1);
					code_word       = (unsigned int)vlc2_code_tab[czero][suffix];
					
					numcoeff        = code_word & 31;
					numtrailingones = (code_word >> 5)  & 3;
					code_length		= (code_word >> 7 ) & 31;
					currStream->frame_bitoffset += code_length;
				}
			}
			else
			{
				int byteoffset , bitoffset;

				byteoffset = frame_bitoffset>>3;
				bitoffset = 7-(frame_bitoffset&7);				
				
				streamWord = (((unsigned int)buf[byteoffset])<<8) | (((unsigned int)buf[byteoffset+1]));				

				code = (streamWord >> (3+bitoffset)) & 63;		// read 6-bit code
				currStream->frame_bitoffset += 6;
				numtrailingones = code & 3;
				numcoeff = (code >> 2);
				
				if (!numcoeff && numtrailingones == 3)
				{
					numtrailingones = 0;
				}
				else
					numcoeff++;
			}
		}
	}
	else
	{	//readSyntaxElement_NumCoeffTrailingOnesChromaDC(&currSE, dP,dec_params); //no need to touch 

		////////////////////
		/*underprocess*/
		int code,codelength;
		int byteoffset , frame_bitoffset ,bitoffset;						
		
	
		frame_bitoffset = currStream->frame_bitoffset;	

		byteoffset = frame_bitoffset>>3;
		bitoffset = 9+7-(frame_bitoffset&7);				
				
		streamWord = (((unsigned int)buf[byteoffset])<<8) | (((unsigned int)buf[byteoffset+1]));				

		code = (streamWord >> (bitoffset-8)) & 0xff;		// read 8-bit code
	

		tabrow = norm_x(code);

		if (tabrow == 7) 
		{
			numcoeff        =  1;
			numtrailingones =  1;
			currStream->frame_bitoffset += 1;
		}
		else if (tabrow == 6) 
		{
			numcoeff        =  0;
			numtrailingones =  0;
			currStream->frame_bitoffset += 2;
		}
		else if (tabrow == 5) 
		{
			numcoeff        =  2;
			numtrailingones =  2;
			currStream->frame_bitoffset += 3;
		}
		else if (tabrow == 4) 
		{
			code = (code >> 2 ) & 3;

			if (code == 3) 
			{
				numcoeff        =  1;
				numtrailingones =  0;
			}
			else if (code == 2) 
			{
				numcoeff        =  2;
				numtrailingones =  1;

			}
			else if (code == 1) 
			{
				numcoeff        =  3;
				numtrailingones =  3;
			}
			else 
			{
				numcoeff        =  2;
				numtrailingones =  0;
			}

			currStream->frame_bitoffset += 6;
		}
		else if (tabrow == 3) 
		{
			code = (code >> 2 ) & 1;

			if (code == 1) 
			{
				numcoeff        =  3;
				numtrailingones =  0;
			}
			else 
			{
				numcoeff        =  4;
				numtrailingones =  0;
			}

			currStream->frame_bitoffset += 6;
		}		
		else if (tabrow == 2) 
		{
			code = (code >> 1 ) & 1;

			if (code == 1) 
			{
				numcoeff        =  3;
				numtrailingones =  1;
			}
			else 
			{
				numcoeff        =  3;
				numtrailingones =  2;
			}

			currStream->frame_bitoffset += 7;
		}
		else if (tabrow == 1) 
		{
			code = (code ) & 1;

			if (code == 1) 
			{
				numcoeff        =  4;
				numtrailingones =  1;
			}
			else 
			{
				numcoeff        =  4;
				numtrailingones =  2;
			}

			currStream->frame_bitoffset += 8;
		}
		////////////////////		
		else
		{
			
			//int code;
			
			//if (code_from_bitstream_2d(&currSE, dP, &lentab_NumCoeffTrailingOnesChromaDC[0][0][0],&codtab_NumCoeffTrailingOnesChromaDC[0][0][0], 17, 4, &code))
			//{
			//	printf("ERROR: failed to find NumCoeff/TrailingOnes ChromaDC\n");
			//	exit(-1);
			//}
			//numcoeff =  currSE.value1;
			//numtrailingones =  currSE.value2;

			numcoeff        =  4;
			numtrailingones =  3;			

			currStream->frame_bitoffset += 7;
			
		}

	}

	numones = numtrailingones;
	*number_coefficients = numcoeff;
	
	if (numcoeff)
	{
		memset(runarr, 0, max_coeff_num*sizeof(int));
		if (numtrailingones)// construct the trailing ones
		{   			
		  	//readSyntaxElement_FLC(&currSE, dP->bitstream);
			{
				unsigned int frame_bitoffset = currStream->frame_bitoffset;
				unsigned int BitstreamLengthInBytes = currStream->bitstream_length;
					

				byteoffset= frame_bitoffset>>3;	
				bitoffset = (frame_bitoffset&7);

				streamWord = (( (unsigned int)buf[byteoffset])<<8 ) | (((unsigned int)buf[byteoffset+1])) ;
				//streamWord = streamWord<<(bitoffset);

				util_1 = ((streamWord)>>(16-numtrailingones-bitoffset)) & ((1<<numtrailingones) - 1);


				
				currStream->frame_bitoffset += numtrailingones; // move bitstream pointer
			}
						
			k = numcoeff-numtrailingones;
			///??? need to reduce the dimension of trailing one table

			for(i=0;i<numtrailingones;i++)
			{
				levarr[k++] = trailing_ones[numtrailingones][util_1][i];
			}

			//levarr[k++] = trailing_ones[numtrailingones][util_1][0];
			//levarr[k++] = trailing_ones[numtrailingones][util_1][1];
			//levarr[k] = trailing_ones[numtrailingones][util_1][2];
		}
		
		// decode levels
		level_two_or_higher = 1;
		if (numcoeff > 3 && numtrailingones == 3)
			level_two_or_higher = 0;
		
		if (numcoeff > 10 && numtrailingones < 3)
			vlcnum = 1;
		else
			vlcnum = 0;
		
		for (k = numcoeff - 1 - numtrailingones; k >= 0; k--)
		{
			
			//unsigned long byteoffset;	// byte from start of buffer
			//int bitoffset;		// bit from start of byte
			
			unsigned int frame_bitoffset = currStream->frame_bitoffset;
			
			unsigned int BitstreamLengthInBytes = currStream->bitstream_length;
			int  sign=0;
			int  addbit;
			

			byteoffset = frame_bitoffset>>3;
			bitoffset = frame_bitoffset&7;		
		
			streamWord = ((((unsigned int)buf[byteoffset]) << 24) | (((unsigned int)buf[byteoffset+1]) << 16) | (((unsigned int)buf[byteoffset+2]) << 8) | ((unsigned int)buf[byteoffset+3]) );
			streamWord = streamWord << bitoffset;			

		
			streamWord = 31 - norm_x(streamWord);
			


			if (vlcnum == 0)
			{
				//readSyntaxElement_Level_VLC0(&currSE, dP);
				streamWord++;
				byteoffset = 1;
				frame_bitoffset += streamWord;
				
				if (streamWord < 15)
				{
					sign = (streamWord - 1) & 1;
					bitoffset = ((streamWord-1) >> 1) + 1;
				}
				else if (streamWord == 15)
				{	
					// escape code
					byteoffset = (byteoffset << 4) | ShowBits(buf, frame_bitoffset, BitstreamLengthInBytes, 4);
					streamWord += 4;
					frame_bitoffset += 4;
					sign = (byteoffset & 1);
					bitoffset = ((byteoffset >> 1) & 0x7) + 8;
					
				}
				else if (streamWord >= 16)
				{
					
					// escape code
					addbit=streamWord-16;
					
					streamWord  = (streamWord-4);
					byteoffset = ShowBits(buf, frame_bitoffset, BitstreamLengthInBytes, streamWord);
					
					frame_bitoffset += streamWord;
					sign =  (byteoffset & 1);
					
					bitoffset = (byteoffset >> 1) + ((2048<<addbit)+16-2048);
					byteoffset |= (1 << (streamWord)); // for display purpose only
					streamWord += addbit + 16;
				}
				
				if (sign)
					bitoffset = -bitoffset;
				util_1 = bitoffset;
				currStream->frame_bitoffset = frame_bitoffset;
				
			}
			else
			{
				//readSyntaxElement_Level_VLCN(&currSE, vlcnum, dP);
				int byteoffset , bitoffset;
				int shift = vlcnum-1;
				bitoffset =  streamWord +1;
				if (streamWord < 15)
				{
					streamWord = (streamWord<<shift) + 1;
					byteoffset =  ShowBits(buf, frame_bitoffset+bitoffset, BitstreamLengthInBytes, shift+1);
					if (shift)
					{
						streamWord += (byteoffset>>1);
						bitoffset += (shift);
						byteoffset = byteoffset & 1;
					}
					bitoffset ++;
				}
				else // escape
				{
					streamWord = streamWord - 15;
					byteoffset = ShowBits(buf, frame_bitoffset+bitoffset, BitstreamLengthInBytes, (12+streamWord));
					
					bitoffset   += (11+streamWord);
					streamWord = (byteoffset>>1) + (2048<<streamWord)+((15<<shift)+1)-2048;
					
					byteoffset = byteoffset & 1;
					bitoffset++;
				}
				
				util_1 = (byteoffset)?-(int)streamWord:streamWord;
				
				currStream->frame_bitoffset = frame_bitoffset+bitoffset;
			}
			
			if (level_two_or_higher)
			{
				if (util_1 > 0)
					util_1 ++;
				else
					util_1 --;
				level_two_or_higher = 0;
			}
			
			level = levarr[k] = util_1;
			if (absz(level) == 1)
				numones ++;
			
			// update VLC table
			if (absz(level)>incVlc[vlcnum])
				vlcnum++;
			
			if (k == numcoeff - 1 - numtrailingones && absz(level)>3)
				vlcnum = 2;
			
		}
		
		if (numcoeff < max_coeff_num)
		{
									
			if (block_type ==1)
			{
				//readSyntaxElement_TotalZerosChromaDC(&currSE, dP,dec_params);
				//int bitoffset;		// bit from start of byte
				int frame_bitoffset = currStream->frame_bitoffset;
				

				byteoffset= frame_bitoffset>>3;	
				bitoffset = (frame_bitoffset&7);

				streamWord = (( (unsigned int)buf[byteoffset])<<8 ) | (((unsigned int)buf[byteoffset+1])) ;
				streamWord = (streamWord<<bitoffset) &0xffff;

				streamWord = streamWord>>(12+numcoeff);
				
				bitoffset = numcoeff-1;				
				
				totzeros = CH_TOT_ZERO[bitoffset][streamWord][0];
				currStream->frame_bitoffset += CH_TOT_ZERO[bitoffset][streamWord][1];
				
			}
			else
			{
				//readSyntaxElement_TotalZeros(&currSE, dP);
				int vlcnum;
				int code,codelength;
				int byteoffset , frame_bitoffset ,bitoffset;
				
				currSE.value1 = numcoeff-1;
				vlcnum = currSE.value1;
										
				//retval = code_from_bitstream_2d(&currSE, dP, &lentab_TotalZeros[vlcnum][0], &codtab_TotalZeros[vlcnum][0], 16, 1, &code);
					
				//if (code_from_bitstream_2d(&currSE, dP, &lentab_TotalZeros[vlcnum][0], &codtab_TotalZeros[vlcnum][0], 16, 1, &code))
				//{
				//	printf("ERROR: failed to find Total Zeros\n");
				//	exit(-1);
				//}
				//totzeros = currSE.value1;   
				codelength = totalzeros_lengths[numcoeff];

				frame_bitoffset = currStream->frame_bitoffset;	

				byteoffset = frame_bitoffset>>3;
				bitoffset = 9+7-(frame_bitoffset&7);				
				
				streamWord = (((unsigned int)buf[byteoffset])<<8) | (((unsigned int)buf[byteoffset+1]));				

				code = (streamWord >> (bitoffset-codelength)) & ((1<<codelength) - 1);		// read n-bit code

				if (code) 
				{


					tabrow = norm_x(code);


					if (numcoeff == 1) 
					{
						tabcol = (code >> (tabrow-1)) & 1;
						totzeros = (totalzeros_tc1[8-tabrow][tabcol])&15;
						currStream->frame_bitoffset += (totalzeros_tc1[8-tabrow][tabcol]) >> 4;
					}
					else if (numcoeff == 2)
					{
						if (code == 2) 
						{
							totzeros = 12;
						    currStream->frame_bitoffset += 6;
						}
						else if (code == 3) 
						{
							totzeros = 11;
						    currStream->frame_bitoffset += 6;
						}
						else
						{
							tabcol = (code >> (tabrow-2)) & 3;
							totzeros = (totalzeros_tc2[5-tabrow][tabcol])&15;
							currStream->frame_bitoffset += (totalzeros_tc2[5-tabrow][tabcol]) >> 4;					
						}
					}
					else if (numcoeff == 3)
					{
						tabcol = (code >> (tabrow-2)) & 3;
						totzeros = (totalzeros_tc3[5-tabrow][tabcol])&15;
						currStream->frame_bitoffset += (totalzeros_tc3[5-tabrow][tabcol]) >> 4;

					}
					else if (numcoeff == 4)
					{
						if (code == 2) 
						{
							totzeros = 10;
						    currStream->frame_bitoffset += 5;
						}
						else if (code == 3) 
						{
							totzeros = 0;
						    currStream->frame_bitoffset += 5;
						}
						else
						{
						  tabcol = (code >> (tabrow-2)) & 3;
						  totzeros = (totalzeros_tc4[4-tabrow][tabcol])&15;
						  currStream->frame_bitoffset += (totalzeros_tc4[4-tabrow][tabcol]) >> 4;
						}
						

					}
					else if (numcoeff == 5)
					{
						tabcol = (code >> (tabrow-2)) & 3;
						totzeros = (totalzeros_tc5[4-tabrow][tabcol])&15;
						currStream->frame_bitoffset += (totalzeros_tc5[4-tabrow][tabcol]) >> 4;

					}
					else if (numcoeff == 6)
					{
						tabcol = (code >> (tabrow-2)) & 3;
						totzeros = (totalzeros_tc6[5-tabrow][tabcol])&15;
						currStream->frame_bitoffset += (totalzeros_tc6[5-tabrow][tabcol]) >> 4;

					}
					else if (numcoeff == 7)
					{
						tabcol = (code >> (tabrow-2)) & 3;
						totzeros = (totalzeros_tc7[5-tabrow][tabcol])&15;
						currStream->frame_bitoffset += (totalzeros_tc7[5-tabrow][tabcol]) >> 4;

					}
					else if (numcoeff == 8)
					{
						tabcol = (code >> (tabrow-1)) & 1;
						totzeros = (totalzeros_tc8[5-tabrow][tabcol])&15;
						currStream->frame_bitoffset += (totalzeros_tc8[5-tabrow][tabcol]) >> 4;

					}
					else if (numcoeff == 9)
					{
						tabcol = (code >> (tabrow-1)) & 1;
						totzeros = (totalzeros_tc9[5-tabrow][tabcol])&15;
						currStream->frame_bitoffset += (totalzeros_tc9[5-tabrow][tabcol]) >> 4;
					}
					else if (numcoeff == 10)
					{
						tabcol = (code >> (tabrow-1)) & 1;
						totzeros = (totalzeros_tc10[4-tabrow][tabcol])&15;
						currStream->frame_bitoffset += (totalzeros_tc10[4-tabrow][tabcol]) >> 4;
					}
					else if (numcoeff == 11)
					{
						tabcol = (code >> (tabrow-1)) & 1;
						totzeros = (totalzeros_tc11[3-tabrow][tabcol])&15;
						currStream->frame_bitoffset += (totalzeros_tc11[3-tabrow][tabcol]) >> 4;
					}
					else if (numcoeff == 12)
					{
						tabcol = (code >> (tabrow-1)) & 1;
						totzeros = (totalzeros_tc12[3-tabrow][tabcol])&15;
						currStream->frame_bitoffset += (totalzeros_tc12[3-tabrow][tabcol]) >> 4;
					}
					else if (numcoeff == 13)
					{
						tabcol = (code >> (tabrow-1)) & 1;
						totzeros = (totalzeros_tc13[2-tabrow][tabcol])&15;
						currStream->frame_bitoffset += (totalzeros_tc13[2-tabrow][tabcol]) >> 4;
					}
					else if (numcoeff == 14)
					{
						if(code == 1)
						{
							totzeros = 1;
							currStream->frame_bitoffset += 2;
						}
						else
						{
							totzeros = 2;
							currStream->frame_bitoffset += 1;
						}						
					}
					else //if (numcoeff == 15)
					{
							totzeros = 1;
							currStream->frame_bitoffset += 1;
					}
					//else
					//{
					//	if (code_from_bitstream_2d(&currSE, dP, &lentab_TotalZeros[vlcnum][0], &codtab_TotalZeros[vlcnum][0], 16, 1, &code))
					//	{
					//		printf("ERROR: failed to find Total Zeros\n");
					//		exit(-1);
					//	}
					//	totzeros = currSE.value1;   
					//}					
				}
				else
				{
					totzeros = tatalzeros_codezero[numcoeff];
					currStream->frame_bitoffset += totalzeros_lengths[numcoeff];
				}
			}
		}
		else
		{
			totzeros = 0;
		}
		
		// decode run before each coefficient
		zerosleft = totzeros;
		i = numcoeff-1;
		if (zerosleft > 0 && i > 0)
		{
			do 
			{	// select VLC for runbefore
				vlcnum = run_before_table[zerosleft - 1];
				{
					int frame_bitoffset = currStream->frame_bitoffset;
					int BitstreamLengthInBytes = currStream->bitstream_length;
					int length = 0;
					int run = 0;
									

	
					byteoffset= frame_bitoffset>>3;	
					bitoffset = (frame_bitoffset&7);

					//streamWord = (( (unsigned int)buf[byteoffset])<<8 ) | (((unsigned int)buf[byteoffset+1])) ;
					streamWord = ((((unsigned int)buf[byteoffset]) << 24) | (((unsigned int)buf[byteoffset+1]) << 16) | (((unsigned int)buf[byteoffset+2]) << 8) | ((unsigned int)buf[byteoffset+3]) );
					streamData = ((streamWord<<bitoffset) >> 16);								

					if((vlcnum) < 6)
					{
						streamData = streamData>>shift_table[vlcnum];
						run = run_table[vlcnum][streamData];
						length = length_table[vlcnum][streamData];
					}
					else
					{
						if((streamData)>= 8192)
						{
							
							streamData = streamData>>shift_table[vlcnum];
							run = run_table[vlcnum][streamData];
							
							length = 3;//length_table[vlcnum][streamData];
						}
						else
						{


							runlength = norm_x(streamData);

							run    = 15 - runlength + 4;
							length = 15 - runlength + 1;
						}
					}
					zerosleft -= runarr[i] = run;//
					currStream->frame_bitoffset += length; // move bitstream pointer
				}
				i --;
			} while (zerosleft != 0 && i != 0);
		}
		runarr[i] = zerosleft;
	} // if numcoeff
}