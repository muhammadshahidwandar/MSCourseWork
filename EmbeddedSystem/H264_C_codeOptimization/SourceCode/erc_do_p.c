/*!
*************************************************************************************
* \file 
*      erc_do_p.c
*
* \brief
*      Inter (P) frame error concealment algorithms for decoder
*
*************************************************************************************
*/

#include <stdlib.h>
#include <assert.h>
#include "mbuffer.h"
#include "global.h"
#include "memalloc.h"
#include "erc_do.h"
#include "image.h"

//extern int erc_mvperMB;	/*Changed by Saad Bin Shams [Removing Global Variables]*/
//struct img_par *erc_img;	/*Changed by Saad Bin Shams [Removing Global Variables]*/

// static function declarations

/*Function Argument List Changed [Removing Global Variables] <saad.shams@inforient.com>*/
static int concealByCopy(frame *recfr, int currMBNum,
  objectBuffer_t *object_list, int32 picSizeX, h264_decoder* dec_params);

/*Function Argument List Changed [Removing Global Variables] <saad.shams@inforient.com>*/
static int concealByTrial(frame *recfr, imgpel *predMB, 
                          int currMBNum, objectBuffer_t *object_list, int predBlocks[], 
                          int32 picSizeX, int32 picSizeY, int *yCondition, h264_decoder*dec_params);
static int edgeDistortion (int predBlocks[], int currYBlockNum, imgpel *predMB, 
                           imgpel *recY, int32 picSizeX, int32 regionSize);

/*Function Argument List Changed [Removing Global Variables] <saad.shams@inforient.com>*/
static void copyBetweenFrames (frame *recfr, 
/*Function Argument List Changed [Removing Global Variables] <saad.shams@inforient.com>*/
int currYBlockNum, int32 picSizeX, int32 regionSize, h264_decoder* dec_params);

/*Function Argument List Changed [Removing Global Variables] <saad.shams@inforient.com>*/
static void buildPredRegionYUV(ImageParameters *img, int32 *mv, int x, int y, imgpel *predMB, h264_decoder* dec_params);

/*Function Argument List Changed [Removing Global Variables] <saad.shams@inforient.com>*/
static void copyPredMB (int currYBlockNum, imgpel *predMB, frame *recfr, 
                        int32 picSizeX, int32 regionSize, h264_decoder*dec_params);

//extern const unsigned char subblk_offset_y[3][8][4];
//extern const unsigned char subblk_offset_x[3][8][4];
static int uv_div[2][4] = {{0, 1, 1, 0}, {0, 1, 0, 0}}; //[x/y][yuv_format]

/*!
 ************************************************************************
 * \brief
 *      The main function for Inter (P) frame concealment.
 * \return
 *      0, if the concealment was not successful and simple concealment should be used
 *      1, otherwise (even if none of the blocks were concealed)
 * \param recfr
 *      Reconstructed frame buffer
 * \param object_list
 *      Motion info for all MBs in the frame
 * \param picSizeX
 *      Width of the frame in pixels
 * \param picSizeY
 *      Height of the frame in pixels
 * \param errorVar   
 *      Variables for error concealment
 * \param chroma_format_idc   
 *      Chroma format IDC
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */

int ercConcealInterFrame( frame *recfr, objectBuffer_t *object_list, 
                          int32 picSizeX, int32 picSizeY, ercVariables_t *errorVar,
						  h264_decoder* dec_params ) 
{
  int lastColumn = 0, lastRow = 0, predBlocks[8];
  int lastCorruptedRow = -1, firstCorruptedRow = -1, currRow = 0, 
    row, column, columnInd, areaHeight = 0, i = 0;
  imgpel *predMB;
  
  /* if concealment is on */
  if ( errorVar && errorVar->concealment ) 
  {
    /* if there are segments to be concealed */
    if ( errorVar->nOfCorruptedSegments ) 
    {
        predMB = (imgpel *) h264_malloc ( (256 + (MB_CR_SIZE_X*MB_CR_SIZE_Y)*2) * sizeof (imgpel));
      
      if ( predMB == NULL ) 
	  {
		  printf("ercConcealInterFrame: predMB");
		  exit(0);
	  }
      
      lastRow = (int) (picSizeY>>4);
      lastColumn = (int) (picSizeX>>4);
      
      for( columnInd = 0; columnInd < lastColumn; columnInd ++) 
      {
        
        column = ((columnInd%2) ? (lastColumn - columnInd/2 -1) : (columnInd/2));
        
        for( row = 0; row < lastRow; row++) 
        {
          
          if ( errorVar->yCondition[MBxy2YBlock(column, row, 0, picSizeX)] <= ERC_BLOCK_CORRUPTED ) 
          {                           // ERC_BLOCK_CORRUPTED (1) or ERC_BLOCK_EMPTY (0)
            firstCorruptedRow = row;
            /* find the last row which has corrupted blocks (in same continuous area) */
            for( lastCorruptedRow = row+1; lastCorruptedRow < lastRow; lastCorruptedRow++) 
            {
              /* check blocks in the current column */
              if (errorVar->yCondition[MBxy2YBlock(column, lastCorruptedRow, 0, picSizeX)] > ERC_BLOCK_CORRUPTED) 
              {
                /* current one is already OK, so the last was the previous one */
                lastCorruptedRow --;
                break;
              }
            }
            if ( lastCorruptedRow >= lastRow ) 
            {
              /* correct only from above */
              lastCorruptedRow = lastRow-1;
              for( currRow = firstCorruptedRow; currRow < lastRow; currRow++ ) 
              {
                
                ercCollect8PredBlocks (predBlocks, (currRow<<1), (column<<1), 
                  errorVar->yCondition, (lastRow<<1), (lastColumn<<1), 2, 0);      
                
                if(dec_params->erc_mvperMB >= MVPERMB_THR)
                  concealByTrial(recfr, predMB, 
                    currRow*lastColumn+column, object_list, predBlocks, 
                    picSizeX, picSizeY,
                    errorVar->yCondition,dec_params);
                else 
                  concealByCopy(recfr, currRow*lastColumn+column, 
                    object_list, picSizeX,dec_params);
                
                ercMarkCurrMBConcealed (currRow*lastColumn+column, -1, picSizeX, errorVar);
              }
              row = lastRow;
            } 
            else if ( firstCorruptedRow == 0 ) 
            {
              /* correct only from below */
              for( currRow = lastCorruptedRow; currRow >= 0; currRow-- ) 
              {
                ercCollect8PredBlocks (predBlocks, (currRow<<1), (column<<1), 
                  errorVar->yCondition, (lastRow<<1), (lastColumn<<1), 2, 0);      
                
                if(dec_params->erc_mvperMB >= MVPERMB_THR)
				{
                  concealByTrial(recfr, predMB, 
                    currRow*lastColumn+column, object_list, predBlocks, 
                    picSizeX, picSizeY,
                    errorVar->yCondition,dec_params);
				}
                else 
				{
                  concealByCopy(recfr, currRow*lastColumn+column, 
                    object_list, picSizeX,dec_params);
				}
                
                ercMarkCurrMBConcealed (currRow*lastColumn+column, -1, picSizeX, errorVar);
              }
              
              row = lastCorruptedRow+1;
            }
            else 
            {
              /* correct bi-directionally */
              
              row = lastCorruptedRow+1;
              
              areaHeight = lastCorruptedRow-firstCorruptedRow+1;
              
              /* 
              *  Conceal the corrupted area switching between the up and the bottom rows 
              */
              for( i = 0; i < areaHeight; i++) 
              {
                if ( i % 2 ) 
                {
                  currRow = lastCorruptedRow;
                  lastCorruptedRow --;
                }
                else 
                {
                  currRow = firstCorruptedRow;
                  firstCorruptedRow ++; 
                }
                
                ercCollect8PredBlocks (predBlocks, (currRow<<1), (column<<1), 
                  errorVar->yCondition, (lastRow<<1), (lastColumn<<1), 2, 0);      
                
                if(dec_params->erc_mvperMB >= MVPERMB_THR)
                  concealByTrial(recfr, predMB, 
                    currRow*lastColumn+column, object_list, predBlocks, 
                    picSizeX, picSizeY,
                    errorVar->yCondition,dec_params);
                else
                  concealByCopy(recfr, currRow*lastColumn+column, 
                    object_list, picSizeX,dec_params);
                
                ercMarkCurrMBConcealed (currRow*lastColumn+column, -1, picSizeX, errorVar);
                
              }
            }
            lastCorruptedRow = -1;
            firstCorruptedRow = -1;
          }
        }
      }
    
      h264_free(predMB);
    }
    return 1;
  }
  else
    return 0;
}

/*!
 ************************************************************************
 * \brief
 *      It conceals a given MB by simply copying the pixel area from the reference image 
 *      that is at the same location as the macroblock in the current image. This correcponds 
 *      to COPY MBs. 
 * \return
 *      Always zero (0).
 * \param recfr
 *      Reconstructed frame buffer
 * \param currMBNum
 *      current MB index
 * \param object_list
 *      Motion info for all MBs in the frame
 * \param picSizeX
 *      Width of the frame in pixels
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */

static int concealByCopy( frame *recfr, int currMBNum, objectBuffer_t *object_list, 
						  int32 picSizeX, h264_decoder* dec_params )
{
  objectBuffer_t *currRegion;
   
  currRegion = object_list+(currMBNum<<2);
  currRegion->regionMode = REGMODE_INTER_COPY;
   
  currRegion->xMin = (xPosMB(currMBNum,picSizeX)<<4);
  currRegion->yMin = (yPosMB(currMBNum,picSizeX)<<4);
   
  copyBetweenFrames (recfr, MBNum2YBlock(currMBNum,0,picSizeX), picSizeX, 16, dec_params);
   
  return 0;
}

/*!
 ************************************************************************
 * \brief
 *      Copies the co-located pixel values from the reference to the current frame. 
 *      Used by concealByCopy
 * \param recfr
 *      Reconstructed frame buffer
 * \param currYBlockNum
 *      index of the block (8x8) in the Y plane
 * \param picSizeX
 *      Width of the frame in pixels
 * \param regionSize      
 *      can be 16 or 8 to tell the dimension of the region to copy
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */

static void copyBetweenFrames (frame *recfr,int currYBlockNum, int32 picSizeX, int32 regionSize,
                               h264_decoder* dec_params)
{
    int j, k, location, xmin, ymin;
    //StorablePicture* refPic = dec_params->listX[0][0];
    StorablePicture* refPic = dec_params->listX[0][0];
    imgpel* imgY = refPic->imgY;
    unsigned int stride = dec_params->img->width+64;
    unsigned int stride_cr = dec_params->img->width_cr+32;
    // set the position of the region to be copied 
    xmin = (xPosYBlock(currYBlockNum,picSizeX)<<3);
    ymin = (yPosYBlock(currYBlockNum,picSizeX)<<3);
    imgY += (ymin+32)*stride +xmin +32;
    for (j = ymin; j < ymin + regionSize; j++)
    {
        for (k = xmin; k < xmin + regionSize; k++)
        {
            location = j * picSizeX + k; 
            recfr->yptr[location] = *(imgY);
            imgY++;
        }
        imgY+=stride-regionSize;
    }    
    for (j = ymin >> uv_div[1][1]; j < (ymin + regionSize) >> uv_div[1][1]; j++)
    {
        for (k = xmin >> uv_div[0][1]; k < (xmin + regionSize) >> uv_div[0][1]; k++)
        {
            //        location = j * picSizeX / 2 + k;
            location = ((j * picSizeX) >> uv_div[0][1]) + k;
            recfr->uptr[location] = refPic->imgU[(j+16)*stride_cr+k+16];
            recfr->vptr[location] = refPic->imgV[(j+16)*stride_cr+k+16];
        }
    }
}

/*!
 ************************************************************************
 * \brief
 *      It conceals a given MB by using the motion vectors of one reliable neighbor. That MV of a 
 *      neighbor is selected wich gives the lowest pixel difference at the edges of the MB 
 *      (see function edgeDistortion). This corresponds to a spatial smoothness criteria.
 * \return
 *      Always zero (0).
 * \param recfr
 *      Reconstructed frame buffer
 * \param predMB
 *      memory area for storing temporary pixel values for a macroblock
 *      the Y,U,V planes are concatenated y = predMB, u = predMB+256, v = predMB+320
 * \param currMBNum
 *      current MB index
 * \param object_list
 *      array of region structures storing region mode and mv for each region
 * \param predBlocks
 *      status array of the neighboring blocks (if they are OK, concealed or lost)
 * \param picSizeX
 *      Width of the frame in pixels
 * \param picSizeY
 *      Height of the frame in pixels
 * \param yCondition
 *      array for conditions of Y blocks from ercVariables_t
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */

static int concealByTrial(frame *recfr, imgpel *predMB, 
                          int currMBNum, objectBuffer_t *object_list, int predBlocks[], 
                          int32 picSizeX, int32 picSizeY, int *yCondition, h264_decoder*dec_params)
{
  int predMBNum = 0, numMBPerLine,
      compSplit1 = 0, compSplit2 = 0, compLeft = 1, comp = 0, compPred, order = 1,
      fInterNeighborExists, numIntraNeighbours,
      fZeroMotionChecked, predSplitted = 0,
      threshold = ERC_BLOCK_OK,
      minDist, currDist, i, k, bestDir;
  int32 regionSize;
  objectBuffer_t *currRegion;
  int32 mvBest[3] , mvPred[3], *mvptr;
  
  numMBPerLine = (int) (picSizeX>>4);
  
  comp = 0;
  regionSize = 16;
  
  do 
  { /* 4 blocks loop */
    
    currRegion = object_list+(currMBNum<<2)+comp;
    
    /* set the position of the region to be concealed */
    
    currRegion->xMin = (xPosYBlock(MBNum2YBlock(currMBNum,comp,picSizeX),picSizeX)<<3);
    currRegion->yMin = (yPosYBlock(MBNum2YBlock(currMBNum,comp,picSizeX),picSizeX)<<3);
    
    do 
    { /* reliability loop */
      
      minDist = 0; 
      fInterNeighborExists = 0; 
      numIntraNeighbours = 0; 
      fZeroMotionChecked = 0;
      
      /* loop the 4 neighbours */
      for (i = 4; i < 8; i++) 
      {
        
        /* if reliable, try it */
        if (predBlocks[i] >= threshold) 
        {
          switch (i) 
          {
          case 4:
            predMBNum = currMBNum-numMBPerLine;
            compSplit1 = 2;
            compSplit2 = 3;
            break;
              
          case 5:
            predMBNum = currMBNum-1;
            compSplit1 = 1;
            compSplit2 = 3;
            break;
              
          case 6:
            predMBNum = currMBNum+numMBPerLine;
            compSplit1 = 0;
            compSplit2 = 1;
            break;
              
          case 7:
            predMBNum = currMBNum+1;
            compSplit1 = 0;
            compSplit2 = 2;
            break;
          }
          
          /* try the concealment with the Motion Info of the current neighbour
          only try if the neighbour is not Intra */
          if (isBlock(object_list,predMBNum,compSplit1,INTRA) || 
            isBlock(object_list,predMBNum,compSplit2,INTRA))
          {            
            numIntraNeighbours++;
          } 
          else 
          {
            /* if neighbour MB is splitted, try both neighbour blocks */
            for (predSplitted = isSplitted(object_list, predMBNum), 
              compPred = compSplit1;
              predSplitted >= 0;
              compPred = compSplit2,
              predSplitted -= ((compSplit1 == compSplit2) ? 2 : 1)) 
            {
              
              /* if Zero Motion Block, do the copying. This option is tried only once */
              if (isBlock(object_list, predMBNum, compPred, INTER_COPY)) 
              {
                
                if (fZeroMotionChecked) 
                {
                  continue;
                }
                else 
                {
                  fZeroMotionChecked = 1;

                  mvPred[0] = mvPred[1] = 0;
                  mvPred[2] = 0;
                  
                  buildPredRegionYUV(dec_params->erc_img, mvPred, currRegion->xMin, currRegion->yMin, predMB,dec_params);
                }
              }
              /* build motion using the neighbour's Motion Parameters */
              else if (isBlock(object_list,predMBNum,compPred,INTRA)) 
              {
                continue;
              }
              else 
              {
                mvptr = getParam(object_list, predMBNum, compPred, mv);
                
                mvPred[0] = mvptr[0];
                mvPred[1] = mvptr[1];
                mvPred[2] = mvptr[2];

                buildPredRegionYUV(dec_params->erc_img, mvPred, currRegion->xMin, currRegion->yMin, predMB,dec_params);
              }
              
              /* measure absolute boundary pixel difference */
              currDist = edgeDistortion(predBlocks, 
                MBNum2YBlock(currMBNum,comp,picSizeX),
                predMB, recfr->yptr, picSizeX, regionSize);
              
              /* if so far best -> store the pixels as the best concealment */
              if (currDist < minDist || !fInterNeighborExists) 
              {
                
                minDist = currDist;
                bestDir = i;
                
                for (k=0;k<3;k++) 
                  mvBest[k] = mvPred[k];
                
                currRegion->regionMode = 
                  (isBlock(object_list, predMBNum, compPred, INTER_COPY)) ? 
                  ((regionSize == 16) ? REGMODE_INTER_COPY : REGMODE_INTER_COPY_8x8) : 
                  ((regionSize == 16) ? REGMODE_INTER_PRED : REGMODE_INTER_PRED_8x8);
                
                copyPredMB(MBNum2YBlock(currMBNum,comp,picSizeX), predMB, recfr, 
                  picSizeX, regionSize,dec_params);
              }
              
              fInterNeighborExists = 1;
            }
          }
        }
    }
    
    threshold--;
    
    } while ((threshold >= ERC_BLOCK_CONCEALED) && (fInterNeighborExists == 0));
    
    /* always try zero motion */
    if (!fZeroMotionChecked) 
    {
      mvPred[0] = mvPred[1] = 0;
      mvPred[2] = 0;

      buildPredRegionYUV(dec_params->erc_img, mvPred, currRegion->xMin, currRegion->yMin, predMB,dec_params);
      
      currDist = edgeDistortion(predBlocks, 
        MBNum2YBlock(currMBNum,comp,picSizeX),
        predMB, recfr->yptr, picSizeX, regionSize);
      
      if (currDist < minDist || !fInterNeighborExists) 
      {
        
        minDist = currDist;            
        for (k=0;k<3;k++) 
          mvBest[k] = mvPred[k];
        
        currRegion->regionMode = 
          ((regionSize == 16) ? REGMODE_INTER_COPY : REGMODE_INTER_COPY_8x8);
        
        copyPredMB(MBNum2YBlock(currMBNum,comp,picSizeX), predMB, recfr, 
          picSizeX, regionSize,dec_params);
      }
    }
    
    for (i=0; i<3; i++)
      currRegion->mv[i] = mvBest[i];
    
    yCondition[MBNum2YBlock(currMBNum,comp,picSizeX)] = ERC_BLOCK_CONCEALED;
    comp = (comp+order+4)%4;
    compLeft--;
    
    } while (compLeft);
    
    return 0;
}

/*!
************************************************************************
 * \brief
 *      Builds the motion prediction pixels from the given location (in 1/4 pixel units) 
 *      of the reference frame. It not only copies the pixel values but builds the interpolation 
 *      when the pixel positions to be copied from is not full pixel (any 1/4 pixel position).
 *      It copies the resulting pixel vlaues into predMB.
 * \param img
 *      The pointer of img_par struture of current frame
 * \param mv
 *      The pointer of the predicted MV of the current (being concealed) MB
 * \param x
 *      The x-coordinate of the above-left corner pixel of the current MB
 * \param y
 *      The y-coordinate of the above-left corner pixel of the current MB
 * \param predMB
 *      memory area for storing temporary pixel values for a macroblock
 *      the Y,U,V planes are concatenated y = predMB, u = predMB+256, v = predMB+320
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
*/

static void buildPredRegionYUV(ImageParameters *img, int32 *mv, int x, int y, imgpel *predMB, h264_decoder* dec_params)
{
    int tmp_block[BLOCK_SIZE][BLOCK_SIZE];
    int i=0,j=0,ii=0,jj=0,i1=0,j1=0,j4=0,i4=0;
    int jf=0;
    int uv;
    int vec1_x=0,vec1_y=0;
    int ioff,joff;
    imgpel *pMB = predMB;
    imgpel* imgUV = NULL;
    int ii0,jj0,ii1,jj1,if1,jf1,if0,jf0;
    int mv_mul;
    int stride_cr = img->width_cr+32;
    //FRExt
    int f1_x, f1_y, f2_x, f2_y, f3, f4, ifx;
    int b8, b4;
    
    int ref_frame = mv[2];
    
    /* Update coordinates of the current concealed macroblock */
    img->mb_x = x/MB_BLOCK_SIZE;
    img->mb_y = y/MB_BLOCK_SIZE;
//    img->block_y = img->mb_y * BLOCK_SIZE;
//    img->pix_c_y = img->mb_y * MB_CR_SIZE_Y;
//    img->block_x = img->mb_x * BLOCK_SIZE;
//    img->pix_c_x = img->mb_x * MB_CR_SIZE_X;
    
    mv_mul=4;
    
    // luma *******************************************************
    
    for(j=0;j<MB_BLOCK_SIZE/BLOCK_SIZE;j++)
    {
        joff=j*4;
        //j4=img->block_y+j;
        for(i=0;i<MB_BLOCK_SIZE/BLOCK_SIZE;i++)
        {
            ioff=i*4;
            //i4=img->block_x+i;
            
            vec1_x = i4*4*mv_mul + mv[0];
            vec1_y = j4*4*mv_mul + mv[1];
            
            for(ii=0;ii<BLOCK_SIZE;ii++)
            {
                for(jj=0;jj<MB_BLOCK_SIZE/BLOCK_SIZE;jj++)
                {
                    img->mpr1[0][ii+ioff][jj+joff]=tmp_block[ii][jj];
                }
            }
        }
    }
    
    for (i = 0; i < 16; i++)
    {
        for (j = 0; j < 16; j++)
        {
            pMB[i*16+j] = img->mpr1[0][j][i];
        }
    }
    pMB += 256;
    
    // chroma *******************************************************
    f1_x = 64/MB_CR_SIZE_X;
    f2_x=f1_x-1;
    
    f1_y = 64/MB_CR_SIZE_Y;
    f2_y=f1_y-1;
    
    f3=f1_x*f1_y;
    f4=f3>>1;
    
    for(uv=0;uv<2;uv++)
    {
        imgUV = uv ? dec_params->listX[0][ref_frame]->imgV : dec_params->listX[0][ref_frame]->imgU;
        for (b8=0;b8<2;b8++)
        {
            for(b4=0;b4<4;b4++)
            {
                //joff = subblk_offset_y[0][b8][b4];
                joff = (b4&2)<<1;
//                j4=img->pix_c_y+joff;
                //ioff = subblk_offset_x[0][b8][b4];
                ioff = (b4&1)<<2;
//                i4=img->pix_c_x+ioff;
                
                for(jj=0;jj<4;jj++)
                {
                    jf=(j4+jj)/(MB_CR_SIZE_Y/4);     // jf  = Subblock_y-coordinate
                    for(ii=0;ii<4;ii++)
                    {
                        ifx=(i4+ii)/(MB_CR_SIZE_X/4);  // ifx = Subblock_x-coordinate
                        
                        i1=(i4+ii)*f1_x + mv[0];
                        j1=(j4+jj)*f1_y + mv[1];
                        
                        ii0=max (0, min (i1/f1_x,   dec_params->img->width_cr-1));
                        jj0=max (0, min (j1/f1_y,   dec_params->img->height_cr-1));
                        ii1=max (0, min ((i1+f2_x)/f1_x, dec_params->img->width_cr-1));
                        jj1=max (0, min ((j1+f2_y)/f1_y, dec_params->img->height_cr-1));
                        
                        if1=(i1 & f2_x);
                        jf1=(j1 & f2_y);
                        if0=f1_x-if1;
                        jf0=f1_y-jf1;
                        
                        //              img->mpr[ii+ioff][jj+joff]=(if0*jf0*dec_params->listX[0][ref_frame]->imgUV[uv][jj0][ii0]+
                        //                                          if1*jf0*dec_params->listX[0][ref_frame]->imgUV[uv][jj0][ii1]+
                        //                                          if0*jf1*dec_params->listX[0][ref_frame]->imgUV[uv][jj1][ii0]+
                        //                                          if1*jf1*dec_params->listX[0][ref_frame]->imgUV[uv][jj1][ii1]+f4)/f3;
                        img->mpr1[1][ii+ioff][jj+joff]=(
                            if0*jf0*imgUV[(jj0+16)*stride_cr+ii0+16]+
                            if1*jf0*imgUV[(jj0+16)*stride_cr+ii1+16]+
                            if0*jf1*imgUV[(jj1+16)*stride_cr+ii0+16]+
                            if1*jf1*imgUV[(jj1+16)*stride_cr+ii1+16]+f4)/f3;
                    }
                }
            }
        }
        
        for (i = 0; i < 8; i++)
        {
            for (j = 0; j < 8; j++)
            {
                pMB[i*8+j] = img->mpr1[1][j][i];
            }
        }
        pMB += 64;
        
    }
}
/*!
 ************************************************************************
 * \brief
 *      Copies pixel values between a YUV frame and the temporary pixel value storage place. This is
 *      used to save some pixel values temporarily before overwriting it, or to copy back to a given 
 *      location in a frame the saved pixel values.
 * \param currYBlockNum   
 *      index of the block (8x8) in the Y plane
 * \param predMB          
 *      memory area where the temporary pixel values are stored
 *      the Y,U,V planes are concatenated y = predMB, u = predMB+256, v = predMB+320
 * \param recfr           
 *      pointer to a YUV frame
 * \param picSizeX        
 *      picture width in pixels
 * \param regionSize      
 *      can be 16 or 8 to tell the dimension of the region to copy
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */

static void copyPredMB (int currYBlockNum, imgpel *predMB, frame *recfr, 
                        int32 picSizeX, int32 regionSize, h264_decoder*dec_params) 
{
    
    int j, k, xmin, ymin, xmax, ymax;
    int32 locationTmp, locationPred;
    int uv_x = uv_div[0][1];
    int uv_y = uv_div[1][1];
    imgpel* imgY = dec_params->dec_picture->imgY;
    imgpel* imgU = dec_params->dec_picture->imgU;
    imgpel* imgV = dec_params->dec_picture->imgV;
    unsigned int stride = dec_params->img->width+64;
    unsigned int stride_cr = dec_params->img->width_cr+32;
    xmin = (xPosYBlock(currYBlockNum,picSizeX)<<3);
    ymin = (yPosYBlock(currYBlockNum,picSizeX)<<3);
    xmax = xmin + regionSize -1;
    ymax = ymin + regionSize -1;

	
    //imgY = (ymin+32)*stride + xmin + 32;

    for (j = ymin; j <= ymax; j++) 
    {
        for (k = xmin; k <= xmax; k++)
        {
            locationPred = j * picSizeX + k;
            locationTmp = (j-ymin) * 16 + (k-xmin);
            *(imgY++) = predMB[locationTmp];
        }
        imgY += stride-regionSize;
    }
    
    for (j = (ymin>>uv_y); j <= (ymax>>uv_y); j++) 
    {
        for (k = (xmin>>uv_x); k <= (xmax>>uv_x); k++)
        {
            locationPred = ((j * picSizeX) >> uv_x) + k;
            locationTmp = (j-(ymin>>uv_y)) * MB_CR_SIZE_X + (k-(xmin>>1)) + 256;
            imgU[(j+16)*stride_cr+k+16] = predMB[locationTmp];
            
            locationTmp += 64;
            
            imgV[(j+16)*stride_cr+k+16] = predMB[locationTmp];
        }
    }
}

/*!
 ************************************************************************
 * \brief
 *      Calculates a weighted pixel difference between edge Y pixels of the macroblock stored in predMB
 *      and the pixels in the given Y plane of a frame (recY) that would become neighbor pixels if 
 *      predMB was placed at currYBlockNum block position into the frame. This "edge distortion" value
 *      is used to determine how well the given macroblock in predMB would fit into the frame when
 *      considering spatial smoothness. If there are correctly received neighbor blocks (status stored 
 *      in predBlocks) only they are used in calculating the edge distorion; otherwise also the already
 *      concealed neighbor blocks can also be used.
 * \return 
 *      The calculated weighted pixel difference at the edges of the MB.
 * \param predBlocks      
 *      status array of the neighboring blocks (if they are OK, concealed or lost)
 * \param currYBlockNum   
 *      index of the block (8x8) in the Y plane
 * \param predMB          
 *      memory area where the temporary pixel values are stored
 *      the Y,U,V planes are concatenated y = predMB, u = predMB+256, v = predMB+320
 * \param recY            
 *      pointer to a Y plane of a YUV frame
 * \param picSizeX        
 *      picture width in pixels
 * \param regionSize      
 *      can be 16 or 8 to tell the dimension of the region to copy
 ************************************************************************
 */
static int edgeDistortion (int predBlocks[], int currYBlockNum, imgpel *predMB, 
                           imgpel *recY, int32 picSizeX, int32 regionSize)
{
  int i, j, distortion, numOfPredBlocks, threshold = ERC_BLOCK_OK;
  imgpel *currBlock = NULL, *neighbor = NULL;
  int32 currBlockOffset = 0;
  
  currBlock = recY + (yPosYBlock(currYBlockNum,picSizeX)<<3)*picSizeX + (xPosYBlock(currYBlockNum,picSizeX)<<3);
  
  do 
  {
    
    distortion = 0; numOfPredBlocks = 0;
    
    /* loop the 4 neighbours */
    for (j = 4; j < 8; j++) 
    {
      /* if reliable, count boundary pixel difference */
      if (predBlocks[j] >= threshold) 
      {
        
        switch (j) 
        {
        case 4:
          neighbor = currBlock - picSizeX;
          for ( i = 0; i < regionSize; i++ ) 
          {
            distortion += mabs((int)(predMB[i] - neighbor[i]));
          }
          break;          
        case 5:
          neighbor = currBlock - 1;
          for( i = 0; i < regionSize; i++ ) 
          {
            distortion += mabs((int)(predMB[i*16] - neighbor[i*picSizeX]));
          }
          break;                
        case 6:
          neighbor = currBlock + regionSize*picSizeX;
          currBlockOffset = (regionSize-1)*16;
          for( i = 0; i < regionSize; i++ ) 
          {
            distortion += mabs((int)(predMB[i+currBlockOffset] - neighbor[i]));
          }
          break;                
        case 7:
          neighbor = currBlock + regionSize;
          currBlockOffset = regionSize-1;
          for( i = 0; i < regionSize; i++ ) 
          {
            distortion += mabs((int)(predMB[i*16+currBlockOffset] - neighbor[i*picSizeX]));
          }
          break;
        }
        
        numOfPredBlocks++;
      }
    }
    
    threshold--;
    if (threshold < ERC_BLOCK_CONCEALED)
      break;
  } while (numOfPredBlocks == 0);
  
  if(numOfPredBlocks == 0)
    assert (numOfPredBlocks != 0);
  return (distortion/numOfPredBlocks);
}

