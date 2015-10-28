
/*!
 ************************************************************************
 * \file erc_globals.h
 *
 * \brief
 *      global header file for error concealment module
 *
 * \author
 *      - Viktor Varsa                     <viktor.varsa@nokia.com>
 *      - Ye-Kui Wang                   <wyk@ieee.org>
 ************************************************************************
 */

#ifndef _ERC_GLOBALS_H_
#define _ERC_GLOBALS_H_

#include <string.h>
#include "defines.h"

/* "block" means an 8x8 pixel area */

/* Region modes */
#define REGMODE_INTER_COPY       0  /* Copy region */
#define REGMODE_INTER_PRED       1  /* Inter region with motion vectors */
#define REGMODE_INTRA            2  /* Intra region */
#define REGMODE_SPLITTED         3  /* Any region mode higher than this indicates that the region 
                                       is splitted which means 8x8 block */
#define REGMODE_INTER_COPY_8x8   4
#define REGMODE_INTER_PRED_8x8   5
#define REGMODE_INTRA_8x8        6

/* YUV pixel domain image arrays for a video frame */
typedef struct
{
  imgpel *yptr;
  imgpel *uptr;
  imgpel *vptr;
} frame;


#endif

