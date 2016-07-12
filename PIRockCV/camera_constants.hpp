#pragma once

#include <opencv2/opencv.hpp>


using namespace cv;
using namespace std;


/* Camera width and height */
const int STILL_WIDTH = 2592;
const int STILL_HEIGHT = 1944;

/* Size of debug images */
const float DEBUG_SIZE = 1;


const int DOWN_SCALING = 1; 
/*
  Constants for the camera. This includes the camera matrix, and
  distortion coefficients. These were gotten by running the calib3d
  calibration. See the out_camera_data.xml file.

 */


/* Camera matrices and distortion coefficients */
extern Mat CAMERA_MATRIX;
extern Mat CAMERA_MATRIX_INV;

extern Mat OPT_MATRIX;
extern Mat OPT_MATRIX_INV;

extern Mat DIST_COEFFS;

/* Perspective matrices */
extern Mat PERSPECTIVE_MATRIX;
extern Mat PERSPECTIVE_MATRIX_INV;

/* Camera location and orientation */
extern Mat CAMERA_ROTATION;
extern Mat CAMERA_ROTATION_INV;
extern Mat CAMERA_TRANSLATION;

extern Size PERSPECTIVE_SIZE;

/*
  Calculate camera constants given the corners of the backline and
  hogline area.

  screen_corners is a vector containing, in this order, the screen
  coordinates of:

  - backline thrower's right
  - hogline thrower's right
  - backline thrower's left
  - hogline thrower's left

  Assumes that the sheet has a width of 4300mm.
  
 */

void camera_setup(const Size &size, vector<Point2d> screen_corners);
