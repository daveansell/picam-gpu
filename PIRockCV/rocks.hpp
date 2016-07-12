#pragma once

#include <opencv2/opencv.hpp>


using namespace cv;
using namespace std;

const int FRAME_SLEEP = 1;

/* Area for a "blob" to count as a rock. */
const double ROCK_AREA = 10;

/* Amount that we can be off for the size of the rock to count. */
const double ROCK_AREA_THRESHOLD = 500;


const double ROCK_RADIUS_MAX = 160;
const double ROCK_RADIUS_MIN = 100;
const double ROCK_RADIUS = 90;

const double ROCK_HEIGHT = 110;


const int BLUR_KERNEL_SIZE = 11;

//Value of color detection at which a rock is considered to have no colour
const int NOCOLOUR_VALUE = 0;

//Draw colors
const Scalar YELLOW = Scalar(0, 220, 220);
const Scalar BLUE = Scalar(255, 0, 0);
const Scalar RED = Scalar(0, 0, 255);

/* HSV thresholds for colour separation of rocks */
const Scalar RED_LOW_HSV  = Scalar(160, 50, 50);
const Scalar RED_HIGH_HSV  = Scalar(179, 180, 180);

const Scalar YELLOW_LOW_HSV  = Scalar(2, 50, 50);
const Scalar YELLOW_HIGH_HSV  = Scalar(15, 180, 180);

/* HSV thresholds for colour separation of rocks in the circle detector */

const int LOW_LVL = 5;
const Scalar CIRCLES_RED_LOW_HSV_HIGH  = Scalar(160, LOW_LVL, LOW_LVL);
const Scalar CIRCLES_RED_HIGH_HSV_HIGH  = Scalar(179, 255, 255);
const Scalar CIRCLES_RED_LOW_HSV_LOW  = Scalar(0, LOW_LVL, LOW_LVL);
const Scalar CIRCLES_RED_HIGH_HSV_LOW  = Scalar(10, 255, 255);
const Scalar CIRCLES_YELLOW_LOW_HSV  = Scalar(20, LOW_LVL, LOW_LVL);
const Scalar CIRCLES_YELLOW_HIGH_HSV  = Scalar(40, 255, 255);


//Hough things
const double HOUGH_DP = 1;
const double HOUGH_MINDIST = 60;
const double HOUGH_PARAM1 = 1;
const double HOUGH_PARAM2 = 32;
const double HOUGH_MINRADIUS = 30;
const double HOUGH_MAXRADIUS = 40;




/* Type of rock */
enum RockColour {RED_ROCK, YELLOW_ROCK, NO_COLOUR};
enum ImgType {BGR, HSV, GRAY};

/* Structure for rocks */
typedef struct Rock {
    /* Properties of the rock */
    Point2d center;
    Point2d pixelCenter;
    float radius;
    RockColour colour;    
} Rock;


const int BASIC_CIRCLE_RADIUS = 50;
extern vector<Point2i> BASIC_CIRCLE;


/*
  Find rocks based on colour, size, and shape from an image.

  Arguments:
  - background: Backround image for subtraction.
  - image: The image of the playing area that we wish to find rocks in.
  - red_rocks: The vector to store any red rocks we find.
  - yellow_rocks: The vector to store any yellow rocks we find..

  Return value:
  - Vectors of rocks found. May be empty. The positions
    of the rocks are given in real space, not screen space.

  Any supposed rocks in the scene should be appended to the `red_rocks`
  and `yellow_rocks` vectors respectively.

 */

void findRocks(Mat background, Mat image, vector<Rock> &red_rocks, vector<Rock> &yellow_rocks);


/*
  Find rocks using Hough circle detection.

  Arguments:
  - background: Backround image for subtraction.
  - image: The image of the playing area that we wish to find rocks in.
  - red_rocks: The vector to store any red rocks we find.
  - yellow_rocks: The vector to store any yellow rocks we find..

  Return value:
  - Vectors of rocks found. May be empty. The positions
    of the rocks are given in real space, not screen space.

  Any supposed rocks in the scene should be appended to the `red_rocks`
  and `yellow_rocks` vectors respectively.

  NOTE: Currently just puts everything in red_rocks.

 */

int findRocksCircles(Mat background, Mat image, vector<Rock> &red_rocks, vector<Rock> &yellow_rocks, bool enableGPU, bool debug=false, String output_file="flat_perspective.jpg");
void findRocksCirclesFast(Mat background, Mat image, vector<Rock> &red_rocks, vector<Rock> &yellow_rocks);
/*
  Draw rocks on the image with a certain center colour, and circle colour.
 */

void drawRocks(Mat image, vector<Rock> &rocks, const Scalar &center_colour, const Scalar &circle_colour);


/* Set up basic circle for circle detector */
void setupBasicCircle();


/* Calculate distance between rocks */
double rockDist(Rock r1, Rock r2);

/*
  Currently the best method for rock detection, kind of slow but GPU
  optimization should solve problems
*/
void rocksFromCircles(Mat image, RockColour colour, vector<Rock> &rocks);

void debugRockInformation(vector<Rock> &rocks);

string getNumberString(string str, int number);
