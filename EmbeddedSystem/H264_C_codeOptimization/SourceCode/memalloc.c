 
/*!
************************************************************************
* \file  memalloc.c
*
* \brief
*    Memory allocation and free helper funtions
*
* \author
*    Main contributors (see contributors.h for copyright, address and affiliation details)
************************************************************************
*/

#include <stdlib.h>
#include "memalloc.h"


void *h264_malloc( int i_size )
{
    unsigned char * buf;
    unsigned char * align_buf;
    buf = (unsigned char *) malloc( i_size + 15 + sizeof( void ** ) +
              sizeof( int ) );

    //buf = (unsigned char *) malloc( i_size * sizeof( unsigned char) );

	if(buf != NULL)
	{
	    align_buf = buf + 15 + sizeof( void ** ) + sizeof( int );
	    align_buf -= (long) align_buf & 15;
	    *( (void **) ( align_buf - sizeof( void ** ) ) ) = buf;
	    *( (int *) ( align_buf - sizeof( void ** ) - sizeof( int ) ) ) = i_size;
		//align_buf = buf;
	    return align_buf;

	}
	else
	{
		return NULL;
	}
}

void *h264_calloc( int n, int i_size )
{
	unsigned char * buf;
    buf = (unsigned char *) h264_malloc(n*i_size);
	memset(buf, 0, n*i_size);
    
    return buf;
}

void h264_free( void *p )
{
    if( p )
    {
        free( *( ( ( void **) p ) - 1 ) );
    }
}
/*!
 ************************************************************************
 * \brief
 *    Allocate 2D memory array -> imgpel array2D[rows][columns]
 *
 * \par Output:
 *    memory size in bytes
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************/
int get_mem2Dpel(imgpel ***array2D, int rows, int columns)
{
  int i;

  if((*array2D      = (imgpel**) h264_malloc(rows *        sizeof(imgpel*))) == NULL)
  {
	  printf("get_mem2Dpel: array2D");
	  exit(0);
  }

  if(((*array2D)[0] = (imgpel* ) h264_malloc(rows*columns * sizeof(imgpel ))) == NULL)
  {
	  printf("get_mem2Dpel: array2D");
	  exit(0);
  }

  for(i=1 ; i<rows ; i++)
    (*array2D)[i] =  (*array2D)[i-1] + columns  ;

  return rows*columns*sizeof(imgpel);
}

/*!
 ************************************************************************
 * \brief
 *    Allocate 3D memory array -> imgpel array3D[frames][rows][columns]
 *
 * \par Output:
 *    memory size in bytes
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
int get_mem3Dpel(imgpel ****array3D, int frames, int rows, int columns)
{
  int  j;

  if(((*array3D) = (imgpel***) h264_malloc(frames * sizeof(imgpel**))) == NULL)
  {
	  printf("get_mem3Dpel: array3D");
	  exit(0);
  }

  for(j=0;j<frames;j++)
    get_mem2Dpel( (*array3D)+j, rows, columns) ;

  return frames*rows*columns*sizeof(imgpel);
}

/*!
 ************************************************************************
 * \brief
 *    free 2D memory array
 *    which was alocated with get_mem2Dpel()
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
void free_mem2Dpel(imgpel **array2D)
{
  if (array2D)
  {
    if (array2D[0])
      h264_free (array2D[0]);
    else 
	{
		printf("free_mem2Dpel: trying to free unused memory",100);
		exit(0);
	}

    h264_free (array2D);
  } 
  else
  {
    printf("free_mem2Dpel: trying to free unused memory");
	exit(0);
  }
}

/*!
 ************************************************************************
 * \brief
 *    free 3D memory array
 *    which was alocated with get_mem3Dpel()
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
void free_mem3Dpel(imgpel ***array3D, int frames)
{
  int i;

  if (array3D)
  {
    for (i=0;i<frames;i++)
    { 
      free_mem2Dpel(array3D[i]);
    }
   h264_free (array3D);
  } 
  else
  {
    printf("free_mem3Dpel: trying to free unused memory");
	exit(0);
  }
}

/*!
 ************************************************************************
 * \brief
 *    Allocate 2D memory array -> unsigned char array2D[rows][columns]
 *
 * \par Output:
 *    memory size in bytes
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************/
// Change 9-Aug-2001 P. List: dont allocate independant row arrays anymore
// but one complete array and move row-pointers to array. Now you can step
// to the next line with an offset of img->width
int get_mem2D(byte ***array2D, int rows, int columns)
{
  int i;

  if((*array2D      = (byte**) h264_malloc(rows *        sizeof(byte*))) == NULL)
  {
	  printf("get_mem2D: array2D");
	  exit(0);
  }

  if(((*array2D)[0] = (byte* ) h264_malloc(columns*rows * sizeof(byte ))) == NULL)
  {
	  printf("get_mem2D: array2D");
	  exit(0);
  }

  for(i=1;i<rows;i++)
    (*array2D)[i] = (*array2D)[i-1] + columns ;

  return rows*columns;
}

/*!
 ************************************************************************
 * \brief
 *    Allocate 2D memory array -> int array2D[rows][columns]
 *
 * \par Output:
 *    memory size in bytes
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
// same change as in get_mem2Dint
int get_mem2Dint(int ***array2D, int rows, int columns)
{
  int i;
  if((*array2D      = (int**) h264_malloc(rows *        sizeof(int*))) == NULL)
  {
    printf("get_mem2Dint: array2D");
	exit(0);
  }

  if(((*array2D)[0] = (int* ) h264_malloc(rows*columns * sizeof(int ))) == NULL)
  {
    printf("get_mem2Dint: array2D");
	exit(0);
  }
  for(i=1 ; i<rows ; i++)
  {
    (*array2D)[i] =  (*array2D)[i-1] + columns  ;
  }
  return rows*columns*sizeof(int);
}

/*!
 ************************************************************************
 * \brief
 *    Allocate 2D memory array -> int64 array2D[rows][columns]
 *
 * \par Output:
 *    memory size in bytes
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
// same change as in get_mem2Dint
int get_mem2Dint64(int64 ***array2D, int rows, int columns)
{
  int i;
  if((*array2D      = (int64**)h264_malloc(rows*        sizeof(int64*))) == NULL)
  {
	  printf("get_mem2Dint64: array2D");
	  exit(0);
  }
  if(((*array2D)[0] = (int64* )h264_malloc(rows*columns*sizeof(int64 ))) == NULL)
  {
	  printf("get_mem2Dint64: array2D");
	  exit(0);
  }
  for(i=1 ; i<rows ; i++)
  {
	  (*array2D)[i] =  (*array2D)[i-1] + columns  ;
  }
  return rows*columns*sizeof(int64);
}

/*!
 ************************************************************************
 * \brief
 *    Allocate 3D memory array -> unsigned char array3D[frames][rows][columns]
 *
 * \par Output:
 *    memory size in bytes
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
// same change as in get_mem2Dint
int get_mem3D(byte ****array3D, int frames, int rows, int columns)
{
  int  j;
  if(((*array3D) = (byte***) h264_malloc(frames * sizeof(byte**))) == NULL)
  {
    printf("get_mem3D: array3D");
	exit(0);
  }
  for(j=0;j<frames;j++)
  {
    get_mem2D( (*array3D)+j, rows, columns) ;
  }
  return frames*rows*columns;
}

/*!
 ************************************************************************
 * \brief
 *    Allocate 3D memory array -> int array3D[frames][rows][columns]
 *
 * \par Output:
 *    memory size in bytes
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
// same change as in get_mem2Dint
int get_mem3Dint(int ****array3D, int frames, int rows, int columns)
{
  int  j;
  
  if(((*array3D) = (int***) h264_malloc(frames*sizeof(int**))) == NULL)
  {
    printf("get_mem3Dint: array3D");
	exit(0);
  }
  for(j=0;j<frames;j++)
  {
    get_mem2Dint( (*array3D)+j, rows, columns) ;
  }
  return frames*rows*columns*sizeof(int);
}

/*!
 ************************************************************************
 * \brief
 *    Allocate 3D memory array -> int64 array3D[frames][rows][columns]
 *
 * \par Output:
 *    memory size in bytes
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
// same change as in get_mem2Dint
int get_mem3Dint64(int64 ****array3D, int frames, int rows, int columns)
{
  int  j;

  if(((*array3D) = (int64***)h264_malloc(frames*sizeof(int64**))) == NULL)
  {
    printf("get_mem3Dint64: array3D");
	exit(0);
  }
  for(j=0;j<frames;j++)
  {
    get_mem2Dint64( (*array3D)+j, rows, columns) ;
  }
  return frames*rows*columns*sizeof(int64);
}

/*!
 ************************************************************************
 * \brief
 *    Allocate 4D memory array -> int array3D[frames][rows][columns][component]
 *
 * \par Output:
 *    memory size in bytes
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
// same change as in get_mem2Dint
int get_mem4Dint(int *****array4D, int idx, int frames, int rows, int columns)
{
  int  j;

  if(((*array4D) = (int****) h264_malloc(idx*sizeof(int**))) == NULL)
  {
    printf("get_mem4Dint: array4D");
	exit(0);
  }
  for(j=0;j<idx;j++)
  {
    get_mem3Dint( (*array4D)+j, frames, rows, columns) ;
  }
  return idx*frames*rows*columns*sizeof(int);
}

/*!
 ************************************************************************
 * \brief
 *    free 2D memory array
 *    which was alocated with get_mem2D()
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
void free_mem2D(byte **array2D)
{
  if (array2D)
  {
    if (array2D[0])
	{
      h264_free (array2D[0]);
	}
    else
	{
		printf("free_mem2D: trying to free unused memory");
	}
    h264_free (array2D);
  }
  else
  {
    printf("free_mem2D: trying to free unused memory");
  }
}

/*!
 ************************************************************************
 * \brief
 *    free 2D memory array
 *    which was alocated with get_mem2Dint()
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
void free_mem2Dint(int **array2D)
{
  if (array2D)
  {
    if (array2D[0])
	{
      h264_free (array2D[0]);
	}
    else
	{
		printf("free_mem2Dint: trying to free unused memory");
	}
    h264_free (array2D);
  }
  else
  {
    printf("free_mem2Dint: trying to free unused memory");
  }
}

/*!
 ************************************************************************
 * \brief
 *    free 2D memory array
 *    which was alocated with get_mem2Dint64()
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
void free_mem2Dint64(int64 **array2D)
{
  if (array2D)
  {
    if (array2D[0]) 
	{
      h264_free (array2D[0]);
	}
    else
	{
		printf("free_mem2Dint64: trying to free unused memory");
	}
    h264_free (array2D);
  }
  else
  {
    printf("free_mem2Dint64: trying to free unused memory");
	exit(0);
  }
}


/*!
 ************************************************************************
 * \brief
 *    free 3D memory array
 *    which was alocated with get_mem3D()
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
void free_mem3D(byte ***array3D, int frames)
{
  int i;

  if (array3D)
  {
    for (i=0;i<frames;i++)
    { 
      free_mem2D(array3D[i]);
    }
   h264_free (array3D);
  }
  else
  {
    printf("free_mem3D: trying to free unused memory");
	exit(0);
  }
}

/*!
 ************************************************************************
 * \brief
 *    free 3D memory array 
 *    which was alocated with get_mem3Dint()
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
void free_mem3Dint(int ***array3D, int frames)
{
  int i;

  if (array3D)
  {
    for (i=0;i<frames;i++)
    { 
      free_mem2Dint(array3D[i]);
    }
   h264_free (array3D);
  }
  else
  {
    printf("free_mem3D: trying to free unused memory");
	exit(0);
  }
}


/*!
 ************************************************************************
 * \brief
 *    free 3D memory array 
 *    which was alocated with get_mem3Dint64()
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
void free_mem3Dint64(int64 ***array3D, int frames)
{
  int i;

  if (array3D)
  {
    for (i=0;i<frames;i++)
    { 
      free_mem2Dint64(array3D[i]);
    }
   h264_free (array3D);
  }
  else
  {
    printf("free_mem3Dint64: trying to free unused memory");
	exit(0);
  }
}

/*!
 ************************************************************************
 * \brief
 *    free 4D memory array 
 *    which was alocated with get_mem4Dint()
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
void free_mem4Dint(int ****array4D, int idx, int frames)
{
  int  j;

  if (array4D)
  {
    for(j=0;j<idx;j++)
	{
      free_mem3Dint( array4D[j], frames) ;
	}
    h264_free (array4D);
  }
  else
  {
    printf("free_mem4D: trying to free unused memory");
	exit(0);
  }
}


/*!
 ************************************************************************
 * \brief
 *    Exit program if memory allocation failed (using error())
 * \param where
 *    string indicating which memory allocation failed
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
void no_mem_exit(char *where)
{
   //snprintf(dec_params->errortext, ET_SIZE, "Could not allocate memory: %s",where);
   printf ("Could not allocate memory: %s\n",where);
   exit(0);
}

/*!
 ************************************************************************
 * \brief
 *    Allocate 2D short memory array -> short array2D[rows][columns]
 *
 * \par Output:
 *    memory size in bytes
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
int get_mem2Dshort(short ***array2D, int rows, int columns)
{
  int i;
  
  if((*array2D      = (short**)h264_calloc(rows,        sizeof(short*))) == NULL)
  {
    printf("get_mem2Dshort: array2D");
	exit(0);
  }
  
  if(((*array2D)[0] = (short* )h264_calloc(rows * columns,sizeof(short ))) == NULL)
  {
    printf("get_mem2Dshort: array2D");
	exit(0);
  }
  
  for(i=1 ; i<rows ; i++)
  {
	  (*array2D)[i] =  (*array2D)[i-1] + columns  ;
  }
  return rows*columns*sizeof(short);
}

/*!
 ************************************************************************
 * \brief
 *    Allocate 3D memory short array -> short array3D[frames][rows][columns]
 *
 * \par Output:
 *    memory size in bytes
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
int get_mem3Dshort(short ****array3D, int frames, int rows, int columns)
{
  int  j;

  if(((*array3D) = (short***)h264_calloc(frames,sizeof(short**))) == NULL)
  {
    printf("get_mem3Dshort: array3D");
	exit(0);
  }
  for(j=0;j<frames;j++)
  {
	  get_mem2Dshort( (*array3D)+j, rows, columns) ;
  }
  return frames*rows*columns*sizeof(short);
}

/*!
 ************************************************************************
 * \brief
 *    Allocate 4D memory short array -> short array3D[frames][rows][columns][component]
 *
 * \par Output:
 *    memory size in bytes
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
int get_mem4Dshort(short *****array4D, int idx, int frames, int rows, int columns)
{
  int  j;

  if(((*array4D) = (short****)h264_calloc(idx,sizeof(short**))) == NULL)
  {
	  printf("get_mem4Dshort: array4D");
	  exit(0);
  }
  for(j=0;j<idx;j++)
  {
	  get_mem3Dshort( (*array4D)+j, frames, rows, columns ) ;
  }
  return idx*frames*rows*columns*sizeof(short);
}

/*!
 ************************************************************************
 * \brief
 *    free 2D short memory array
 *    which was allocated with get_mem2Dshort()
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
void free_mem2Dshort(short **array2D)
{
  if (array2D)
  {
    if (array2D[0]) 
	{
      h264_free (array2D[0]);
	}
    else
	{
		printf("free_mem2Dshort: trying to free unused memory");
		exit(0);
	}
    h264_free (array2D);

  }
  else
  {
    printf("free_mem2Dshort: trying to free unused memory");
	exit(0);
  }
}

/*!
 ************************************************************************
 * \brief
 *    free 3D short memory array 
 *    which was allocated with get_mem3Dshort()
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
void free_mem3Dshort(short ***array3D, int frames)
{
  int i;

  if (array3D)
  {
    for (i=0;i<frames;i++)
    { 
      free_mem2Dshort(array3D[i]);
    }
   h264_free (array3D);
  }
  else
  {
    printf("free_mem3Dshort: trying to free unused memory");
	exit(0);
  }
}

/*!
 ************************************************************************
 * \brief
 *    free 4D short memory array 
 *    which was allocated with get_mem4Dshort()
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
void free_mem4Dshort(short ****array4D, int idx, int frames )
{
  int  j;

  if (array4D)
  {
	  for(j=0;j<idx;j++)
	  {
      free_mem3Dshort( array4D[j], frames) ;
	  }
    h264_free (array4D);
  }
  else
  {
    printf("free_mem4Dshort: trying to free unused memory");
	exit(0);
  }
}


