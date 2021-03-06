#pragma once

#include <opencv2/opencv.hpp>
#include <string>


#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <algorithm>
#include <time.h>

#include "GLES2/gl2.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
//#include "lodepng.h"


void InitGraphics();
void ReleaseGraphics();
void BeginFrame();
void EndFrame();

class GfxShader
{
	GLchar* Src;
	GLuint Id;
	GLuint GlShaderType;

public:

	GfxShader() : Src(NULL), Id(0), GlShaderType(0) {}
	~GfxShader() { if(Src) delete[] Src; }

	bool LoadVertexShader(const char* filename);
	bool LoadFragmentShader(const char* filename);
	GLuint GetId() { return Id; }
};

class GfxProgram
{
	GfxShader* VertexShader;
	GfxShader* FragmentShader;
	GLuint Id;

public:

	GfxProgram() {}
	~GfxProgram() {}

	bool Create(GfxShader* vertex_shader, GfxShader* fragment_shader);
	GLuint GetId() { return Id; }
};

class GfxTexture
{
	int Width;
	int Height;
	GLuint Id;
	bool IsRGBA;

	GLuint FramebufferId;
public:

	GfxTexture() : Width(0), Height(0), Id(0), FramebufferId(0) {}
	~GfxTexture() {}

	bool CreateRGBA(int width, int height, const void* data = NULL);
	bool CreateGreyScale(int width, int height, const void* data = NULL);
	bool GenerateFrameBuffer();
	void SetPixels(const void* data);
	GLuint GetId() { return Id; }
	GLuint GetFramebufferId() { return FramebufferId; }
	int GetWidth() {return Width;}
	int GetHeight() {return Height;}
	//void Save(const char* fname);
	cv::Mat Save(const char* fname, std::string type, void * image);
};

//void SaveFrameBuffer(const char* fname);

void DrawTextureRect(GfxTexture* texture, float x0, float y0, float x1, float y1, GfxTexture* render_target);
void DrawYUVTextureRect(GfxTexture* ytexture, GfxTexture* utexture, GfxTexture* vtexture, float x0, float y0, float x1, float y1, GfxTexture* render_target);
void DrawBlurredRect(GfxTexture* texture, float x0, float y0, float x1, float y1, GfxTexture* render_target);
void DrawSobelRect(GfxTexture* texture, float x0, float y0, float x1, float y1, GfxTexture* render_target);
void DrawMedianRect(GfxTexture* texture, float x0, float y0, float x1, float y1, GfxTexture* render_target);
void DrawMultRect(GfxTexture* texture, float x0, float y0, float x1, float y1, float r, float g, float b, GfxTexture* render_target);
void DrawThreshRect(GfxTexture* texture, float x0, float y0, float x1, float y1, float r, float g, float b, GfxTexture* render_target);
void DrawDilateRect(GfxTexture* texture, float x0, float y0, float x1, float y1, GfxTexture* render_target);
void DrawErodeRect(GfxTexture* texture, float x0, float y0, float x1, float y1, GfxTexture* render_target);

cv::Mat getSaturation(cv::Mat& in);
void HoughTransform(cv::Mat edges, cv::Mat centres);
std::vector<cv::Point> createCircle();
