#pragma once

#include "time.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/video/background_segm.hpp>
#include <fstream>
#include <string>

#define FALSE_POS 1
#define FALSE_NEG -1
#define OK 0

//If enabled, uses groundtruth to produce a log with shot detection accuracy
//If enable, adjust INIT_IMG, SHOTS_IN_* to your dataset
const bool ENABLE_GROUNDTRUTH_CHECK = false;

//VERY IMPORTANT: INIT_IMG must be the initial number of the 
//images of the dataset if using groundtruth for testing

//const int INIT_IMG = 12255; //end
//const int INIT_IMG = 13295; //two-shot
const int INIT_IMG = 0; //dir
//const int INIT_IMG = 6930; //rocks

//Change depending on your dataset for groundtruth check
const int SHOTS_IN_END = 65; 
const int MAX_SHOTS_IN_GAME = 200;

//DO NOT CHANGE THESE PARAMETERS UNLESS YOU WANT 
//TO EXPERIMENT WITH DIFFERENT ACCURACY IN DETECTION
const int SHOT_DOWNSCALING = 8;

//Change to your display for debug stuff to show correctly
const int DISPLAY_WIDTH = 1366/2;
const int DISPLAY_HEIGHT = 768/2;

//Region of interest (middle of curling sheet)
//Should be automatic, currently manual
const int ROIX = 0;
const int ROIY = 850/SHOT_DOWNSCALING;
const int ROI_WIDTH = 2580/SHOT_DOWNSCALING;
const int ROI_HEIGHT = 300/SHOT_DOWNSCALING;

//(measured) how much "pixel value" a human has, serves as a safety threshold
const int HUMAN_PIXEL_VALUE = 2000000;

//Reduzing number of regions makes speed more important,
//Staying on the same region is considered not a moving shot
const int PARTS_NUM = 8;

//Tracking rectangle options
const int RECT_COLOR = 255;
const int RECT_LINE_TYPE = 8; 
const int RECT_LINE_THICK = 1;
const int RECT_SHIFT = 0;

//Threshold options
const int THRESH_MAXVAL = 255;
const int THRESH_VAL = 50;

//Morphological operation size, not in use
const int MORPH_SIZE = 11;

//Shot detection options
//Moving human has to been seen DETECT_RELIABILITY + 1 times
const int DETECT_RELIABILITY = 5;
const int SAME_REGION_LIMIT = 1;




//Shows img in a easier way than opencv default
void debugImgShow(cv::Mat img, std::string title);



/* Shot Detector class definition */
class ShotDetector{
	private:
		bool SHOT_DEBUG;

		cv::Mat background;
		Timer shotTimer;
		
		//shot log stuff
		std::ofstream shot_text;
		std::ifstream shot_reader;
		
		//Background subtractor
		cv::Ptr<cv::BackgroundSubtractorMOG2> MOG;

		//Variables for shot detection logic
		int lastPosition;
		int detectCount;
		int sameRegionCount;
		int shotCount;
		int WAIT_TIME;

		void resetDetectionVars();
		

		//Ground truth info
		int falsePos;
		int falseNeg;
		int truthShots;

		int trueShots[SHOTS_IN_END];
		int gameShots[200];

		cv::Mat applyCvSubtraction(cv::Mat image);
		cv::Mat applyMySubtraction(cv::Mat image);

		//Finds if the sequence of tracked frames is really a shot 
		//By checking for DETECT_RELIABILITY consecutive incremented positions
		bool isShot(int trackingPos);

		//Tracks where in the image the foreground is 
		int findTrackingPos (int *array);

		//Initializes shot log statistics
		void initStatistics();

		//Checks for false negatives comparing to groundtruth
		void checkForFalseNegs();
		int checkShot(int shotNumber);

	public:
		ShotDetector(cv::Mat bg, bool debug);
		~ShotDetector();
		//Prepares image data for tracking 
		//starting detection pipeline 
		bool analyzeFrame(cv::Mat frame);


		
		/**/
};
