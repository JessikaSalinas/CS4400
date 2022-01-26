/*******************************************
 * Solutions for the CS:APP Performance Lab
 ********************************************/

#include <stdio.h>
#include <stdlib.h>
#include "defs.h"

/* 
 * Please fill in the following student struct 
 */
student_t student = {
  "Jessika Jimenez",     /* Full name */
  "u0864868@umail.utah.edu",  /* Email address */
};

/***************
 * COMPLEX KERNEL
 ***************/

/******************************************************
 * Your different versions of the complex kernel go here
 ******************************************************/

/* 
 * naive_complex - The naive baseline version of complex 
 */
char naive_complex_descr[] = "naive_complex: Naive baseline implementation";
void naive_complex(int dim, pixel *src, pixel *dest)
{
  int i, j;

  for(i = 0; i < dim; i++)
    for(j = 0; j < dim; j++)
    {

      dest[RIDX(dim - j - 1, dim - i - 1, dim)].red = ((int)src[RIDX(i, j, dim)].red +
						      (int)src[RIDX(i, j, dim)].green +
						      (int)src[RIDX(i, j, dim)].blue) / 3;
      
      dest[RIDX(dim - j - 1, dim - i - 1, dim)].green = ((int)src[RIDX(i, j, dim)].red +
							(int)src[RIDX(i, j, dim)].green +
							(int)src[RIDX(i, j, dim)].blue) / 3;
      
      dest[RIDX(dim - j - 1, dim - i - 1, dim)].blue = ((int)src[RIDX(i, j, dim)].red +
						       (int)src[RIDX(i, j, dim)].green +
						       (int)src[RIDX(i, j, dim)].blue) / 3;

    }
}


/* 
 * complex - Your current working version of complex
 * IMPORTANT: This is the version you will be graded on
 */
char complex_descr[] = "complex: Current working version";
void complex(int dim, pixel *src, pixel *dest)
{
  int i, j, k, l, W;

  // max 256 char long; large is 32, small is 16
  if (dim > 255) {
    W = 32;
  }
  else {
    W = 16;
  }

  for (i = 0; i < dim; i += W) {
    int dim1 = dim - 1;
    
    for (j = 0; j < dim; j += W) {

      for (k = i; k < i + W; k++) {
	int dim1k = dim1 - k;
	
	for (l = j; l < j + W; l++) {
	  int dim1l = dim1 - l;
	  int source = RIDX(l, k, dim);
	  int destination = RIDX(dim1k, dim1l, dim);
	  unsigned short colorSum = (src[source].red + src[source].green + src[source].blue)/3;
	  dest[destination].red = colorSum;
	  dest[destination].green = colorSum;
	  dest[destination].blue = colorSum;
	}
	
      }
      
    }
    
  }
  // naive_complex(dim, src, dest);
}

/*********************************************************************
 * register_complex_functions - Register all of your different versions
 *     of the complex kernel with the driver by calling the
 *     add_complex_function() for each test function. When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_complex_functions() {
  add_complex_function(&complex, complex_descr);
  add_complex_function(&naive_complex, naive_complex_descr);
}


/***************
 * MOTION KERNEL
 **************/

/***************************************************************
 * Various helper functions for the motion kernel
 * You may modify these or add new ones any way you like.
 **************************************************************/


/* 
 * weighted_combo - Returns new pixel value at (i,j) 
 */
__attribute__((always_inline)) static pixel weighted_combo(int dim, int i, int j, pixel *src) 
{
  int ii, jj;
  pixel current_pixel;

  int red, green, blue;
  red = green = blue = 0;

  int num_neighbors = 0;
  for(ii=0; ii < 3; ii++)
    for(jj=0; jj < 3; jj++) 
      if ((i + ii < dim) && (j + jj < dim)) 
      {
	num_neighbors++;
	red += (int) src[RIDX(i+ii,j+jj,dim)].red;
	green += (int) src[RIDX(i+ii,j+jj,dim)].green;
	blue += (int) src[RIDX(i+ii,j+jj,dim)].blue;
      }
  
  current_pixel.red = (unsigned short) (red / num_neighbors);
  current_pixel.green = (unsigned short) (green / num_neighbors);
  current_pixel.blue = (unsigned short) (blue / num_neighbors);
  
  return current_pixel;
}


__attribute__((always_inline)) static pixel normalCombo(int dim, int i, int j, pixel *src)
{
  pixel current_pixel;
  int red, green, blue;
  red = 0;
  green = 0;
  blue = 0;
  int neighbors = 9;
  int i1TimesN = i * dim;
  int i2TimesN = (i + 1) * dim;
  int i3TimesN = (i + 2) * dim;

  red += (int)src[i1TimesN + j].red;
  green += (int)src[i1TimesN + j].green;
  blue += (int)src[i1TimesN + j].blue;

  red += (int)src[i1TimesN + j + 1].red;
  green += (int)src[i1TimesN + j + 1].green;
  blue += (int)src[i1TimesN + j + 1].blue;

  red += (int)src[i1TimesN + j + 2].red;
  green += (int)src[i1TimesN + j + 2].green;
  blue += (int)src[i1TimesN + j + 2].blue;

  red += (int)src[i2TimesN + j].red;
  green += (int)src[i2TimesN + j].green;
  blue += (int)src[i2TimesN + j].blue;

  red += (int)src[i2TimesN + j + 1].red;
  green += (int)src[i2TimesN + j + 1].green;
  blue += (int)src[i2TimesN + j + 1].blue;

  red += (int)src[i2TimesN + j + 2].red;
  green += (int)src[i2TimesN + j + 2].green;
  blue += (int)src[i2TimesN + j + 2].blue;

  red += (int)src[i3TimesN + j].red;
  green += (int)src[i3TimesN + j].green;
  blue += (int)src[i3TimesN + j].blue;

  red += (int)src[i3TimesN + j + 1].red;
  green += (int)src[i3TimesN + j + 1].green;
  blue += (int)src[i3TimesN + j + 1].blue;

  red += (int)src[i3TimesN + j + 2].red;
  green += (int)src[i3TimesN + j + 2].green;
  blue += (int)src[i3TimesN + j + 2].blue;

  current_pixel.red = (unsigned short)(red / neighbors);
  current_pixel.green = (unsigned short)(green / neighbors);
  current_pixel.blue = (unsigned short)(blue / neighbors);

  return current_pixel;
}


__attribute__((always_inline)) static pixel sortaEdgeCombo(int dim, int i, int j, pixel *src)
{
  pixel current_pixel;
  int red, green, blue;
  red = 0;
  green = 0;
  blue = 0;
  int neighbors = 6;
  int i1TimesN = i * dim;
  int i2TimesN = i1TimesN + dim;
  int i3TimesN = i2TimesN + dim;

  red += (int)src[i1TimesN + j].red;
  green += (int)src[i1TimesN + j].green;
  blue += (int)src[i1TimesN + j].blue;
  
  red += (int)src[i1TimesN + j + 1].red;
  green += (int)src[i1TimesN + j + 1].green;
  blue += (int)src[i1TimesN + j + 1].blue;

  red += (int)src[i2TimesN + j].red;
  green += (int)src[i2TimesN + j].green;
  blue += (int)src[i2TimesN + j].blue;

  red += (int)src[i2TimesN + j + 1].red;
  green += (int)src[i2TimesN + j + 1].green;
  blue += (int)src[i2TimesN + j + 1].blue;

  red += (int)src[i3TimesN + j].red;
  green += (int)src[i3TimesN + j].green;
  blue += (int)src[i3TimesN + j].blue;

  red += (int)src[i3TimesN + j + 1].red;
  green += (int)src[i3TimesN + j + 1].green;
  blue += (int)src[i3TimesN + j + 1].blue;

  current_pixel.red = (unsigned short)(red / neighbors);
  current_pixel.green = (unsigned short)(green / neighbors);
  current_pixel.blue = (unsigned short)(blue / neighbors);

  return current_pixel;
}


__attribute__((always_inline)) static pixel sortaEdgeBottomCombo(int dim, int i, pixel *src)
{
  pixel current_pixel;
  int red, green, blue;
  red = 0;
  green = 0;
  blue = 0;
  int neighbors = 6;
  int i1TimesN = dim * (dim - 1) + i;
  int i2TimesN = dim * (dim - 2) + i;

  red += (int)src[i1TimesN].red;
  green += (int)src[i1TimesN].green;
  blue += (int)src[i1TimesN].blue;

  red += (int)src[i1TimesN + 1].red;
  green += (int)src[i1TimesN + 1].green;
  blue += (int)src[i1TimesN + 1].blue;

  red += (int)src[i1TimesN + 2].red;
  green += (int)src[i1TimesN + 2].green;
  blue += (int)src[i1TimesN + 2].blue;

  red += (int)src[i2TimesN].red;
  green += (int)src[i2TimesN].green;
  blue += (int)src[i2TimesN].blue;

  red += (int)src[i2TimesN + 1].red;
  green += (int)src[i2TimesN + 1].green;
  blue += (int)src[i2TimesN + 1].blue;

  red += (int)src[i2TimesN + 2].red;
  green += (int)src[i2TimesN + 2].green;
  blue += (int)src[i2TimesN + 2].blue;
  
  current_pixel.red = (unsigned short)(red / neighbors);
  current_pixel.green = (unsigned short)(green / neighbors);
  current_pixel.blue = (unsigned short)(blue / neighbors);

  return current_pixel;
}


__attribute__((always_inline)) static pixel edgeCombo(int dim, int i, pixel *src)
{
  pixel current_pixel;
  int red, green, blue;
  red = 0;
  green = 0;
  blue = 0;
  int neighbors = 3;
  int i1TimesN = i * dim + dim - 1;
  int i2TimesN = ((i + 1) * dim) + dim - 1;
  int i3TimesN = ((i + 2) * dim) + dim - 1;

  red += (int)src[i1TimesN].red;
  green += (int)src[i1TimesN].green;
  blue += (int)src[i1TimesN].blue;

  red += (int)src[i2TimesN].red;
  green += (int)src[i2TimesN].green;
  blue += (int)src[i2TimesN].blue;

  red += (int)src[i3TimesN].red;
  green += (int)src[i3TimesN].green;
  blue += (int)src[i3TimesN].blue;
  
  current_pixel.red = (unsigned short)(red / neighbors);
  current_pixel.green = (unsigned short)(green / neighbors);
  current_pixel.blue = (unsigned short)(blue / neighbors);

  return current_pixel;
}


__attribute__((always_inline)) static pixel edgeBottomCombo(int dim, int i, pixel *src)
{
  pixel current_pixel;
  int red, green, blue;
  red = 0;
  green = 0;
  blue = 0;
  int neighbors = 3;
  int i1TimesN = ((dim - 1) * dim) + i;

  red += (int)src[i1TimesN].red;
  green += (int)src[i1TimesN].green;
  blue += (int)src[i1TimesN].blue;

  red += (int)src[i1TimesN + 1].red;
  green += (int)src[i1TimesN + 1].green;
  blue += (int)src[i1TimesN + 1].blue;

  red += (int)src[i1TimesN + 2].red;
  green += (int)src[i1TimesN + 2].green;
  blue += (int)src[i1TimesN + 2].blue;

  current_pixel.red = (unsigned short)(red / neighbors);
  current_pixel.green = (unsigned short)(green / neighbors);
  current_pixel.blue = (unsigned short)(blue / neighbors);

  return current_pixel;
}


__attribute__((always_inline)) static pixel inCornerCombo(int pixnum, int dim, pixel *src)
{
  pixel current_pixel;
  int red, green, blue;
  red = 0;
  green = 0;
  blue = 0;
  int neighbors = 4;

  red += src[pixnum].red;
  green += src[pixnum].green;
  blue += src[pixnum].blue;

  red += src[pixnum + 1].red;
  green += src[pixnum + 1].green;
  blue += src[pixnum + 1].blue;

  red += src[pixnum + dim].red;
  green += src[pixnum + dim].green;
  blue += src[pixnum + dim].blue;

  red += src[pixnum + 1 + dim].red;
  green += src[pixnum + 1 + dim].green;
  blue += src[pixnum + 1 + dim].blue;

  current_pixel.red = (unsigned short)(red / neighbors);
  current_pixel.green = (unsigned short)(green / neighbors);
  current_pixel.blue = (unsigned short)(blue / neighbors);

  return current_pixel;
}


__attribute__((always_inline)) static pixel twoPixels(int start, int offset, pixel *src)
{
  pixel current_pixel;
  int red, green, blue;
  red = 0;
  green = 0;
  blue = 0;
  int neighbors = 2;

  red += src[start].red;
  green += src[start].green;
  blue += src[start].blue;

  red += src[start + offset].red;
  green += src[start + offset].green;
  blue += src[start + offset].blue;

  current_pixel.red = (unsigned short)(red / neighbors);
  current_pixel.green = (unsigned short)(green / neighbors);
  current_pixel.blue = (unsigned short)(blue / neighbors);

  return current_pixel;
}


/******************************************************
 * Your different versions of the motion kernel go here
 ******************************************************/


/*
 * naive_motion - The naive baseline version of motion 
 */
char naive_motion_descr[] = "naive_motion: Naive baseline implementation";
void naive_motion(int dim, pixel *src, pixel *dst) 
{
  int i, j;
    
  for (i = 0; i < dim; i++)
    for (j = 0; j < dim; j++)
      dst[RIDX(i, j, dim)] = weighted_combo(dim, i, j, src);
}


/*
 * motion - Your current working version of motion. 
 * IMPORTANT: This is the version you will be graded on
 */
char motion_descr[] = "motion: Current working version";
void motion(int dim, pixel *src, pixel *dst) 
{
  int i, j;
  int squareDim = dim * dim;
  
  for (i = 0; i < dim - 2; i++) {
    int iTimesN = i * dim;

    for (j = 0; j < dim - 2; j++) {
      dst[iTimesN + j] = normalCombo(dim, i, j, src);
    }

    dst[iTimesN + (dim - 2)] = sortaEdgeCombo(dim, i, j, src);
    dst[squareDim - 2 * dim + i] = sortaEdgeBottomCombo(dim, i, src);
    dst[iTimesN + (dim - 1)] = edgeCombo(dim, i, src);
    dst[squareDim - dim + i] = edgeBottomCombo(dim, i, src);
  }

  dst[squareDim - dim - 2] = inCornerCombo(squareDim - dim - 2, dim, src);
  dst[squareDim - 1] = src[squareDim - 1];  
  dst[squareDim - dim - 1] = twoPixels(squareDim - dim - 1, dim, src);
  dst[squareDim - 2] = twoPixels(squareDim - 2, 1, src);
  
  // naive_motion(dim, src, dst);
}

/********************************************************************* 
 * register_motion_functions - Register all of your different versions
 *     of the motion kernel with the driver by calling the
 *     add_motion_function() for each test function.  When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.  
 *********************************************************************/

void register_motion_functions() {
  add_motion_function(&motion, motion_descr);
  add_motion_function(&naive_motion, naive_motion_descr);
}
