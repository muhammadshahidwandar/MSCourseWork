 
/*!
*************************************************************************************
* \file cabac.c
*
* \brief
*    CABAC entropy coding routines
*
* \author
*    Main contributors (see contributors.h for copyright, address and affiliation details)
*    - Detlev Marpe                    <marpe@hhi.de>
**************************************************************************************
*/
#include "global.h"
//#ifdef __CABAC__

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "cabac.h"
#include "memalloc.h"
#include "elements.h"
#include "image.h"
#include "biaridecod.h"
#include "mb_access.h"


int symbolCount = 0;
int last_dquant = 0;

extern int assignSE2partition[][SE_MAX_ELEMENTS];
extern const unsigned char allowed_pixel_value[512];
extern const byte SNGL_SCAN[16][2];
extern const unsigned char subblk_offset_x[3][8][4];
extern const unsigned char subblk_offset_y[3][8][4];
extern const int BLOCK_STEP[8][2];
extern const byte QP_SCALE_CR[52];
extern const int dequant_coef[6][4][4];

extern void SetMotionVectorPredictor_baseline(short           *pmv_x,
											  short           *pmv_y,
											  char            ref_frame,
											  byte            list,
											  int             block_x,
											  int             block_y,
											  int             blockshape_x,
											  int             blockshape_y,
											  h264_decoder* dec_params);
/***********************************************************************
* L O C A L L Y   D E F I N E D   F U N C T I O N   P R O T O T Y P E S
***********************************************************************
*/
unsigned int unary_bin_decode(DecodingEnvironmentPtr dep_dp,
                              BiContextTypePtr ctx,
                              int ctx_offset);


unsigned int unary_bin_max_decode(DecodingEnvironmentPtr dep_dp,
                                  BiContextTypePtr ctx,
                                  int ctx_offset,
                                  unsigned int max_symbol);

unsigned int unary_exp_golomb_level_decode( DecodingEnvironmentPtr dep_dp,
                                           BiContextTypePtr ctx);

unsigned int unary_exp_golomb_mv_decode(DecodingEnvironmentPtr dep_dp,
                                        BiContextTypePtr ctx,
                                        unsigned int max_bin);


void CheckAvailabilityOfNeighborsCABAC(h264_decoder* dec_params)
{
    ImageParameters *img = dec_params->img;
    Macroblock *currMB = &img->mb_data[img->current_mb_nr];
    PixelPos up, left;
    
    //getNeighbour(img->current_mb_nr, -1,  0, 1, &left, dec_params, dec_outputs);
    //getNeighbour(img->current_mb_nr,  0, -1, 1, &up, dec_params, dec_outputs);
    
    if (currMB->mbAvailB)
    {
        currMB->mb_available_up = &img->mb_data[currMB->mbAddrB];
    }
    else
    {
        currMB->mb_available_up = NULL;
    }
    if (currMB->mbAvailA)
    {
        currMB->mb_available_left = &img->mb_data[currMB->mbAddrA];
    }
    else
    {
        currMB->mb_available_left = NULL;
    }
}

__inline void cabac_new_slice()
{
    last_dquant=0;
}

/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode the motion
*    vector data of a B-frame MB.
************************************************************************
*/
void readMVD_CABAC(SyntaxElement *se, h264_decoder* dec_params, DecodingEnvironmentPtr dep_dp)
{
    ImageParameters *img = dec_params->img;
    int i;// = img->subblock_x;
    int j;// = img->subblock_y;
    int a, b;
    int act_ctx;
    int act_sym;
    int mv_local_err;
    int mv_sign;
    int list_idx = se->value2 & 0x01;
    int k = (se->value2>>1); // MVD component
    PixelPos block_a, block_b;
    
    MotionInfoContexts *ctx = dec_params->mot_ctx;
    Macroblock *currMB = &img->mb_data[img->current_mb_nr];
    
    getLuma4x4Neighbour(i, j, -1,  0, &block_a, dec_params);
    getLuma4x4Neighbour(i, j,  0, -1, &block_b, dec_params);
    
    if (block_b.available)
    {
        b = absm(img->mb_data[block_b.mb_addr].mvd[list_idx][block_b.y][block_b.x][k]);
        /*
        if (img->MbaffFrameFlag && (k==1)) 
        {
        if ((currMB->mb_field==0) && (img->mb_data[block_b.mb_addr].mb_field==1))
        {
        b *= 2;
        }
        else if ((currMB->mb_field==1) && (img->mb_data[block_b.mb_addr].mb_field==0))
        {
        b /= 2;
        }
        }
        */
    }
    else
    {
        b = 0;
    }
    
    if (block_a.available)
    {
        a = absm(img->mb_data[block_a.mb_addr].mvd[list_idx][block_a.y][block_a.x][k]);
        /*
        if (img->MbaffFrameFlag && (k==1)) 
        {
        if ((currMB->mb_field==0) && (img->mb_data[block_a.mb_addr].mb_field==1))
        {
        a *= 2;
        }
        else if ((currMB->mb_field==1) && (img->mb_data[block_a.mb_addr].mb_field==0))
        {
        a /= 2;
        }
        }
        */
    }
    else
    {
        a = 0;
    }
    
    if ((mv_local_err=a+b)<3)
    {
        act_ctx = 5*k;
    }
    else
    {
        if (mv_local_err>32)
        {
            act_ctx = 5*k+3;
        }
        else
        {
            act_ctx = 5*k+2;
        }
    }
    se->context = act_ctx;
    
    act_sym = biari_decode_symbol(dep_dp,&ctx->mv_res_contexts[0][act_ctx] );
    
    if (act_sym != 0)
    {
        act_ctx = 5*k;
        act_sym = unary_exp_golomb_mv_decode(dep_dp,ctx->mv_res_contexts[1]+act_ctx,3);
        act_sym++;
        mv_sign = biari_decode_symbol_eq_prob(dep_dp);
        
        if(mv_sign)
        {
            act_sym = -act_sym;
        }
    }
    se->value1 = act_sym;
    
#if TRACE
    fprintf(p_trace, "@%d %s\t\t\t%d \n",symbolCount++, se->tracestring, se->value1);
    fflush(p_trace);
#endif
}


/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode the 8x8 block type.
************************************************************************
*/
void readB8_typeInfo_CABAC(SyntaxElement *se, h264_decoder* dec_params, DecodingEnvironmentPtr dep_dp)
{
    ImageParameters *img = dec_params->img;
    int act_sym = 0;
    int bframe  = (img->type==B_SLICE);
    
    MotionInfoContexts *ctx = dec_params->mot_ctx;
    
    
    if (!bframe)
    {
        if (biari_decode_symbol (dep_dp, &ctx->b8_type_contexts[0][1]))
        {
            act_sym = 0;
        }
        else
        {
            if (biari_decode_symbol (dep_dp, &ctx->b8_type_contexts[0][3]))
            {
                if (biari_decode_symbol (dep_dp, &ctx->b8_type_contexts[0][4])) act_sym = 2;
                else                                                            act_sym = 3;
            }
            else
            {
                act_sym = 1;
            }
        }
    }
    else
    {
        if (biari_decode_symbol (dep_dp, &ctx->b8_type_contexts[1][0]))
        {
            if (biari_decode_symbol (dep_dp, &ctx->b8_type_contexts[1][1]))
            {
                if (biari_decode_symbol (dep_dp, &ctx->b8_type_contexts[1][2]))
                {
                    if (biari_decode_symbol (dep_dp, &ctx->b8_type_contexts[1][3]))
                    {
                        act_sym = 10;
                        if (biari_decode_symbol (dep_dp, &ctx->b8_type_contexts[1][3])) act_sym++;
                    }
                    else
                    {
                        act_sym = 6;
                        if (biari_decode_symbol (dep_dp, &ctx->b8_type_contexts[1][3])) act_sym+=2;
                        if (biari_decode_symbol (dep_dp, &ctx->b8_type_contexts[1][3])) act_sym++;
                    }
                }
                else
                {
                    act_sym=2;
                    if (biari_decode_symbol (dep_dp, &ctx->b8_type_contexts[1][3])) act_sym+=2;
                    if (biari_decode_symbol (dep_dp, &ctx->b8_type_contexts[1][3])) act_sym+=1;
                }
            }
            else
            {
                if (biari_decode_symbol (dep_dp, &ctx->b8_type_contexts[1][3])) act_sym = 1;
                else                                                            act_sym = 0;
            }
            act_sym++;
        }
        else
        {
            act_sym= 0;
        }
    }
    se->value1 = act_sym;
    
#if TRACE
    fprintf(p_trace, "@%d %s\t\t%d\n",symbolCount++, se->tracestring, se->value1);
    fflush(p_trace);
#endif
}

/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode the macroblock
*    type info of a given MB.
************************************************************************
*/
void readMB_skip_flagInfo_CABAC(SyntaxElement *se, h264_decoder* dec_params, DecodingEnvironmentPtr dep_dp)
{
    ImageParameters *img = dec_params->img;
    int a, b;
    int act_ctx;
    int bframe=(img->type==B_SLICE);
    MotionInfoContexts *ctx = dec_params->mot_ctx;
    Macroblock *currMB = &img->mb_data[img->current_mb_nr];
    
    
    if (bframe)
    {
        if (currMB->mb_available_up == NULL)
            b = 0;
        else
            b = (currMB->mb_available_up->skip_flag==0 ? 1 : 0);
        if (currMB->mb_available_left == NULL)
            a = 0;
        else
            a = (currMB->mb_available_left->skip_flag==0 ? 1 : 0);
        
        act_ctx = 7 + a + b;
        
        if (biari_decode_symbol (dep_dp, &ctx->mb_type_contexts[2][act_ctx]) == 1)
            se->value1 = se->value2 = 0;
        else
            se->value1 = se->value2 = 1;
    }
    else
    {
        if (currMB->mb_available_up == NULL)
            b = 0;
        else
            b = (( (currMB->mb_available_up)->skip_flag == 0) ? 1 : 0 );
        if (currMB->mb_available_left == NULL)
            a = 0;
        else
            a = (( (currMB->mb_available_left)->skip_flag == 0) ? 1 : 0 );
        
        act_ctx = a + b;
        
        if (biari_decode_symbol(dep_dp, &ctx->mb_type_contexts[1][act_ctx]) == 1)
            se->value1 = 0;
        else
            se->value1 = 1;
    }
    
    
#if TRACE
    fprintf(p_trace, "@%d %s\t\t%d\t%d %d\n",symbolCount++, se->tracestring, se->value1,a,b);
    fflush(p_trace);
#endif
    if (!se->value1)
    {
        last_dquant=0;
    }
    return;
}

/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode the macroblock
*    type info of a given MB.
************************************************************************
*/
void readMB_typeInfo_CABAC(SyntaxElement *se, h264_decoder* dec_params, DecodingEnvironmentPtr dep_dp)
{
    ImageParameters *img = dec_params->img;
    int a, b;
    int act_ctx;
    int act_sym;
    int mode_sym;
    int ct = 0;
    int curr_mb_type;
    MotionInfoContexts *ctx = dec_params->mot_ctx;
    Macroblock *currMB = &img->mb_data[img->current_mb_nr];
    int bframe=(img->type==B_SLICE);
    
    if(img->type == I_SLICE)  // INTRA-frame
    {
        if (currMB->mb_available_up == NULL)
        {
            b = 0;
        }
        else {
            //b = (((currMB->mb_available_up)->mb_type != I4MB && currMB->mb_available_up->mb_type != I8MB) ? 1 : 0 );
            b = (((currMB->mb_available_up)->mb_type != I4MB) ? 1 : 0 );
        }
        
        if (currMB->mb_available_left == NULL){
            a = 0;
        }
        else 
        {
            //a = (((currMB->mb_available_left)->mb_type != I4MB && currMB->mb_available_left->mb_type != I8MB) ? 1 : 0 );
            a = (((currMB->mb_available_left)->mb_type != I4MB) ? 1 : 0 );
        }
        
        act_ctx = a + b;
        act_sym = biari_decode_symbol(dep_dp, ctx->mb_type_contexts[0] + act_ctx);
        se->context = act_ctx; // store context
        
        if (act_sym==0) // 4x4 Intra
        {
            curr_mb_type = act_sym;
        }
        else // 16x16 Intra
        {
            mode_sym = biari_decode_final(dep_dp);
            if(mode_sym == 1)
            {
                curr_mb_type = 25;
            }
            else
            {
                act_sym = 1;
                act_ctx = 4;
                mode_sym =  biari_decode_symbol(dep_dp, ctx->mb_type_contexts[0] + act_ctx ); // decoding of AC/no AC
                act_sym += mode_sym*12;
                act_ctx = 5;
                // decoding of cbp: 0,1,2
                mode_sym =  biari_decode_symbol(dep_dp, ctx->mb_type_contexts[0] + act_ctx );
                if (mode_sym!=0)
                {
                    act_ctx=6;
                    mode_sym = biari_decode_symbol(dep_dp, ctx->mb_type_contexts[0] + act_ctx );
                    act_sym+=4;
                    if (mode_sym!=0)
                        act_sym+=4;
                }
                // decoding of I pred-mode: 0,1,2,3
                act_ctx = 7;
                mode_sym =  biari_decode_symbol(dep_dp, ctx->mb_type_contexts[0] + act_ctx );
                act_sym += mode_sym*2;
                act_ctx = 8;
                mode_sym =  biari_decode_symbol(dep_dp, ctx->mb_type_contexts[0] + act_ctx );
                act_sym += mode_sym;
                curr_mb_type = act_sym;
            }
        }
    }
    /*
    else if(img->type == SI_SLICE)  // SI-frame
    {
    // special ctx's for SI4MB
    if (currMB->mb_available_up == NULL)
    b = 0;
    else 
    b = (( (currMB->mb_available_up)->mb_type != SI4MB) ? 1 : 0 );
    if (currMB->mb_available_left == NULL)
    a = 0;
    else 
    a = (( (currMB->mb_available_left)->mb_type != SI4MB) ? 1 : 0 );
    
      act_ctx = a + b;
      act_sym = biari_decode_symbol(dep_dp, ctx->mb_type_contexts[1] + act_ctx);
      se->context = act_ctx; // store context
      
        if (act_sym==0) //  SI 4x4 Intra
        {
        curr_mb_type = 0;
        }
        else // analog INTRA_IMG
        {
        if (currMB->mb_available_up == NULL)
        b = 0;
        else 
        b = (( (currMB->mb_available_up)->mb_type != I4MB) ? 1 : 0 );
        if (currMB->mb_available_left == NULL)
        a = 0;
        else 
        a = (( (currMB->mb_available_left)->mb_type != I4MB) ? 1 : 0 );
        
          act_ctx = a + b;
          act_sym = biari_decode_symbol(dep_dp, ctx->mb_type_contexts[0] + act_ctx);
          se->context = act_ctx; // store context
          
            
              if (act_sym==0) // 4x4 Intra
              {
              curr_mb_type = 1;
              }
              else // 16x16 Intra
              {
              mode_sym = biari_decode_final(dep_dp);
              if( mode_sym==1 )
              {
              curr_mb_type = 26;
              }
              else
              {
              act_sym = 2;
              act_ctx = 4;
              mode_sym =  biari_decode_symbol(dep_dp, ctx->mb_type_contexts[0] + act_ctx ); // decoding of AC/no AC
              act_sym += mode_sym*12;
              act_ctx = 5;
              // decoding of cbp: 0,1,2
              mode_sym =  biari_decode_symbol(dep_dp, ctx->mb_type_contexts[0] + act_ctx );
              if (mode_sym!=0)
              {
              act_ctx=6;
              mode_sym = biari_decode_symbol(dep_dp, ctx->mb_type_contexts[0] + act_ctx );
              act_sym+=4;
              if (mode_sym!=0)
              act_sym+=4;
              }
              // decoding of I pred-mode: 0,1,2,3
              act_ctx = 7;
              mode_sym =  biari_decode_symbol(dep_dp, ctx->mb_type_contexts[0] + act_ctx );
              act_sym += mode_sym*2;
              act_ctx = 8;
              mode_sym =  biari_decode_symbol(dep_dp, ctx->mb_type_contexts[0] + act_ctx );
              act_sym += mode_sym;
              curr_mb_type = act_sym;
              }
              }
              }
              }
              */
              else
              {
                  if (bframe)
                  {
                      ct = 1;
                      if (currMB->mb_available_up == NULL)
                      {
                          b = 0;
                      }
                      else
                      {
                          b = (( (currMB->mb_available_up)->mb_type != 0) ? 1 : 0 );
                      }
                      if (currMB->mb_available_left == NULL)
                      {
                          a = 0;
                      }
                      else
                      {
                          a = (( (currMB->mb_available_left)->mb_type != 0) ? 1 : 0 );
                      }
                      
                      act_ctx = a + b;
                      
                      if (biari_decode_symbol (dep_dp, &ctx->mb_type_contexts[2][act_ctx]))
                      {
                          if (biari_decode_symbol (dep_dp, &ctx->mb_type_contexts[2][4]))
                          {
                              if (biari_decode_symbol (dep_dp, &ctx->mb_type_contexts[2][5]))
                              {
                                  act_sym=12;
                                  if (biari_decode_symbol (dep_dp, &ctx->mb_type_contexts[2][6]))
                                  {
                                      act_sym+=8;
                                  }
                                  if (biari_decode_symbol (dep_dp, &ctx->mb_type_contexts[2][6]))
                                  {
                                      act_sym+=4;
                                  }
                                  if (biari_decode_symbol (dep_dp, &ctx->mb_type_contexts[2][6]))
                                  {
                                      act_sym+=2;
                                  }
                                  
                                  if      (act_sym==24)
                                  {
                                      act_sym=11;
                                  }
                                  else if (act_sym==26)
                                  {
                                      act_sym=22;
                                  }
                                  else
                                  {
                                      if (act_sym==22)
                                      {
                                          act_sym=23;
                                      }
                                      if (biari_decode_symbol (dep_dp, &ctx->mb_type_contexts[2][6]))
                                      {
                                          act_sym+=1; 
                                      }
                                  }
                              }
                              else
                              {
                                  act_sym=3;
                                  if (biari_decode_symbol (dep_dp, &ctx->mb_type_contexts[2][6]))
                                  {
                                      act_sym+=4;
                                  }
                                  if (biari_decode_symbol (dep_dp, &ctx->mb_type_contexts[2][6]))
                                  {
                                      act_sym+=2;
                                  }
                                  if (biari_decode_symbol (dep_dp, &ctx->mb_type_contexts[2][6]))
                                  {
                                      act_sym+=1;
                                  }
                              }
                          }
                          else
                          {
                              if (biari_decode_symbol (dep_dp, &ctx->mb_type_contexts[2][6]))
                              {
                                  act_sym=2;
                              }
                              else
                              {
                                  act_sym=1;
                              }
                          }
                      }
                      else
                      {
                          act_sym = 0;
                      }
                  }
                  else // P-frame
                  {
                      {
                          if (biari_decode_symbol(dep_dp, &ctx->mb_type_contexts[1][4] )) 
                          {
                              if (biari_decode_symbol(dep_dp, &ctx->mb_type_contexts[1][7] ))   act_sym = 7;
                              else                                                              act_sym = 6;
                          }
                          else
                          {
                              if (biari_decode_symbol(dep_dp, &ctx->mb_type_contexts[1][5] ))
                              {
                                  if (biari_decode_symbol(dep_dp, &ctx->mb_type_contexts[1][7] )) act_sym = 2;
                                  else                                                            act_sym = 3;
                              }
                              else
                              {
                                  if (biari_decode_symbol(dep_dp, &ctx->mb_type_contexts[1][6] )) act_sym = 4;
                                  else                                                            act_sym = 1;
                              }
                          }
                      }
                  }
                  
                  if (act_sym<=6 || (((img->type == B_SLICE)?1:0) && act_sym<=23))
                  {
                      curr_mb_type = act_sym;
                  }
                  else  // additional info for 16x16 Intra-mode
                  {
                      mode_sym = biari_decode_final(dep_dp);
                      if( mode_sym==1 )
                      {
                          if(bframe)  // B frame
                          {
                              curr_mb_type = 48;
                          }
                          else      // P frame
                          {
                              curr_mb_type = 31;
                          }
                      }
                      else
                      {
                          act_ctx = 8;
                          mode_sym =  biari_decode_symbol(dep_dp, ctx->mb_type_contexts[1] + act_ctx ); // decoding of AC/no AC
                          act_sym += mode_sym*12;
                          
                          // decoding of cbp: 0,1,2
                          act_ctx = 9;
                          mode_sym = biari_decode_symbol(dep_dp, ctx->mb_type_contexts[1] + act_ctx );
                          if (mode_sym != 0)
                          {
                              act_sym+=4;
                              mode_sym = biari_decode_symbol(dep_dp, ctx->mb_type_contexts[1] + act_ctx );
                              if (mode_sym != 0)
                                  act_sym+=4;
                          }
                          
                          // decoding of I pred-mode: 0,1,2,3
                          act_ctx = 10;
                          mode_sym = biari_decode_symbol(dep_dp, ctx->mb_type_contexts[1] + act_ctx );
                          act_sym += mode_sym*2;
                          mode_sym = biari_decode_symbol(dep_dp, ctx->mb_type_contexts[1] + act_ctx );
                          act_sym += mode_sym;
                          curr_mb_type = act_sym;
                      }
                  }
  }
  se->value1 = curr_mb_type;
  
  //  if (curr_mb_type >= 23)       printf(" stopx");
#if TRACE
  fprintf(p_trace, "@%d %s\t\t\t%d\n",symbolCount++, se->tracestring, se->value1);
  fflush(p_trace);
#endif
}

/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode a pair of
*    intra prediction modes of a given MB.
************************************************************************
*/
void readIntraPredMode_CABAC(SyntaxElement *se, h264_decoder* dec_params, DecodingEnvironmentPtr dep_dp)
{
    ImageParameters *img = dec_params->img;
    TextureInfoContexts *ctx     = dec_params->tex_ctx;
    int act_sym;
    
    // use_most_probable_mode
    act_sym = biari_decode_symbol(dep_dp, ctx->ipr_contexts);
    
    // remaining_mode_selector
    if (act_sym == 1)
        se->value1 = -1;
    else
    {
        se->value1  = 0;
        se->value1 |= (biari_decode_symbol(dep_dp, ctx->ipr_contexts+1)     );
        se->value1 |= (biari_decode_symbol(dep_dp, ctx->ipr_contexts+1) << 1);
        se->value1 |= (biari_decode_symbol(dep_dp, ctx->ipr_contexts+1) << 2);
    }
    
#if TRACE
    fprintf(p_trace, "@%d %s\t\t\t%d\n",symbolCount++, se->tracestring, se->value1);
    fflush(p_trace);
#endif
}
/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode the reference
*    parameter of a given MB.
************************************************************************
*/
void readRefFrame_CABAC(SyntaxElement *se, h264_decoder* dec_params, DecodingEnvironmentPtr dep_dp)
{
    ImageParameters *img = dec_params->img;
    MotionInfoContexts *ctx = dec_params->mot_ctx;
    Macroblock *currMB = &img->mb_data[img->current_mb_nr];
    StorablePicture *dec_picture = dec_params->dec_picture;
    
    int   addctx  = 0;
    int   a, b;
    int   act_ctx;
    int   act_sym;
    //char** refframe_array = dec_picture->ref_idx[se->value2];// list 0 or 1
    int   b8a, b8b;
    
    PixelPos block_a, block_b;
    
	// faisal ref_idx
	char* refframe_array;
	
	if(!se->value2)// list 0 or 1
	{
		refframe_array = dec_picture->ref_idx_l0;
	}
	else
	{
		refframe_array = dec_picture->ref_idx_l1;
	}
    
//    getLuma4x4Neighbour(img->subblock_x, img->subblock_y, -1,  0, &block_a,dec_params);
//    getLuma4x4Neighbour(img->subblock_x, img->subblock_y,  0, -1, &block_b,dec_params);
    
    b8a=((block_a.x/2)%2)+2*((block_a.y/2)%2);
    b8b=((block_b.x/2)%2)+2*((block_b.y/2)%2);
    
    if (!block_b.available)
    {
        b=0;
    }
    else if ( (img->mb_data[block_b.mb_addr].mb_type==IPCM) || IS_DIRECT(&img->mb_data[block_b.mb_addr]) || (img->mb_data[block_b.mb_addr].b8mode[b8b]==0 && img->mb_data[block_b.mb_addr].b8pdir[b8b]==2))
    {
        b=0;
    }
    else 
    {
		int mb_nr_b,b_add;
	
		mb_nr_b = ((block_b.pos_y>>2) * img->FrameWidthInMbs) + (block_b.pos_x>>2);
	    b_add = ((block_b.pos_y%4) * 4) + (block_b.pos_x%4);
	
        //if (img->MbaffFrameFlag && (currMB->mb_field == 0) && (img->mb_data[block_b.mb_addr].mb_field == 1))
        //{
        //b = (refframe_array[block_b.pos_y][block_b.pos_x] > 1 ? 1 : 0);
        //}
        //else
        {
            //b = (refframe_array[block_b.pos_y][block_b.pos_x] > 0 ? 1 : 0);
			b = (*(refframe_array+(16*mb_nr_b) + b_add) > 0 ? 1 : 0);
        }
    }
    
    if (!block_a.available)
    {
        a=0;
    }
    else if ((img->mb_data[block_a.mb_addr].mb_type==IPCM) || IS_DIRECT(&img->mb_data[block_a.mb_addr]) || (img->mb_data[block_a.mb_addr].b8mode[b8a]==0 && img->mb_data[block_a.mb_addr].b8pdir[b8a]==2))
    {
        a=0;
    }
    else 
    {
		int mb_nr_a,a_add;
	
		mb_nr_a = ((block_a.pos_y>>2) * img->FrameWidthInMbs) + (block_a.pos_x>>2);
	    a_add = ((block_a.pos_y%4) * 4) + (block_a.pos_x%4);
        //if (img->MbaffFrameFlag && (currMB->mb_field == 0) && (img->mb_data[block_a.mb_addr].mb_field == 1))
        //{
        //a = (refframe_array[block_a.pos_y][block_a.pos_x] > 1 ? 1 : 0);
        //}
        //else
        {
            //a = (refframe_array[block_a.pos_y][block_a.pos_x] > 0 ? 1 : 0);
			a = (*(refframe_array+(16*mb_nr_a) + a_add) > 0 ? 1 : 0);
        }
    }
    
    act_ctx = a + 2*b;
    se->context = act_ctx; // store context
    
    act_sym = biari_decode_symbol(dep_dp,ctx->ref_no_contexts[addctx] + act_ctx );
    
    if (act_sym != 0)
    {
        act_ctx = 4;
        act_sym = unary_bin_decode(dep_dp,ctx->ref_no_contexts[addctx]+act_ctx,1);
        act_sym++;
    }
    se->value1 = act_sym;
    
#if TRACE
    fprintf(p_trace, "@%d %s\t\t\t%d \n",symbolCount++, se->tracestring, se->value1);
    //  fprintf(p_trace," c: %d :%d \n",ctx->ref_no_contexts[addctx][act_ctx].cum_freq[0],ctx->ref_no_contexts[addctx][act_ctx].cum_freq[1]);
    fflush(p_trace);
#endif
}


/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode the delta qp
*     of a given MB.
************************************************************************
*/
void readDquant_CABAC(SyntaxElement *se, h264_decoder* dec_params, DecodingEnvironmentPtr dep_dp)
{
    ImageParameters *img = dec_params->img;
    MotionInfoContexts *ctx = dec_params->mot_ctx;
    
    int act_ctx;
    int act_sym;
    int dquant;
    
    act_ctx = ( (last_dquant != 0) ? 1 : 0);
    
    act_sym = biari_decode_symbol(dep_dp,ctx->delta_qp_contexts + act_ctx );
    if (act_sym != 0)
    {
        act_ctx = 2;
        act_sym = unary_bin_decode(dep_dp,ctx->delta_qp_contexts+act_ctx,1);
        act_sym++;
    }
    
    dquant = (act_sym+1)/2;
    if((act_sym & 0x01)==0)                           // lsb is signed bit
        dquant = -dquant;
    se->value1 = dquant;
    
    last_dquant = dquant;
    
#if TRACE
    fprintf(p_trace, "@%d %s\t\t\t%d\n",symbolCount++, se->tracestring, se->value1);
    fflush(p_trace);
#endif
}
/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode the coded
*    block pattern of a given MB.
************************************************************************
*/
void readCBP_CABAC(SyntaxElement *se, h264_decoder* dec_params, DecodingEnvironmentPtr dep_dp)
{
    ImageParameters *img = dec_params->img;
    TextureInfoContexts *ctx = dec_params->tex_ctx;
    Macroblock *currMB = &img->mb_data[img->current_mb_nr];
    
    int mb_x, mb_y;
    int a, b;
    int curr_cbp_ctx, curr_cbp_idx;
    int cbp = 0;
    int cbp_bit;
    int mask;
    PixelPos block_a;
    
    //  coding of luma part (bit by bit)
    for (mb_y=0; mb_y < 4; mb_y += 2)
    {
        for (mb_x=0; mb_x < 4; mb_x += 2)
        {
            if (currMB->b8mode[mb_y+(mb_x/2)]==IBLOCK)
                curr_cbp_idx = 0;
            else
                curr_cbp_idx = 1;
            
            if (mb_y == 0)
            {
                if (currMB->mb_available_up == NULL)
                    b = 0;
                else
                {
                    if((currMB->mb_available_up)->mb_type==IPCM)
                        b=0;
                    else
                        b = (( ((currMB->mb_available_up)->cbp & (1<<(2+mb_x/2))) == 0) ? 1 : 0);
                }
                
            }
            else
                b = ( ((cbp & (1<<(mb_x/2))) == 0) ? 1: 0);
            
            if (mb_x == 0)
            {
                getLuma4x4Neighbour(mb_x, mb_y, -1, 0, &block_a, dec_params);
                if (block_a.available)
                {
                    {
                        if(img->mb_data[block_a.mb_addr].mb_type==IPCM)
                        {
                            a = 0;
                        }
                        else
                        {
                            a = (( (img->mb_data[block_a.mb_addr].cbp & (1<<(2*(block_a.y/2)+1))) == 0) ? 1 : 0);
                        }
                    }
                    
                }
                else
                    a=0;
            }
            else
                a = ( ((cbp & (1<<mb_y)) == 0) ? 1: 0);
            
            curr_cbp_ctx = a+2*b;
            mask = (1<<(mb_y+mb_x/2));
            cbp_bit = biari_decode_symbol(dep_dp, ctx->cbp_contexts[0] + curr_cbp_ctx );
            if (cbp_bit)
            {
                cbp += mask;
            }
        }
    }
    
    
    //if (dec_picture->chroma_format_idc != YUV400)
    {
        // coding of chroma part
        // CABAC decoding for BinIdx 0
        b = 0;
        if (currMB->mb_available_up != NULL)
        {
            if((currMB->mb_available_up)->mb_type==IPCM)
                b=1;
            else
                b = ((currMB->mb_available_up)->cbp > 15) ? 1 : 0;
        }
        
        
        a = 0;
        if (currMB->mb_available_left != NULL)
        {
            if((currMB->mb_available_left)->mb_type==IPCM)
                a=1;
            else
                a = ((currMB->mb_available_left)->cbp > 15) ? 1 : 0;
        }
        
        
        curr_cbp_ctx = a+2*b;
        cbp_bit = biari_decode_symbol(dep_dp, ctx->cbp_contexts[1] + curr_cbp_ctx );
        
        // CABAC decoding for BinIdx 1 
        if (cbp_bit) // set the chroma bits
        {
            b = 0;
            if (currMB->mb_available_up != NULL)
            {
                if((currMB->mb_available_up)->mb_type==IPCM)
                    b=1;
                else
                    if ((currMB->mb_available_up)->cbp > 15)
                        b = (( ((currMB->mb_available_up)->cbp >> 4) == 2) ? 1 : 0);
            }
            
            
            a = 0;
            if (currMB->mb_available_left != NULL)
            {
                if((currMB->mb_available_left)->mb_type==IPCM)
                    a=1;
                else
                    if ((currMB->mb_available_left)->cbp > 15)
                        a = (( ((currMB->mb_available_left)->cbp >> 4) == 2) ? 1 : 0);
            }
            
            
            curr_cbp_ctx = a+2*b;
            cbp_bit = biari_decode_symbol(dep_dp, ctx->cbp_contexts[2] + curr_cbp_ctx );
            cbp += (cbp_bit == 1) ? 32 : 16;
        }
    }
    
    se->value1 = cbp;
    
    if (!cbp)
    {
        last_dquant=0;
    }
    
#if TRACE
    fprintf(p_trace, "@%d %s\t\t\t%d\n",symbolCount++, se->tracestring, se->value1);
    fflush(p_trace);
#endif
}

/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode the chroma
*    intra prediction mode of a given MB.
************************************************************************
*/  //GB
void readCIPredMode_CABAC(SyntaxElement *se, h264_decoder* dec_params, DecodingEnvironmentPtr dep_dp)
{
    ImageParameters *img = dec_params->img;
    TextureInfoContexts *ctx = dec_params->tex_ctx;
    Macroblock          *currMB  = &img->mb_data[img->current_mb_nr];
    int                 act_ctx,a,b;
    int                 act_sym  = se->value1;
    
    if (currMB->mb_available_up == NULL)
    {
        b = 0;
    }
    else
    {
        if( (currMB->mb_available_up)->mb_type==IPCM)
        {
            b=0;
        }
        else
        {
            b = ( ((currMB->mb_available_up)->c_ipred_mode != 0) ? 1 : 0);
        }
    }
    
    
    if (currMB->mb_available_left == NULL) a = 0;
    else
    {
        if( (currMB->mb_available_left)->mb_type==IPCM)
        {
            a=0;
        }
        else
        {
            a = ( ((currMB->mb_available_left)->c_ipred_mode != 0) ? 1 : 0);
        }
    }
    
    
    act_ctx = a+b;
    
    act_sym = biari_decode_symbol(dep_dp, ctx->cipr_contexts + act_ctx );
    
    if (act_sym!=0)
    {
        act_sym = unary_bin_max_decode(dep_dp,ctx->cipr_contexts+3,0,2)+1;
    }
    
    
    se->value1 = act_sym;
    
    
#if TRACE
    fprintf(p_trace, "@%d %s\t\t\t%d\n",symbolCount++, se->tracestring, se->value1);
    fflush(p_trace);
#endif
    
}

static const int maxpos       [] = {16, 15, 64, 32, 32, 16,  4, 15,  8, 16};
static const int c1isdc       [] = { 1,  0,  1,  1,  1,  1,  1,  0,  1,  1};

static const int type2ctx_bcbp[] = { 0,  1,  2,  2,  3,  4,  5,  6,  5,  5}; // 7
static const int type2ctx_map [] = { 0,  1,  2,  3,  4,  5,  6,  7,  6,  6}; // 8
static const int type2ctx_last[] = { 0,  1,  2,  3,  4,  5,  6,  7,  6,  6}; // 8
static const int type2ctx_one [] = { 0,  1,  2,  3,  3,  4,  5,  6,  5,  5}; // 7
static const int type2ctx_abs [] = { 0,  1,  2,  3,  3,  4,  5,  6,  5,  5}; // 7
static const int max_c2       [] = { 4,  4,  4,  4,  4,  4,  3,  4,  3,  3}; // 9

                                                                             /*!
                                                                             ************************************************************************
                                                                             * \brief
                                                                             *    Read CBP4-BIT
                                                                             ************************************************************************
*/
int read_and_store_CBP_block_bit (Macroblock*				currMB,
                                  DecodingEnvironmentPtr	dep_dp,
                                  h264_decoder*				dec_params,
                                  int						type)
{
#define BIT_SET(x,n)  ((int)(((x)&((int64)1<<(n)))>>(n)))
    ImageParameters *img = dec_params->img;
    int y_ac        = (type==LUMA_16AC || type==LUMA_8x8 || type==LUMA_8x4 || type==LUMA_4x8 || type==LUMA_4x4);
    int y_dc        = (type==LUMA_16DC);
    int u_ac;//        = (type==CHROMA_AC && !img->is_v_block);
    int v_ac;//        = (type==CHROMA_AC &&  img->is_v_block);
    int chroma_dc   = (type==CHROMA_DC || type==CHROMA_DC_2x4 || type==CHROMA_DC_4x4);
    int u_dc;//        = (chroma_dc && !img->is_v_block);
    int v_dc;//        = (chroma_dc &&  img->is_v_block);
    int j;//           = (y_ac || u_ac || v_ac ? img->subblock_y : 0);
    int i;//           = (y_ac || u_ac || v_ac ? img->subblock_x : 0);
    int bit         = (y_dc ? 0 : y_ac ? 1 : u_dc ? 17 : v_dc ? 18 : u_ac ? 19 : 35);
    int default_bit;// = (img->is_intra_block ? 1 : 0);
    int upper_bit   = default_bit;
    int left_bit    = default_bit;
    int cbp_bit     = 1;  // always one for 8x8 mode
    int ctx;
    int bit_pos_a   = 0;
    int bit_pos_b   = 0;
    
    PixelPos block_a, block_b;
    if (y_ac || y_dc)
    {
        getLuma4x4Neighbour(i, j, -1,  0, &block_a, dec_params);
        getLuma4x4Neighbour(i, j,  0, -1, &block_b, dec_params);
        if (y_ac)
        {
            if (block_a.available)
                bit_pos_a = 4*block_a.y + block_a.x;
            if (block_b.available)
                bit_pos_b = 4*block_b.y + block_b.x;
        }
    }
    else
    {
        getChroma4x4Neighbour(img->current_mb_nr, i, j, -1,  0, &block_a, dec_params);
        getChroma4x4Neighbour(img->current_mb_nr, i, j,  0, -1, &block_b, dec_params);
        if (u_ac||v_ac)
        {
            if (block_a.available)
                bit_pos_a = 4*block_a.y + block_a.x;
            if (block_b.available)
                bit_pos_b = 4*block_b.y + block_b.x;
        }
    }
    
    if (type!=LUMA_8x8)
    {
        //--- get bits from neighbouring blocks ---
        if (block_b.available)
        {
            if(img->mb_data[block_b.mb_addr].mb_type==IPCM)
                upper_bit=1;
            else
                upper_bit = BIT_SET(img->mb_data[block_b.mb_addr].cbp_bits,bit+bit_pos_b);
        }
        
        
        if (block_a.available)
        {
            if(img->mb_data[block_a.mb_addr].mb_type==IPCM)
                left_bit=1;
            else
                left_bit = BIT_SET(img->mb_data[block_a.mb_addr].cbp_bits,bit+bit_pos_a);
        }
        
        
        ctx = 2*upper_bit+left_bit;
        
        
        //===== encode symbol =====
        cbp_bit = biari_decode_symbol (dep_dp, dec_params->tex_ctx->bcbp_contexts[type2ctx_bcbp[type]] + ctx);
    }
    
    //--- set bits for current block ---
    bit         = (y_dc ? 0 : y_ac ? 1+4*j+i : u_dc ? 17 : v_dc ? 18 : u_ac ? 19+4*j+i : 35+4*j+i);
    
    if (cbp_bit)
    {
        if (type==LUMA_8x8)
        {
            currMB->cbp_bits   |= (1<< bit   );
            currMB->cbp_bits   |= (1<<(bit+1));
            currMB->cbp_bits   |= (1<<(bit+4));
            currMB->cbp_bits   |= (1<<(bit+5));
        }
        else if (type==LUMA_8x4)
        {
            currMB->cbp_bits   |= (1<< bit   );
            currMB->cbp_bits   |= (1<<(bit+1));
        }
        else if (type==LUMA_4x8)
        {
            currMB->cbp_bits   |= (1<< bit   );
            currMB->cbp_bits   |= (1<<(bit+4));
        }
        else
        {
            currMB->cbp_bits   |= ((int64)1<<bit);
        }
    }
    
    return cbp_bit;
}





//===== position -> ctx for MAP =====
//--- zig-zag scan ----
static const int  pos2ctx_map8x8 [] = { 0,  1,  2,  3,  4,  5,  5,  4,  4,  3,  3,  4,  4,  4,  5,  5,
4,  4,  4,  4,  3,  3,  6,  7,  7,  7,  8,  9, 10,  9,  8,  7,
7,  6, 11, 12, 13, 11,  6,  7,  8,  9, 14, 10,  9,  8,  6, 11,
12, 13, 11,  6,  9, 14, 10,  9, 11, 12, 13, 11 ,14, 10, 12, 14}; // 15 CTX
static const int  pos2ctx_map8x4 [] = { 0,  1,  2,  3,  4,  5,  7,  8,  9, 10, 11,  9,  8,  6,  7,  8,
9, 10, 11,  9,  8,  6, 12,  8,  9, 10, 11,  9, 13, 13, 14, 14}; // 15 CTX
static const int  pos2ctx_map4x4 [] = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 14}; // 15 CTX
static const int  pos2ctx_map2x4c[] = { 0,  0,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2}; // 15 CTX
static const int  pos2ctx_map4x4c[] = { 0,  0,  0,  0,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2}; // 15 CTX
static const int* pos2ctx_map    [] = {pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map8x8, pos2ctx_map8x4,
pos2ctx_map8x4, pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map4x4,
pos2ctx_map2x4c, pos2ctx_map4x4c};
//--- interlace scan ----
//taken from ABT
static const int  pos2ctx_map8x8i[] = { 0,  1,  1,  2,  2,  3,  3,  4,  5,  6,  7,  7,  7,  8,  4,  5,
6,  9, 10, 10,  8, 11, 12, 11,  9,  9, 10, 10,  8, 11, 12, 11,
9,  9, 10, 10,  8, 11, 12, 11,  9,  9, 10, 10,  8, 13, 13,  9,
9, 10, 10,  8, 13, 13,  9,  9, 10, 10, 14, 14, 14, 14, 14, 14}; // 15 CTX
static const int  pos2ctx_map8x4i[] = { 0,  1,  2,  3,  4,  5,  6,  3,  4,  5,  6,  3,  4,  7,  6,  8,
9,  7,  6,  8,  9, 10, 11, 12, 12, 10, 11, 13, 13, 14, 14, 14}; // 15 CTX
static const int  pos2ctx_map4x8i[] = { 0,  1,  1,  1,  2,  3,  3,  4,  4,  4,  5,  6,  2,  7,  7,  8,
8,  8,  5,  6,  9, 10, 10, 11, 11, 11, 12, 13, 13, 14, 14, 14}; // 15 CTX
static const int* pos2ctx_map_int[] = {pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map8x8i,pos2ctx_map8x4i,
pos2ctx_map4x8i,pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map4x4,
pos2ctx_map2x4c, pos2ctx_map4x4c};


//===== position -> ctx for LAST =====
static const int  pos2ctx_last8x8 [] = { 0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  4,  4,
5,  5,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  8}; //  9 CTX
static const int  pos2ctx_last8x4 [] = { 0,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,
3,  3,  3,  3,  4,  4,  4,  4,  5,  5,  6,  6,  7,  7,  8,  8}; //  9 CTX

static const int  pos2ctx_last4x4 [] = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15}; // 15 CTX
static const int  pos2ctx_last2x4c[] = { 0,  0,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2}; // 15 CTX
static const int  pos2ctx_last4x4c[] = { 0,  0,  0,  0,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2}; // 15 CTX
static const int* pos2ctx_last    [] = {pos2ctx_last4x4, pos2ctx_last4x4, pos2ctx_last8x8, pos2ctx_last8x4,
pos2ctx_last8x4, pos2ctx_last4x4, pos2ctx_last4x4, pos2ctx_last4x4,
pos2ctx_last2x4c, pos2ctx_last4x4c};





/*!
************************************************************************
* \brief
*    Read Significance MAP
************************************************************************
*/
int read_significance_map (Macroblock              *currMB,
                           DecodingEnvironmentPtr  dep_dp,
                           h264_decoder *dec_params,
                           int                     type,
                           int                     coeff[])
{
    ImageParameters          *img = dec_params->img;
    int   i, sig;
    int   coeff_ctr = 0;
    int   i0        = 0;
    int   i1        = maxpos[type]-1;    
    //int               fld       = ( img->structure!=FRAME || currMB->mb_field );
    int               fld       = 0;
    //BiContextTypePtr  map_ctx   = ( fld ? dec_params->tex_ctx-> fld_map_contexts[type2ctx_map [type]]
    //: dec_params->tex_ctx->     map_contexts[type2ctx_map [type]] );
    //BiContextTypePtr  last_ctx  = ( fld ? dec_params->tex_ctx->fld_last_contexts[type2ctx_last[type]]
    //: dec_params->tex_ctx->    last_contexts[type2ctx_last[type]] );
    
    BiContextTypePtr  map_ctx   = dec_params->tex_ctx->     map_contexts[type2ctx_map [type]];
    BiContextTypePtr  last_ctx  = dec_params->tex_ctx->     last_contexts[type2ctx_last[type]];
    
    if (!c1isdc[type])
    {
        i0++; i1++; coeff--;
    }
    
    for (i=i0; i<i1; i++) // if last coeff is reached, it has to be significant
    {
        //--- read significance symbol ---
        //if (img->structure!=FRAME || currMB->mb_field)
        //  sig = biari_decode_symbol   (dep_dp, map_ctx + pos2ctx_map_int [type][i]);
        //else
        sig = biari_decode_symbol   (dep_dp, map_ctx + pos2ctx_map     [type][i]);
        if (sig)
        {
            coeff[i] = 1;
            coeff_ctr++;
            //--- read last coefficient symbol ---
            if (biari_decode_symbol (dep_dp, last_ctx + pos2ctx_last[type][i]))
            {
                for (i++; i<i1+1; i++) coeff[i] = 0;
            }
        }
        else
        {
            coeff[i] = 0;
        }
    }
    //--- last coefficient must be significant if no last symbol was received ---
    if (i<i1+1)
    {
        coeff[i] = 1;
        coeff_ctr++;
    }    
    return coeff_ctr;
}



/*!
************************************************************************
* \brief
*    Read Levels
************************************************************************
*/
void read_significant_coefficients (Macroblock              *currMB,
                                    DecodingEnvironmentPtr  dep_dp,
                                    h264_decoder *dec_params,
                                    int                     type,
                                    int                     coeff[])
{
    ImageParameters          *img = dec_params->img;
    int   i, ctx;
    int   c1 = 1;
    int   c2 = 0;
    
    for (i=maxpos[type]-1; i>=0; i--)
    {
        if (coeff[i]!=0)
        {
            ctx = min (c1,4);
            coeff[i] += biari_decode_symbol (dep_dp, dec_params->tex_ctx->one_contexts[type2ctx_one[type]] + ctx);
            if (coeff[i]==2)
            {
                ctx = min (c2, max_c2[type]);
                coeff[i] += unary_exp_golomb_level_decode (dep_dp, dec_params->tex_ctx->abs_contexts[type2ctx_abs[type]]+ctx);
                c1=0;
                c2++;
            }
            else if (c1)
            {
                c1++;
            }
            if (biari_decode_symbol_eq_prob(dep_dp))
            {
                coeff[i] *= -1;
            }
        }
    }
}


/*!
************************************************************************
* \brief
*    Read Block-Transform Coefficients
************************************************************************
*/
void readRunLevel_CABAC(SyntaxElement *se, h264_decoder* dec_params, DecodingEnvironmentPtr dep_dp)
{
    ImageParameters *img = dec_params->img;
    static int  coeff[64]; // one more for EOB
    static int  coeff_ctr = -1;
    static int  pos       =  0;
    
    Macroblock *currMB = &img->mb_data[img->current_mb_nr];
    
    //--- read coefficients for whole block ---
    if (coeff_ctr < 0)
    {
        //===== decode CBP-BIT =====
        if ((coeff_ctr = read_and_store_CBP_block_bit (currMB, dep_dp, dec_params, se->context)))
        {
            //===== decode significance map =====
            coeff_ctr = read_significance_map (currMB, dep_dp, dec_params, se->context, coeff);
            
            //===== decode significant coefficients =====
            read_significant_coefficients     (currMB, dep_dp, dec_params, se->context, coeff);
        }
    }
    
    //--- set run and level ---
    if (coeff_ctr)
    {
        //--- set run and level (coefficient) ---
        for (se->value2=0; coeff[pos]==0; pos++, se->value2++);
        se->value1=coeff[pos++];
    }
    else
    {
        //--- set run and level (EOB) ---
        se->value1 = se->value2 = 0;
    }
    //--- decrement coefficient counter and re-set position ---
    if (coeff_ctr-- == 0) pos=0;
    
#if TRACE
    fprintf(p_trace, "@%d %s\t\t\t%d\t%d\n",symbolCount++, se->tracestring, se->value1,se->value2);
    fflush(p_trace);
#endif
}



/*!
************************************************************************
* \brief
*    arithmetic decoding
************************************************************************
*/
int readSyntaxElement_CABAC(SyntaxElement *se, DataPartition *this_dataPart )
{
    int curr_len;
    DecodingEnvironmentPtr dep_dp = &(this_dataPart->de_cabac);
/*    
    curr_len = arideco_bits_read(dep_dp);
    
    // perform the actual decoding by calling the appropriate method
    se->reading(se, dec_params, dep_dp, dec_outputs);
*/    
    return (se->len = (arideco_bits_read(dep_dp) - curr_len));
}


/*!
************************************************************************
* \brief
*    decoding of unary binarization using one or 2 distinct
*    models for the first and all remaining bins; no terminating
*    "0" for max_symbol
***********************************************************************
*/
unsigned int unary_bin_max_decode(DecodingEnvironmentPtr dep_dp,
                                  BiContextTypePtr ctx,
                                  int ctx_offset,
                                  unsigned int max_symbol)
{
    unsigned int l;
    unsigned int symbol;
    BiContextTypePtr ictx;
    
    symbol =  biari_decode_symbol(dep_dp, ctx );
    
    if (symbol==0)
        return 0;
    else
    {
        if (max_symbol == 1)
            return symbol;
        symbol=0;
        ictx=ctx+ctx_offset;
        do
        {
            l=biari_decode_symbol(dep_dp, ictx);
            symbol++;
        }
        while( (l!=0) && (symbol<max_symbol-1) );
        if ((l!=0) && (symbol==max_symbol-1))
            symbol++;
        return symbol;
    }
}


/*!
************************************************************************
* \brief
*    decoding of unary binarization using one or 2 distinct
*    models for the first and all remaining bins
***********************************************************************
*/
unsigned int unary_bin_decode(DecodingEnvironmentPtr dep_dp,
                              BiContextTypePtr ctx,
                              int ctx_offset)
{
    unsigned int l;
    unsigned int symbol;
    BiContextTypePtr ictx;
    
    symbol = biari_decode_symbol(dep_dp, ctx );
    
    if (symbol==0)
        return 0;
    else
    {
        symbol=0;
        ictx=ctx+ctx_offset;
        do
        {
            l=biari_decode_symbol(dep_dp, ictx);
            symbol++;
        }
        while( l!=0 );
        return symbol;
    }
}


/*!
************************************************************************
* \brief
*    finding end of a slice in case this is not the end of a frame
*
* Unsure whether the "correction" below actually solves an off-by-one
* problem or whether it introduces one in some cases :-(  Anyway,
* with this change the bit stream format works with CABAC again.
* StW, 8.7.02
************************************************************************
*/
int cabac_startcode_follows(int eos_bit, h264_decoder* dec_params)
{
    ImageParameters* img = dec_params->img;
    Slice         *currSlice  = img->currentSlice;
    //int           *partMap    = assignSE2partition[currSlice->dp_mode];
    int           *partMap    = assignSE2partition[PAR_DP_1];
    DataPartition *dP;
    unsigned int  bit;
    DecodingEnvironmentPtr dep_dp;
    
    dP = &(currSlice->partArr[partMap[SE_MBTYPE]]);
    dep_dp = &(dP->de_cabac);
    
    if( eos_bit )
    {
        bit = biari_decode_final (dep_dp); //GB
        
#if TRACE
        //  strncpy(se->tracestring, "Decode Sliceterm", TRACESTRING_SIZE);
        fprintf(p_trace, "@%d %s\t\t%d\n",symbolCount++, "Decode Sliceterm", bit);
        fflush(p_trace);
#endif
    }
    else
    {
        bit = 0;
    }
    
    return (bit==1?1:0);
}





/*!
************************************************************************
* \brief
*    Exp Golomb binarization and decoding of a symbol
*    with prob. of 0.5
************************************************************************
*/
unsigned int exp_golomb_decode_eq_prob( DecodingEnvironmentPtr dep_dp,
                                       int k)
{
    unsigned int l;
    int symbol = 0;
    int binary_symbol = 0;
    
    do
    {
        l=biari_decode_symbol_eq_prob(dep_dp);
        if (l==1) 
        {
            symbol += (1<<k); 
            k++;
        }
    }
    while (l!=0);
    
    while (k--)                             //next binary part
        if (biari_decode_symbol_eq_prob(dep_dp)==1) 
            binary_symbol |= (1<<k);
        
        return (unsigned int) (symbol+binary_symbol);
}


/*!
************************************************************************
* \brief
*    Exp-Golomb decoding for LEVELS
***********************************************************************
*/
unsigned int unary_exp_golomb_level_decode( DecodingEnvironmentPtr dep_dp,
                                           BiContextTypePtr ctx)
{
    unsigned int l,k;
    unsigned int symbol;
    unsigned int exp_start = 13;
    
    symbol = biari_decode_symbol(dep_dp, ctx );
    
    if (symbol==0)
        return 0;
    else
    {
        symbol=0;
        k=1;
        do
        {
            l=biari_decode_symbol(dep_dp, ctx);
            symbol++;
            k++;
        }
        while((l!=0) && (k!=exp_start));
        if (l!=0)
            symbol += exp_golomb_decode_eq_prob(dep_dp,0)+1;
        return symbol;
    }
}




/*!
************************************************************************
* \brief
*    Exp-Golomb decoding for Motion Vectors
***********************************************************************
*/
unsigned int unary_exp_golomb_mv_decode(DecodingEnvironmentPtr dep_dp,
                                        BiContextTypePtr ctx,
                                        unsigned int max_bin)
{
    unsigned int l,k;
    unsigned int bin=1;
    unsigned int symbol;
    unsigned int exp_start = 8;
    
    BiContextTypePtr ictx=ctx;
    
    symbol = biari_decode_symbol(dep_dp, ictx );
    
    if (symbol==0)
        return 0;
    else
    {
        symbol=0;
        k=1;
        
        ictx++;
        do
        {
            l=biari_decode_symbol(dep_dp, ictx  );
            if ((++bin)==2) ictx++;
            if (bin==max_bin) ictx++;
            symbol++;
            k++;
        }
        while((l!=0) && (k!=exp_start));
        if (l!=0)
            symbol += exp_golomb_decode_eq_prob(dep_dp,3)+1;
        return symbol;
    }
}


/*!
************************************************************************
* \brief
*    Read one byte from CABAC-partition. 
*    Bitstream->read_len will be modified
*    (for IPCM CABAC  28/11/2003)
*
* \author
*    Dong Wang <Dong.Wang@bristol.ac.uk>  
************************************************************************
*/
void readIPCMBytes_CABAC(SyntaxElement *sym, Bitstream *currStream)
{
    int read_len = currStream->read_len;
    int code_len = currStream->code_len;
    byte *buf = currStream->streamBuffer;
    
    sym->len=8;
    
    if(read_len<code_len)
        sym->inf=buf[read_len++];
    
    sym->value1=sym->inf;
    
    currStream->read_len=read_len;
    
#if TRACE
    tracebits2(sym->tracestring, sym->len, sym->inf);
#endif
    
}

void readMotionInfoFromNAL_CABAC(h264_decoder* dec_params)
{
#if 0
    ImageParameters* img = dec_params->img;
    //InputParameters* inp = dec_params->input;
    StorablePicture *dec_picture = dec_outputs->dec_picture;
    int i,j,k;
    int step_h,step_v;
    int curr_mvd;
    Macroblock *currMB  = &img->mb_data[img->current_mb_nr];
    SyntaxElement currSE;
    Slice *currSlice    = img->currentSlice;
    DataPartition *dP;
    int *partMap        = assignSE2partition[PAR_DP_1];
	
    int bframe          = (img->type==B_SLICE);
    int partmode        = (IS_P8x8(currMB)?4:currMB->mb_type);
    int step_h0         = BLOCK_STEP [partmode][0];
    int step_v0         = BLOCK_STEP [partmode][1];
    
	int *ref_pic_num_L0 = dec_picture->ref_pic_num[img->current_slice_nr][LIST_0];
	int *ref_pic_num_L1 = dec_picture->ref_pic_num[img->current_slice_nr][LIST_1];
	
    int mv_mode, i0, j0;
    char refframe;
    short pmv[2];
    int j4, i4, ii,jj;
    int vec;
    int mv_scale = 0;
    int flag_mode;
    unsigned int mvWidth = img->width>>1;
    
    int list_offset = 0;
    
	short* ref_pic_idL0 = dec_outputs->dec_picture->ref_pic_idL0;
	short* ref_pic_idL1 = dec_outputs->dec_picture->ref_pic_idL1;

	short* curr_ref_pic_idL0;
	short* curr_ref_pic_idL1;
    
    byte  **    moving_block;
	short ****  co_located_mv;
	char  ***   co_located_ref_idx;
	int ***   co_located_ref_id;

		//faisal ref_idx
	char *curr_colocated_ref_idxl0 = dec_params->Co_located->ref_idx_l0;
	char *curr_colocated_ref_idxl1 = dec_params->Co_located->ref_idx_l1;
	char *curr_ref_idxl0 = dec_outputs->dec_picture->ref_idx_l0;
	char *curr_ref_idxl1 = dec_outputs->dec_picture->ref_idx_l1;
	char dummy_reframe;

	short *   co_located_ref_idL0;
	short *   co_located_ref_idL1;
    
    currSE.type = 0;
    currSE.value1 = 0;
    currSE.value2 = 0;
    currSE.len = 0;
    currSE.inf = 0;
    currSE.bitpattern = 0;
    currSE.context = 0;
    currSE.k = 0;
    currSE.mapping = NULL;
    currSE.reading = NULL;


	moving_block = dec_params->Co_located->moving_block;
	co_located_mv = dec_params->Co_located->mv;
	co_located_ref_idx = dec_params->Co_located->ref_idx;
	co_located_ref_id = dec_params->Co_located->ref_pic_id;

	co_located_ref_idL0 = dec_params->Co_located->ref_pic_idL0;
	co_located_ref_idL1 = dec_params->Co_located->ref_pic_idL1;


	
	if (bframe && IS_P8x8 (currMB))
	{
		if (img->direct_spatial_mv_pred_flag)
		{
		
			//int imgblock_y= ((img->MbaffFrameFlag)&&(currMB->mb_field))? (img->current_mb_nr%2) ? (img->block_y-4)/2:img->block_y/2: img->block_y;
			int imgblock_y=  img->block_y;
			int fw_rFrameL, fw_rFrameU, fw_rFrameUL, fw_rFrameUR;
			int bw_rFrameL, bw_rFrameU, bw_rFrameUL, bw_rFrameUR;
			
			PixelPos mb_left, mb_up, mb_upleft, mb_upright;
			
			char  fw_rFrame,bw_rFrame;
			short pmvfw[2]={0,0},
				pmvbw[2]={0,0};
			int posx,posy;
			int mb_nr = img->current_mb_nr;
			
			// faisal ref_idx
			int dummy_fw_rFrameL, dummy_fw_rFrameU, dummy_fw_rFrameUL, dummy_fw_rFrameUR;
			int dummy_bw_rFrameL, dummy_bw_rFrameU, dummy_bw_rFrameUL, dummy_bw_rFrameUR;
			int mb_nr_l,mb_nr_u,mb_nr_ul,mb_nr_ur;
			int l_add,u_add,ul_add,ur_add;
			
			
			/*getLuma4x4Neighbour( 0, 0, -1,  0, &mb_left,dec_params,dec_outputs);
			getLuma4x4Neighbour( 0, 0,  0, -1, &mb_up,dec_params,dec_outputs);
			getLuma4x4Neighbour( 0, 0, 16, -1, &mb_upright,dec_params,dec_outputs);
			getLuma4x4Neighbour( 0, 0, -1, -1, &mb_upleft,dec_params,dec_outputs);*/

			posx = ((mb_nr % img->FrameWidthInMbs)<<2);
			posy = ((mb_nr / img->FrameWidthInMbs)<<2);

			if(currMB->mbAvailA)
			{
				mb_left.available = 1;
				mb_left.pos_x = posx-1;
				mb_left.pos_y = posy;
			}
			else
			{
				mb_left.available = 0;
			}
			if(currMB->mbAvailB)
			{
				mb_up.available =1;
				// position of block B
				mb_up.pos_x = posx;
				mb_up.pos_y = posy-1;
			}	
			else
			{
				mb_up.available = 0;
			}


			if(currMB->mbAvailC)
			{
				mb_upright.available =1;
				mb_upright.pos_x = ((currMB->mbAddrC % img->FrameWidthInMbs) << 2);
				//mb_upright.pos_y = ((((currMB->mbAddrC / img->FrameWidthInMbs) << 4) +15 ) >> 2) ;
				mb_upright.pos_y = ((currMB->mbAddrC / img->FrameWidthInMbs) << 2) + 3 ;
			}
			else
			{
				mb_upright.available = 0;
			}


			if(currMB->mbAvailD)
			{
				mb_upleft.available =1;
				// position of block B
				mb_upleft.pos_x = posx-1;
				mb_upleft.pos_y = posy-1;
			}
			else
			{
				mb_upleft.available = 0;
			}



		
	//		if (!img->MbaffFrameFlag)
	//		{
				/*
				fw_rFrameL = mb_left.available ? dec_outputs->dec_picture->ref_idx[LIST_0][mb_left.pos_y][mb_left.pos_x] : -1;
				fw_rFrameU = mb_up.available ? dec_outputs->dec_picture->ref_idx[LIST_0][mb_up.pos_y][mb_up.pos_x] : -1;
				fw_rFrameUL = mb_upleft.available ? dec_outputs->dec_picture->ref_idx[LIST_0][mb_upleft.pos_y][mb_upleft.pos_x] : -1;
				fw_rFrameUR = mb_upright.available ? dec_outputs->dec_picture->ref_idx[LIST_0][mb_upright.pos_y][mb_upright.pos_x] : fw_rFrameUL;      
				
				bw_rFrameL = mb_left.available ? dec_outputs->dec_picture->ref_idx[LIST_1][mb_left.pos_y][mb_left.pos_x] : -1;
				bw_rFrameU = mb_up.available ? dec_outputs->dec_picture->ref_idx[LIST_1][mb_up.pos_y][mb_up.pos_x] : -1;
				bw_rFrameUL = mb_upleft.available ? dec_outputs->dec_picture->ref_idx[LIST_1][mb_upleft.pos_y][mb_upleft.pos_x] : -1;
				bw_rFrameUR = mb_upright.available ? dec_outputs->dec_picture->ref_idx[LIST_1][mb_upright.pos_y][mb_upright.pos_x] : bw_rFrameUL;      
				*/

				
				//faisal ref_idx
				mb_nr_l = ((mb_left.pos_y>>2) * img->FrameWidthInMbs) + (mb_left.pos_x>>2);
				mb_nr_u = ((mb_up.pos_y>>2) * img->FrameWidthInMbs) + (mb_up.pos_x>>2);
				mb_nr_ul = ((mb_upleft.pos_y>>2) * img->FrameWidthInMbs) + (mb_upleft.pos_x>>2);
				mb_nr_ur = ((mb_upright.pos_y>>2) * img->FrameWidthInMbs) + (mb_upright.pos_x>>2);
				
				l_add = ((mb_left.pos_y%4) * 4) + (mb_left.pos_x%4);
				u_add = ((mb_up.pos_y%4) * 4) + (mb_up.pos_x%4);
				ul_add = ((mb_upleft.pos_y%4) * 4) + (mb_upleft.pos_x%4);
				ur_add = ((mb_upright.pos_y%4) * 4) + (mb_upright.pos_x%4);

				fw_rFrameL = mb_left.available ? *(curr_ref_idxl0 + (16*mb_nr_l) + l_add): -1;
				fw_rFrameU = mb_up.available ?   *(curr_ref_idxl0 + (16*mb_nr_u) + u_add): -1;
				fw_rFrameUL = mb_upleft.available ? *(curr_ref_idxl0 + (16*mb_nr_ul) + ul_add) : -1;
				fw_rFrameUR = mb_upright.available ? *(curr_ref_idxl0 + (16*mb_nr_ur) + ur_add) : fw_rFrameUL;      
				
				bw_rFrameL = mb_left.available ? *(curr_ref_idxl1 + (16*mb_nr_l) + l_add): -1;
				bw_rFrameU = mb_up.available ?   *(curr_ref_idxl1 + (16*mb_nr_u) + u_add): -1;
				bw_rFrameUL = mb_upleft.available ? *(curr_ref_idxl1 + (16*mb_nr_ul) + ul_add) : -1;
				bw_rFrameUR = mb_upright.available ? *(curr_ref_idxl1 + (16*mb_nr_ur) + ur_add) : bw_rFrameUL;  
	//		}
		
	/*		else
			{
				if (img->mb_data[img->current_mb_nr].mb_field)
				{
					fw_rFrameL = mb_left.available ? 
						img->mb_data[mb_left.mb_addr].mb_field  || dec_outputs->dec_picture->ref_idx[LIST_0][mb_left.pos_y][mb_left.pos_x] < 0? 
						dec_outputs->dec_picture->ref_idx[LIST_0][mb_left.pos_y][mb_left.pos_x] : 
					dec_outputs->dec_picture->ref_idx[LIST_0][mb_left.pos_y][mb_left.pos_x] * 2: -1;
					fw_rFrameU = mb_up.available ? 
						img->mb_data[mb_up.mb_addr].mb_field || dec_outputs->dec_picture->ref_idx[LIST_0][mb_up.pos_y][mb_up.pos_x] < 0? 
						dec_outputs->dec_picture->ref_idx[LIST_0][mb_up.pos_y][mb_up.pos_x] : 
					dec_outputs->dec_picture->ref_idx[LIST_0][mb_up.pos_y][mb_up.pos_x] * 2: -1;
					
					fw_rFrameUL = mb_upleft.available ? 
						img->mb_data[mb_upleft.mb_addr].mb_field || dec_outputs->dec_picture->ref_idx[LIST_0][mb_upleft.pos_y][mb_upleft.pos_x] < 0?
						dec_outputs->dec_picture->ref_idx[LIST_0][mb_upleft.pos_y][mb_upleft.pos_x] : 
					dec_outputs->dec_picture->ref_idx[LIST_0][mb_upleft.pos_y][mb_upleft.pos_x] *2: -1;      
					
					fw_rFrameUR = mb_upright.available ? 
						img->mb_data[mb_upright.mb_addr].mb_field || dec_outputs->dec_picture->ref_idx[LIST_0][mb_upright.pos_y][mb_upright.pos_x] < 0 ?
						dec_outputs->dec_picture->ref_idx[LIST_0][mb_upright.pos_y][mb_upright.pos_x] : 
					dec_outputs->dec_picture->ref_idx[LIST_0][mb_upright.pos_y][mb_upright.pos_x] * 2: fw_rFrameUL;      
					
					bw_rFrameL = mb_left.available ? 
						img->mb_data[mb_left.mb_addr].mb_field || dec_outputs->dec_picture->ref_idx[LIST_1][mb_left.pos_y][mb_left.pos_x]  < 0 ?
						dec_outputs->dec_picture->ref_idx[LIST_1][mb_left.pos_y][mb_left.pos_x] : 
					dec_outputs->dec_picture->ref_idx[LIST_1][mb_left.pos_y][mb_left.pos_x] * 2: -1;
					
					bw_rFrameU = mb_up.available ? 
						img->mb_data[mb_up.mb_addr].mb_field || dec_outputs->dec_picture->ref_idx[LIST_1][mb_up.pos_y][mb_up.pos_x]  < 0 ?
						dec_outputs->dec_picture->ref_idx[LIST_1][mb_up.pos_y][mb_up.pos_x] : 
					dec_outputs->dec_picture->ref_idx[LIST_1][mb_up.pos_y][mb_up.pos_x] * 2: -1;
					
					bw_rFrameUL = mb_upleft.available ? 
						img->mb_data[mb_upleft.mb_addr].mb_field || dec_outputs->dec_picture->ref_idx[LIST_1][mb_upleft.pos_y][mb_upleft.pos_x]  < 0 ?
						dec_outputs->dec_picture->ref_idx[LIST_1][mb_upleft.pos_y][mb_upleft.pos_x] : 
					dec_outputs->dec_picture->ref_idx[LIST_1][mb_upleft.pos_y][mb_upleft.pos_x] *2: -1;      
					
					bw_rFrameUR = mb_upright.available ? 
						img->mb_data[mb_upright.mb_addr].mb_field || dec_outputs->dec_picture->ref_idx[LIST_1][mb_upright.pos_y][mb_upright.pos_x]  < 0 ?
						dec_outputs->dec_picture->ref_idx[LIST_1][mb_upright.pos_y][mb_upright.pos_x] : 
					dec_outputs->dec_picture->ref_idx[LIST_1][mb_upright.pos_y][mb_upright.pos_x] * 2: bw_rFrameUL;      
					
				}
				else
				{
					fw_rFrameL =
						mb_left.available ? 
							img->mb_data[mb_left.mb_addr].mb_field || dec_outputs->dec_picture->ref_idx[LIST_0][mb_left.pos_y][mb_left.pos_x]  < 0 ?
								dec_outputs->dec_picture->ref_idx[LIST_0][mb_left.pos_y][mb_left.pos_x] >> 1 : 
								dec_outputs->dec_picture->ref_idx[LIST_0][mb_left.pos_y][mb_left.pos_x]
							: -1;
					
					fw_rFrameU = mb_up.available ? 
						img->mb_data[mb_up.mb_addr].mb_field || dec_outputs->dec_picture->ref_idx[LIST_0][mb_up.pos_y][mb_up.pos_x]  < 0 ?
						dec_outputs->dec_picture->ref_idx[LIST_0][mb_up.pos_y][mb_up.pos_x] >> 1 :  
					dec_outputs->dec_picture->ref_idx[LIST_0][mb_up.pos_y][mb_up.pos_x] : -1;
					
					fw_rFrameUL = mb_upleft.available ? 
						img->mb_data[mb_upleft.mb_addr].mb_field || dec_outputs->dec_picture->ref_idx[LIST_0][mb_upleft.pos_y][mb_upleft.pos_x] < 0 ?
						dec_outputs->dec_picture->ref_idx[LIST_0][mb_upleft.pos_y][mb_upleft.pos_x]>> 1 : 
					dec_outputs->dec_picture->ref_idx[LIST_0][mb_upleft.pos_y][mb_upleft.pos_x] : -1;      
					
					fw_rFrameUR = mb_upright.available ? 
						img->mb_data[mb_upright.mb_addr].mb_field || dec_outputs->dec_picture->ref_idx[LIST_0][mb_upright.pos_y][mb_upright.pos_x]  < 0 ?
						dec_outputs->dec_picture->ref_idx[LIST_0][mb_upright.pos_y][mb_upright.pos_x] >> 1 :  
					dec_outputs->dec_picture->ref_idx[LIST_0][mb_upright.pos_y][mb_upright.pos_x] : fw_rFrameUL;      
					
					bw_rFrameL = mb_left.available ? 
						img->mb_data[mb_left.mb_addr].mb_field || dec_outputs->dec_picture->ref_idx[LIST_1][mb_left.pos_y][mb_left.pos_x] < 0 ?
						dec_outputs->dec_picture->ref_idx[LIST_1][mb_left.pos_y][mb_left.pos_x] >> 1 :  
					dec_outputs->dec_picture->ref_idx[LIST_1][mb_left.pos_y][mb_left.pos_x] : -1;
					bw_rFrameU = mb_up.available ? 
						img->mb_data[mb_up.mb_addr].mb_field || dec_outputs->dec_picture->ref_idx[LIST_1][mb_up.pos_y][mb_up.pos_x] < 0 ?
						dec_outputs->dec_picture->ref_idx[LIST_1][mb_up.pos_y][mb_up.pos_x] >> 1 : 
					dec_outputs->dec_picture->ref_idx[LIST_1][mb_up.pos_y][mb_up.pos_x] : -1;
					
					bw_rFrameUL = mb_upleft.available ? 
						img->mb_data[mb_upleft.mb_addr].mb_field || dec_outputs->dec_picture->ref_idx[LIST_1][mb_upleft.pos_y][mb_upleft.pos_x]  < 0 ?
						dec_outputs->dec_picture->ref_idx[LIST_1][mb_upleft.pos_y][mb_upleft.pos_x] >> 1 : 
					dec_outputs->dec_picture->ref_idx[LIST_1][mb_upleft.pos_y][mb_upleft.pos_x] : -1;      
					
					bw_rFrameUR = mb_upright.available ? 
						img->mb_data[mb_upright.mb_addr].mb_field || dec_outputs->dec_picture->ref_idx[LIST_1][mb_upright.pos_y][mb_upright.pos_x] < 0 ?
						dec_outputs->dec_picture->ref_idx[LIST_1][mb_upright.pos_y][mb_upright.pos_x] >> 1: 
					dec_outputs->dec_picture->ref_idx[LIST_1][mb_upright.pos_y][mb_upright.pos_x] : bw_rFrameUL;      
				}
			}
			*/
			fw_rFrame = (fw_rFrameL >= 0 && fw_rFrameU >= 0) ? min(fw_rFrameL,fw_rFrameU): max(fw_rFrameL,fw_rFrameU);
			fw_rFrame = (fw_rFrame >= 0 && fw_rFrameUR >= 0) ? min(fw_rFrame,fw_rFrameUR): max(fw_rFrame,fw_rFrameUR);
			
			bw_rFrame = (bw_rFrameL >= 0 && bw_rFrameU >= 0) ? min(bw_rFrameL,bw_rFrameU): max(bw_rFrameL,bw_rFrameU);
			bw_rFrame = (bw_rFrame >= 0 && bw_rFrameUR >= 0) ? min(bw_rFrame,bw_rFrameUR): max(bw_rFrame,bw_rFrameUR);
			
			
			if (fw_rFrame >=0)
				//SetMotionVectorPredictor_baseline (pmvfw, pmvfw+1, fw_rFrame, LIST_0, 0, 0, 16, 16,dec_params,dec_outputs);
				SetMotionVectorPredictor_n (pmvfw, pmvfw+1, fw_rFrame, LIST_0, mb_left, mb_up, mb_upright, mb_upleft, dec_params,dec_outputs);
			
			if (bw_rFrame >=0)
				//SetMotionVectorPredictor_baseline(pmvbw, pmvbw+1, bw_rFrame, LIST_1, 0, 0, 16, 16,dec_params,dec_outputs);
				SetMotionVectorPredictor_n (pmvbw, pmvbw+1, bw_rFrame, LIST_1, mb_left, mb_up, mb_upright, mb_upleft, dec_params,dec_outputs);
			
			
			for (i=0;i<4;i++)
			{
				if (currMB->b8mode[i] == 0)
					// for(j=2*(i/2);j<2*(i/2)+2;j++)
					for(j=(i&2);j<(i&2)+2;j++)
						//for(k=2*(i%2);k<2*(i%2)+2;k++)
						for(k=(i&1)<<1;k<((i&1)<<1)+2;k++)
						{
							int j6 = imgblock_y+j;
							int stride_add;
							
							j4 = (img->block_y+j);
							i4 = (img->block_x+k);
							
							stride_add = ((j4%4) * 4) + (i4%4);
							
							if (fw_rFrame >= 0)
							{
							
								if  (!fw_rFrame  && ((!moving_block[j6][i4]) && (!dec_params->listX[1][0]->is_long_term)))
								{                    
									*(dec_outputs->dec_picture->mvL0+j4*mvWidth+(i4<<1)) = 0;
									*(dec_outputs->dec_picture->mvL0+j4*mvWidth+(i4<<1)+1) = 0;
									//dec_outputs->dec_picture->ref_idx[LIST_0][j4][i4] = 0;                    
									*(curr_ref_idxl0+ (16*mb_nr) + stride_add) = 0;                    
								}
								else
								{
									
									*(dec_outputs->dec_picture->mvL0+j4*mvWidth+(i4<<1)) = pmvfw[0];
									*(dec_outputs->dec_picture->mvL0+j4*mvWidth+(i4<<1)+1) = pmvfw[1];
									//dec_outputs->dec_picture->ref_idx[LIST_0][j4][i4] = fw_rFrame;
									*(curr_ref_idxl0+ (16*mb_nr) + stride_add) = 0;                    
								}
							}
							else
							{
								*(dec_outputs->dec_picture->mvL0+j4*mvWidth+(i4<<1)) = 0;
								*(dec_outputs->dec_picture->mvL0+j4*mvWidth+(i4<<1)+1) = 0;
								//dec_outputs->dec_picture->ref_idx[LIST_0][j4][i4] = -1;
								//check, already done in init
							}
							if (bw_rFrame >= 0)
							{
									
								if  (bw_rFrame==0 && ((!moving_block[j6][i4])&& (!dec_params->listX[1][0]->is_long_term)))
								{
									*(dec_outputs->dec_picture->mvL1+j4*mvWidth+(i4<<1)) = 0;
								    *(dec_outputs->dec_picture->mvL1+j4*mvWidth+(i4<<1)+1) = 0;
									//dec_outputs->dec_picture->ref_idx[LIST_1][j4][i4] = 0;
									*(curr_ref_idxl1+ (16*mb_nr) + stride_add) = 0;                    
								}
								else
								{
									*(dec_outputs->dec_picture->mvL1+j4*mvWidth+(i4<<1)) = pmvbw[0];
									*(dec_outputs->dec_picture->mvL1+j4*mvWidth+(i4<<1)+1) = pmvbw[1];
									//dec_outputs->dec_picture->ref_idx[LIST_1][j4][i4] = bw_rFrame;
									*(curr_ref_idxl1+ (16*mb_nr) + stride_add) = bw_rFrame;                    
								}
							}
							else
							{
								*(dec_outputs->dec_picture->mvL1+j4*mvWidth+(i4<<1)) = 0;
								*(dec_outputs->dec_picture->mvL1+j4*mvWidth+(i4<<1)+1) = 0;
								//dec_outputs->dec_picture->ref_idx[LIST_1][j4][i4] = -1;                               
								//check, no need to done as already done in init
							}
							
							if (fw_rFrame <0 && bw_rFrame <0)
							{
								//dec_outputs->dec_picture->ref_idx[LIST_0][j4][i4] = 0;
								//dec_outputs->dec_picture->ref_idx[LIST_1][j4][i4] = 0;                  
								*(curr_ref_idxl0+ (16*mb_nr) + stride_add) = 0;                    
								*(curr_ref_idxl1+ (16*mb_nr) + stride_add) = 0;   
							}
						}
			}
    }
    else
    {
		for (i=0;i<4;i++)
		{
			if (currMB->b8mode[i] == 0)
			{
				for(j=2*(i/2);j<2*(i/2)+2;j++)
				{
					for(k=2*(i%2);k<2*(i%2)+2;k++)
					{
					
						//int list_offset = ((img->MbaffFrameFlag)&&(currMB->mb_field))? img->current_mb_nr%2 ? 4 : 2 : 0;

						
						//int imgblock_y= ((img->MbaffFrameFlag)&&(currMB->mb_field))? (img->current_mb_nr%2) ? (img->block_y-4)/2 : img->block_y/2 : img->block_y;
						int imgblock_y= img->block_y;
						int refList;// = co_located_ref_idx[LIST_0 ][imgblock_y+j][img->block_x+k]== -1 ? LIST_1 : LIST_0;
						int ref_idx;// = co_located_ref_idx[refList][imgblock_y + j][img->block_x + k];
						int mapped_idx=-1, iref;                             
						int mb_nr = img->current_mb_nr;

						// faisal ref_idx
						int stride_add;
						int dummy_ref_idx;
						int dummy_refList;

						stride_add = (((imgblock_y+j)%4) * 4) + ((img->block_x+k)%4);
						refList = *(curr_colocated_ref_idxl0+(16*mb_nr)+stride_add)==-1? LIST_1 : LIST_0;

						if(refList == LIST_0)
						{
							ref_idx = *(curr_colocated_ref_idxl0+(16*mb_nr)+stride_add);
						}
						else
						{
							ref_idx = *(curr_colocated_ref_idxl1+(16*mb_nr)+stride_add);
						}
						
						
						if (ref_idx == -1)
						{
							//dec_outputs->dec_picture->ref_idx [LIST_0][img->block_y + j][img->block_x + k] = 0;
							//dec_outputs->dec_picture->ref_idx [LIST_1][img->block_y + j][img->block_x + k] = 0;                
							*(curr_ref_idxl0+(16*mb_nr)+stride_add) = 0;
							*(curr_ref_idxl1+(16*mb_nr)+stride_add) = 0;
						}
						else
						{
						
							for (iref=0;iref<min(img->num_ref_idx_l0_active,dec_params->listXsize[LIST_0]);iref++)
							{
							
								if (dec_outputs->dec_picture->ref_pic_num[img->current_slice_nr][LIST_0][iref]==co_located_ref_id[refList][imgblock_y + j][img->block_x + k])
								{
									mapped_idx=iref;
									break;
								}
								else //! invalid index. Default to zero even though this case should not happen
									mapped_idx=INVALIDINDEX;
							}
							if (INVALIDINDEX == mapped_idx)
							{
								error("temporal direct error\ncolocated block has ref that is unavailable",-1111, dec_params,dec_outputs);
							}
							//dec_outputs->dec_picture->ref_idx [LIST_0][img->block_y + j][img->block_x + k] = mapped_idx;
							//dec_outputs->dec_picture->ref_idx [LIST_1][img->block_y + j][img->block_x + k] = 0;                
							*(curr_ref_idxl0+(16*mb_nr)+stride_add) = mapped_idx;
							*(curr_ref_idxl1+(16*mb_nr)+stride_add) = 0;
						}
					}
				}
			}
		}
    }
  }
    //  If multiple ref. frames, read reference frame for the MB *********************************
	//  If multiple ref. frames for slice, read reference frame for current MB 
    if(img->num_ref_idx_l0_active>1) 
    {
		int stride_add1;
		int mb_nr = img->current_mb_nr;
		
        flag_mode = ( img->num_ref_idx_l0_active == 2 ? 1 : 0);
        
        currSE.type = SE_REFFRAME;
        dP = &(currSlice->partArr[partMap[SE_REFFRAME]]);
        
        //if (dec_params->active_pps->entropy_coding_mode_flag == UVLC || dP->bitstream->ei_flag)
        //{
            //currSE.mapping = linfo_ue;
        //}
        //else
        {
            currSE.reading = readRefFrame_CABAC;
        }
        
        for (j0=0; j0<4; j0+=step_v0)
        {
            for (i0=0; i0<4; i0+=step_h0)
            {
			
				 k=2*(j0>>1)+(i0>>1);
                if ((currMB->b8pdir[k]==0 || currMB->b8pdir[k]==2) && currMB->b8mode[k]!=0)
                {
                    //TRACE_STRING("ref_idx_l0");
				
//                    img->subblock_x = i0;
//                    img->subblock_y = j0;

                    
	  			  	//if (!IS_P8x8 (currMB) || bframe || (!bframe && !img->allrefzero))
					if (!IS_P8x8 (currMB) || bframe || (!bframe ))
                    {
                        if (currMB->b8mode[k]<4)
                        {
                            currSE.context = 0;
                        }
                        else
                        {
                            currSE.context = 1;
                        }
                        
                        //if( (dec_params->active_pps->entropy_coding_mode_flag == UVLC || dP->bitstream->ei_flag) && flag_mode )
                        //{
                            //currSE.len = 1;
                            //readSyntaxElement_FLC(&currSE, dP->bitstream);
                            //currSE.value1 = 1 - currSE.value1;
                        //}
                        //else
                        {
                            currSE.value2 = LIST_0;
                            dP->readSyntaxElement (&currSE,dec_params,dP,dec_outputs);
                        }
                        refframe = currSE.value1;
                        
                    }
                    else
                    {
                        refframe = 0;
                    }
                    
                    
for (j=j0; j<j0+step_v0;j++)
                    {
                        for (i=i0; i<i0+step_h0;i++)
                        {
							stride_add1 = (((img->block_y+j)%4) * 4) + ((img->block_x+i)%4);
                            //dec_picture->ref_idx[LIST_0][img->block_y + j][img->block_x + i] = refframe;
							*(curr_ref_idxl0+ (16*mb_nr) + stride_add1) = refframe;                    
                        }
                    }
                    
                }
            }
        }
    }
    else
    /*{
        for (j0=0; j0<4; j0+=step_v0)
        {
            for (i0=0; i0<4; i0+=step_h0)
            {
                k=2*(j0/2)+(i0/2);
                if ((currMB->b8pdir[k]==0 || currMB->b8pdir[k]==2) && currMB->b8mode[k]!=0)
                {
                    for (j=j0; j<j0+step_v0;j++)
                    {
                        for (i=i0; i<i0+step_h0;i++)
                        {
                            dec_picture->ref_idx[LIST_0][img->block_y + j][img->block_x + i] = 0;
                        }
                    }
                }
            }
        }
    }*/
	{	
		int stride_add2;
		int mb_nr = img->current_mb_nr;
		///ppp pragma frequency hit never
		for (j0=0; j0<4; j0++)
		{
			for (i0=0; i0<4; i0++) 
			{
				
				 k=2*(j0>>1)+(i0>>1);
				 if ((currMB->b8pdir[k]==0 || currMB->b8pdir[k]==2) && currMB->b8mode[k]!=0)
			  	{
					//??? use xt_memset
					//dec_outputs->dec_picture->ref_idx[LIST_0][img->block_y + j0][img->block_x + i0] = 0;
					 stride_add2 = (((img->block_y+j0)%4) * 4) + ((img->block_x+i0)%4);
					*(curr_ref_idxl0+ (16*mb_nr) + stride_add2) = 0;                    
					//dec_outputs->dec_picture->ref_idx[img->block_y + j0][img->block_x + i0] = 0;
				 }
			}
		}
	}

	

	  //  If backward multiple ref. frames, read backward reference frame for the MB *********************************
	if(img->num_ref_idx_l1_active>1)
	{
		int stride_add3;
		int mb_nr = img->current_mb_nr;
		
		flag_mode = ( img->num_ref_idx_l1_active == 2 ? 1 : 0);

		currSE.type = SE_REFFRAME;
		
		dP = &(currSlice->partArr[partMap[SE_REFFRAME]]);
		//dP = &(currSlice->partArr[0]);
		
		//if (dec_params->active_pps->entropy_coding_mode_flag == UVLC || dP->bitstream->ei_flag)   
		        //currSE.mapping = linfo_ue;
		//else                                                      
		  currSE.reading = readRefFrame_CABAC;

		for (j0=0; j0<4; j0+=step_v0)
		{
		  	for (i0=0; i0<4; i0+=step_h0)
			{
				k=2*(j0>>1)+(i0>>1);
				if ((currMB->b8pdir[k]==1 || currMB->b8pdir[k]==2) && currMB->b8mode[k]!=0)
				{
					//TRACE_STRING("ref_idx_l1");
				  

			  
//          img->subblock_x = i0;
//          img->subblock_y = j0;
          
//currSE.context = BType2CtxRef (currMB->b8mode[k]);
//int
//BType2CtxRef (int btype)
//{
//  if (btype<4)  return 0;
//  else          return 1;
//}
                    currSE.context = currMB->b8mode[k]<4 ? 0 : 1;
					if( (dec_params->active_pps->entropy_coding_mode_flag == UVLC || dP->bitstream->ei_flag) && flag_mode )
					{
						currSE.len = 1;
						readSyntaxElement_FLC(&currSE, dP->bitstream);
						currSE.value1 = 1-currSE.value1;
					}
					else
					{
						currSE.value2 = LIST_1;
						dP->readSyntaxElement (&currSE,dec_params,dP,dec_outputs);
					}
					refframe = currSE.value1;

					for (j=j0; j<j0+step_v0;j++)
					{
						for (i=i0; i<i0+step_h0;i++)
						{
							//dec_outputs->dec_picture->ref_idx[LIST_1][img->block_y + j][img->block_x + i] = refframe;
							stride_add3 = (((img->block_y+j)%4) * 4) + ((img->block_x+i)%4);
							*(curr_ref_idxl1+ (16*mb_nr) + stride_add3) = refframe;                    
						}
					}
				}
			}
		}
	}
	else
	{
		int stride_add3;
		int mb_nr = img->current_mb_nr;
		
		for (j0=0; j0<4; j0+=step_v0)
		{
			for (i0=0; i0<4; i0+=step_h0)
			{
				k=2*(j0>>1)+(i0>>1);        
				if ((currMB->b8pdir[k]==1 || currMB->b8pdir[k]==2) && currMB->b8mode[k]!=0)
				{
					
					/*for (j=j0; j<j0+step_v0;j++)
						for (i=i0; i<i0+step_h0;i++)
						{
						  dec_outputs->dec_picture->ref_idx[LIST_1][img->block_y + j][img->block_x + i] = 0;
						}
					*/
					for (j=j0; j<j0+step_v0;j++)
						for (i=i0; i<i0+step_h0;i++)
						{
							//dec_outputs->dec_picture->ref_idx[LIST_1][img->block_y + j][img->block_x + i] = 0;
							stride_add3 = (((img->block_y+j)%4) * 4) + ((img->block_x+i)%4);
							*(curr_ref_idxl1+ (16*mb_nr) + stride_add3) = 0;                    
						}
					
				}
			}
		}
	}



	//==============================================================================================
	//==============================================================================================
	//==============================================================================================	
	//  Read forward motion vectors
	currSE.type = SE_MVD;
	dP = &(currSlice->partArr[partMap[SE_MVD]]);
  //if (active_pps->entropy_coding_mode_flag == UVLC || dP->bitstream->ei_flag)
      //currSE.mapping = linfo_se;
  //else
      currSE.reading = readMVD_CABAC;

	for (j0=0; j0<4; j0+=step_v0)
	{
		for (i0=0; i0<4; i0+=step_h0)
		{
			k=(j0)+(i0>>1);     
			
			if ((currMB->b8pdir[k]==0 || currMB->b8pdir[k]==2) && (currMB->b8mode[k] !=0))//has forward vector
		  	{
				int stride_add3;
				int mb_nr = img->current_mb_nr;
				
				mv_mode  = currMB->b8mode[k];
				step_h   = BLOCK_STEP [mv_mode][0];
				step_v   = BLOCK_STEP [mv_mode][1];
				
				stride_add3 = (((img->block_y+j0)%4) * 4) + ((img->block_x+i0)%4);
				refframe = *(curr_ref_idxl0+ (16*mb_nr) + stride_add3) ;  
				//refframe = dec_outputs->dec_picture->ref_idx[LIST_0][img->block_y+j0][img->block_x+i0];
				
				for (j=j0; j<j0+step_v0; j+=step_v)
				{
					for (i=i0; i<i0+step_h0; i+=step_h)
					{
						j4 = img->block_y+j;
						i4 = img->block_x+i;
						
						// first make mv-prediction
						SetMotionVectorPredictor_baseline(pmv,pmv+1, refframe, LIST_0, i, j, step_h<<2, step_v<<2,dec_params,dec_outputs);
						
						for (k=0; k < 2; k++) 
						{
							//TRACE_STRING("mvd_l0");
//                            img->subblock_x = i; // position used for context determination
//                            img->subblock_y = j; // position used for context determination
                            currSE.value2 = k<<1; // identifies the component; only used for context determination
                            dP->readSyntaxElement(&currSE, dec_params, dP, dec_outputs);
                            curr_mvd = currSE.value1; 							

                            vec=curr_mvd+pmv[k];           /* find motion vector */
						
							for(ii=0;ii<step_h;ii++)
							{
								for(jj=0;jj<step_v;jj++)
								{
									//dec_outputs->dec_picture->mvL0[j4+jj][i4+ii][k] = vec;
									*(dec_outputs->dec_picture->mvL0+(j4+jj)*mvWidth+((i4+ii)<<1)+ii) = 0;
									currMB->mvd      [LIST_0][j+jj] [i+ii] [k] = curr_mvd;
                                    //dec_outputs->dec_picture->mv[j4+jj][i4+ii][k] = vec;
								}
							}
						}
					}
				}
			}

			
//			else if (currMB->b8mode[k=2*(j0/2)+(i0/2)]==0)      
			else if (currMB->b8mode[k]==0)      
			{  
				int mb_nr = img->current_mb_nr;
				if (!img->direct_spatial_mv_pred_flag)
				{
									
					//int list_offset = ((img->MbaffFrameFlag)&&(currMB->mb_field))? img->current_mb_nr%2 ? 4 : 2 : 0;
					
					
					//int imgblock_y= ((img->MbaffFrameFlag)&&(currMB->mb_field))? (img->current_mb_nr%2) ? (img->block_y-4)/2:img->block_y/2 : img->block_y;
					int imgblock_y= img->block_y;

					int refList;// = (co_located_ref_idx[LIST_0][imgblock_y+j0][img->block_x+i0]== -1 ? LIST_1 : LIST_0);
					int ref_idx;// =  co_located_ref_idx[refList][imgblock_y+j0][img->block_x+i0];          

					// faisal ref_idx
					int stride_add6;
					//int dummy_ref_idx;
					//int dummy_refList;

					stride_add6 = (((imgblock_y+j0)%4) * 4) + ((img->block_x+i0)%4);
					refList = *(curr_colocated_ref_idxl0+(16*mb_nr)+stride_add6)==-1? LIST_1 : LIST_0;

					if(refList == LIST_0)
					{
						ref_idx = *(curr_colocated_ref_idxl0+(16*mb_nr)+stride_add6);
					}
					else
					{
						ref_idx = *(curr_colocated_ref_idxl1+(16*mb_nr)+stride_add6);
					}

					if (ref_idx==-1)
					{
						for (j=j0; j<j0+step_v0; j++)
							for (i=i0; i<i0+step_h0; i++)
							{            
								stride_add6 = (((img->block_y+j)%4) * 4) + ((img->block_x+i)%4);
						
								*(curr_ref_idxl0+(16*mb_nr)+stride_add6) = 0;
								*(curr_ref_idxl1+(16*mb_nr)+stride_add6) = 0;

								//dec_outputs->dec_picture->ref_idx [LIST_1][img->block_y+j][img->block_x+i]=0;
								//dec_outputs->dec_picture->ref_idx [LIST_0][img->block_y+j][img->block_x+i]=0; 
								j4 = img->block_y+j;
								i4 = img->block_x+i;            
								for (ii=0; ii < 2; ii++) 
								{                                    
									//dec_outputs->dec_picture->mvL0[j4][i4][ii]=0;
									*(dec_outputs->dec_picture->mvL0+j4*mvWidth+(i4<<1)+ii) = 0;
									*(dec_outputs->dec_picture->mvL1+j4*mvWidth+(i4<<1)+ii)=0;                  
								}
							}
					}
					else 
					{        
						int mapped_idx=-1, iref;                             
						int j6;
						
					//	for (iref=0;iref<min(img->num_ref_idx_l0_active,dec_params->listXsize[LIST_0 + list_offset]);iref++)
						for (iref=0;iref<min(img->num_ref_idx_l0_active,dec_params->listXsize[LIST_0]);iref++)
						{
#if 1

						
							

							//int curr_mb_field = ((img->MbaffFrameFlag)&&(currMB->mb_field));

							//if(img->structure==0 && curr_mb_field==0)
							{
								// If the current MB is a frame MB and the colocated is from a field picture, 
								// then the co_located_ref_id may have been generated from the wrong value of 
								// frame_poc if it references it's complementary field, so test both POC values
								/*if(dec_params->listX[0][iref]->top_poc*2 == co_located_ref_id[refList][imgblock_y + j0][img->block_x + i0] || dec_params->listX[0][iref]->bottom_poc*2 == co_located_ref_id[refList][imgblock_y + j0][img->block_x + i0])
								{
								mapped_idx=iref;
								break;
								}
								else //! invalid index. Default to zero even though this case should not happen
								mapped_idx=INVALIDINDEX;
								continue;*/
							}   
#endif                                        
							//if (dec_outputs->dec_picture->ref_pic_num[img->current_slice_nr][LIST_0 + list_offset][iref]==co_located_ref_id[refList][imgblock_y+j0][img->block_x+i0])
							if (dec_outputs->dec_picture->ref_pic_num[img->current_slice_nr][LIST_0][iref]==co_located_ref_id[refList][imgblock_y+j0][img->block_x+i0])
							{
								mapped_idx=iref;
								break;
							}
							else //! invalid index. Default to zero even though this case should not happen
								mapped_idx=INVALIDINDEX;
						}

						if (INVALIDINDEX == mapped_idx)
						{
							error("temporal direct error\ncolocated block has ref that is unavailable",-1111,dec_params,dec_outputs);
						}


						for (j=j0; j<j0+step_v0; j++)
						{
							for (i=i0; i<i0+step_h0; i++)
							{
								{
								
							//		mv_scale = img->mvscale[LIST_0 + list_offset][mapped_idx];
									mv_scale = img->mvscale[LIST_0][mapped_idx];

									stride_add6 = (((img->block_y+j)%4) * 4) + ((img->block_x+i)%4);
						
									*(curr_ref_idxl0+(16*mb_nr)+stride_add6) = mapped_idx;
									*(curr_ref_idxl1+(16*mb_nr)+stride_add6) = 0;

									//dec_outputs->dec_picture->ref_idx [LIST_0][img->block_y+j][img->block_x+i] = mapped_idx;
									//dec_outputs->dec_picture->ref_idx [LIST_1][img->block_y+j][img->block_x+i] = 0;

									j4 = img->block_y+j;
									j6 = imgblock_y+j;
									i4 = img->block_x+i;

									for (ii=0; ii < 2; ii++) 
									{              
										
										//if (mv_scale == 9999 || dec_params->listX[LIST_0+list_offset][mapped_idx]->is_long_term)
										if (mv_scale == 9999 || dec_params->listX[LIST_0][mapped_idx]->is_long_term)
										{                      
											//*(dec_outputs->dec_picture->mvL0+j4*mvWidth+(i4<<1)+ii) = 0;
											*(dec_outputs->dec_picture->mvL0+j4*mvWidth+(i4<<1)+ii)=co_located_mv[refList][j6][i4][ii];
											*(dec_outputs->dec_picture->mvL1+j4*mvWidth+(i4<<1)+ii)=0;
										}
										else
										{
											*(dec_outputs->dec_picture->mvL0+j4*mvWidth+(i4<<1)+ii)=(mv_scale * co_located_mv[refList][j6][i4][ii] + 128 ) >> 8;
											*(dec_outputs->dec_picture->mvL1+j4*mvWidth+(i4<<1)+ii)=*(dec_outputs->dec_picture->mvL0+j4*mvWidth+(i4<<1)+ii) - co_located_mv[refList][j6][i4][ii];
										}
									}
								} 
							}
						}
					}  
				} 
			}
		}
	}
   /* 
    //=====  READ FORWARD MOTION VECTORS =====
    currSE.type = SE_MVD;
    dP = &(currSlice->partArr[partMap[SE_MVD]]);
    
    //if (dec_params->active_pps->entropy_coding_mode_flag == UVLC || dP->bitstream->ei_flag)
        //currSE.mapping = linfo_se;
    //else
        currSE.reading = readMVD_CABAC;
    
    for (j0=0; j0<4; j0+=step_v0)
    {
        for (i0=0; i0<4; i0+=step_h0)
        {
            k=2*(j0/2)+(i0/2);
            
            if ((currMB->b8pdir[k]==0 || currMB->b8pdir[k]==2) && (currMB->b8mode[k] !=0))//has forward vector
            {
                mv_mode  = currMB->b8mode[k];
                step_h   = BLOCK_STEP [mv_mode][0];
                step_v   = BLOCK_STEP [mv_mode][1];
                
                refframe = dec_picture->ref_idx[LIST_0][img->block_y+j0][img->block_x+i0];
                
                for (j=j0; j<j0+step_v0; j+=step_v)
                {
                    for (i=i0; i<i0+step_h0; i+=step_h)
                    {
                        j4 = img->block_y+j;
                        i4 = img->block_x+i;
                        
                        // first make mv-prediction
                        SetMotionVectorPredictor_baseline (pmv, pmv+1, refframe, LIST_0, i, j, 4*step_h, 4*step_v, dec_params, dec_outputs);
                        
                        for (k=0; k < 2; k++) 
                        {
                            //TRACE_STRING("mvd_l0");
                            img->subblock_x = i; // position used for context determination
                            img->subblock_y = j; // position used for context determination
                            currSE.value2 = k<<1; // identifies the component; only used for context determination
                            dP->readSyntaxElement(&currSE,dec_params,dP,dec_outputs);
                            curr_mvd = currSE.value1; 
                            
                            vec=curr_mvd+pmv[k];           // find motion vector //
                            
                            for(ii=0;ii<step_h;ii++)
                            {
                                for(jj=0;jj<step_v;jj++)
                                {
                                    dec_picture->mv[LIST_0]     [j4+jj][i4+ii][k] = vec;
                                    currMB->mvd [LIST_0][j+jj] [i+ii] [k] = curr_mvd;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    */



	//=====  READ BACKWARD MOTION VECTORS =====
	currSE.type = SE_MVD;
	dP = &(currSlice->partArr[partMap[SE_MVD]]);
	
	//if (dec_params->active_pps->entropy_coding_mode_flag == UVLC || dP->bitstream->ei_flag) 
		//currSE.mapping = linfo_se;
	//else                                                    
		currSE.reading = readMVD_CABAC;

	for (j0=0; j0<4; j0+=step_v0)
	{
		for (i0=0; i0<4; i0+=step_h0)
		{
			int stride_add6;
			int mb_nr = img->current_mb_nr;
			
			k=2*(j0/2)+(i0/2);
			if ((currMB->b8pdir[k]==1 || currMB->b8pdir[k]==2) && (currMB->b8mode[k]!=0))//has backward vector
			{
				mv_mode  = currMB->b8mode[k];
				step_h   = BLOCK_STEP [mv_mode][0];
				step_v   = BLOCK_STEP [mv_mode][1];

				//refframe = dec_outputs->dec_picture->ref_idx[LIST_1][img->block_y+j0][img->block_x+i0];
				stride_add6 = (((img->block_y+j0)%4) * 4) + ((img->block_x+i0)%4);
				refframe = *(curr_ref_idxl1+(16*mb_nr)+stride_add6);

				for (j=j0; j<j0+step_v0; j+=step_v)
				{
					for (i=i0; i<i0+step_h0; i+=step_h)
					{
						j4 = img->block_y+j;
						i4 = img->block_x+i;

						// first make mv-prediction
						SetMotionVectorPredictor_baseline (pmv, pmv+1, refframe, LIST_1, i, j, 4*step_h, 4*step_v,dec_params,dec_outputs);

						for (k=0; k < 2; k++) 
						{
							//TRACE_STRING("mvd_l1");
//              img->subblock_x = i; // position used for context determination
//              img->subblock_y = j; // position used for context determination
              currSE.value2   = (k<<1) +1; // identifies the component; only used for context determination
              dP->readSyntaxElement(&currSE, dec_params, dP, dec_outputs);
              curr_mvd = currSE.value1; 
              
              vec=curr_mvd+pmv[k];           /* find motion vector */
							for(ii=0;ii<step_h;ii++)
							{
								for(jj=0;jj<step_v;jj++)
								{
									*(dec_outputs->dec_picture->mvL1+(j4+jj)*mvWidth+((i4+ii)<<1)+k) = vec;
									currMB->mvd      [LIST_1][j+jj] [i+ii] [k] = curr_mvd;
								}
							}
						}
					}
				}
			}
		}
	}
	
	{
		
		int mb_nr = img->current_mb_nr;

		//curr_ref_idxl0 += (mb_nr<<4);
		curr_ref_pic_idL0 = ref_pic_idL0 + (mb_nr<<4);
		
		
		for(j4=0; j4 < 16; j4++)
		{
			if( *(curr_ref_idxl0) >= 0 )
			{
				*(curr_ref_pic_idL0++) = ref_pic_num_L0[(short)*(curr_ref_idxl0++)];
			}
			else
			{
				*(curr_ref_pic_idL0++) = -32768;
			}
		}

		if(dec_params->active_sps->profile_idc > 66)
		{
			curr_ref_pic_idL1 = ref_pic_idL1 + (mb_nr<<4);
			for(j4=0; j4 < 16; j4++)
			{
				if( *(curr_ref_idxl1) >= 0 )
				{
					*(curr_ref_pic_idL1++) = ref_pic_num_L1[(short)*(curr_ref_idxl1++)];  
				}
				else
				{
					*(curr_ref_pic_idL1++) = -32768;
				}
			}
		}
	}
#endif
}

void read_ipred_modes_CABAC(h264_decoder* dec_params)
{
    ImageParameters *img = dec_params->img;
    int b8,i,j,bi,bj,bx,by,dec;
    SyntaxElement currSE;
    Slice *currSlice;
    DataPartition *dP;
    int *partMap;
    Macroblock *currMB;
    int ts, ls;
    int mostProbableIntraPredMode;
    int upIntraPredMode;
    int leftIntraPredMode;
    int IntraChromaPredModeFlag;
    int bs_x, bs_y;
    int ii,jj;
    pic_parameter_set_rbsp_t* active_pps = dec_params->active_pps;
    //seq_parameter_set_rbsp_t* active_sps = dec_params->active_sps;
    
    int abcdef = 0;
    PixelPos left_block;
    PixelPos top_block;
//FILE* ipred_mode;   
currSE.type = 0;
currSE.value1 = 0;
currSE.value2 = 0;
currSE.len = 0;
currSE.inf = 0;
currSE.bitpattern = 0;
currSE.context = 0;
currSE.k = 0;
currSE.mapping = NULL;
currSE.reading = NULL;
//ipred_mode = fopen("ipred_mode","ab");


    currMB = &img->mb_data[img->current_mb_nr];
    
    IntraChromaPredModeFlag = IS_INTRA(currMB);
    
    currSlice = img->currentSlice;
    partMap = assignSE2partition[PAR_DP_1];
    
    currSE.type = SE_INTRAPREDMODE;
    
    //TRACE_STRING("intra4x4_pred_mode");
    dP = &(currSlice->partArr[partMap[currSE.type]]);
    
    //if (!(active_pps->entropy_coding_mode_flag == UVLC || dP->bitstream->ei_flag)) 
        currSE.reading = readIntraPredMode_CABAC;
    
    for(b8=0;b8<4;b8++)  //loop 8x8 blocks
    {
        if(currMB->b8mode[b8]==IBLOCK )
        {
            bs_x = bs_y = 4;
            
            IntraChromaPredModeFlag = 1;
            
            ii=(bs_x>>2);
            jj=(bs_y>>2);
            
            for(j=0;j<2;j+=jj)  //loop subblocks
            {
                for(i=0;i<2;i+=ii)
                {
                    //get from stream
                    //if (active_pps->entropy_coding_mode_flag == UVLC || dP->bitstream->ei_flag)
                    //{
                        //readSyntaxElement_Intra4x4PredictionMode(&currSE,img,dec_params->input,dP);
                    //}
                    //else 
                    {
                        currSE.context=(b8<<2)+(j<<1)+i;
                        dP->readSyntaxElement(&currSE, dP);
                    }
                    
                    bx = ((b8&1)<<1) + i;
                    by = (b8&2)      + j;
                    
                    getLuma4x4Neighbour(bx, by, -1,  0, &left_block, dec_params);
                    getLuma4x4Neighbour(bx, by,  0, -1, &top_block, dec_params);

                    
                    //get from array and decode
                    bi = (img->mb_x<<2) + bx;
                    bj = (img->mb_y<<2) + by;
                    
                    if (active_pps->constrained_intra_pred_flag)
                    {
                        left_block.available = left_block.available ? img->intra_block[left_block.mb_addr] : 0;
                        top_block.available  = top_block.available  ? img->intra_block[top_block.mb_addr]  : 0;
                    }
                    
                    // !! KS: not sure if the follwing is still correct...
                    ts=ls=0;   // Check to see if the neighboring block is SI
                    
                    upIntraPredMode            = (top_block.available  &&(ts == 0)) ? img->ipredmode[top_block.pos_x ][top_block.pos_y ] : -1;
                    leftIntraPredMode          = (left_block.available &&(ls == 0)) ? img->ipredmode[left_block.pos_x][left_block.pos_y] : -1;
                    
                    mostProbableIntraPredMode  = (upIntraPredMode < 0 || leftIntraPredMode < 0) ? DC_PRED : upIntraPredMode < leftIntraPredMode ? upIntraPredMode : leftIntraPredMode;
                    
                    dec = (currSE.value1 == -1) ? mostProbableIntraPredMode : currSE.value1 + (currSE.value1 >= mostProbableIntraPredMode);
                    
                    //set
                    for(jj=0;jj<(bs_y>>2);jj++)   //loop 4x4s in the subblock for 8x8 prediction setting
                    {
                        for(ii=0;ii<(bs_x>>2);ii++)
                        {
                            img->ipredmode[bi+ii][bj+jj]=dec;
                            //printf("x = %d, y = %d, m = %d\n",bi+ii,bj+jj,dec);
                                    abcdef = dec;
        //fwrite(&abcdef,sizeof(int),1,ipred_mode);
                        }
                    }
                }
            }
        }
    }
    
    if (IntraChromaPredModeFlag)
    {
        currSE.type = SE_INTRAPREDMODE;
//        TRACE_STRING("intra_chroma_pred_mode");
        dP = &(currSlice->partArr[partMap[currSE.type]]);
        
        //if (active_pps->entropy_coding_mode_flag == UVLC || dP->bitstream->ei_flag)
            //currSE.mapping = linfo_ue;
        //else
            currSE.reading = readCIPredMode_CABAC;
        
        dP->readSyntaxElement(&currSE, dP);
        currMB->c_ipred_mode = currSE.value1;
        abcdef = currSE.value1;
        //fwrite(&abcdef,sizeof(int),1,ipred_mode);
        
        if (currMB->c_ipred_mode < DC_PRED_8 || currMB->c_ipred_mode > PLANE_8)
        {
            printf("illegal chroma intra pred mode!\n");
			exit(0);
        }
    }
    //fclose(ipred_mode);
}

/*!
************************************************************************
* \brief
*    Sets mode for 8x8 block
************************************************************************
*/
void SetB8Mode (ImageParameters *img, Macroblock* currMB, int value, int i)
{
    static const int p_v2b8 [ 5] = {4, 5, 6, 7, IBLOCK};
    static const int p_v2pd [ 5] = {0, 0, 0, 0, -1};
    static const int b_v2b8 [14] = {0, 4, 4, 4, 5, 6, 5, 6, 5, 6, 7, 7, 7, IBLOCK};
    static const int b_v2pd [14] = {2, 0, 1, 2, 0, 0, 1, 1, 2, 2, 0, 1, 2, -1};
    
    if (img->type==B_SLICE)
    {
        currMB->b8mode[i]   = b_v2b8[value];
        currMB->b8pdir[i]   = b_v2pd[value];
        
    }
    else
    {
        currMB->b8mode[i]   = p_v2b8[value];
        currMB->b8pdir[i]   = p_v2pd[value];
    }
}


/*!
************************************************************************
* \brief
*    Interpret the mb mode for P-Frames
************************************************************************
*/
void interpret_mb_mode_P(ImageParameters *img)
{
    int i;
    const int ICBPTAB[6] = {0,16,32,15,31,47};
    Macroblock *currMB = &img->mb_data[img->current_mb_nr];
    int         mbmode = currMB->mb_type;
    
#define ZERO_P8x8     (mbmode==5)
#define MODE_IS_P8x8  (mbmode==4 || mbmode==5)
#define MODE_IS_I4x4  (mbmode==6)
#define I16OFFSET     (mbmode-7)
#define MODE_IS_IPCM  (mbmode==31)
    
    if(mbmode <4)
    {
        currMB->mb_type = mbmode;
        for (i=0;i<4;i++)
        {
            currMB->b8mode[i]   = mbmode;
            currMB->b8pdir[i]   = 0;
        }
    }
    else if(MODE_IS_P8x8)
    {
        currMB->mb_type = P8x8;
//        img->allrefzero = ZERO_P8x8;
    }
    else if(MODE_IS_I4x4)
    {
        currMB->mb_type = I4MB;
        for (i=0;i<4;i++)
        {
            currMB->b8mode[i] = IBLOCK;
            currMB->b8pdir[i] = -1;
        }
    }
    else if(MODE_IS_IPCM)
    {
        currMB->mb_type=IPCM;
        
        for (i=0;i<4;i++) 
        {
            currMB->b8mode[i]=0; currMB->b8pdir[i]=-1; 
        }
        currMB->cbp= -1;
        currMB->i16mode = 0;
    }
    else
    {
        currMB->mb_type = I16MB;
        for (i=0;i<4;i++) {currMB->b8mode[i]=0; currMB->b8pdir[i]=-1; }
        currMB->cbp= ICBPTAB[(I16OFFSET)>>2];
        currMB->i16mode = (I16OFFSET) & 0x03;
    }
}

/*!
************************************************************************
* \brief
*    Interpret the mb mode for I-Frames
************************************************************************
*/
void interpret_mb_mode_I(ImageParameters *img)
{
    int i;
    const int ICBPTAB[6] = {0,16,32,15,31,47};
    Macroblock *currMB   = &img->mb_data[img->current_mb_nr];
    int         mbmode   = currMB->mb_type;
    
    if (mbmode==0)
    {
        currMB->mb_type = I4MB;
        for (i=0;i<4;i++) {currMB->b8mode[i]=IBLOCK; currMB->b8pdir[i]=-1; }
    }
    else if(mbmode==25)
    {
        currMB->mb_type=IPCM;
        
        for (i=0;i<4;i++) {currMB->b8mode[i]=0; currMB->b8pdir[i]=-1; }
        currMB->cbp= -1;
        currMB->i16mode = 0;
        
    }
    else
    {
        currMB->mb_type = I16MB;
        for (i=0;i<4;i++) {currMB->b8mode[i]=0; currMB->b8pdir[i]=-1; }
        currMB->cbp= ICBPTAB[(mbmode-1)>>2];
        currMB->i16mode = (mbmode-1) & 0x03;
    }
}

/*!
 ************************************************************************
 * \brief
 *    Interpret the mb mode for B-Frames
 ************************************************************************
 */
void interpret_mb_mode_B(ImageParameters *img)
{
  static const int offset2pdir16x16[12]   = {0, 0, 1, 2, 0,0,0,0,0,0,0,0};
  static const int offset2pdir16x8[22][2] = {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{1,1},{0,0},{0,1},{0,0},{1,0},
                                             {0,0},{0,2},{0,0},{1,2},{0,0},{2,0},{0,0},{2,1},{0,0},{2,2},{0,0}};
  static const int offset2pdir8x16[22][2] = {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{1,1},{0,0},{0,1},{0,0},
                                             {1,0},{0,0},{0,2},{0,0},{1,2},{0,0},{2,0},{0,0},{2,1},{0,0},{2,2}};

  const int ICBPTAB[6] = {0,16,32,15,31,47};
  Macroblock *currMB = &img->mb_data[img->current_mb_nr];

  int i, mbmode;
  int mbtype  = currMB->mb_type;
  int *b8mode = currMB->b8mode;
  int *b8pdir = currMB->b8pdir;

  //--- set mbtype, b8type, and b8pdir ---
  if (mbtype==0)       // direct
  {
      mbmode=0;       for(i=0;i<4;i++) {b8mode[i]=0;          b8pdir[i]=2; }
  }
  else if (mbtype==23) // intra4x4
  {
    mbmode=I4MB;    for(i=0;i<4;i++) {b8mode[i]=IBLOCK;     b8pdir[i]=-1; }
  }
  else if ((mbtype>23) && (mbtype<48) ) // intra16x16
  {
    mbmode=I16MB;   for(i=0;i<4;i++) {b8mode[i]=0;          b8pdir[i]=-1; }
    currMB->cbp     = ICBPTAB[(mbtype-24)>>2];
    currMB->i16mode = (mbtype-24) & 0x03;
  }
  else if (mbtype==22) // 8x8(+split)
  {
    mbmode=P8x8;       // b8mode and pdir is transmitted in additional codewords
  }
  else if (mbtype<4)   // 16x16
  {
    mbmode=1;       for(i=0;i<4;i++) {b8mode[i]=1;          b8pdir[i]=offset2pdir16x16[mbtype]; }
  }
  else if(mbtype==48)
  {
    mbmode=IPCM;
    for (i=0;i<4;i++) {currMB->b8mode[i]=0; currMB->b8pdir[i]=-1; }
    currMB->cbp= -1;
    currMB->i16mode = 0;
  }

  else if (mbtype%2==0) // 16x8
  {
    mbmode=2;       for(i=0;i<4;i++) {b8mode[i]=2;          b8pdir[i]=offset2pdir16x8 [mbtype][i/2]; }
  }
  else
  {
    mbmode=3;       for(i=0;i<4;i++) {b8mode[i]=3;          b8pdir[i]=offset2pdir8x16 [mbtype][i%2]; }
  }
  currMB->mb_type = mbmode;
}


//#endif // __CABAC__