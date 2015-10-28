
/*!
 ***************************************************************************
 * \file
 *    cabac.h
 *
 * \brief
 *    Headerfile for entropy coding routines
 *
 * \author
 *    Detlev Marpe                                                         \n
 *    Copyright (C) 2000 HEINRICH HERTZ INSTITUTE All Rights Reserved.
 *
 * \date
 *    21. Oct 2000 (Changes by Tobias Oelbaum 28.08.2001)
 ***************************************************************************
 */
#ifndef _CABAC_H_
#define _CABAC_H_

//#ifdef __CABAC__

//void cabac_new_slice();

void readMB_typeInfo_CABAC      (SyntaxElement *se, h264_decoder* dec_params, DecodingEnvironmentPtr dep_dp );
void readB8_typeInfo_CABAC      (SyntaxElement *se, h264_decoder* dec_params, DecodingEnvironmentPtr dep_dp );
void readIntraPredMode_CABAC    (SyntaxElement *se, h264_decoder* dec_params, DecodingEnvironmentPtr dep_dp );
void readRefFrame_CABAC         (SyntaxElement *se, h264_decoder* dec_params, DecodingEnvironmentPtr dep_dp );
void readMVD_CABAC              (SyntaxElement *se, h264_decoder* dec_params, DecodingEnvironmentPtr dep_dp );
void readCBP_CABAC              (SyntaxElement *se, h264_decoder* dec_params, DecodingEnvironmentPtr dep_dp );
void readRunLevel_CABAC         (SyntaxElement *se, h264_decoder* dec_params, DecodingEnvironmentPtr dep_dp );
void readDquant_CABAC           (SyntaxElement *se, h264_decoder* dec_params, DecodingEnvironmentPtr dep_dp );
void readCIPredMode_CABAC       (SyntaxElement *se, h264_decoder *dec_params, DecodingEnvironmentPtr dep_dp );
void readMB_skip_flagInfo_CABAC (SyntaxElement *se, h264_decoder* dec_params, DecodingEnvironmentPtr dep_dp );
int  readSyntaxElement_CABAC    (SyntaxElement *se, DataPartition *this_dataPart );
void CheckAvailabilityOfNeighborsCABAC(h264_decoder* dec_params);
void read_ipred_modes_CABAC(h264_decoder* dec_params);
void SetB8Mode (ImageParameters *img, Macroblock* currMB, int value, int i);
void reset_coeffs(ImageParameters *img);
void interpret_mb_mode_P(ImageParameters *img);
void interpret_mb_mode_I(ImageParameters *img);
void interpret_mb_mode_B(ImageParameters *img);



void readMotionInfoFromNAL_CABAC(h264_decoder* dec_params);

void readCBPandCoeffsFromNAL_CABAC(h264_decoder* dec_params);








//#endif  // __CABAC__

#endif  // _CABAC_H_
