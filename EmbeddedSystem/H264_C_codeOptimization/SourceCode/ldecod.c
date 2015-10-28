 
/*!
***********************************************************************
*  \mainpage
*     This is the H.264/AVC decoder reference software. For detailed documentation
*     see the comments in each file.
*
*  \file
*     ldecod.c
*  \brief
*     H.264/AVC reference decoder project main()
*
***********************************************************************
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
//#include <sys/timeb.h>

/*#if defined WIN32
#include <io.h>
#else
//#include <unistd.h>
#endif*/

//#include <sys/stat.h>
//#include <fcntl.h>


#include <assert.h>

#include "global.h"
#include "rtp.h"
#include "memalloc.h"
#include "mbuffer.h"
#include "leaky_bucket.h"
#include "fmo.h"
#include "annexb.h"
#include "output.h"
//#include "cabac.h"
#include "nalu.h"
#include "erc_api.h"
#include "parset.h"

#define JM          "9 (FRExt)"
#define VERSION     "9.8"
#define EXT_VERSION "(FRExt)"

#define LOGFILE     "log.dec"
#define DATADECFILE "dataDec.txt"
#define TRACEFILE   "trace_dec.txt"

//extern objectBuffer_t *erc_object_list;/*Changed by Saad Bin Shams [Removing Global Variables]*/
//extern ercVariables_t *erc_errorVar;   /*Changed by Saad Bin Shams [Removing Global Variables]*/
//extern ColocatedParams *Co_located;    /*Changed by Saad Bin Shams [Removing Global Variables]*/

// I have started to move the inp and img structures into global variables.
// They are declared in the following lines.  Since inp is defined in conio.h
// and cannot be overridden globally, it is defined here as input
//
// Everywhere, input-> and img-> can now be used either globally or with
// the local override through the formal parameter mechanism

//extern FILE* bits;					/*Changed by Saad Bin Shams [Removing Global Variables]*/	
//extern StorablePicture* dec_picture;  /*Changed by Saad Bin Shams [Removing Global Variables]*/
//struct inp_par    *input;				/*Changed by Saad Bin Shams [Removing Global Variables]*/		
//struct snr_par    *snr;				/*Changed by Saad Bin Shams [Removing Global Variables]*/	
//struct img_par    *img;               /*Changed by Saad Bin Shams [Removing Global Variables]*/ 

//int global_init_done = 0;


/*!
***********************************************************************
* \brief
*   print help message and exit
***********************************************************************
*/


extern void idct4x4dc( short d[4][4] );
extern void add4x4_idct( imgpel *p_dst, unsigned int dst_stride, short dct[4][4] );



void JMDecHelpExit ()
{
	fprintf( stderr, "\n   ldecod [-h] {[defdec.cfg] | {[-i bitstream.264]...[-o output.yuv] [-r reference.yuv] [-uv]}}\n\n"    
		"## Parameters\n\n"
		
		"## Options\n"                  
		"   -h  :  prints function usage\n"
		"       :  parse <defdec.cfg> for decoder operation.\n"
		"   -i  :  Input file name. \n"
		"   -o  :  Output file name. If not specified default output is set as test_dec.yuv\n\n"
		"   -r  :  Reference file name. If not specified default output is set as test_rec.yuv\n\n"
		"   -uv :  write chroma components for monochrome streams(4:2:0)\n\n"
		
		"## Supported video file formats\n"
		"   Input : .264 -> H.264 bitstream files. \n"
		"   Output: .yuv -> RAW file. Format depends on bitstream information. \n\n"
		
		"## Examples of usage:\n"
		"   ldecod\n"
		"   ldecod  -h\n"
		"   ldecod  default.cfg\n"
		"   ldecod  -i bitstream.264 -o output.yuv -r reference.yuv\n");
	
	exit(-1);
}



/*
*******************************************************************
*	Function Argument List Changed [Removing Global Variables] 
*	Input parameters added are 
*		- h264_decoder* dec_params, 
*
*	<saad.shams@inforient.com>
*******************************************************************
*/
void Configure(int ac, char *av[],   h264_decoder* dec_params )
{
    int CLcount;
    char *config_filename=NULL;
    CLcount = 1;
    
   
    strcpy(dec_params->input->infile,"test.264");      //! set default bitstream name

    strcpy(dec_params->input->outfile,"test_dec.yuv"); //! set default output file name

    //strcpy(dec_params->input->reffile,"test_rec.yuv"); //! set default reference file name
    dec_params->input->FileFormat = PAR_OF_ANNEXB;
    //dec_params->input->ref_offset=0;
    dec_params->input->poc_scale=1;
    
#ifdef _LEAKYBUCKET_

    dec_params->input->R_decoder=500000;          //! Decoder rate
    dec_params->input->B_decoder=104000;          //! Decoder buffer size
    dec_params->input->F_decoder=73000;           //! Decoder initial delay
    strcpy(dec_params->input->LeakyBucketParamFile,"leakybucketparam.cfg");    // file where Leaky Bucket params (computed by encoder) are stored

#endif
    
    if (ac==2)
    {

        if (0 == strncmp (av[1], "-h", 2))
        {
            JMDecHelpExit();

        }
        else
        {
            config_filename=av[1];

            init_conf(dec_params->input, av[1],dec_params);

        }
        CLcount=2;
    }
    
    if (ac>=3)
    {

        if (0 == strncmp (av[1], "-i", 2))
        {
            strcpy(dec_params->input->infile,av[2]);
            CLcount = 3;
        }
        if (0 == strncmp (av[1], "-h", 2))
        {
            JMDecHelpExit();
        }
    }
    
    // Parse the command line
    
    while (CLcount < ac)
    {
        if (0 == strncmp (av[CLcount], "-h", 2))
        {
            JMDecHelpExit();
        }
        
        if (0 == strncmp (av[CLcount], "-i", 2))  //! Input file
        {
            strcpy(dec_params->input->infile,av[CLcount+1]);      
            CLcount += 2;
        } 
        else if (0 == strncmp (av[CLcount], "-o", 2))  //! Output File
        {
            strcpy(dec_params->input->outfile,av[CLcount+1]);
            CLcount += 2;
        }
        //else if (0 == strncmp (av[CLcount], "-r", 2))  //! Reference File
        //{
        //	strcpy(dec_params->input->reffile,av[CLcount+1]);
        //	CLcount += 2;
        //}
        //else if (0 == strncmp (av[CLcount], "-uv", 2))  //! indicate UV writing for 4:0:0
        //{
        //dec_params->input->write_uv = 1;
        //	CLcount ++;
        //}
        else
        {
            //config_filename=av[CLcount];
            //init_conf(input, config_filename);
            //snprintf(dec_params->errortext, ET_SIZE, "Invalid syntax. Use ldecod -h for proper usage");
            printf("Invalid syntax. Use ldecod -h for proper usage.\n");
            exit(0);
        }
    }
    
    
#if TRACE
    if ((p_trace=fopen(TRACEFILE,"w"))==0)             // append new statistic at the end
    {
        snprintf(dec_params->errortext, ET_SIZE, "Error open file %s!",TRACEFILE);
        printf(dec_params->errortext);
		exit(0);
    }
#endif
    
    
//#ifdef WRITE_PICTURE
    //if ((dec_params->p_out=open(dec_params->input->outfile, OPENFLAGS_WRITE, OPEN_PERMISSIONS))==-1)
    //{
    //    //		snprintf(dec_params->errortext, ET_SIZE, "Error open file %s ",dec_params->input->outfile);
    //    printf("Error open file %s \n",dec_params->input->outfile);
    //    exit(0);
    //}
	
	if ((dec_params->f_out = fopen(dec_params->input->outfile,"wb"))==0)
    {
        //		snprintf(dec_params->errortext, ET_SIZE, "Error open file %s ",dec_params->input->outfile);
        printf("Error open file %s \n",dec_params->input->outfile);
        exit(0);
    }


//#endif
    /*  if ((p_out2=fopen("out.yuv","wb"))==0)
    {
    snprintf(errortext, ET_SIZE, "Error open file %s ",input->outfile);
    error(errortext,500);
  }*/
    
    
    fprintf(stdout,"----------------------------- JM %s %s -----------------------------\n", VERSION, EXT_VERSION);
        fprintf(stdout,"--------------------------------------------------------------------------\n");
    fprintf(stdout," Input H.264 bitstream                  : %s \n",dec_params->input->infile);
    fprintf(stdout," Output decoded YUV                     : %s \n",dec_params->input->outfile);
        
    fprintf(stdout,"--------------------------------------------------------------------------\n");
#ifdef _LEAKYBUCKET_
    fprintf(stdout," Rate_decoder        : %8ld \n",dec_params->input->R_decoder);
    fprintf(stdout," B_decoder           : %8ld \n",dec_params->input->B_decoder);
    fprintf(stdout," F_decoder           : %8ld \n",dec_params->input->F_decoder);
    fprintf(stdout," LeakyBucketParamFile: %s \n",dec_params->input->LeakyBucketParamFile); // Leaky Bucket Param file
    calc_buffer(dec_params->input);
    fprintf(stdout,"--------------------------------------------------------------------------\n");
#endif
    fprintf(stdout,"--------------------------------------------------------------------------\n");
    fprintf(stdout,"  Frame       POC   Pic#   QP   SnrY    SnrU    SnrV   Y:U:V  Time(ms)\n");
    fprintf(stdout,"--------------------------------------------------------------------------\n");
    
}

/***************************************************************************
	Function Added to Initialize the "h264_decoder" Structure
	Variable [Removing Global Variables] 
	Input parameters added are 
		- h264_decoder* dec_params, 
		
	<saad.shams@inforient.com>
***************************************************************************/
void Init_int_binCount(  h264_decoder*dec_params)
{
//	dec_params->binCount=0;
}
/***************************************************************************
	Function Added to Initialize the "h264_decoder" Structure
	Variable [Removing Global Variables] 
	Input parameters added are 
		- h264_decoder* dec_params, 
		
	<saad.shams@inforient.com>
***************************************************************************/
void Init_erc_errorVar(  h264_decoder* dec_params)
{
	dec_params->erc_errorVar = NULL;
}
/***************************************************************************
	Function Added to Initialize the "h264_decoder" Structure
	Variable [Removing Global Variables] 
	Input parameters added are 
		- h264_decoder* dec_params, 
		
	<saad.shams@inforient.com>
***************************************************************************/
void Init_erc_object_list(  h264_decoder* dec_params)
{
	dec_params->erc_object_list = NULL;
}
/***************************************************************************
	Function Added to Initialize the "h264_decoder" Structure
	Variable [Removing Global Variables] 
	Input parameters added are 
		- h264_decoder* dec_params, 
		
	<saad.shams@inforient.com>
***************************************************************************/
void Init_global_init_done(  h264_decoder*dec_params)
{
	dec_params->global_init_done = 0;
}
/***************************************************************************
	Function Added to Initialize the "h264_decoder" Structure
	Variable [Removing Global Variables] 
	Input parameters added are 
		- h264_decoder* dec_params, 
		
	<saad.shams@inforient.com>
***************************************************************************/
void Init_IsFirstByteStreamNALU(  h264_decoder*dec_params)
{
	dec_params->IsFirstByteStreamNALU=1;
}
/***************************************************************************
	Function Added to Initialize the "h264_decoder" Structure
	Variable [Removing Global Variables] 
	Input parameters added are 
		- h264_decoder* dec_params, 
		
	<saad.shams@inforient.com>
***************************************************************************/
void Init_last_dquant(  h264_decoder*dec_params)
{
	dec_params->last_dquant = 0;
}
/***************************************************************************
	Function Added to Initialize the "h264_decoder" Structure
	Variable [Removing Global Variables] 
	Input parameters added are 
		- h264_decoder* dec_params, 
		
	<saad.shams@inforient.com>
***************************************************************************/
void Init_LastAccessUnitExists(  h264_decoder*dec_params)
{
	dec_params->LastAccessUnitExists=0;
}
/***************************************************************************
	Function Added to Initialize the "h264_decoder" Structure
	Variable [Removing Global Variables] 
	Input parameters added are 
		- h264_decoder* dec_params, 
		
	<saad.shams@inforient.com>
***************************************************************************/
void Init_MapUnitToSliceGroupMap(  h264_decoder* dec_params)
{
	dec_params->MapUnitToSliceGroupMap = NULL; 
}
/***************************************************************************
	Function Added to Initialize the "h264_decoder" Structure
	Variable [Removing Global Variables] 
	Input parameters added are 
		- h264_decoder* dec_params, 
		
	<saad.shams@inforient.com>
***************************************************************************/
void Init_MbToSliceGroupMap(  h264_decoder* dec_params)
{
	dec_params->MbToSliceGroupMap = NULL;
}
/***************************************************************************
	Function Added to Initialize the "h264_decoder" Structure
	Variable [Removing Global Variables] 
	Input parameters added are 
		- h264_decoder* dec_params, 
		
	<saad.shams@inforient.com>
***************************************************************************/
void Init_NALUCount(  h264_decoder* dec_params)
{
	dec_params->NALUCount=0;
}
void Init_pending_output_state( h264_decoder* dec_params)
{
//dec_params->pending_output_state=FRAME;
}
/***************************************************************************
	Function Added to Initialize the "h264_decoder" Structure
	Variable [Removing Global Variables] 
	Input parameters added are 
		- h264_decoder* dec_params, 
		
	<saad.shams@inforient.com>
**************************************************************************/
void Init_bitcounter( h264_decoder* dec_params)
{
	dec_params->bitcounter=0;
}
/***************************************************************************
	Function Added to Call the various variable-initialization functions
	[Removing Global Variables] 
	Input parameters added are 
		- h264_decoder* dec_params, 
		
	<saad.shams@inforient.com>
***************************************************************************/
void Initializations( h264_decoder*dec_params )
{
	Init_int_binCount(dec_params);
	Init_erc_errorVar(dec_params);
	Init_erc_object_list(dec_params);
	Init_global_init_done(dec_params);
	Init_IsFirstByteStreamNALU(dec_params);
	Init_last_dquant(dec_params);
	Init_LastAccessUnitExists(dec_params);
	Init_MapUnitToSliceGroupMap(dec_params);
	Init_MbToSliceGroupMap(dec_params);
	Init_NALUCount(dec_params);
	Init_pending_output_state(dec_params);
	Init_bitcounter(dec_params);

	/* Initialize the global buffer pointers */

//	dec_params->img->nz_coeff    = NULL;
	dec_params->img->mb_data     = NULL; 
	dec_params->img->intra_block = NULL;
	dec_params->img->ipredmode   = NULL;
	dec_params->img->wp_weight   = NULL;
	dec_params->img->wp_offset   = NULL;
	dec_params->img->wbp_weight  = NULL;
//	dec_params->img->quad        = NULL;
	dec_params->frame_no		 = 0;




	dec_params->img->p_add4x4_idct = add4x4_idct;
	dec_params->img->p_idct4x4dc = idct4x4dc;


}

/*!
***********************************************************************
* \brief
*    main function for TML decoder
*---------------------------------------------------------------------
*	- Conditional structure introduced to handle baseline profile
*	  seperately.
*	- Structured the code such that ProcessSPS() and ProcessPPS()
*	  are called from here and not in the function 
*
*	 "Muhammad Tahir Awan" <tahir.awan@inforient.com>,
*	 "Umair Razzak" <umair.razzak@inforient.com>
****************************
*	 Changes till 21-11-2005
***********************************************************************
*/

int main(int argc, char **argv)
{
	int Flag = TRUE;

	//long ftell_position;//, expected_slice_type;
	//int ret;
	NALU_t *nalu;
	
	h264_decoder* dec_params;
	char *ptr;
	
	ptr =  (  char*)malloc(10 * sizeof(char)); 

    if ((dec_params =  (  h264_decoder*)h264_malloc(1 * sizeof(h264_decoder)))==NULL) 
 //	if ((dec_params =  (  h264_decoder*)h264_malloc(1 * 100))==NULL) 
	{
		Flag=FALSE;
	}

  	if ((dec_params->input =  (InputParameters *)h264_malloc(1 * sizeof(InputParameters)))==NULL) 
	{
		printf("main: input");
		exit(0);
	}
	if ((dec_params->img   =  (ImageParameters *)h264_malloc(1 * sizeof(ImageParameters)))==NULL) 
	{
		printf("main: img");
		exit(0);
	}

	////////// ADDED INITIALIZATIONS //////
	dec_params->active_sps       = NULL;
	dec_params->active_pps       = NULL;
 	dec_params->Co_located       = NULL;
	dec_params->dec_picture     = NULL;
	dec_params->global_init_done = 0;
	dec_params->dpb.init_done	 = 0;
	dec_params->img->DeblockCall = 0;
//	dec_params->img->structure   = 0;
	dec_params->MapUnitToSliceGroupMap = NULL;
	dec_params->MbToSliceGroupMap      = NULL;
	dec_params->img->errorConcealmentFlag = 0;			// ERROR CONCEALMENT FLAG SET TO ZERO , NO ERROR CONCEALMENT
	dec_params->img->no_output_of_prior_pics_flag = -1;

	///////////////////////////////////////
	Initializations(dec_params);
	
	Configure ( argc, argv,dec_params );
	
		
	init_old_slice(dec_params);
	
	switch (dec_params->input->FileFormat)
	{
	case 0:
		OpenBitstreamFile (dec_params->input->infile,dec_params);
		break;
	case 1:
		OpenRTPFile (dec_params->input->infile,dec_params);
		break;
	default:
		printf ("Unsupported file format %d, exit\n", dec_params->input->FileFormat);
	}
	
	// Allocate Slice data struct
	malloc_slice(dec_params->input,dec_params->img,dec_params);
	
	init(dec_params->img,dec_params);
	
	dec_params->dec_picture = NULL;
	
	dec_params->dpb.init_done = 0;
	//dec_params->g_nFrame = 0;
	
//	init_out_buffer(dec_params);
	
  //dec_params->img->idr_psnr_number=dec_params->input->ref_offset;
  //dec_params->img->psnr_number=0;
	
	dec_params->img->number=0;
	dec_params->img->type = I_SLICE;
	dec_params->img->dec_ref_pic_marking_buffer = NULL;
	
	// B pictures
	dec_params->Bframe_ctr=0;

	// time for total decoding session
	dec_params->tot_time = 0;

	dec_params->nalu = AllocNALU(MAX_CODED_FRAME_SIZE,dec_params);
	nalu = dec_params->nalu;

	dec_params->dp   = AllocPartition(1, dec_params,1024);
	dec_params->sps  = AllocSPS();
	dec_params->pps  = AllocPPS();


	{
		while ( decode_one_frame( dec_params ) != EOS );
	}	
	//report( dec_params->input, dec_params->img, dec_params->snr,dec_params,);
	report( dec_params->input, dec_params->img,dec_params);
	free_slice( dec_params->input,dec_params->img);

	FreeNALU(dec_params->nalu);
	FreePartition (dec_params->dp, 1);
	FreeSPS (dec_params->sps);
    if (dec_params->PicParSet[dec_params->pps->pic_parameter_set_id].Valid == TRUE && dec_params->PicParSet[dec_params->pps->pic_parameter_set_id].slice_group_id != NULL)
	{
		h264_free (dec_params->PicParSet[dec_params->pps->pic_parameter_set_id].slice_group_id);
		dec_params->PicParSet[dec_params->pps->pic_parameter_set_id].slice_group_id = NULL;
	}
	
	// IF FMO PRESENT
	if (dec_params->active_pps->num_slice_groups_minus1) 
	{
	FmoFinit(dec_params);
	}	

	FreePPS (dec_params->pps);
	
	
	free_global_buffers_baseline(dec_params);
	
	flush_dpb(dec_params);
	
#ifdef PAIR_FIELDS_IN_OUTPUT
	flush_pending_output(dec_params->p_out);
#endif
	
	CloseBitstreamFile(dec_params);
	
	//close(dec_params->p_out);
	fclose (dec_params->f_out);
	//  fclose(p_out2);

#if TRACE
	fclose(p_trace);
#endif
	
	//ercClose(dec_params->erc_errorVar,dec_params);
	ercClose(dec_params);
	
	free_dpb(dec_params);
//	uninit_out_buffer(dec_params);
	
	free_colocated(dec_params);
	if(dec_params->dec_picture != NULL)
	{
		free (dec_params->dec_picture);
		dec_params->dec_picture = NULL;
	}
	if(dec_params->input != NULL)
	{
		h264_free (dec_params->input);
		dec_params->input = NULL;
	}
	if(dec_params->img != NULL)
	{
		h264_free (dec_params->img);
		dec_params->img = NULL;
	}
	if(dec_params != NULL)
	{
		h264_free (dec_params);
		dec_params = NULL;
	}
	return 0;
}


/*!
***********************************************************************
* \brief
*   Initilize some arrays
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params, 
*
*		<saad.shams@inforient.com>
*

***********************************************************************
*/
void init(ImageParameters *img,  h264_decoder* dec_params)  //!< image parameters
{
//	img->oldFrameSizeInMbs = -1;
	//dec_params->imgY_ref  = NULL;
	//dec_params->imgUV_ref = NULL;
}

/*!
***********************************************************************
* \brief
*    Initilize FREXT variables
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params, 
*
*		<saad.shams@inforient.com>
*
***********************************************************************
*/
void init_frext(h264_decoder* dec_params)  //!< image parameters
{
	ImageParameters *img=dec_params->img;
	//pel bitdepth init
	//img->bitdepth_luma_qp_scale   = 0;
	//img->pic_unit_bitsize_on_disk = 8;
	//img->dc_pred_value = 1<<7;
	//img->max_imgpel_value = (1<<8) - 1;
	//for chrominance part
	//img->bitdepth_chroma_qp_scale = 0;
	//img->max_imgpel_value_uv      = (1<<8) - 1;
	//img->num_blk8x8_uv = 2;//(1<<1)&(~(0x1));
	//img->num_cdc_coeff = 4;//img->num_blk8x8_uv<<1;
	//Residue Color Transform
//	if(img->residue_transform_flag)
//	{
//		img->bitdepth_chroma_qp_scale += 6;
//	}
}


/*!
************************************************************************
* \brief
*    Read input from configuration file
*
* \par Input:
*    Name of configuration filename
*
* \par Output
*    none
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params, 
*
*		<saad.shams@inforient.com>
*
************************************************************************
*/
 void init_conf(InputParameters *inp, char *config_filename,  h264_decoder* dec_params )
{
	FILE *fd;
	int NAL_mode;
	int write_uv;
	int snr_offset;
	char reffile[100];
	// read the decoder configuration file
	if((fd=fopen(config_filename,"r")) == NULL)
	{
		//snprintf(dec_params->errortext, ET_SIZE, "Error: Control file %s not found\n",config_filename);
		printf("Error: Control file %s not found\n",config_filename);
		exit(0);
	}
	fscanf(fd,"%s",inp->infile);                // H.264 compressed input bitstream
	fscanf(fd,"%*[^\n]");
	
	fscanf(fd,"%s",inp->outfile);               // RAW (YUV/RGB) output file
	fscanf(fd,"%*[^\n]");
	
	fscanf(fd,"%s",reffile);               // reference file
	fscanf(fd,"%*[^\n]");
	
	fscanf(fd,"%d",&(write_uv));           // write UV in YUV 4:0:0 mode
	fscanf(fd,"%*[^\n]");
	
	fscanf(fd,"%d",&(NAL_mode));                // NAL mode
	fscanf(fd,"%*[^\n]");
	
	switch(NAL_mode)
	{
	case 0:
	
		inp->FileFormat = PAR_OF_ANNEXB;
		break;
	case 1:
	
		inp->FileFormat = PAR_OF_RTP;
		break;
	default:
		//snprintf(dec_params->errortext, ET_SIZE, "NAL mode %i is not supported", NAL_mode);
		printf("NAL mode %i is not supported\n", NAL_mode);
		exit(0);
	}
	fscanf(fd,"%d,",&(snr_offset));   // offset used for SNR computation
	fscanf(fd,"%*[^\n]");

	fscanf(fd,"%d,",&inp->poc_scale);   // offset used for SNR computation
	fscanf(fd,"%*[^\n]");
	

	if (inp->poc_scale < 1 || inp->poc_scale > 10)
	{
			
		//snprintf(dec_params->errortext, ET_SIZE, "Poc Scale is %d. It has to be within range 1 to 10",inp->poc_scale);
		printf("Poc Scale is %d. It has to be within range 1 to 10\n",inp->poc_scale);
		exit(0);
	}
	
	//inp->write_uv=1;
	
#ifdef _LEAKYBUCKET_
	fscanf(fd,"%ld,",&inp->R_decoder);             // Decoder rate
	fscanf(fd, "%*[^\n]");
	fscanf(fd,"%ld,",&inp->B_decoder);             // Decoder buffer size
	fscanf(fd, "%*[^\n]");
	fscanf(fd,"%ld,",&inp->F_decoder);             // Decoder initial delay
	fscanf(fd, "%*[^\n]"); 
	fscanf(fd,"%s",inp->LeakyBucketParamFile);    // file where Leaky Bucket params (computed by encoder) are stored
	fscanf(fd,"%*[^\n]");
#endif
		
	fclose (fd);
		
	
}

/*!
************************************************************************
* \brief
*    Reports the gathered information to appropriate outputs
*
* \par Input:
*    struct inp_par *inp,
*    struct img_par *img,
*    struct snr_par *stat
*
* \par Output:
*    None
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params, 
*
*		<saad.shams@inforient.com>
*
************************************************************************
*/
void report(InputParameters *inp, ImageParameters *img, h264_decoder* dec_params )
{
#define OUTSTRING_SIZE 255
	char string[OUTSTRING_SIZE];
	FILE *p_log;
	char yuv_formats[4][4]= { {"400"}, {"420"}, {"422"}, {"444"} };
	
#ifndef WIN32
	time_t  now;
	struct tm *l_time;
#else
	char timebuf[128];
#endif
	
	fprintf(stdout,"--------------------------------------------------------------------------\n");
	fprintf(stdout," Image Resolution    : %d x %d\n",img->width,img->height);
	fprintf(stdout," Total Frames        : %d\n",img->number);
	fprintf(stdout," Total decoding time : %.3f sec \n",dec_params->tot_time*0.001);
	//fprintf(stdout," fps                 : %.3f \n",(snr->frame_ctr/(dec_params->tot_time*0.001)));
	fprintf(stdout," fps                 : %.3f \n",(img->number/(dec_params->tot_time*0.001)));
	fprintf(stdout,"--------------------------------------------------------------------------\n");
	fprintf(stdout," Exit JM %s decoder, ver %s ",JM, VERSION);
	fprintf(stdout,"\n");
	// write to log file
	
	//snprintf(string, OUTSTRING_SIZE, "%s", LOGFILE);

	return; /// to avoid the writing for file
	//printf("%s\n", LOGFILE);
	
	if ((p_log=fopen(string,"r"))==0)                    // check if file exist
	{
		if ((p_log=fopen(string,"a"))==0)
		{
			//snprintf(dec_params->errortext, ET_SIZE, "Error open file %s for appending",string);
			printf("Error open file %s for appending\n",string);
			exit(0);
		}
		else                                              // Create header to new file
		{
			fprintf(p_log," -------------------------------------------------------------------------------------------------------------------\n");
			fprintf(p_log,"|  Decoder statistics. This file is made first time, later runs are appended               |\n");
			fprintf(p_log," ------------------------------------------------------------------------------------------------------------------- \n");
			fprintf(p_log,"|   ver  | Date  | Time  |    Sequence        |#Img| Format  | YUV |Coding|SNRY 1|SNRU 1|SNRV 1|SNRY N|SNRU N|SNRV N|\n");
			fprintf(p_log," -------------------------------------------------------------------------------------------------------------------\n");
		}
	}
	else
	{ 
		fclose(p_log);
		p_log=fopen(string,"a");                    // File exist,just open for appending
	}
	
	fprintf(p_log,"|%s/%-4s", VERSION, EXT_VERSION);
	
#ifdef WIN32
	_strdate( timebuf );
	fprintf(p_log,"| %1.5s |",timebuf );
	
	_strtime( timebuf);
	fprintf(p_log," % 1.5s |",timebuf);
#else
	now = time ((time_t *) NULL); // Get the system time and put it into 'now' as 'calender time'
	time (&now);
	l_time = localtime (&now);
	strftime (string, sizeof string, "%d-%b-%Y", l_time);
	fprintf(p_log,"| %1.5s |",string );
	
	strftime (string, sizeof string, "%H:%M:%S", l_time);
	fprintf(p_log,"| %1.5s |",string );
#endif
	
	fprintf(p_log,"%20.20s|",inp->infile);
	
	fprintf(p_log,"%3d |",img->number);
	fprintf(p_log,"%4dx%-4d|", img->width, img->height);
	fprintf(p_log," %s |", &(yuv_formats[1][0]));
	
	fprintf(p_log," CAVLC|");
	
	//fprintf(p_log,"%6.3f|",snr->snr_y1);
	//fprintf(p_log,"%6.3f|",snr->snr_u1);
	//fprintf(p_log,"%6.3f|",snr->snr_v1);
	//fprintf(p_log,"%6.3f|",snr->snr_ya);
	//fprintf(p_log,"%6.3f|",snr->snr_ua);
	//fprintf(p_log,"%6.3f|\n",snr->snr_va);
	
	fclose(p_log);
	
	//snprintf(string, OUTSTRING_SIZE,"%s", DATADECFILE);
	printf("%s\n", DATADECFILE);
	p_log=fopen(string,"a");
	
	
fprintf(p_log, "%3d %2d %2d %2.2f %2.2f %2.2f %5d "
	"%2.2f %2.2f %2.2f %5d "
	"%2.2f %2.2f %2.2f %5d %.3f\n",
	img->number, 0, img->qp,
	0.0,//snr->snr_y1,
	0.0,//snr->snr_u1,
	0.0,//snr->snr_v1,
	0,
	0.0,
	0.0,
	0.0,
	0,
	0.0,//snr->snr_ya,
	0.0,//snr->snr_ua,
	0.0,//snr->snr_va,
	0,
	(double)0.001*dec_params->tot_time/img->number);

	fclose(p_log);
}

/*!
************************************************************************
* \brief
*    Allocates a stand-alone partition structure.  Structure should
*    be freed by FreePartition();
*    data structures
*
* \par Input:
*    n: number of partitions in the array
* \par return
*    pointer to DataPartition Structure, zero-initialized
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params, 
*
*		<saad.shams@inforient.com>
*
************************************************************************
*/

DataPartition *AllocPartition(int n,  h264_decoder* dec_params,int partitionSize)
{
	DataPartition *partArr, *dataPart;
	int i;
	
	partArr = (DataPartition *) h264_malloc(n * sizeof(DataPartition));

	if (partArr == NULL)
	{
		//snprintf(dec_params->errortext, ET_SIZE, "AllocPartition: Memory allocation for Data Partition failed");
		printf("AllocPartition: Memory allocation for Data Partition failed\n");
		exit(0);
	}
	
	for (i=0; i<n; i++) // loop over all data partitions
	{
		dataPart = &(partArr[i]);
		dataPart->bitstream = (Bitstream *) h264_malloc(1 * sizeof(Bitstream));
		
		if (dataPart->bitstream == NULL)
		{
			//snprintf(dec_params->errortext, ET_SIZE, "AllocPartition: Memory allocation for Bitstream failed");
			printf("AllocPartition: Memory allocation for Bitstream failed\n");
			exit(0);
		}

		dataPart->bitstream->streamBuffer = NULL;
	}
	return partArr;
}




/*!
************************************************************************
* \brief
*    Frees a partition structure (array).  
*
* \par Input:
*    Partition to be freed, size of partition Array (Number of Partitions)
*
* \par return
*    None
*
* \note
*    n must be the same as for the corresponding call of AllocPartition
************************************************************************
*/


void FreePartition (DataPartition *dp, int n)
{
	int i;
	
	assert (dp != NULL);
	assert (dp->bitstream != NULL);
	assert (dp->bitstream->streamBuffer != NULL);
	for (i=0; i<n; i++)
	{
		//h264_free (dp[i].bitstream->streamBuffer);
		h264_free (dp[i].bitstream);
	}
	h264_free (dp);
}


/*!
 ************************************************************************
 * \brief
 *    Allocates the slice structure along with its dependent
 *    data structures
 *
 * \par Input:
 *    Input Parameters struct inp_par *inp,  struct img_par *img
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
*/

void malloc_slice (InputParameters *inp, ImageParameters *img,h264_decoder *dec_params )
{
	Slice *currSlice;
	
	img->currentSlice = (Slice *) h264_malloc(1 * sizeof(Slice));
	
	if ( (currSlice = img->currentSlice) == NULL)
	{
		//snprintf(dec_params->errortext, ET_SIZE, "Memory allocation for Slice datastruct in NAL-mode %d failed", inp->FileFormat);
		printf("Memory allocation for Slice datastruct in NAL-mode %d failed\n", inp->FileFormat);
		exit(0);
	}
	currSlice->max_part_nr = 1;  //! assume data partitioning (worst case) for the following mallocs()
	currSlice->partArr = AllocPartition(currSlice->max_part_nr, dec_params, MAX_CODED_FRAME_SIZE);
}


/*!
************************************************************************
* \brief
*    Memory frees of the Slice structure and of its dependent
*    data structures
*
* \par Input:
*    Input Parameters struct inp_par *inp,  struct img_par *img
************************************************************************
*/
void free_slice(InputParameters *inp,ImageParameters *img)
{
	Slice *currSlice = img->currentSlice;
	
	//FreePartition (currSlice->partArr, 3);
	FreePartition (currSlice->partArr, currSlice->max_part_nr);
	//      if (inp->symbol_mode == CABAC)
	if (1)
	{
		// delete all context models
		//delete_contexts_MotionInfo(currSlice->mot_ctx);
		//delete_contexts_TextureInfo(currSlice->tex_ctx);
	}
	h264_free(img->currentSlice);
	
	currSlice = NULL;
}

/*!
 ************************************************************************
 * \brief
 *    Dynamic memory allocation of frame size related global buffers
 *    buffers are defined in global.h, allocated memory must be freed in
 *    void free_global_buffers()
 *
 *  \par Input:
 *    Input Parameters struct inp_par *inp, Image Parameters struct img_par *img
 *
 *  \par Output:
 *     Number of allocated bytes
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ***********************************************************************
*/
int init_global_buffers_baseline( h264_decoder* dec_params )
{
	ImageParameters* img = dec_params->img;
	int memory_size=0;
	int quad_range, i;
	
	if (dec_params->global_init_done)
	{
		free_global_buffers_baseline( dec_params );
	}
	
	if(((img->slice_nr) = (char *) h264_malloc(img->FrameSizeInMbs * sizeof(char))) == NULL)
	{
		printf("init_global_buffers: img->slice_nr");
		exit(0);
	}
	if(((img->ei_flag) = (char *) h264_malloc(img->FrameSizeInMbs * sizeof(char))) == NULL)
	{
		printf("init_global_buffers: img->ei_flag");
		exit(0);
	}

	///////////////////////////////////////////////////////////////////////////////////
	// Initializations for the new mb_data structure inside the ImageParam structure //
	///////////////////////////////////////////////////////////////////////////////////
	if((img->mb_data1.cbp_blk = (short*) h264_malloc(img->FrameSizeInMbs*sizeof(short))) == NULL)
	{
		printf("init_global_buffers: img->mb_data1->cbp_blk");
		exit(0);
	}

	if((img->mb_data1.LFAlphaC0Offset = (char*) h264_malloc(img->FrameSizeInMbs*sizeof(char))) == NULL)
	{
		printf("init_global_buffers: img->mb_data1.LFAlphaC0Offset");
		exit(0);
	}

	if((img->mb_data1.LFBetaOffset = (char*) h264_malloc(img->FrameSizeInMbs*sizeof(char))) == NULL)
	{
		printf("init_global_buffers: img->mb_data1.LFBetaC0Offset");
		exit(0);
	}

	if((img->mb_data1.LFDisableIdc = (char*) h264_malloc(img->FrameSizeInMbs*sizeof(char))) == NULL)
	{
		printf("init_global_buffers: img->mb_data1.LFDisableIdc");
		exit(0);
	}

	if((img->mb_data1.mb_type = (char*) h264_malloc(img->FrameSizeInMbs*sizeof(char))) == NULL)
	{
		printf("init_global_buffers: img->mb_data1.mb_type");
		exit(0);
	}

	if((img->mb_data1.partition = (char*) h264_malloc(img->FrameSizeInMbs*sizeof(char))) == NULL)
	{
		printf("init_global_buffers: img->mb_data1.partition");
		exit(0);
	}

	if((img->mb_data1.mbAvailA = (char*) h264_malloc(img->FrameSizeInMbs*sizeof(char))) == NULL)
	{
		printf("init_global_buffers: img->mb_data1.mbAvailA");
		exit(0);
	}

	if((img->mb_data1.mbAvailB = (char*) h264_malloc(img->FrameSizeInMbs*sizeof(char))) == NULL)
	{
		printf("init_global_buffers: img->mb_data1.mbAvailB");
		exit(0);
	}

	if((img->mb_data1.NoMbPartLessThan8x8Flag = (char*) h264_malloc(img->FrameSizeInMbs*sizeof(char))) == NULL)
	{
		printf("init_global_buffers: img->mb_data1.NoMbPartLessThan8x8Flag");
		exit(0);
	}

	if((img->mb_data1.qp = (char*) h264_malloc(img->FrameSizeInMbs*sizeof(char))) == NULL)
	{
		printf("init_global_buffers: img->mb_data1.qp");
		exit(0);
	}
	///////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////

	if(((dec_params->img->intra_block) = (int*)h264_malloc(dec_params->img->FrameSizeInMbs * sizeof(int))) == NULL)
	{
		printf("init_global_buffers: img->intra_block");
		exit(0);
	}
	
	memory_size += get_mem2D(&(dec_params->img->ipredmode), dec_params->img->FrameWidthInMbs<<2 , dec_params->img->FrameHeightInMbs<<2);

	memory_size += get_mem3Dint(&(dec_params->img->wp_weight), 2, MAX_REFERENCE_PICTURES, 3);
	memory_size += get_mem3Dint(&(dec_params->img->wp_offset), 6, MAX_REFERENCE_PICTURES, 3);
	memory_size += get_mem4Dint(&(dec_params->img->wbp_weight), 6, MAX_REFERENCE_PICTURES, MAX_REFERENCE_PICTURES, 3);

//	dec_params->img->nz_coeff = h264_malloc ((dec_params->img->FrameSizeInMbs<<5)*sizeof(char));

	dec_params->img->nz_coeff1 = (char *)h264_malloc ((dec_params->img->FrameSizeInMbs*24)*sizeof(char));
//	dec_params->img->nz_coeff1[1] = dec_params->img->nz_coeff1[0] + dec_params->img->FrameSizeInMbs*16;
//	dec_params->img->nz_coeff1[2] = dec_params->img->nz_coeff1[1] + dec_params->img->FrameSizeInMbs*4;

	memory_size += (dec_params->img->FrameSizeInMbs<<5)*sizeof(char);

	
	quad_range = (MAX_IMGPEL_VALUE + 1) <<1;
	
	//if ((dec_params->img->quad = (int*)calloc (quad_range, sizeof(int))) == NULL)
/*	if ((dec_params->img->quad = (int*) h264_malloc (quad_range * sizeof(int))) == NULL)
	{
		printf("init_img: img->quad");
		exit(0);
	}
	
	for (i=0; i < quad_range>>1; ++i)
	{
		dec_params->img->quad[i]=i*i;
	}
*/	
    // create all context models
    dec_params->mot_ctx = (MotionInfoContexts *)create_contexts_MotionInfo();
    dec_params->tex_ctx = (TextureInfoContexts *)create_contexts_TextureInfo();
	dec_params->global_init_done = 1;
	
//	dec_params->img->oldFrameSizeInMbs = dec_params->img->FrameSizeInMbs;
	
	return (memory_size);
}

/*!
 ************************************************************************
 * \brief
 *    Free allocated memory of frame size related global buffers
 *    buffers are defined in global.h, allocated memory is allocated in
 *    int init_global_buffers()
 *
 * \par Input:
 *    Input Parameters struct inp_par *inp, Image Parameters struct img_par *img
 *
 * \par Output:
 *    none
 *
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
  
 ************************************************************************
*/
void free_global_buffers_baseline( h264_decoder* dec_params )
{
	ImageParameters* img = dec_params->img;

//	h264_free (dec_params->img->nz_coeff);

	h264_free (dec_params->img->nz_coeff1);
	
	// free mem, allocated for structure img
//	if (img->mb_data != NULL) 
//		h264_free(img->mb_data);

	// faisal
	h264_free(img->slice_nr);
	h264_free(img->ei_flag);
	
	///////////////////////////////////////////////////////////////////////////////////
	// memory deallocation for the new mb_data structure in the ImageParam structure //
	///////////////////////////////////////////////////////////////////////////////////
	h264_free(img->mb_data1.cbp_blk);
	h264_free(img->mb_data1.LFAlphaC0Offset);
	h264_free(img->mb_data1.LFBetaOffset);
	h264_free(img->mb_data1.LFDisableIdc);
	h264_free(img->mb_data1.mb_type);
	h264_free(img->mb_data1.partition);
	h264_free(img->mb_data1.mbAvailA);
	h264_free(img->mb_data1.mbAvailB);
	h264_free(img->mb_data1.NoMbPartLessThan8x8Flag);
	h264_free(img->mb_data1.qp);
	///////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////

	h264_free (dec_params->img->intra_block);
	free_mem2D ( (byte **) dec_params->img->ipredmode);
	free_mem3Dint(dec_params->img->wp_weight, 2);
	free_mem3Dint(dec_params->img->wp_offset, 6);
	free_mem4Dint(dec_params->img->wbp_weight, 6, MAX_REFERENCE_PICTURES);
	/* End of Change*/
//	h264_free (dec_params->img->quad);
    delete_contexts_MotionInfo(dec_params->mot_ctx);
    delete_contexts_TextureInfo(dec_params->tex_ctx);
	dec_params->global_init_done = 0;
}



