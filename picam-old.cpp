#include <stdio.h>
#include <unistd.h>
#include "camera.h"
#include "graphics.h"
#include <time.h>
#include <curses.h>
#include <iostream>
#include<string>
#include <opencv2/opencv.hpp>
#include <sys/types.h>
#include <dirent.h>
#include <omp.h>



#define PIC_WIDTH 2592
#define PIC_HEIGHT 1944

#define HSIZE 1
#define WSIZE 0.79012345678901234

//#define MAIN_TEXTURE_WIDTH PIC_WIDTH*DSIZE
//#define MAIN_TEXTURE_HEIGHT PIC_HEIGHT*DSIZE


const int MAIN_TEXTURE_WIDTH = PIC_WIDTH*WSIZE;
const int MAIN_TEXTURE_HEIGHT = PIC_HEIGHT*HSIZE;

#define TEXTURE_GRID_COLS 2
#define TEXTURE_GRID_ROWS 2
#define LOAD_ERROR -1

const char *input_dir = "/home/pi/dataset/thousand";

using namespace std;
using namespace cv;
char tmpbuff[MAIN_TEXTURE_WIDTH*MAIN_TEXTURE_HEIGHT*4];

bool ends_with(const std::string &suffix, const std::string &str)
{
    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}


/*Intended pipeline of this function: 
	-Get image (read or camera) (ok)
	-Background subtract (?)
	-Blur (ok, kernel size?)
	-Edge (sobel working, need canny)
	-Hough (have implementation in graphics.cpp, not tested)
	-Return circles
*/
int main(int argc, const char **argv)
{
	//initialize graphics and the camera
	InitGraphics();
	cout << "graphics initialized" << endl;
	//CCamera* cam = StartCamera(MAIN_TEXTURE_WIDTH, MAIN_TEXTURE_HEIGHT,15,1,false);
	//cout << "camera initialized" << endl;
	
	//create 4 textures of decreasing size
	//declare texture holders
	GfxTexture ytexture, greysobeltexture, mediantexture;
	GfxTexture ytexture2, greysobeltexture2, mediantexture2;
	//GfxTexture utexture,vtexture,rgbtextures[32],blurtexture,sobeltexture,redtexture,dilatetexture,erodetexture,threshtexture;
	ytexture.CreateGreyScale(PIC_WIDTH/2,MAIN_TEXTURE_HEIGHT);
	ytexture2.CreateGreyScale(PIC_WIDTH/2,MAIN_TEXTURE_HEIGHT);
		
	//utexture.CreateGreyScale(MAIN_TEXTURE_WIDTH/2,MAIN_TEXTURE_HEIGHT/2);
	//vtexture.CreateGreyScale(MAIN_TEXTURE_WIDTH/2,MAIN_TEXTURE_HEIGHT/2);

	/*GfxTexture yreadtexture,ureadtexture,vreadtexture;
	yreadtexture.CreateRGBA(MAIN_TEXTURE_WIDTH,MAIN_TEXTURE_HEIGHT);
	yreadtexture.GenerateFrameBuffer();
	ureadtexture.CreateRGBA(MAIN_TEXTURE_WIDTH/2,MAIN_TEXTURE_HEIGHT/2);
	ureadtexture.GenerateFrameBuffer();
	vreadtexture.CreateRGBA(MAIN_TEXTURE_WIDTH/2,MAIN_TEXTURE_HEIGHT/2);
	vreadtexture.GenerateFrameBuffer();
	*/
	GfxTexture* texture_grid[TEXTURE_GRID_COLS*TEXTURE_GRID_ROWS];
	memset(texture_grid,0,sizeof(texture_grid));
	int next_texture_grid_entry = 0;
	//texture_grid[next_texture_grid_entry++] = &ytexture;
	//texture_grid[next_texture_grid_entry++] = &utexture;
	//texture_grid[next_texture_grid_entry++] = &vtexture;
	
	/*int levels=1;
	while( (MAIN_TEXTURE_WIDTH>>levels)>16 && (MAIN_TEXTURE_HEIGHT>>levels)>16 && (levels+1)<32)
		levels++;
	printf("Levels used: %d, smallest level w/h: %d/%d\n",levels,MAIN_TEXTURE_WIDTH>>(levels-1),MAIN_TEXTURE_HEIGHT>>(levels-1));
	
	//Create multiple levels of downscaling RGB
	for(int i = 0; i < levels; i++)
	{
		rgbtextures[i].CreateRGBA(MAIN_TEXTURE_WIDTH>>i,MAIN_TEXTURE_HEIGHT>>i);
		rgbtextures[i].GenerateFrameBuffer();
		//texture_grid[next_texture_grid_entry++] = &rgbtextures[i];
	}*/
	//THOSE ARE INTERESTING//
	/*blurtexture.CreateRGBA(MAIN_TEXTURE_WIDTH/2,MAIN_TEXTURE_HEIGHT/2);
	blurtexture.GenerateFrameBuffer();
	texture_grid[0] = &blurtexture;

	sobeltexture.CreateRGBA(MAIN_TEXTURE_WIDTH/2,MAIN_TEXTURE_HEIGHT/2);
	sobeltexture.GenerateFrameBuffer();
	texture_grid[1] = &sobeltexture;
	*/
	greysobeltexture.CreateRGBA(PIC_WIDTH/2, MAIN_TEXTURE_HEIGHT);
	greysobeltexture.GenerateFrameBuffer();
	texture_grid[0] = &greysobeltexture;

	greysobeltexture2.CreateRGBA(PIC_WIDTH/2, MAIN_TEXTURE_HEIGHT);
	greysobeltexture2.GenerateFrameBuffer();
	texture_grid[1] = &greysobeltexture2;

	mediantexture.CreateRGBA(PIC_WIDTH/2, MAIN_TEXTURE_HEIGHT);
	mediantexture.GenerateFrameBuffer();
	texture_grid[2] = &mediantexture;

	mediantexture2.CreateRGBA(PIC_WIDTH/2, MAIN_TEXTURE_HEIGHT);
	mediantexture2.GenerateFrameBuffer();
	texture_grid[3] = &mediantexture2;
	
	//THOSE ARE INTERESTING//
/*
	redtexture.CreateRGBA(MAIN_TEXTURE_WIDTH/2,MAIN_TEXTURE_HEIGHT/2);
	redtexture.GenerateFrameBuffer();
	texture_grid[next_texture_grid_entry++] = &redtexture;

	dilatetexture.CreateRGBA(MAIN_TEXTURE_WIDTH/2,MAIN_TEXTURE_HEIGHT/2);
	dilatetexture.GenerateFrameBuffer();
	texture_grid[next_texture_grid_entry++] = &dilatetexture;

	erodetexture.CreateRGBA(MAIN_TEXTURE_WIDTH/2,MAIN_TEXTURE_HEIGHT/2);
	erodetexture.GenerateFrameBuffer();
	texture_grid[next_texture_grid_entry++] = &erodetexture;

	threshtexture.CreateRGBA(MAIN_TEXTURE_WIDTH/2,MAIN_TEXTURE_HEIGHT/2);
	threshtexture.GenerateFrameBuffer();
	texture_grid[next_texture_grid_entry++] = &threshtexture;
*/

	float texture_grid_col_size = 2.f/TEXTURE_GRID_COLS;
	float texture_grid_row_size = 2.f/TEXTURE_GRID_ROWS;
	int selected_texture = -1;

	printf("Running frame loop\n");
	
	//read start time
	/*long int start_time;
	long int time_difference;
	struct timespec gettime_now;
	clock_gettime(CLOCK_REALTIME, &gettime_now);
	start_time = gettime_now.tv_nsec ;
	double total_time_s = 0;
	*/	
	bool do_pipeline = false;
	
	initscr();      /* initialize the curses library */
	keypad(stdscr, TRUE);  /* enable keyboard mapping */
	nonl();         /* tell curses not to do NL->CR/NL on output */
	cbreak();       /* take input chars one at a time, no wait for \n */
	clear();
	nodelay(stdscr, TRUE);

	// MY CODE TRYING TO DO SOMETHING
	void *pic_data;
	void *pic_data2;
	vector<Mat> channels;
	//loadImage("background.jpg", pic_data);
	Mat image = imread("background.jpg");
	Mat outputImage, concat1, concat2;
	Mat centres;
	//cvtColor(image, image, CV_BGR2GRAY);
	if (image.empty()){
		cout << "reference image not found" << endl;
		return LOAD_ERROR; 	
	}
	int imgSize = image.total();
	
	//printf("image.total: %d, image.rows*i + j: %d\n", imgSize, image.rows*image.cols);

	//Allocate buffers for pixel Data and U and V fillings
	uchar * pixelData = new uchar[imgSize/2];
	uchar * pixelData2 = new uchar[imgSize/2];
	//uchar * y = new uchar[imgSize];
	//uchar * u = new uchar[imgSize];
	//uchar * v = new uchar[imgSize];
	uchar* zeros = new uchar[imgSize];
	
	for(int i =0; i < image.rows; i++)
		for(int j = 0; j < image.cols; j++){
			zeros[image.cols*i + j] = 125;
		}


	//Sort files 
	 //Array of file names
	vector<String> all_files;
	//Pointer to input directory
	DIR *dir_ptr = opendir(input_dir);
	while (true) {
		dirent *dir_ent = readdir(dir_ptr);

		if (NULL == dir_ent) {
			break;
		}

		all_files.push_back(string(dir_ent->d_name));
	}
	closedir(dir_ptr);

	//Sort file names 
	sort(all_files.begin(), all_files.end());

	vector<String> image_files;
	for (String file : all_files) {
		if (ends_with(".jpg", file)) {
			image_files.push_back(file);
		}
	}
	//
	//for(int i = 0; i < 1000; i++)
	int loop = 0;
	printf("starting loop\n");

	//Fixes bit allignment problems
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	
	void * sobel1 = malloc(greysobeltexture.GetWidth()*greysobeltexture.GetHeight()*4);
	void * sobel2 = malloc(greysobeltexture.GetWidth()*greysobeltexture.GetHeight()*4);
	for (String file : image_files)
	{	
		clock_t read_start = clock();
		image = imread(string(input_dir) + "/" + file, 1);
		
		//resize(image, image, Size(), WSIZE, HSIZE);
		cvtColor(image, image, CV_BGR2HSV);
		split(image, channels);
		image = channels[1];
				
		if (image.empty()){
			cout << "image not found" << endl;
			return LOAD_ERROR; 	
		}
		
		// Pre load pixel data
		printw("starting pixel data filling\n");
		for(int i = 0; i < MAIN_TEXTURE_HEIGHT; i++){
			for(int j = 0; j < PIC_WIDTH; j++){
				if (j < PIC_WIDTH/2)
					pixelData[PIC_WIDTH/2*i + j] = image.at<uchar>(i, j);
				else
					pixelData2[PIC_WIDTH/2*i + (j - PIC_WIDTH/2)] = image.at<uchar>(i, j);	
			}		
		}
		printw("after pixel data\n");		

		pic_data = (void*)pixelData;
		pic_data2 = (void*)pixelData2;
		printw("time passed for reading image: %f\n", (float)(clock() - read_start)/CLOCKS_PER_SEC );

		int ch = getch();
		if(ch != ERR)
		{
			if(ch == 'q')
				break;
			else if(ch == 'a')
				selected_texture = (selected_texture+1)%(TEXTURE_GRID_ROWS*TEXTURE_GRID_ROWS);
			else if(ch == 's')
				selected_texture = -1;
			else if(ch == 'd')
				do_pipeline = !do_pipeline;
			else if(ch == 'w')
			{
				//SaveFrameBuffer("tex_fb.png");
				//yreadtexture.Save("tex_y.png");
				//ureadtexture.Save("tex_u.png");
				//vreadtexture.Save("tex_v.png");
				//rgbtextures[0].Save("tex_rgb_hi.png");
				//rgbtextures[levels-1].Save("tex_rgb_low.png");
				//mediantexture.Save("tex_blur.png", "blur", image);
				//blurtexture.Save("tex_blur.png", "blur");
				//sobeltexture.Save("tex_sobel.png", "sobel");
				//greysobeltexture.Save("tex_sobel.png", "sobel", image);				
				//dilatetexture.Save("tex_dilate.png");
				//erodetexture.Save("tex_erode.png");
				//threshtexture.Save("tex_thresh.png");
			}

			move(0,0);
			refresh();
		}
		
		//spin until we have a camera frame
		//const void* frame_data; int frame_sz; while(!cam->BeginReadFrame(0,frame_data,frame_sz)) {};

		//Show frame data
		//lock the chosen frame buffer, and copy it directly into the corresponding open gl texture
		{
			//const uint8_t* data = (const uint8_t*)frame_data;
			const uint8_t* data = (const uint8_t*)pic_data;
			const uint8_t* data2 = (const uint8_t*)pic_data2;
			//int ypitch = MAIN_TEXTURE_WIDTH;
			//int ysize = ypitch*MAIN_TEXTURE_HEIGHT;
			//int uvpitch = MAIN_TEXTURE_WIDTH/2;
			//int uvsize = uvpitch*MAIN_TEXTURE_HEIGHT/2;
			//int upos = ysize+16*uvpitch;
			//int vpos = upos+uvsize+4*uvpitch;
			
			//int upos = ysize;
			//int upos = 0;
			//int vpos = upos+uvsize;
			//int vpos = 0;
			//printf("Frame data len: 0x%x, ypitch: 0x%x ysize: 0x%x, uvpitch: 0x%x, uvsize: 0x%x, u at 0x%x, v at 0x%x, total 0x%x\n", frame_sz, ypitch, ysize, uvpitch, uvsize, upos, vpos, vpos+uvsize);
			ytexture.SetPixels(data);
			ytexture2.SetPixels(data2);
			//utexture.SetPixels(data+upos);
			//utexture.SetPixels(zeros);
			//vtexture.SetPixels(data+vpos);
			//vtexture.SetPixels(zeros);
			//cam->EndReadFrame(0);
		}

		//begin frame, draw the texture then end frame (the bit of maths just fits the image to the screen while maintaining aspect ratio)
		
		//float aspect_ratio = float(MAIN_TEXTURE_WIDTH)/float(MAIN_TEXTURE_HEIGHT);
		//float screen_aspect_ratio = 1280.f/720.f;
		//for(int texidx = 0; texidx<levels; texidx++)
		//	DrawYUVTextureRect(&ytexture,&utexture,&vtexture,-1.f,-1.f,1.f,1.f,&rgbtextures[texidx]);

		//these are just here so we can access the yuv data cpu side - opengles doesn't let you read grey ones cos they can't be frame buffers!
		//DrawTextureRect(&ytexture,-1,-1,1,1,&yreadtexture);
		//DrawTextureRect(&utexture,-1,-1,1,1,&ureadtexture);
		//DrawTextureRect(&vtexture,-1,-1,1,1,&vreadtexture);

		//Show midsteps
		if(do_pipeline)
		{
			BeginFrame();
			//if(selected_texture == -1 || &blurtexture == texture_grid[selected_texture])
			//	DrawBlurredRect(&rgbtextures[1],-1.f,-1.f,1.f,1.f,&blurtexture);

			//if(selected_texture == -1 || &sobeltexture == texture_grid[selected_texture])
			//	DrawSobelRect(&rgbtextures[1],-1.f,-1.f,1.f,1.f,&sobeltexture);

			
			//if(selected_texture == -1 || &mediantexture == texture_grid[selected_texture])
			//	DrawMedianRect(&rgbtextures[1],-1.f,-1.f,1.f,1.f,&mediantexture);

			//if(selected_texture == -1 || &greysobeltexture == texture_grid[selected_texture])
			//	DrawSobelRect(&ytexture,-1.f,-1.f,1.f,1.f,&greysobeltexture);
	
			if(selected_texture == -1 || &mediantexture == texture_grid[selected_texture])
				DrawMedianRect(&ytexture,-1.f,-1.f,1.f,1.f,&mediantexture);
			if(selected_texture == -1 || &mediantexture2 == texture_grid[selected_texture])
				DrawMedianRect(&ytexture2,-1.f,-1.f,1.f,1.f,&mediantexture2);
			

			if(selected_texture == -1 || &greysobeltexture == texture_grid[selected_texture])
				DrawSobelRect(&mediantexture,-1.f,-1.f,1.f,1.f,&greysobeltexture);
			if(selected_texture == -1 || &greysobeltexture2 == texture_grid[selected_texture])
				DrawSobelRect(&mediantexture2,-1.f,-1.f,1.f,1.f,&greysobeltexture2);

			
			/*if(selected_texture == -1 || &redtexture == texture_grid[selected_texture])
				//DrawMultRect(&rgbtextures[1],-1.f,-1.f,1.f,1.f,1,0,0,&redtexture);

			if(selected_texture == -1 || &dilatetexture == texture_grid[selected_texture])
				//DrawDilateRect(&rgbtextures[1],-1.f,-1.f,1.f,1.f,&dilatetexture);

			if(selected_texture == -1 || &erodetexture == texture_grid[selected_texture])
				//DrawErodeRect(&rgbtextures[1],-1.f,-1.f,1.f,1.f,&erodetexture);

			if(selected_texture == -1 || &threshtexture == texture_grid[selected_texture])
				//DrawThreshRect(&ytexture,-1.f,-1.f,1.f,1.f,0.25f,0.25f,0.25f,&threshtexture);
			*/
			if(selected_texture == -1)
			{
				for(int col = 0; col < TEXTURE_GRID_COLS; col++)
				{
					for(int row = 0; row < TEXTURE_GRID_ROWS; row++)
					{		
						if(GfxTexture* tex = texture_grid[col+row*TEXTURE_GRID_COLS])
						{
							float colx = -1.f+col*texture_grid_col_size;
							float rowy = -1.f+row*texture_grid_row_size;
							DrawTextureRect(tex,colx,rowy,colx+texture_grid_col_size,rowy+texture_grid_row_size,NULL);
						}				
					}
				}
			}
			else
			{
				if(GfxTexture* tex = texture_grid[selected_texture])
					DrawTextureRect(tex,-1,-1,1,1,NULL);
			}
			
			EndFrame();
		}
		//Or just do the processing
		else
		{
			clock_t begin_proc = clock();
			DrawMedianRect(&ytexture,-1.f,-1.f,1.f,1.f,&mediantexture);
			printw("median time: %f\n", (float)(clock() - begin_proc)/CLOCKS_PER_SEC );
			DrawSobelRect(&mediantexture,-1.f,-1.f,1.f,1.f,&greysobeltexture);
			printw("sobel time: %f\n", (float)(clock() - begin_proc)/CLOCKS_PER_SEC );
			DrawMedianRect(&ytexture2,-1.f,-1.f,1.f,1.f,&mediantexture2);
			printw("median time: %f\n", (float)(clock() - begin_proc)/CLOCKS_PER_SEC );
			DrawSobelRect(&mediantexture2,-1.f,-1.f,1.f,1.f,&greysobeltexture2);
			printw("sobel time: %f\n", (float)(clock() - begin_proc)/CLOCKS_PER_SEC );
			//DrawErodeRect(&sobeltexture,-1.f,-1.f,1.f,1.f,&erodetexture);
			//DrawDilateRect(&erodetexture,-1.f,-1.f,1.f,1.f,&dilatetexture);
			//DrawThreshRect(&erodetexture,-1.f,-1.f,1.f,1.f,0.05f,0.05f,0.05f,&threshtexture);
			//DrawTextureRect(&threshtexture,-1,-1,1,1,NULL);

			clock_t debug_time = clock();
			if (debug){			
				mediantexture.Save("tex_blur.png", "blur", NULL);
				mediantexture2.Save("tex_blur2.png", "blur", NULL);
			}			
			concat1 = greysobeltexture.Save("tex_sobel.png", "sobel", sobel1);
			concat2 = greysobeltexture2.Save("tex_sobel2.png", "sobel", sobel2);	
			hconcat(concat1, concat2, outputImage);		
			if (debug) imwrite("out.jpg", outputImage);	
			printw("concat time: %f\n", (float)(clock() - debug_time)/CLOCKS_PER_SEC );	
			
			clock_t hought = clock();
			//HoughTransform(outputImage, centres);
			printw("hough time: %f\n", (float)(clock() - hought)/CLOCKS_PER_SEC );	
			
		}
		
			
			
		//read current time
		/*clock_gettime(CLOCK_REALTIME, &gettime_now);
		time_difference = gettime_now.tv_nsec - start_time;
		if(time_difference < 0) time_difference += 1000000000;
		total_time_s += double(time_difference)/1000000000.0;
		start_time = gettime_now.tv_nsec;

		//print frame rate
		float fr = float(double((loop++)+1)/total_time_s);
		if((loop%30)==0)
		{
			mvprintw(0,0,"Framerate: %g\n",fr);
			move(0,0);
			refresh();
		}*/
		printw("time passed for whole frame: %f\n", (float)(clock() - read_start)/CLOCKS_PER_SEC );
	}

	//StopCamera();
	free(sobel1);
	free(sobel2);
	//End curses library
	endwin();
}
