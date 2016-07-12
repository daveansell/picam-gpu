#include "camera_constants.hpp"
#include <opencv2/opencv.hpp>

/*
  Constants for the camera. This includes the camera matrix, and
  distortion coefficients. These were gotten by running the calib3d
  calibration. See the out_camera_data.xml file.

 */

static double camM[3][3] = { {2.4212858299428826e+03, 0, 1.2955000000000000e+03},
                             {0, 2.4047866702403944e+03, 9.7150000000000000e+02},
                             {0, 0, 1} };

Mat CAMERA_MATRIX = Mat(3, 3, CV_64F, camM);
Mat CAMERA_MATRIX_INV;
Mat OPT_MATRIX;
Mat OPT_MATRIX_INV;

static double distC[5] = {-4.3803013078760922e-01, 4.1785077470077364e-01, 0, 0, -3.7373995430837226e-01};
Mat DIST_COEFFS = Mat(5, 1, CV_64F, distC);


static const double BACK_LINE_WIDTH = 4280;
static const double HOG_LINE_WIDTH = 4280;
static const double BACK_LINE = 1830;
static const double HOG_LINE = -6400;
Size PERSPECTIVE_SIZE = Size (BACK_LINE - HOG_LINE, BACK_LINE_WIDTH > HOG_LINE_WIDTH ? BACK_LINE_WIDTH : HOG_LINE_WIDTH);

static vector<Point2d> real_corners { Point2d(BACK_LINE, -BACK_LINE_WIDTH/2),
                                      Point2d(HOG_LINE, -HOG_LINE_WIDTH/2),
                                      Point2d(BACK_LINE, BACK_LINE_WIDTH/2),
                                      Point2d(HOG_LINE, HOG_LINE_WIDTH/2)
                                    };

Mat CAMERA_ROTATION;
Mat CAMERA_ROTATION_INV;
Mat CAMERA_TRANSLATION;

Mat PERSPECTIVE_MATRIX;
Mat PERSPECTIVE_MATRIX_INV;


void camera_setup(const Size &size, vector<Point2d> screen_corners) {
    /* Calculate our camera matrix's inverse. */
    CAMERA_MATRIX_INV = CAMERA_MATRIX.inv();

    Size undistorted_size = Size(100, 100) + size;
    OPT_MATRIX = getOptimalNewCameraMatrix(CAMERA_MATRIX, DIST_COEFFS, size, 1, undistorted_size);
    OPT_MATRIX_INV = OPT_MATRIX.inv();

    /* getPerspectiveTransform demands floats and not doubles... */
    vector<Point2f> screen_floats;
    vector<Point2f> real_floats;

    Mat(screen_corners).copyTo(screen_floats);
    Mat(real_corners).copyTo(real_floats);

    /* want perspect to transform to a screenspace with 0,0 as before hogline right */
    Point2f hogline_right = real_floats[1] - Point2f(1000, 250);
    for (size_t i = 0; i < real_floats.size(); ++i) {
        real_floats[i] -= hogline_right;
        real_floats[i] *= (1.0/3.0);
    }

    /* Use undistorted points to get the perspective transform */
    vector<Point2f> undistorted_corners;
    undistortPoints(screen_floats, undistorted_corners, CAMERA_MATRIX, DIST_COEFFS, Mat(), OPT_MATRIX);

    /* Get perspective matrix and inverse */
    PERSPECTIVE_MATRIX = getPerspectiveTransform(undistorted_corners, real_floats);
    PERSPECTIVE_MATRIX_INV = PERSPECTIVE_MATRIX.inv();

    /* Get our cameras location and orientation */
    Mat rvec;

    vector<Point3d> real3_corners;

    for (Point2d point : real_corners) {
        real3_corners.push_back(Point3d(point.x, point.y, 0));
    }
    
    solvePnP(real3_corners, screen_corners, CAMERA_MATRIX, DIST_COEFFS, rvec, CAMERA_TRANSLATION);
    
    Rodrigues(rvec, CAMERA_ROTATION);
    CAMERA_ROTATION_INV = CAMERA_ROTATION.inv();
}
