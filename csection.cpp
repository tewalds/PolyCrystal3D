
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <stdint.h>
#include "gd.h"

using namespace std;

int main(int argc, char **argv){
	int size = (1<<10); //1024
	int height = size;
	int start = 0;
	int end = 0;

	for(unsigned int i = 1; i < (unsigned int)argc; i++){
		char * ptr = argv[i];
		if(strcmp(ptr, "-h") == 0 || strcmp(ptr, "--help") == 0){
			printf("Usage: %s <options>\n"
				"Ex: %s -d data -H 1024\n"
				"\t-h --help     Show this help\n"
				"\t-d --dir      Directory to dump output files [./]\n"
				"\t-s --size     Size of the layer maps [%d]\n"
 				"\t-H --height   Max Height [%d]\n"
 				"\t-A --start    First cross section [%d]\n"
 				"\t-B --end      Last cross section  [%d]\n"
				"\n",
				argv[0], argv[0], size, height, start, end);
			exit(255);
		} else if(strcmp(ptr, "-d") == 0 || strcmp(ptr, "--dir") == 0) {
			ptr = argv[++i];
			if(ptr == NULL) { printf("Please specify output directory\n"); exit(1); }
			if(chdir(ptr) == -1)  { printf("Couldn't switch directories to %s\n", ptr); exit(2); }
		} else if(strcmp(ptr, "-s") == 0 || strcmp(ptr, "--size") == 0) {
			ptr = argv[++i];
			if(ptr == NULL) { printf("Please specify the Size\n"); exit(1); }
			size = atoi(ptr);
			if(size < 1){ printf("Size out of range\n"); exit(2); }
		} else if(strcmp(ptr, "-H") == 0 || strcmp(ptr, "--height") == 0) {
			ptr = argv[++i];
			if(ptr == NULL) { printf("Please specify the Height\n"); exit(1); }
			height = atoi(ptr);
			if(height < 1){ printf("Height out of range\n"); exit(2); }
		} else if(strcmp(ptr, "-A") == 0 || strcmp(ptr, "--start") == 0) {
			ptr = argv[++i];
			if(ptr == NULL) { printf("Please specify the Start\n"); exit(1); }
			start = atoi(ptr);
		} else if(strcmp(ptr, "-B") == 0 || strcmp(ptr, "--end") == 0) {
			ptr = argv[++i];
			if(ptr == NULL) { printf("Please specify the End\n"); exit(1); }
			end = atoi(ptr);
		} else {
			printf("Unknown argument %s\n", ptr);
			exit(1);
		}
	}

	if(start < 0 || end >= size || start > end){
		printf("Start and end must be between 0 and %d, with start < end\n", size);
		exit(1);
	}

	for(int y = start; y <= end; y++){
		char filename[50];
		FILE *fd;

		gdImagePtr im = gdImageCreateTrueColor(size, height);
		gdImageFill(im, 0, 0, gdImageColorAllocate(im, 0, 0, 0));

		for(int h = 0; h < height; h++){
			sprintf(filename, "level.%05d.png", h);
			fd = fopen(filename, "rb");
			if(fd == NULL)
				break;
			gdImagePtr levelim = gdImageCreateFromPng(fd);
			fclose(fd);

			if(gdImageSX(levelim) != size || gdImageSY(levelim) != size){
				printf("File %s is not the expected size\n", filename);
				exit(1);
			}

			gdImageCopy(im, levelim, 0, height - h - 1, 0, y, size, 1); //copy one line into place

			gdImageDestroy(levelim);
		}

		sprintf(filename, "crosssection.%05d.png", y);
		fd = fopen(filename, "wb");
		gdImagePng(im, fd);
		fclose(fd);
		gdImageDestroy(im);
		printf(".");
	}
	printf("\n");
		
	return 0;
}

