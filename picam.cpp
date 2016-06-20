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
//#define MAIN_TEXTURE_WIDTH 768
//#define MAIN_TEXTURE_HEIGHT 512

#define PIC_WIDTH 2592
#define PIC_HEIGHT 1944

#define DSIZE 0.5

//#define MAIN_TEXTURE_WIDTH PIC_WIDTH*DSIZE
//#define MAIN_TEXTURE_HEIGHT PIC_HEIGHT*DSIZE

const int MAIN_TEXTURE_WIDTH = PIC_WIDTH*DSIZE;
const int MAIN_TEXTURE_HEIGHT = PIC_HEIGHT*DSIZE;

#define TEXTURE_GRID_COLS 4
#define TEXTURE_GRID_ROWS 4
#define LOAD_ERROR -1

const char *input_dir = "/home/pi/dataset/thousand";

using namespace std;
using namespace cv;
char tmpbuff[MAIN_TEXTURE_WIDTH*MAIN_TEXTURE_HEIGHT*4];
/*
int loadImage(string path, const void* data){
	//cout << path << endl;
	Mat image = imread(path);
	cvtColor(image, image, CV_BGR2YUV);	
	//cvtColor(image, image, CV_BGR2GRAY);
	if (image.empty()){
		cout << "image not found" << endl;
		return LOAD_ERROR; 	
	}
	int imgSize = image.total();

	uchar * pixelData = new uchar[imgSize];
	
	for(int i =0; i < image.rows; i++)
		for(int j = 0; j < image.cols; j++)
			pixelData[image.rows*i + j] = image.at<uchar>(i, j);

	namedWindow("dont lag pls", WINDOW_NORMAL);
	imshow("dont lag pls", image);
	waitKey(10000);
	
	data = (void*)pixelData;
	return 0;
}*/
bool ends_with(const std::string &suffix, const std::string &str)
{
    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}
//entry point
int main(int argc, const char **argv)
{
	//init graphics and the camera
	InitGraphics();
	cout << "graphics initialized" << endl;
	//CCamera* cam = StartCamera(MAIN_TEXTURE_WIDTH, MAIN_TEXTURE_HEIGHT,15,1,false);
	//cout << "camera initialized" << endl;
	//create 4 textures of decreasing size
	GfxTexture ytexture,utexture,vtexture,rgbtextures[32],blurtexture,greysobeltexture,sobeltexture,mediantexture,redtexture,dilatetexture,erodetexture,threshtexture;
	ytexture.CreateGreyScale(MAIN_TEXTURE_WIDTH,MAIN_TEXTURE_HEIGHT);
	utexture.CreateGreyScale(MAIN_TEXTURE_WIDTH/2,MAIN_TEXTURE_HEIGHT/2);
	vtexture.CreateGreyScale(MAIN_TEXTURE_WIDTH/2,MAIN_TEXTURE_HEIGHT/2);

	GfxTexture yreadtexture,ureadtexture,vreadtexture;
	yreadtexture.CreateRGBA(MAIN_TEXTURE_WIDTH,MAIN_TEXTURE_HEIGHT);
	yreadtexture.GenerateFrameBuffer();
	ureadtexture.CreateRGBA(MAIN_TEXTURE_WIDTH/2,MAIN_TEXTURE_HEIGHT/2);
	ureadtexture.GenerateFrameBuffer();
	vreadtexture.CreateRGBA(MAIN_TEXTURE_WIDTH/2,MAIN_TEXTURE_HEIGHT/2);
	vreadtexture.GenerateFrameBuffer();

	GfxTexture* texture_grid[TEXTURE_GRID_COLS*TEXTURE_GRID_ROWS];
	memset(texture_grid,0,sizeof(texture_grid));
	int next_texture_grid_entry = 0;

	texture_grid[next_texture_grid_entry++] = &ytexture;
	texture_grid[next_texture_grid_entry++] = &utexture;
	texture_grid[next_texture_grid_entry++] = &vtexture;

	int levels=1;
	while( (MAIN_TEXTURE_WIDTH>>levels)>16 && (MAIN_TEXTURE_HEIGHT>>levels)>16 && (levels+1)<32)
		levels++;
	printf("Levels used: %d, smallest level w/h: %d/%d\n",levels,MAIN_TEXTURE_WIDTH>>(levels-1),MAIN_TEXTURE_HEIGHT>>(levels-1));

	//Create multiple levels of downscaling RGB
	for(int i = 0; i < levels; i++)
	{
		rgbtextures[i].CreateRGBA(MAIN_TEXTURE_WIDTH>>i,MAIN_TEXTURE_HEIGHT>>i);
		rgbtextures[i].GenerateFrameBuffer();
		texture_grid[next_texture_grid_entry++] = &rgbtextures[i];
	}

	blurtexture.CreateRGBA(MAIN_TEXTURE_WIDTH/2,MAIN_TEXTURE_HEIGHT/2);
	blurtexture.GenerateFrameBuffer();
	texture_grid[next_texture_grid_entry++] = &blurtexture;

	sobeltexture.CreateRGBA(MAIN_TEXTURE_WIDTH/2,MAIN_TEXTURE_HEIGHT/2);
	sobeltexture.GenerateFrameBuffer();
	texture_grid[next_texture_grid_entry++] = &sobeltexture;

	greysobeltexture.CreateRGBA(MAIN_TEXTURE_WIDTH/2,MAIN_TEXTURE_HEIGHT/2);
	greysobeltexture.GenerateFrameBuffer();
	texture_grid[next_texture_grid_entry++] = &greysobeltexture;

	mediantexture.CreateRGBA(MAIN_TEXTURE_WIDTH/2,MAIN_TEXTURE_HEIGHT/2);
	mediantexture.GenerateFrameBuffer();
	texture_grid[next_texture_grid_entry++] = &mediantexture;

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


	float texture_grid_col_size = 2.f/TEXTURE_GRID_COLS;
	float texture_grid_row_size = 2.f/TEXTURE_GRID_ROWS;
	int selected_texture = -1;

	printf("Running frame loop\n");

	//read start time
	long int start_time;
	long int time_difference;
	struct timespec gettime_now;
	clock_gettime(CLOCK_REALTIME, &gettime_now);
	start_time = gettime_now.tv_nsec ;
	double total_time_s = 0;
	bool do_pipeline = false;

	initscr();      /* initialize the curses library */
	keypad(stdscr, TRUE);  /* enable keyboard mapping */
	nonl();         /* tell curses not to do NL->CR/NL on output */
	cbreak();       /* take input chars one at a time, no wait for \n */
	clear();
	nodelay(stdscr, TRUE);

	// MY CODE TRYING TO DO SOMETHING
	void *pic_data;
	//loadImage("background.jpg", pic_data);
	Mat image = imread("background.jpg");
	cvtColor(image, image, CV_BGR2GRAY);
	if (image.empty()){
		cout << "image not found" << endl;
		return LOAD_ERROR; 	
	}
	int imgSize = image.total();
	
	printf("image.total: %d, image.rows*i + j: %d\n", imgSize, image.rows*image.cols);
	
	sleep(1);

	uchar * pixelData = new uchar[imgSize];
	uchar* zeros = new uchar[imgSize];

	for(int i =0; i < image.rows; i++)
		for(int j = 0; j < image.cols; j++){
			pixelData[image.cols*i + j] = image.at<uchar>(i, j);
			zeros[image.cols*i + j] = 120;
		}

	pic_data = (void*)pixelData;
	


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
	for (String file : image_files)
	{	
		image = imread(string(input_dir) + "/" + file, 1);
		
		cvtColor(image, image, CV_BGR2GRAY);
		
		resize(image, image, Size(), DSIZE, DSIZE);		
		if (image.empty()){
			cout << "image not found" << endl;
			return LOAD_ERROR; 	
		}
		
		// Pre load pixel data
		printf("starting pixel data filling\n");
		for(int i = 0; i < MAIN_TEXTURE_HEIGHT; i++)
			for(int j = 0; j < MAIN_TEXTURE_WIDTH; j++){
				pixelData[MAIN_TEXTURE_WIDTH*i + j] = image.at<uchar>(i, j);
			}		
		printf("after pixel data\n");		

		pic_data = (void*)pixelData;

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
				SaveFrameBuffer("tex_fb.png");
				yreadtexture.Save("tex_y.png");
				ureadtexture.Save("tex_u.png");
				vreadtexture.Save("tex_v.png");
				rgbtextures[0].Save("tex_rgb_hi.png");
				rgbtextures[levels-1].Save("tex_rgb_low.png");
				blurtexture.Save("tex_blur.png");
				sobeltexture.Save("tex_sobel.png");
				dilatetexture.Save("tex_dilate.png");
				erodetexture.Save("tex_erode.png");
				threshtexture.Save("tex_thresh.png");
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

			int ypitch = MAIN_TEXTURE_WIDTH;
			int ysize = ypitch*MAIN_TEXTURE_HEIGHT;
			int uvpitch = MAIN_TEXTURE_WIDTH/2;
			int uvsize = uvpitch*MAIN_TEXTURE_HEIGHT/2;
			//int upos = ysize+16*uvpitch;
			//int vpos = upos+uvsize+4*uvpitch;
			
			int upos = ysize;
			//int upos = 0;
			int vpos = upos+uvsize;
			//int vpos = 0;
			//printf("Frame data len: 0x%x, ypitch: 0x%x ysize: 0x%x, uvpitch: 0x%x, uvsize: 0x%x, u at 0x%x, v at 0x%x, total 0x%x\n", frame_sz, ypitch, ysize, uvpitch, uvsize, upos, vpos, vpos+uvsize);
			ytexture.SetPixels(data);
			//utexture.SetPixels(data+upos);
			utexture.SetPixels(zeros);
			//vtexture.SetPixels(data+vpos);
			vtexture.SetPixels(zeros);
			//cam->EndReadFrame(0);
		}

		//begin frame, draw the texture then end frame (the bit of maths just fits the image to the screen while maintaining aspect ratio)
		BeginFrame();
		float aspect_ratio = float(MAIN_TEXTURE_WIDTH)/float(MAIN_TEXTURE_HEIGHT);
		float screen_aspect_ratio = 1280.f/720.f;
		for(int texidx = 0; texidx<levels; texidx++)
			DrawYUVTextureRect(&ytexture,&utexture,&vtexture,-1.f,-1.f,1.f,1.f,&rgbtextures[texidx]);

		//these are just here so we can access the yuv data cpu side - opengles doesn't let you read grey ones cos they can't be frame buffers!
		DrawTextureRect(&ytexture,-1,-1,1,1,&yreadtexture);
		DrawTextureRect(&utexture,-1,-1,1,1,&ureadtexture);
		DrawTextureRect(&vtexture,-1,-1,1,1,&vreadtexture);

		if(!do_pipeline)
		{
			if(selected_texture == -1 || &blurtexture == texture_grid[selected_texture])
				DrawBlurredRect(&rgbtextures[1],-1.f,-1.f,1.f,1.f,&blurtexture);

			if(selected_texture == -1 || &sobeltexture == texture_grid[selected_texture])
				DrawSobelRect(&rgbtextures[1],-1.f,-1.f,1.f,1.f,&sobeltexture);

			if(selected_texture == -1 || &greysobeltexture == texture_grid[selected_texture])
				DrawSobelRect(&ytexture,-1.f,-1.f,1.f,1.f,&greysobeltexture);

			if(selected_texture == -1 || &mediantexture == texture_grid[selected_texture])
				DrawMedianRect(&rgbtextures[1],-1.f,-1.f,1.f,1.f,&mediantexture);

			if(selected_texture == -1 || &redtexture == texture_grid[selected_texture])
				DrawMultRect(&rgbtextures[1],-1.f,-1.f,1.f,1.f,1,0,0,&redtexture);

			if(selected_texture == -1 || &dilatetexture == texture_grid[selected_texture])
				DrawDilateRect(&rgbtextures[1],-1.f,-1.f,1.f,1.f,&dilatetexture);

			if(selected_texture == -1 || &erodetexture == texture_grid[selected_texture])
				DrawErodeRect(&rgbtextures[1],-1.f,-1.f,1.f,1.f,&erodetexture);

			if(selected_texture == -1 || &threshtexture == texture_grid[selected_texture])
				DrawThreshRect(&ytexture,-1.f,-1.f,1.f,1.f,0.25f,0.25f,0.25f,&threshtexture);

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
		}
		else
		{
			DrawMedianRect(&ytexture,-1.f,-1.f,1.f,1.f,&mediantexture);
			DrawSobelRect(&mediantexture,-1.f,-1.f,1.f,1.f,&sobeltexture);
			DrawErodeRect(&sobeltexture,-1.f,-1.f,1.f,1.f,&erodetexture);
			//DrawDilateRect(&erodetexture,-1.f,-1.f,1.f,1.f,&dilatetexture);
			DrawThreshRect(&erodetexture,-1.f,-1.f,1.f,1.f,0.05f,0.05f,0.05f,&threshtexture);
			DrawTextureRect(&threshtexture,-1,-1,1,1,NULL);
		}

		EndFrame();

		//read current time
		clock_gettime(CLOCK_REALTIME, &gettime_now);
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
		}

	}

	StopCamera();

	endwin();
}
