#pragma once


#include <string>
#include <opencv2/opencv.hpp>

#include "graphics.h"

#define PIC_WIDTH 2592
#define PIC_HEIGHT 1944

#define HSIZE 1
#define WSIZE 0.79012345678901234 //scales pic to maximum texture width, not in use.
const int MAIN_TEXTURE_WIDTH = PIC_WIDTH*WSIZE;
const int MAIN_TEXTURE_HEIGHT = PIC_HEIGHT*HSIZE;

class GPU{
private:
	GfxTexture ytexture, greysobeltexture, mediantexture;
	GfxTexture ytexture2, greysobeltexture2, mediantexture2;


	void *pic_data;
	void *pic_data2;

	uchar * pixelData;
	uchar * pixelData2;
	uchar * zeros;

	void * output1;
	void * output2;

	
	cv::Mat outputImage, concat1, concat2;

	void imageToGpu();
	void gpuBlur();
	void gpuEdge();
	void clean();
public:
	cv::Mat image;
	cv::Mat background;
	GPU(){};
	GPU(std::string referencePath);
	~GPU();
	void runGpu();
	cv::Mat getGpuOutput();
};
