#include "rocks.hpp"
#include "camera_transforms.hpp"
#include "camera_constants.hpp"
#include "time.hpp"
#include "shot.hpp"
#include <stdio.h>
#include <algorithm>
#include <sstream>
#include "gpu/picam.hpp"

using namespace std;
using namespace cv;

Timer rockTimer;

void drawRocks(Mat image, vector<Rock> &rocks, const Scalar &center_colour, const Scalar &circle_colour) {
    for (Rock rock : rocks) {
        Point2d center = realToPerspective(rock.center);
        double radius;
        if(circle_colour == BLUE)
            radius = rock.radius;
        else
            radius = rock.radius + 10;
        
        /* Circle center */
        circle(image, center, 3, center_colour, -1, 8, 0);
        
        /* Circle radius */
        circle(image, center, radius, circle_colour, 3, 8, 0);
    }
}

//Old method, not in use
void findThresholdedRocks(Mat image, RockColour colour, vector<Rock> &rocks)
{
    // Find contours in the image, we'll use these to find rocks 
    vector< vector<Point> > contours;
    vector<Vec4i> hierarchy;
    findContours(image, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    if (hierarchy.size() == 0) {
        return;
    }

    for (int idx = 0; idx >= 0; idx = hierarchy[idx][0]) {
        // Check that contour appears to be rounded, hinting it's circular 
        if (contours[idx].size() < 10) {
            continue;
        }

        // Undistort our points 
        vector<Point2d> conts;
        Mat(contours[idx]).copyTo(conts);

        vector<Point2d> undistorted;
        undistortPoints(conts, undistorted, CAMERA_MATRIX, DIST_COEFFS, Mat(), CAMERA_MATRIX);

        // Convert contour points into real space 
        vector<Point2d> real_points;
        for (Point2d screen_point : undistorted) {
            Point2d real = screenToReal(screen_point);
            real_points.push_back(real);
        }

        // Check that an ellipse covering the points is roughly circular 
        vector<Point2f> real_floats;
        Mat(real_points).copyTo(real_floats);
        RotatedRect rect = fitEllipse(real_floats);
        if (rect.size.height < rect.size.width / 1.2 || rect.size.height > rect.size.width * 1.2) {
            continue;
        }

        Point2f center;
        float radius;
        minEnclosingCircle(real_floats, center, radius);

        if (radius > ROCK_RADIUS_MIN && radius < ROCK_RADIUS_MAX) {
            Rock rock;
            rock.center = rect.center;
            rock.radius = radius / 5;
            rock.colour = colour;
            
            rocks.push_back(rock);
        }
    }
}

//Old method, not in use
void findRocks(Mat background, Mat image, vector<Rock> &red_rocks, vector<Rock> &yellow_rocks)
{
    Mat subtracted;
    Mat hsv, hsv_background;
    cvtColor(image, hsv, CV_BGR2HSV);
    cvtColor(background, hsv_background, CV_BGR2HSV);

    absdiff(hsv_background, hsv, subtracted);

    Mat gray;
    cvtColor(subtracted, gray, CV_BGR2GRAY);
    GaussianBlur(gray, gray, Size(9, 9), 10, 10);
    GaussianBlur(hsv, hsv, Size(9, 9), 10, 10);

    Mat red, yellow;
    inRange(hsv, RED_LOW_HSV, RED_HIGH_HSV, red);
    inRange(hsv, YELLOW_LOW_HSV, YELLOW_HIGH_HSV, yellow);
    
    Mat thresholded;
    threshold(gray, thresholded, 20, 255, THRESH_BINARY);

    findThresholdedRocks(yellow & thresholded, YELLOW_ROCK, yellow_rocks);
    findThresholdedRocks(red & thresholded, RED_ROCK, red_rocks);


    cout << "# of reds: " << red_rocks.size() << endl;
    cout << "# of yellows: " << yellow_rocks.size() << endl;
    // Debug drawing 
    
    Mat undistorted;
    undistort(image, undistorted, CAMERA_MATRIX, DIST_COEFFS);

    drawRocks(undistorted, red_rocks, Scalar(0,0,255), Scalar(0,0,255));
    drawRocks(undistorted, yellow_rocks, Scalar(0,200,200), Scalar(0,200,200));
    
    imwrite("out-circles.jpg", undistorted);

    

    subtracted.release();
    hsv.release();
    hsv_background.release();
    gray.release();
    red.release();
    yellow.release();
    thresholded.release();
}


//Hough transform is here
void rocksFromCircles(Mat image, RockColour colour, vector<Rock> &rocks)
{
    vector<Vec3f> circles;

    HoughCircles(image, circles, CV_HOUGH_GRADIENT, HOUGH_DP, HOUGH_MINDIST, HOUGH_PARAM1, HOUGH_PARAM2, 
                HOUGH_MINRADIUS/DOWN_SCALING, HOUGH_MAXRADIUS/DOWN_SCALING);
    
    //Adds detected rocks to input vector
    for (Vec3f circle : circles) {
        Point2d position(circle[0], circle[1]);
        Rock rock;
        rock.center = perspectiveToReal(position);
        rock.pixelCenter = position;
        rock.radius = circle[2];
        rock.colour = colour;

        rocks.push_back(rock);
    }
}

//Doest undistort and perspective warp
Mat correctImage(Mat image){
    Mat corrected;
    undistort(image, corrected, CAMERA_MATRIX, DIST_COEFFS, OPT_MATRIX);
    //debugImgShow(corrected, "undistort");
    warpPerspective(corrected, corrected, PERSPECTIVE_MATRIX, Size(PERSPECTIVE_SIZE.width/3 + 500, PERSPECTIVE_SIZE.height/3 + 200));
    //debugImgShow(corrected, "warped");
    return corrected;
}

string getNumberString(string str, int number){
    ostringstream buffer;
    buffer << str << number;
    return buffer.str();
}

//Counts how much of a color an region of the image has
float getColorPoints(Mat image, ImgType imgType){
    vector<Mat> channels;
    Mat img;
    switch(imgType){
        case BGR:
            cvtColor(image, img, CV_BGR2GRAY);
            break;
        case HSV:
            split(image, channels);
            img = channels[2];
            break;
        case GRAY:
            img = image;
            break;
        default:
            break;
    }


    float whiteImgValue = img.size().height*img.size().width*255;
    float whiteValue = sum(image)[0];
    float fillingPercentage = whiteValue/whiteImgValue*100; 
    cout << "img has " <<  fillingPercentage << "% of filling" << endl;
    return fillingPercentage; 
    
}

//Color detection 20 times faster than the original code
//Splits detected rocks into red and yellow rocks
void colorDetection(Mat image, vector<Rock> &rocks, vector<Rock> &red_rocks, vector<Rock> &yellow_rocks){
    Mat rockROI, redRock1, redRock2, redRock, yellowRock;
    int width = image.size().width;
    int height = image.size().height;
    int roiHeightAndWidth;
    int pointx, pointy;
    float redPoints = 0;
    float yellowPoints = 0;

    //Main detection loop
    cout << rocks.size() << " rocks in the vector" << endl;

    //Loops through rock positions, extracting regions around them and searching for color.
    for(unsigned int i = 0; i < rocks.size(); i++){
        roiHeightAndWidth = 3*rocks[i].radius/2;
        
        //Defining ROI points
        pointx = rocks[i].pixelCenter.x - roiHeightAndWidth/2;
        if (pointx < 0) 
            pointx = 0;
        if (pointx > (width - roiHeightAndWidth)) 
            pointx = width - roiHeightAndWidth - 1;

        pointy = rocks[i].pixelCenter.y - roiHeightAndWidth/2;
        if (pointy < 0)
            pointy = 0;
        if (pointy > (height - roiHeightAndWidth))
            pointy = height - roiHeightAndWidth - 1;
        //

        //Get area round rock and color filter it
        rockROI = Mat(image, Rect(pointx, pointy, roiHeightAndWidth, roiHeightAndWidth)).clone();

        //Red filter the ROI
        inRange(rockROI, CIRCLES_RED_LOW_HSV_HIGH, CIRCLES_RED_HIGH_HSV_HIGH, redRock1);
        inRange(rockROI, CIRCLES_RED_LOW_HSV_LOW, CIRCLES_RED_HIGH_HSV_LOW, redRock2);
        addWeighted(redRock1, 1.0, redRock2, 1.0, 0.0, redRock);
        threshold(redRock, redRock, 20, 255, THRESH_BINARY);
        
        //Yellow filtering
        inRange(rockROI, CIRCLES_YELLOW_LOW_HSV, CIRCLES_YELLOW_HIGH_HSV, yellowRock);
        threshold(yellowRock, yellowRock, 20, 255, THRESH_BINARY);
        
        //Measure the quantity of red/yellow 
        redPoints = getColorPoints(redRock, GRAY);
        yellowPoints = getColorPoints(yellowRock, GRAY);

        //Define the colour of the rock
        if ((redPoints <= NOCOLOUR_VALUE)&&(yellowPoints <= NOCOLOUR_VALUE)){
            rocks[i].colour = NO_COLOUR;
            cout << endl << "rock " << i << " is UNDEFINED =(" << endl << endl;
        }
        else if (redPoints >= yellowPoints){
            rocks[i].colour = RED_ROCK;
            red_rocks.push_back(rocks[i]);
            cout << endl << "rock " << i << " is RED!" << endl << endl;
        }
        else{    
            rocks[i].colour = YELLOW_ROCK;
            yellow_rocks.push_back(rocks[i]);
            cout << endl << "rock " << i << " is YELLOW!" << endl << endl;
        }
        
        
    }
        
}


//Detects rock positions and color, currently doesnt use the background
int findRocksCircles(Mat background, Mat image, vector<Rock> &red_rocks, vector<Rock> &yellow_rocks, bool enableGPU, bool debug, String output_file)
{
    //Various storage for different parts of the pipeline
    Mat flat_hsv;
    Mat flat_perspective = image; 
    Mat blurred;

    vector<Rock> possible_rocks;
    vector<Mat> channels;

    //Correct perspective undistortion and downscale (currently turned off) 
    rockTimer.methodTimerStart();
    
    //Correct image doesnt work with GPU yet
    if (!enableGPU)
	flat_perspective = correctImage(image);
    else flat_perspective = image;

    //Not using downscale
    //flat_perspective = downScale(flat_perspective, DOWN_SCALING);
    rockTimer.getMethodTimer("img preparation");

    //cvt to HSV to use the saturation channel for circle detection
    rockTimer.methodTimerStart();
    cvtColor(flat_perspective, flat_hsv, CV_BGR2HSV);
    rockTimer.getMethodTimer("HSV conversion");

    //Array of channels to hold all 3 channels from hsv image 
    split(flat_hsv, channels);
    
    //Blur the saturation channel
    
    rockTimer.methodTimerStart();
    if(enableGPU){
	setGpuInput(channels[1]);
	gpuRun();
	blurred = getGpuOutput();
	cvtColor(blurred, blurred, CV_BGRA2GRAY);
    } else {
        medianBlur(channels[1], blurred, BLUR_KERNEL_SIZE);
    }
    rockTimer.getMethodTimer("total gpu blur time");

    //Run rock position detection
    rockTimer.methodTimerStart();
    rocksFromCircles(blurred, NO_COLOUR, possible_rocks);
    rockTimer.getMethodTimer("hough transform");

    //Run color detection using rock positions
    rockTimer.methodTimerStart();
    colorDetection(flat_hsv, possible_rocks, red_rocks, yellow_rocks);
    rockTimer.getMethodTimer("color detection");

    //After finding, draw them in the debug output
    drawRocks(flat_perspective, possible_rocks, BLUE, BLUE);
    drawRocks(flat_perspective, red_rocks, RED, RED);
    drawRocks(flat_perspective, yellow_rocks, YELLOW, YELLOW);
 
    //Uncomment to show windowed output
    if (debug) {
	//debugImgShow(flat_perspective, "rock positions");
	imwrite(output_file, flat_perspective);
    }        
    
    
    return 0;
}


bool pointEq(Point2d p1, Point2d p2) {
    return p1 == p2;
}

/* Must be unique */
vector<Point2d> getProjectedCircle(size_t col, size_t row)
{
    Point2d real_point = screenToReal(Point2d(col, row));
    
    /* Get the projected circle points */
    vector<Point2d> points;
    for (Point2i circle_point : BASIC_CIRCLE) {
        Point2d pt = Point2d((double) circle_point.x, (double) circle_point.y);
        pt = real_point + (ROCK_RADIUS / BASIC_CIRCLE_RADIUS) * pt;
        
        points.push_back(realToScreen(pt));
    }

    unique(points.begin(), points.end(), pointEq);
    return points;
}

//Some utilities that are left here
vector<Point2i> BASIC_CIRCLE;

void setupBasicCircle() {
    int back_width = BASIC_CIRCLE_RADIUS * 2;

    int radius = BASIC_CIRCLE_RADIUS;
    Point2i center = Point2i(radius, radius);
    Mat back = Mat::zeros(back_width, back_width, CV_8UC1);

    circle(back, center, radius, Scalar(255,255,255));

    findNonZero(back, BASIC_CIRCLE);

    for (size_t i = 0; i < BASIC_CIRCLE.size(); ++i) {
        BASIC_CIRCLE[i] -= Point2i((double) radius, (double) radius);
    }
}

double rockDist(Rock r1, Rock r2)
{
    double dx = r1.center.x - r2.center.x;
    double dy = r1.center.y - r2.center.y;

    return sqrt(dx * dx + dy * dy);
}

void debugRockInformation(vector<Rock> &rocks){
    for(unsigned int i = 0; i < rocks.size(); i++){
        cout << "Real space Center: " << rocks[i].center << endl
             << "Pixel Center" << rocks[i].pixelCenter << endl 
             << "Radius: " << rocks[i].radius << endl
             ;
    }
}
