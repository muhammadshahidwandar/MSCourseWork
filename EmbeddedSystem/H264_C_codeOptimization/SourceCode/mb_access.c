 /*!
 *************************************************************************************
 * \file mb_access.c
 *
 * \brief
 *    Functions for macroblock neighborhoods
 *
 *  \author
 *      Main contributors (see contributors.h for copyright, address and affiliation details)
 *      - Karsten Sühring          <suehring@hhi.de>
 *************************************************************************************
 */
#include <assert.h>

#include "global.h"
#include "mbuffer.h"

//extern StorablePicture *dec_outputs->dec_picture;/*Changed by Saad Bin Shams [Removing Global Variables]*/



/*!
 ************************************************************************
 * \brief
 *    Checks the availability of neighboring macroblocks of
 *    the current macroblock for prediction and context determination;
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
/***********************************************************************
*	- Code not related to baseline removed.
*
*	 "Muhammad Tahir Awan" <tahir.awan@inforient.com>,
*	 "Umair Razzak" <umair.razzak@inforient.com>
****************************
*	 Changes till 21-11-2005
***********************************************************************
*//*
void CheckAvailabilityOfNeighbors_baseline( ImageParameters* img)
{
	const int mb_nr = img->current_mb_nr;

    img->currMB.mbAddrA = mb_nr - 1;
    img->currMB.mbAddrB = mb_nr - img->FrameWidthInMbs;
    img->currMB.mbAddrC = mb_nr - img->FrameWidthInMbs + 1;
    img->currMB.mbAddrD = mb_nr - img->FrameWidthInMbs - 1;

	img->currMB.mbAvailD = mb_is_available(img->currMB.mbAddrD, mb_nr,img) && ((mb_nr % img->FrameWidthInMbs)!=0);
	//img->currMB.mbAvailD = img->mb_x != 0;
    img->currMB.mbAvailB = mb_is_available(img->currMB.mbAddrB, mb_nr,img);
	//img->currMB.mbAvailB = img->mb_y != 0;
    img->currMB.mbAvailC = mb_is_available(img->currMB.mbAddrC, mb_nr,img) && (((mb_nr+1) % img->FrameWidthInMbs)!=0);
	//img->currMB.mbAvailC = ((mb_nr+1) % img->FrameWidthInMbs)!=0;
    img->currMB.mbAvailA = mb_is_available(img->currMB.mbAddrA, mb_nr,img) && ((mb_nr % img->FrameWidthInMbs)!=0);
	//img->currMB.mbAvailA = img->mb_x != 0;
}
*/
/*!
 ************************************************************************
 * \brief
 *    returns the x and y sample coordinates for a given MbAddress
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */

void get_mb_pos_baseline (int mb_addr, int *x, int*y, h264_decoder* dec_params)
{
  //get_mb_block_pos(mb_addr, x, y,dec_outputs);

  *x = (mb_addr % dec_params->img->FrameWidthInMbs);
  *y = (mb_addr / dec_params->img->FrameWidthInMbs);
  
  (*x) *= MB_BLOCK_SIZE;
  (*y) *= MB_BLOCK_SIZE;
}

/*!
 ************************************************************************
 * \brief
 *    get neighbouring positions. MB AFF is automatically used from img structure
 * \param curr_mb_nr
 *   current macroblock number (decoding order)
 * \param xN
 *    input x position
 * \param yN
 *    input y position
 * \param luma
 *    1 if luma coding, 0 for chroma
 * \param pix
 *    returns position informations
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
void getNeighbour(int curr_mb_nr, int xN, int yN, int luma, PixelPos *pix,  h264_decoder* dec_params)
{

  Macroblock *currMb = &dec_params->img->mb_data[curr_mb_nr];
  int maxW, maxH;

  if (curr_mb_nr<0)
  {  
	  printf("getNeighbour: invalid macroblock number");
	  exit(0);
  }

  //if (dec_outputs->dec_picture->MbaffFrameFlag)
  //  getAffNeighbour(curr_mb_nr, xN, yN, luma, pix,dec_outputs,dec_params);
  //else
  //  getNonAffNeighbour(curr_mb_nr, xN, yN, luma, pix,dec_outputs,dec_params);  
  
  if (luma)
  {
    maxW = 16;
    maxH = 16;
  }
  else
  {
    maxW = MB_CR_SIZE_X;
    maxH = MB_CR_SIZE_Y;
  }

  if ((xN<0)&&(yN<0))
  {
    pix->mb_addr   = currMb->mbAddrD;
    pix->available = currMb->mbAvailD;
  }
  else if ((xN<0)&&((yN>=0)&&(yN<maxH)))
  {
    pix->mb_addr  = currMb->mbAddrA;
    pix->available = currMb->mbAvailA;
  }
  else if (((xN>=0)&&(xN<maxW))&&(yN<0))
  {
    pix->mb_addr  = currMb->mbAddrB;
    pix->available = currMb->mbAvailB;
  }
  else if (((xN>=0)&&(xN<maxW))&&((yN>=0)&&(yN<maxH)))
  {
    pix->mb_addr  = curr_mb_nr;
    pix->available = 1;
  }
  else if ((xN>=maxW)&&(yN<0))
  {
    pix->mb_addr  = currMb->mbAddrC;
    pix->available = currMb->mbAvailC;
  }
  else 
  {
    pix->available = 0;
  }

  if (pix->available || dec_params->img->DeblockCall)
  {
    pix->x = (xN + maxW) % maxW;
    pix->y = (yN + maxH) % maxH;
    get_mb_pos_baseline(pix->mb_addr, &(pix->pos_x), &(pix->pos_y),dec_params);
    if (luma)
    {
      pix->pos_x += pix->x;
      pix->pos_y += pix->y;
    }
    else
    {      pix->pos_x = ((pix->pos_x*MB_CR_SIZE_X)>>4) + pix->x;
			// pix->pos_x = pix->pos_x/(16/dec_params->img->mb_cr_size_x) + pix->x;
           pix->pos_y = ((pix->pos_y*MB_CR_SIZE_Y)>>4) + pix->y;// pix->pos_y = pix->pos_y/(16/dec_params->img->mb_cr_size_y) + pix->y;
    }
  }

}

/*!
 ************************************************************************
 * \brief
 *    get neighbouring  get neighbouring 4x4 luma block
 * \param curr_mb_nr
 *   current macroblock number (decoding order)
 * \param block_x
 *    input x block position
 * \param block_y
 *    input y block position
 * \param rel_x
 *    relative x position of neighbor
 * \param rel_y
 *    relative y position of neighbor
 * \param pix
 *    returns position informations
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
void getLuma4x4Neighbour (int block_x, int block_y, int rel_x, int rel_y, PixelPos *pix,  h264_decoder* dec_params)
{
  int x = (block_x<<2) + rel_x;
  int y = (block_y<<2) + rel_y;

  getNeighbour(dec_params->img->current_mb_nr, x, y, 1, pix,dec_params);
  if (pix->available)
  {
    pix->x = pix->x>>2;
    pix->y = pix->y>>2;
    pix->pos_x = pix->pos_x>>2 ;
    pix->pos_y = pix->pos_y>>2;
  }
}


/*!
 ************************************************************************
 * \brief
 *    get neighbouring 4x4 chroma block
 * \param curr_mb_nr
 *   current macroblock number (decoding order)
 * \param block_x
 *    input x block position
 * \param block_y
 *    input y block position
 * \param rel_x
 *    relative x position of neighbor
 * \param rel_y
 *    relative y position of neighbor
 * \param pix
 *    returns position informations
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
void getChroma4x4Neighbour (int curr_mb_nr, int block_x, int block_y, int rel_x, int rel_y, PixelPos *pix,   h264_decoder* dec_params)
{
  int x = (block_x<<2) + rel_x;
  int y = (block_y<<2) + rel_y;

  getNeighbour(curr_mb_nr, x, y, 0, pix,dec_params);

  if (pix->available)
  {
    pix->x = pix->x>>2;
    pix->y = pix->y>>2;
    pix->pos_x = pix->pos_x>>2 ;
    pix->pos_y = pix->pos_y>>2;
	/*
    pix->x /= 4;
    pix->y /= 4;
    pix->pos_x /= 4;
    pix->pos_y /= 4;
	*/
  }
}
