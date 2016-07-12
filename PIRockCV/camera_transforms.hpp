#pragma once

#include <opencv2/opencv.hpp>


using namespace cv;

/*
  Convert from screen space coordinates to real space coordinates.

 */

Point2d screenToReal(Point2d screen_point);


/*
  Convert from real space to screen space coordinates.

 */

Point2d realToScreen(Point2d real_point);


/*
  Convert perspective projected points.
 */

Point2d perspectiveToReal(Point2d screen_point);
Point2d realToPerspective(Point2d real_point);
Mat downScale(Mat input, int factor);