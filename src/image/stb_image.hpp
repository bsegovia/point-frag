#ifndef __STB_IMAGE_HPP__
#define __STB_IMAGE_HPP__

#include <cstdio>

typedef unsigned char stbi_uc;

/*! Load an image from a memory buffer */
extern stbi_uc *stbi_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);

/*! Load an image from a file using a file name */
extern stbi_uc *stbi_load(char const *filename,int *x, int *y, int *comp, int req_comp);

/*! Load an image from a FILE handle */
extern stbi_uc *stbi_load_from_file(FILE *f, int *x, int *y, int *comp, int req_comp);

/*! Write a png image into file "filename" */
extern int stbi_write_png(char const *filename, int w, int h, int comp, const void *data, int stride_in_bytes);

/*! Write a bmp image into file "filename" */
extern int stbi_write_bmp(char const *filename, int w, int h, int comp, const void *data);

/*! Write a tga image into file "filename" */
extern int stbi_write_tga(char const *filename, int w, int h, int comp, const void *data);

#endif /* __STB_IMAGE_HPP__ */

