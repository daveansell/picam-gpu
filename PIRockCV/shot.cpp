#include "shot.hpp"
#include "time.hpp"
#include "camera_transforms.hpp"
#include <iostream>
#include <sstream>

using namespace std;
using namespace cv;

extern int frameCount;

//IMPORTANT: A lot of these functions have to do with using the groundtruth.txt to check for false positives and negatives,
//Not very useful if you are not changing the shot detection. Actual shot detection code starts at the ANALYZEFRAME method.

//Constructor defines stuff when declaring a new detector
ShotDetector::ShotDetector(Mat bg, bool debug){

    SHOT_DEBUG = debug;
	background = bg;
	shotTimer = Timer();
	shotCount = 0;
	
    MOG = createBackgroundSubtractorMOG2();
    if (SHOT_DOWNSCALING > 1)
    		bg = downScale(bg, SHOT_DOWNSCALING);
    background = Mat(bg, Rect(ROIX, ROIY, ROI_WIDTH, ROI_HEIGHT));
    cvtColor(background, background, CV_BGR2GRAY);
    resetDetectionVars();
    if (SHOT_DEBUG) WAIT_TIME = 100;
    else WAIT_TIME = 1;
    for(int i = 0; i < SHOTS_IN_END; i++) trueShots[i] = 0;
    for(int i = 0; i < MAX_SHOTS_IN_GAME; i++) gameShots[i] = 0;

   	falsePos = 0;
    falseNeg = 0;

    if (ENABLE_GROUNDTRUTH_CHECK){
		cout << endl << "SHOT LOG GROUNDTRUTH CHECK IS ENABLED!" << endl << endl;
    	shot_text.open("shots.txt");
    	shot_text << "Shot log" << endl;
    	initStatistics(); 
	} else {
		cout << endl << "SHOTS LOG GENERATION DISABLED" << endl << endl;
	}
}

//Destroys object
ShotDetector::~ShotDetector(){
	if (ENABLE_GROUNDTRUTH_CHECK){
		checkForFalseNegs();
		shot_text << "end of shot log with " << shotCount << "shots detected" << endl
				  << "number of shots in the real game: " << truthShots << endl
				  << "number of false positives: " << falsePos << endl
		          << "number of false negatives: " << falseNeg << endl
				  << "perfomance: " << ((float)(shotCount - falsePos - falseNeg)/shotCount)*100 << "/100" << endl
			      << "# regions: " << PARTS_NUM << endl
				  << "reliability: " << DETECT_RELIABILITY << endl
				  << "same region allowed: " << SAME_REGION_LIMIT << endl
				  << "size of human: " << HUMAN_PIXEL_VALUE << endl;
		shot_text.close();	
	}
}

int ShotDetector::checkShot(int shotNumber){
	int i, currentShot, upperBound, lowerBound;
	//Checks if requested shot is on groundtruth
	for (i = 0; i < SHOTS_IN_END; i++){
		currentShot = trueShots[i];
		upperBound = currentShot + 6;
		lowerBound = currentShot - 6;
		//If the detected shot is in the boundarie of 
		if (shotNumber < upperBound && shotNumber > lowerBound){
			cout << "Thats confirmed." << endl;
			shot_text << " ok ";
			return OK;
		}
	}
	cout << "Thats a false positive." << endl;
	shot_text << " false positive ";
	falsePos++;
	return FALSE_POS;
}

//Similar to check shot but checks whole game with whole game
void ShotDetector::checkForFalseNegs(){
	int i, j, currentShot, upperBound, lowerBound;
	bool fn_detected = true;
	//Checks for every trueshot if it is on the game just processed
	shot_text << "false negatives: " << endl;
	for(i = 0; i < SHOTS_IN_END; i++){
		currentShot = trueShots[i];
		upperBound = currentShot + 6;
		lowerBound = currentShot - 6;
		for(j = 0; j < MAX_SHOTS_IN_GAME; j++){
			if (gameShots[j] < upperBound && gameShots[j] > lowerBound){
				cout << currentShot << " that shot was there." << endl;
				fn_detected = false;
				break;
			}
		}
		if (fn_detected){
			cout << "shot " << currentShot << " wasnt seen! FALSE NEGATIVE!" << endl;
			shot_text << "falseNegative: " << currentShot << endl; 
			falseNeg++;
		}
		fn_detected = true;
	}
}

//Initializes statistics geting the frames of shots
void ShotDetector::initStatistics(){
	//local vars keep statistics, write everything at once in the end
	shot_reader.open("groundtruth.txt");

	string line;
	int i = 0;
	string a, b;

	//Get groundtruth info into program, basically where a shot happens in the dataset being tested
	if (shot_reader.is_open()){
		while(getline(shot_reader,line)){
			cout << "line: " << line << endl;
			stringstream parts(line);
			parts >> a >> b;  
			if(a.compare("end") == 0) 
				break;
			else if(a.compare("Shot-Groundtruth") == 0)
				truthShots = stoi(b);
			else if(a.compare("e") == 0)
				;
			else{
				trueShots[i] = stoi(a);
				i++;
			}

			cout << "a: " << a << " b: " << b << endl;
		}

		for (int j = 0; j < SHOTS_IN_END; j++){
			cout << trueShots[j] << endl;
		}
		cout << "those are frames where there are shots according to groundtruth.txt" << endl;
		shot_reader.close();
	}



}

//Applies OPENCV background subtraction
Mat ShotDetector::applyCvSubtraction(Mat image){
	Mat fg;
	MOG -> apply(image, fg);
	threshold(fg, fg, 127, 255, THRESH_BINARY );
	medianBlur(fg, fg, 5); 
	return fg;
}

//Applies my background subtraction (worse perfomance than CV)
Mat ShotDetector::applyMySubtraction(cv::Mat image){
	Mat subtracted, fg, element;
	absdiff(background, image, subtracted);
	threshold(subtracted, fg, THRESH_VAL, THRESH_MAXVAL, THRESH_BINARY);
	
	return fg;
}


//MAIN SHOT DETECTION CODE
bool ShotDetector::analyzeFrame(Mat frame){
	//Array that holds pixel sum in regions
	int whiteSums[PARTS_NUM];

	int trackingPos = 0;
	
	//Partial images
	Mat roi, gray, foreground;

	//Array of images for image regions
    Mat regions[PARTS_NUM];

    //Downscale
    if (SHOT_DOWNSCALING > 1)
		frame = downScale(frame, SHOT_DOWNSCALING);


	//Work with middle part of the image, adjust in .hpp
	roi = Mat(frame, Rect(ROIX, ROIY, ROI_WIDTH, ROI_HEIGHT));
    cvtColor(roi, gray, CV_BGR2GRAY);

    //Apply background subtraction
    //foreground = applyMySubtraction(gray);
    foreground = applyCvSubtraction(gray);
    
    //Split into PARTS_NUM equal regions for tracking position of humans in shot
    //Checks which part has higher pixel value
    for(int i = 0; i < PARTS_NUM; i++){
    	regions[i] = Mat(foreground, Rect(i*ROI_WIDTH/PARTS_NUM, 0, ROI_WIDTH/PARTS_NUM, ROI_HEIGHT));
    	whiteSums[i] = sum(regions[i])[0];
    }

    //Calls function to get where the pixels are
    trackingPos = findTrackingPos(whiteSums);
  	
  	if (SHOT_DEBUG) cout << "Highest sum value: " << whiteSums[trackingPos] << endl;
  	
  	if (whiteSums[trackingPos] < HUMAN_PIXEL_VALUE/(SHOT_DOWNSCALING*SHOT_DOWNSCALING)){
  		trackingPos = 0;
  	}

	//Show stuff for debugging
	if (SHOT_DEBUG){
		rectangle(foreground, Point(trackingPos*ROI_WIDTH/PARTS_NUM      , 0), 
						  		 Point((trackingPos + 1)*ROI_WIDTH/PARTS_NUM, ROI_HEIGHT),
						  		 RECT_COLOR);

		debugImgShow(foreground, "my subtraction");
		waitKey(WAIT_TIME);
	}

	if (SHOT_DEBUG) cout << "shots detected so far: " << shotCount << endl;
	return isShot(trackingPos);
    
}

void ShotDetector::resetDetectionVars(){
	lastPosition = 0;
	detectCount = 0;
	sameRegionCount = 0;
}

//Finds if the sequence of frames is really a shot 
//By checking for 6 consecutive incremented positions
//The logic could be more understandable on this function, sorry
bool ShotDetector::isShot(int trackingPos){
	
	if(trackingPos > 0 && trackingPos >= lastPosition){
		if (trackingPos == lastPosition){
			sameRegionCount++;
			if (SHOT_DEBUG) cout << "Same region count: " << sameRegionCount << endl;
		} 
			
		lastPosition = trackingPos;
		detectCount++;
		if (SHOT_DEBUG) cout << "Something is moving: " << detectCount << endl;
	} else {
		//No continuity, will only count now on next human coming from 0
		resetDetectionVars();
	}

	//DETECT_RELIABILITY consecutive diff positions detections = shot, consecutive same positions = not shot
	if (sameRegionCount > SAME_REGION_LIMIT){
		//No continuity, will only count now on next human coming from 0
		if (SHOT_DEBUG) cout << "Something stopped on the ice, its not a release." << endl;
		resetDetectionVars();
		return false;
	} 
	else if(detectCount > DETECT_RELIABILITY){
		if (SHOT_DEBUG) for (int i = 0; i < 5; i++) cout << ">>>>>>> SHOT DETECTED <<<<<<" << endl;
		int shotPos = INIT_IMG + frameCount;
		checkShot(shotPos);
		shot_text << "Shot in: " << (shotPos) << endl;
		shot_text.flush();
		gameShots[shotCount] = shotPos;
		shotCount++;
		resetDetectionVars();
		return true;
	} else{
		return false;
	}
}

//Check maximum of windows
int ShotDetector::findTrackingPos (int *array){
	int max = 0;
	int pos = 0;
	int trackingPos = 0;

	for(pos = 0; pos < PARTS_NUM; pos++){
		if(max < array[pos]){
			max = array[pos];
			trackingPos = pos;
		}
	}
	return trackingPos;
}

void debugImgShow(Mat img, string title){
		namedWindow (title, WINDOW_NORMAL);
		imshow(title, img); 
		waitKey(100);
	
}
