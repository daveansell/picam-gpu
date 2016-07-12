
#include <stdio.h>
#include <unistd.h>
#include "camera.h"
#include <time.h>
#include <curses.h>
#include <iostream>
#include <sys/types.h>
#include <dirent.h>
#include <omp.h>
#include "gpu.hpp"

#define HYPERSCALE 4
using namespace std;
using namespace cv;

GPU::GPU(string referencePath){
	//initialize openGL
	InitGraphics();
	cout << "graphics initialized" << endl;

	ytexture.CreateGreyScale(PIC_WIDTH/2,MAIN_TEXTURE_HEIGHT);
	ytexture2.CreateGreyScale(PIC_WIDTH/2,MAIN_TEXTURE_HEIGHT);
		
	
	greysobeltexture.CreateRGBA(PIC_WIDTH/2, MAIN_TEXTURE_HEIGHT);
	greysobeltexture.GenerateFrameBuffer();

	greysobeltexture2.CreateRGBA(PIC_WIDTH/2, MAIN_TEXTURE_HEIGHT);
	greysobeltexture2.GenerateFrameBuffer();
	

	mediantexture.CreateRGBA(PIC_WIDTH/2, MAIN_TEXTURE_HEIGHT);
	mediantexture.GenerateFrameBuffer();

	mediantexture2.CreateRGBA(PIC_WIDTH/2, MAIN_TEXTURE_HEIGHT);
	mediantexture2.GenerateFrameBuffer();


	background = imread(referencePath);
	
	if (background.empty()){
		cout << "reference image not found" << endl;
	}
	int imgSize = background.total() * HYPERSCALE;
	
	
	//Allocate buffers for pixel Data and U and V fillings
	pixelData = new uchar[imgSize/2];
	pixelData2 = new uchar[imgSize/2];
	
	//outputImage = background.clone();


	//Fixes bit allignment problems
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	
	//Allocate output pixel data
	//output1 = malloc(greysobeltexture.GetWidth()*greysobeltexture.GetHeight()*4 * sizeof(char));
	//output2 = malloc(greysobeltexture.GetWidth()*greysobeltexture.GetHeight()*4 * sizeof(char));
	output1 = output2 = NULL;
	

}

//Input: Saturation channel from HSV image
//Currently bugged
void GPU::imageToGpu(){
	clock_t read_start = clock();
	
	imwrite("imagetogpu.jpg", image);

	// Pre load pixel data
	printf("starting pixel data filling\n");
	for(int i = 0; i < MAIN_TEXTURE_HEIGHT; i++){
		//printf(">>> %d\n", i);
		for(int j = 0; j < PIC_WIDTH; j++){
			if (j < PIC_WIDTH/2)
				pixelData[PIC_WIDTH/2*i + j] = image.at<uchar>(i, j);
			else
				pixelData2[PIC_WIDTH/2*i + (j - PIC_WIDTH/2)] = image.at<uchar>(i, j);	
			//printf("%d ", j);		
		}		
	}
	printf("after pixel data\n");		

	pic_data = (void*)pixelData;
	pic_data2 = (void*)pixelData2;
	printf("time passed for reading image: %f\n", (float)(clock() - read_start)/CLOCKS_PER_SEC );


	//Pass data to texture
	const uint8_t* data = (const uint8_t*)pic_data;
	const uint8_t* data2 = (const uint8_t*)pic_data2;
	ytexture.SetPixels(data);
	ytexture2.SetPixels(data2);
}

void GPU::gpuBlur()
{
	
	clock_t begin_proc = clock();
	DrawMedianRect(&ytexture,-1.f,-1.f,1.f,1.f,&mediantexture);
	printf("median time: %f\n", (float)(clock() - begin_proc)/CLOCKS_PER_SEC );
	DrawMedianRect(&ytexture2,-1.f,-1.f,1.f,1.f,&mediantexture2);
	printf("median time: %f\n", (float)(clock() - begin_proc)/CLOCKS_PER_SEC );

	clock_t debug_time = clock();
	mediantexture.Save();
	mediantexture2.Save();
	
	imwrite("median1.jpg", mediantexture.cvImage);

	imwrite("median2.jpg", mediantexture2.cvImage);

	cout << "concats generated with save, merging..." << endl;
	
	hconcat(mediantexture.cvImage, mediantexture2.cvImage, outputImage);	
	printf("concat time: %f\n", (float)(clock() - debug_time)/CLOCKS_PER_SEC );	
}

//Only call after filling mediantexture
void GPU::gpuEdge(){
	DrawSobelRect(&mediantexture,-1.f,-1.f,1.f,1.f,&greysobeltexture);
	DrawSobelRect(&mediantexture2,-1.f,-1.f,1.f,1.f,&greysobeltexture2);
	/*concat1 = greysobeltexture.Save("tex_sobel.png", "sobel", output1);
	concat2 = greysobeltexture2.Save("tex_sobel2.png", "sobel", output2);
	hconcat(concat1, concat2, outputImage);		
*/
}

Mat GPU::getGpuOutput(){
	return outputImage;
}

void GPU::clean(){
	free(output1);
	free(output2);
	free(pixelData);
	free(pixelData2);
}



GPU::~GPU(){

	clean();
}

void GPU::runGpu(){
	
	//
	imageToGpu();
	gpuBlur();
	
	//gpuEdge();
}

//Simulating RockCV executing
/*
int main(){

	GPU gpu("background.jpg");

	//Suppose those are rock cv steps
	cvtColor(gpu.background, gpu.image, CV_BGR2GRAY);

	//Not going to work
	gpu.runGpu();

	imwrite("class.jpg", gpu.getGpuOutput());	
}*/
