/* ============================================================================
 * @File: 	 cammodule.c
 * @Author: 	 Ozgur Eralp [ozgur.eralp@outlook.com]
 * @Description: Camera Interface Application running on ARM Platforms
 *
 * ============================================================================
 *
 * Copyright 2014 Ozgur Eralp.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <asm/errno.h>
#include <jpeglib.h>
#include "v4l2cam.h"
#include "cammodule.h"

struct capture_info capinfo;
static void jpegWrite(unsigned char* img);
static void YUV422toRGB888(int width, int height, unsigned char *src, unsigned char *dst);
static unsigned char jpegQuality = 70;
static char* jpegFilename = NULL;

/* ============================================================================
 * @Function: 	 cammodule_init
 * @Description: Initialize V4L2 capture interface.
 * ============================================================================
 */
int cammodule_init (struct cammodule_arguments *arg)
{
    	capinfo.width = arg->width;
	capinfo.height = arg->height;
    	capinfo.device_name = arg->device_name;
	capinfo.fd = -1;

	if (init_camera(&capinfo) == 0)
	{
    		printf("...camera init'ed successfully\n");
		return 0;
	}else{
		printf("$$ camera initialization error!\n");
		return 1;
	}
}

/* ============================================================================
 * @Function: 	 cammodule_start
 * @Description: Start V4L2 capture interface.
 * ============================================================================
 */
int cammodule_start (void)
{
	if (start_camera(&capinfo) == 0)
	{
    		printf("...camera capturing started.\n");
		return 0;
	}else{
		printf("$$ camera start error!\n");
		return 1;
	}
}

/* ============================================================================
 * @Function: 	 cammodule_stop
 * @Description: Stop V4L2 capture interface.
 * ============================================================================
 */
int cammodule_stop (void)
{
	if (close_camera(&capinfo) == 0)
	{
    		printf("...camera closed.\n");
		return 0;
	}else{
		printf("$$ camera close error!\n");
		return 1;
	}
}

/* ============================================================================
 * @Function: 	 cammodule_getframe
 * @Description: Get and copy the video frame into a pointer.
 * ============================================================================
 */
int cammodule_getframe (char *data)
{
	int 	buf_no;
	int	height = capinfo.height;
	int 	width = capinfo.width;
	char 	*srcPlane;

	/*pointer of the frame captured by driver */
    	buf_no = get_camera_frame(&capinfo);
    	srcPlane = capinfo.userptr[buf_no];

    	memcpy(data, srcPlane, height*width*2);
	
	/*release the driver buffer */
    	put_camera_frame(&capinfo, buf_no);

	return 0;
}

/* ============================================================================
 * @Function: 	 cammodule_saveframe
 * @Description: Save the frame into a JPEG file.
 * ============================================================================
 */
int cammodule_saveframe (char *fileName)
{
	int 	buf_no;
	int	height = capinfo.height;
	int 	width = capinfo.width;
	char 	*srcPlane;

	/*pointer of the frame captured by driver */
    	buf_no = get_camera_frame(&capinfo);
    	srcPlane = capinfo.userptr[buf_no];
	
  	unsigned char* dst = malloc(width*height*3*sizeof(char));

  	// convert from YUV422 to RGB888
  	YUV422toRGB888(width,height,(unsigned char*)srcPlane,dst);

  	// write jpeg
	jpegFilename = fileName;
  	jpegWrite(dst);
	
	/*release the driver buffer */
    	put_camera_frame(&capinfo, buf_no);

	return 0;
}

/* ============================================================================
 * @Function: 	 YUV422toRGB888
 * @Description: Convert from YUV422 format to RGB888. Formulae are described 
 * 		 on http://en.wikipedia.org/wiki/YUV
 * ============================================================================
 */
static void YUV422toRGB888(int width, int height, unsigned char *src, unsigned char *dst)
{
  	int line, column;
  	unsigned char *py, *pu, *pv;
  	unsigned char *tmp = dst;

  	/* In this format each four bytes is two pixels. Each four bytes is two Y's, a Cb and a Cr. 
     	Each Y goes to one of the pixels, and the Cb and Cr belong to both pixels. */
  	py = src;
  	pu = src + 1;
  	pv = src + 3;
	
  	#define CLIP(x) ( (x)>=0xFF ? 0xFF : ( (x) <= 0x00 ? 0x00 : (x) ) )
	
  	for (line = 0; line < height; ++line) {
    		for (column = 0; column < width; ++column) {
      			*tmp++ = CLIP((double)*py + 1.402*((double)*pv-128.0));
      			*tmp++ = CLIP((double)*py - 0.344*((double)*pu-128.0) - 0.714*((double)*pv-128.0));      
      			*tmp++ = CLIP((double)*py + 1.772*((double)*pu-128.0));

      			// increase py every time
      			py += 2;
      			// increase pu,pv every second time
      			if ((column & 1)==1) {
        			pu += 4;
        			pv += 4;
      			}
    		}
 	}
	
	return;
}

/* ============================================================================
 * @Function: 	 jpegWrite
 * @Description: Write image to jpeg file.
 * ============================================================================
 */
static void jpegWrite(unsigned char* img)
{
  	struct jpeg_compress_struct cinfo;
  	struct jpeg_error_mgr jerr;
	
  	JSAMPROW row_pointer[1];
  	FILE *outfile = fopen( jpegFilename, "wb" );

  	// try to open file for saving
  	if (!outfile)
    		printf("error! fopen() jpeg..\n");

  	// create jpeg data
  	cinfo.err = jpeg_std_error( &jerr );
  	jpeg_create_compress(&cinfo);
  	jpeg_stdio_dest(&cinfo, outfile);

  	// set image parameters
  	cinfo.image_width = capinfo.width;	
  	cinfo.image_height = capinfo.height;
  	cinfo.input_components = 3;
  	cinfo.in_color_space = JCS_RGB;

  	// set jpeg compression parameters to default
  	jpeg_set_defaults(&cinfo);
  	// and then adjust quality setting
  	jpeg_set_quality(&cinfo, jpegQuality, TRUE);

  	// start compress 
  	jpeg_start_compress(&cinfo, TRUE);

  	// feed data
  	while (cinfo.next_scanline < cinfo.image_height) {
    		row_pointer[0] = &img[cinfo.next_scanline * cinfo.image_width *  cinfo.input_components];
    		jpeg_write_scanlines(&cinfo, row_pointer, 1);
  	}

  	// finish compression
  	jpeg_finish_compress(&cinfo);

  	// destroy jpeg data
  	jpeg_destroy_compress(&cinfo);

  	// close output file
  	fclose(outfile);

	return;
}

