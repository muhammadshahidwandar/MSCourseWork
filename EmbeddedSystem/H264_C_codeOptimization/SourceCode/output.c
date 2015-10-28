   
/*!
 ************************************************************************
 * \file output.c
 *
 * \brief
 *    Output an image and Trance support
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Karsten Suehring               <suehring@hhi.de>
 ************************************************************************
 */

//#include "contributors.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

//#ifdef WIN32
//#include <io.h>
//#else
//#include <unistd.h>
//#endif

#include "global.h"
#include "mbuffer.h"
#include "image.h"
#include "memalloc.h"


/*!
 ************************************************************************
 * \brief
 *      checks if the System is big- or little-endian
 * \return
 *      0, little-endian (e.g. Intel architectures)
 *      1, big-endian (e.g. SPARC, MIPS, PowerPC)
 ************************************************************************
 */
int testEndian()
{
  short s;
  byte *p;

  p=(byte*)&s;

  s=1;

  return (*p==0);
}


/*!
 ************************************************************************
 * \brief
 *   Convert padded image plane to temporary buffer for file writing
 *	 Function modified in a way that it copies only the frame area out of a 
 *	 padded image buffer.
 *
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<tahir.awan@inforient.com>
************************************************************************
 */
void img2buf_padding (imgpel* imgX, int pad_val, unsigned char* buf, int size_x, int size_y, int symbol_size_in_bytes,
                      int crop_left, int crop_right, int crop_top, int crop_bottom,   h264_decoder* dec_params)
{
    int i,k;
    int stride = size_x + (pad_val<<1);
    
    int twidth  = size_x - crop_left - crop_right;
    int theight = size_y - crop_top - crop_bottom;
    
    int size = 0;

    imgpel* imgZ = imgX;
    imgZ += (crop_top+pad_val)*stride +pad_val +crop_left;

    k=0;
	for(i= pad_val ; i < (theight+pad_val) ; i++)
	{
		memcpy(buf+crop_left+(k*twidth),imgZ, twidth);
		imgZ+=stride;
		k++;
	}    
}

void img2buf_padding_Y (imgpel* imgX, int pad_val, unsigned char* buf, int size_x, int size_y, int symbol_size_in_bytes,
                      int crop_left, int crop_right, int crop_top, int crop_bottom,   h264_decoder* dec_params)
{
    int i,j,k;
    int stride = size_x + (pad_val<<1);
    
    int twidth  = size_x - crop_left - crop_right;
    int theight = size_y - crop_top - crop_bottom;
    
    int size = 0;

    unsigned char  ui8;
    unsigned short tmp16, ui16;
    unsigned long  tmp32, ui32;
    imgpel* imgZ = imgX;
    imgZ += (crop_top+pad_val)*stride +pad_val +crop_left;

    if (( sizeof(char) == sizeof (imgpel)) && ( sizeof(char) == symbol_size_in_bytes))
    {
        k=0;
		buf += crop_left;
        for(i= pad_val ; i < (theight+pad_val) ; i++)
        {

            memcpy(buf,imgZ, twidth);
			imgZ+=stride;
            buf += twidth;

        }    
    }
    else
    {
        // sizeof (imgpel) > sizeof(char)
        if (testEndian())
        {
            // big endian
            switch (symbol_size_in_bytes)
            {
            case 1:
                {
                    for(i=crop_top;i<size_y-crop_bottom;i++)
                        for(j=crop_left;j<size_x-crop_right;j++)
                        {
                            ui8 = (unsigned char) (imgX[(i+pad_val*stride) +j +pad_val]);
                            buf[(j-crop_left+((i-crop_top)*(twidth)))] = ui8;
                        }
                        break;
                }
            case 2:
                {
                    for(i=crop_top;i<size_y-crop_bottom;i++)
                        for(j=crop_left;j<size_x-crop_right;j++)
                        {
                            tmp16 = (unsigned short) (imgX[(i+pad_val*stride) +j +pad_val]);
                            ui16  = (tmp16 >> 8) | ((tmp16&0xFF)<<8);

                            memcpy(buf+((j-crop_left+((i-crop_top)*(twidth)))*2),&(ui16), 2);

                        }
                        break;
                }
            case 4:
                {
                    for(i=crop_top;i<size_y-crop_bottom;i++)
                        for(j=crop_left;j<size_x-crop_right;j++)
                        {
                            tmp32 = (unsigned long) (imgX[(i+pad_val*stride) +j +pad_val]);
                            ui32  = ((tmp32&0xFF00)<<8) | ((tmp32&0xFF)<<24) | ((tmp32&0xFF0000)>>8) | ((tmp32&0xFF000000)>>24);

                            memcpy(buf+((j-crop_left+((i-crop_top)*(twidth)))*4),&(ui32), 4);

                        }
                        break;
                }
            default:
                {
                    printf("writing only to formats of 8, 16 or 32 bit allowed on big endian architecture");
					exit(0);
                    break;
                }
            }
        }
        else
        {
            // little endian
            if (sizeof (imgpel) < symbol_size_in_bytes)
            {
                // this should not happen. we should not have smaller imgpel than our source material.
                size = sizeof (imgpel);
                // clear buffer
                memset (buf, 0, (twidth*theight*symbol_size_in_bytes));
            }
            else
            {
                size = symbol_size_in_bytes;
            }
            
            for(i=crop_top;i<size_y-crop_bottom;i++)
                for(j=crop_left;j<size_x-crop_right;j++)
                {
                    //mymemcpy(buf+((j-crop_left+((i-crop_top)*(twidth)))*symbol_size_in_bytes),&(imgX[(i+pad_val*stride) +j +pad_val]), size);
                    *(buf+((j-crop_left+((i-crop_top)*(twidth)))*symbol_size_in_bytes)) = imgX[(i+pad_val*stride) +j +pad_val];
                }
        }
    }
}

//void write_picture(StorablePicture *p, int p_out, int real_structure,  h264_decoder* dec_params)
void write_picture(StorablePicture *p, FILE *f_out, int real_structure,  h264_decoder* dec_params)
{
    int i;
    int crop_left, crop_right, crop_top, crop_bottom;
    Boolean rgb_output = (dec_params->active_sps->vui_seq_parameters.matrix_coefficients==0);
    unsigned char *buf;
    
    if (p->non_existing)
        return;
    
    if (p->frame_cropping_flag)
    {
        crop_left   =  p->frame_cropping_rect_left_offset;
        crop_right  =  p->frame_cropping_rect_right_offset;
        crop_top    =  p->frame_cropping_rect_top_offset;
        crop_bottom =  p->frame_cropping_rect_bottom_offset;
    }
    else
    {
        crop_left = crop_right = crop_top = crop_bottom = 0;
    }
    
    buf = (unsigned char *)h264_malloc ((dec_params->img->width)*(dec_params->img->height));
    if (buf==NULL)
    {
        printf("write_out_picture: buf");
		exit(0);
    }
    
    if(rgb_output)
    {
        crop_left   = p->frame_cropping_rect_left_offset;
        crop_right  = p->frame_cropping_rect_right_offset;
        crop_top    = p->frame_cropping_rect_top_offset;
        crop_bottom = p->frame_cropping_rect_bottom_offset;
        
        img2buf_padding(p->imgV, 16, buf, dec_params->img->width_cr, dec_params->img->height_cr, 1, crop_left, crop_right, crop_top, crop_bottom,dec_params);
        //write(p_out, buf, (dec_params->img->height_cr-crop_bottom-crop_top)*(dec_params->img->width_cr-crop_right-crop_left));
        
        if (p->frame_cropping_flag)
        {
            crop_left   = p->frame_cropping_rect_left_offset;
            crop_right  = p->frame_cropping_rect_right_offset;
            crop_top    = p->frame_cropping_rect_top_offset;
            crop_bottom = p->frame_cropping_rect_bottom_offset;
        }
        else
        {
            crop_left = crop_right = crop_top = crop_bottom = 0;
        }
    }
    
    // Calling the function img2buf_padding() that copyes the decoded data from padded frame to buffer.

	img2buf_padding_Y(p->imgY, 32, buf, dec_params->img->width, dec_params->img->height, 1, crop_left, crop_right, crop_top, crop_bottom,dec_params);
	
#ifdef WRITE_PICTURE
    //write(p_out, buf, (dec_params->img->height-crop_bottom-crop_top)*(dec_params->img->width-crop_right-crop_left));
	fwrite(buf , (dec_params->img->height-crop_bottom-crop_top)*(dec_params->img->width-crop_right-crop_left) , sizeof(unsigned char) , f_out);
#endif
    
    crop_left   = p->frame_cropping_rect_left_offset;
    crop_right  = p->frame_cropping_rect_right_offset;
    crop_top    = p->frame_cropping_rect_top_offset;
    crop_bottom = p->frame_cropping_rect_bottom_offset;
    
    // Calling the function img2buf_padding() that copyes the decoded data from padded frame to buffer.
    img2buf_padding(p->imgU, 16, buf, dec_params->img->width_cr, dec_params->img->height_cr, 1, crop_left, crop_right, crop_top, crop_bottom,dec_params);

#ifdef WRITE_PICTURE
    //write(p_out, buf, (dec_params->img->height_cr-crop_bottom-crop_top)*(dec_params->img->width_cr-crop_right-crop_left));
	fwrite(buf , (dec_params->img->height_cr-crop_bottom-crop_top)*(dec_params->img->width_cr-crop_right-crop_left) , sizeof(unsigned char) , f_out);
#endif
    
    if (!rgb_output)
    {
        // Calling the function img2buf_padding() that copyes the decoded data from padded frame to buffer.
        img2buf_padding(p->imgV, 16, buf, dec_params->img->width_cr, dec_params->img->height_cr, 1, crop_left, crop_right, crop_top, crop_bottom,dec_params);

#ifdef WRITE_PICTURE
        //write(p_out, buf, (dec_params->img->height_cr-crop_bottom-crop_top)*(dec_params->img->width_cr-crop_right-crop_left));
		fwrite(buf , (dec_params->img->height_cr-crop_bottom-crop_top)*(dec_params->img->width_cr-crop_right-crop_left) , sizeof(unsigned char) , f_out);
#endif
    }
    if(buf != NULL)
    {
        h264_free(buf);
        buf = NULL;
    }

}



//#endif

/*!
 ************************************************************************
 * \brief
 *    Initialize output buffer for direct output
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
 /*
void init_out_buffer(  h264_decoder* dec_params)
{
  dec_params->out_buffer = alloc_frame_store(dec_params);

}
*/
/*!
 ************************************************************************
 * \brief
 *    Uninitialize output buffer for direct output
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 *//*
void uninit_out_buffer(   h264_decoder* dec_params)
{
  free_frame_store(dec_outputs->out_buffer,dec_params);
  dec_params->out_buffer=NULL;
}
*/
/*!
 ************************************************************************
 * \brief
 *    Write out not paired direct output fields. A second empty field is generated
 *    and combined into the frame buffer.
 * \param fs
 *    FrameStore that contains a single field
 * \param p_out
 *    Output file
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
///remove
void write_unpaired_field(FrameStore* fs, int p_out,  h264_decoder* dec_params)
{
  StorablePicture *p;
  assert (fs->is_used<3);

  if(fs->is_used &2)
  {
    // we have a bottom field
    // construct an empty top field
    dpb_combine_field_yuv(fs,dec_params);
  }
  fs->is_used=3;
}

/*!
 ************************************************************************
 * \brief
 *    Write out unpaired fields from output buffer.
 * \param p_out
 *    Output file
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
void flush_direct_output(int p_out,  h264_decoder* dec_params)
{
	/*
  write_unpaired_field(dec_outputs->out_buffer, p_out,dec_params);

  free_storable_picture(dec_outputs->out_buffer->frame,dec_params);
  dec_outputs->out_buffer->frame = NULL;
  dec_outputs->out_buffer->is_used = 0;
  */
}


/*!
 ************************************************************************
 * \brief
 *    Write a frame (from FrameStore)
 * \param fs
 *    FrameStore containing the frame
 * \param p_out
 *    Output file
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
//void write_stored_frame( FrameStore *fs,int p_out,  h264_decoder* dec_params)
void write_stored_frame( FrameStore *fs,FILE *f_out,  h264_decoder* dec_params)
{
  // make sure no direct output field is pending
  //flush_direct_output(p_out,dec_params);

  /*if (fs->is_used<3)
  {
    write_unpaired_field(fs, p_out,dec_params);
  }
  else
  {
 */   //write_picture(fs->frame, p_out, FRAME,dec_params);
	write_picture(fs->frame, f_out, FRAME,dec_params);	
  //}

  fs->is_output = 1;
}

/*!
 ************************************************************************
 * \brief
 *    Directly output a picture without storing it in the DPB. Fields 
 *    are buffered before they are written to the file.
 * \param p
 *    Picture for output
 * \param p_out
 *    Output file
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */

void direct_output(h264_decoder* dec_params)
{
  StorablePicture *p=dec_params->dec_picture;
  int p_out=dec_params->p_out;

  //if (p->structure==FRAME)
  {
    // we have a frame (or complementary field pair)
    // so output it directly
    flush_direct_output(p_out,dec_params);
    //write_picture (p, p_out, FRAME,dec_params);
	write_picture (p, dec_params->f_out, FRAME,dec_params);

	// REQUIREF FOR DEBUGGING
    //if (-1!=dec_outputs->p_ref)
    //  find_snr(p,dec_params,dec_outputs);

    //free_storable_picture(p,dec_params,dec_outputs);
	p->is_empty = 1;			// buffer is empty
	
    return;
  }
/*
  if (dec_outputs->out_buffer->is_used == 3)
  {
    // we have both fields, so output them
    dpb_combine_field_yuv(dec_outputs->out_buffer,dec_params,dec_outputs);
    write_picture (dec_outputs->out_buffer->frame, p_out, FRAME,dec_params,dec_outputs);

	// REQUIRED FOR DEBUGGING 
    //if (-1!=dec_outputs->p_ref)
    //  find_snr(dec_outputs->out_buffer->frame,dec_params,dec_outputs);

    free_storable_picture(dec_outputs->out_buffer->frame,dec_params,dec_outputs);
    dec_outputs->out_buffer->frame = NULL;
    dec_outputs->out_buffer->is_used = 0;

  }
  */
}

