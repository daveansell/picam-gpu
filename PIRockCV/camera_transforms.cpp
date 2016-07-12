#include "camera_transforms.hpp"
#include "camera_constants.hpp"
#include "rocks.hpp"

using namespace std;

Point2d screenToReal(Point2d screen_point)
{
    Mat screen = Mat(Point3d(screen_point.x, screen_point.y, 1));

    Mat left = CAMERA_ROTATION_INV * CAMERA_MATRIX_INV * screen;
    Mat right = CAMERA_ROTATION_INV * CAMERA_TRANSLATION;
    double s_project = (ROCK_HEIGHT + right.at<double>(2,0)) / left.at<double>(2,0);

    Mat p = CAMERA_ROTATION_INV * (s_project * CAMERA_MATRIX_INV * screen - CAMERA_TRANSLATION);

    return Point2d(p.at<double>(0,0), p.at<double>(1,0));
}


Point2d realToScreen(Point2d real_point)
{
    Point3d real(real_point.x, real_point.y, ROCK_HEIGHT);
    Mat p = CAMERA_MATRIX * (CAMERA_ROTATION * Mat(real) + CAMERA_TRANSLATION);
    p = p / p.at<double>(2,0);

    return Point2d(p.at<double>(0,0), p.at<double>(1,0));
}


Point2d perspectiveToReal(Point2d screen_point)
{
    vector<Point2d> screen_vec {screen_point};
    vector<Point2d> projected_vec;

    perspectiveTransform(screen_vec, projected_vec, PERSPECTIVE_MATRIX_INV);
    screen_point = projected_vec[0];

    Mat screen = Mat(Point3d(screen_point.x, screen_point.y, 1));

    Mat left = CAMERA_ROTATION_INV * OPT_MATRIX_INV * screen;
    Mat right = CAMERA_ROTATION_INV * CAMERA_TRANSLATION;
    double s_project = (ROCK_HEIGHT + right.at<double>(2,0)) / left.at<double>(2,0);

    Mat p = CAMERA_ROTATION_INV * (s_project * OPT_MATRIX_INV * screen - CAMERA_TRANSLATION);

    return Point2d(p.at<double>(0,0), p.at<double>(1,0));

}


Point2d realToPerspective(Point2d real_point)
{
    Point3d real(real_point.x, real_point.y, ROCK_HEIGHT);
    Mat p = OPT_MATRIX * (CAMERA_ROTATION * Mat(real) + CAMERA_TRANSLATION);
    p = p / p.at<double>(2,0);
    p = PERSPECTIVE_MATRIX * p;
    p = p / p.at<double>(2,0);
    
    return Point2d(p.at<double>(0,0), p.at<double>(1,0));
}

//Added by me
Mat downScale(Mat input, int factor){
    //Low res test, divide by 4
    Mat output(Size(input.size().width/factor, input.size().height/factor), CV_8UC3);
    
    resize(input, output, output.size(), 0, 0, INTER_LINEAR);

    return output;
}