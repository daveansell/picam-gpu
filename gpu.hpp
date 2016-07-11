#pragma once

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

	Mat image;
	Mat background;
	Mat outputImage, concat1, concat2;

	void imageToGpu();
	void gpuBlur();
	void gpuEdge();
	void clean();
public:
	GPU(std::string referencePath);
	~GPU();
	void runGpu();
	cv::Mat getGpuOutput();
}