#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <algorithm>
#include <time.h>

using namespace cv;
using namespace std;

#include "camera_constants.hpp"
#include "camera_transforms.hpp"



vector<Point> createCircle(){
    Mat Z = Mat::zeros(100,100, CV_8UC1);
    circle(Z, Point(50,50), 50, Scalar(255,255,255));
    vector<Point> circle;
    findNonZero(Z, circle);
    return circle;
}

Mat getSaturation(Mat& in){
    Mat out = Mat::zeros(in.size(), CV_8UC1);
    for(int row = 0; row < in.rows; ++row) {
      uchar* p = in.ptr(row);
      for(int col = 0; col < in.cols; ++col) {
         uchar s = max(p[0],max(p[1],p[2]))-min(p[0],min(p[1],p[2]));
         out.at<uchar>(row,col)=s;
         p += 3;
    }}
    return out;
}

int main( int argc, char** argv )
{
    if( argc != 3)
    {
     cout <<" Usage: test ImageToProcess Background" << endl;
     return -1;
    }

    /* End 1 */
    ///vector<Point2d> screen_corners {Point2d(268, 130), Point2d(2414, 438), Point2d(190, 1538), Point2d(2356, 1468)};

    vector<Point2d> screen_corners { Point2d(374, 442 ),
                                     Point2d(2445, 438 ),
                                     Point2d(488, 1762),
                                     Point2d(2518, 1421)
                                   };

    camera_setup(Size(2592, 1944), screen_corners);

    vector<Point> circle = createCircle();

    Mat image;
    image = imread(argv[1], CV_LOAD_IMAGE_COLOR);   // Read the file

    Mat back;
    back = imread(argv[2], CV_LOAD_IMAGE_COLOR);   // Read the file

    Mat diff;
    absdiff(back, image, diff);

    clock_t first_start = clock();
    clock_t start = clock(), stop;

//    Mat hsv;
//    cvtColor(image,hsv,CV_BGR2HSV);
//
//    vector<cv::Mat> channels;
//    split(hsv, channels);
//
//    stop = clock();
//    cout << " To HSV: " << (((float)stop - (float)start) / CLOCKS_PER_SEC);
//    start = clock();   

    Mat sat = getSaturation(image);
  
    stop = clock();
    cout << " My saturation: " << (((float)stop - (float)start) / CLOCKS_PER_SEC);
    start = clock(); 

    Mat blured;
    //blur(channels[1],blured,Size(7,7));
    blur(sat,blured,Size(7,7));
    
    stop = clock();
    cout << " Blur: " << (((float)stop - (float)start) / CLOCKS_PER_SEC);
    start = clock();   

    Mat edges;
    Canny(blured, edges, 1, 10);

    stop = clock();
    cout << " Edges: " << (((float)stop - (float)start) / CLOCKS_PER_SEC);
    start = clock();   
    
    imwrite( "edges2.png", edges);

    //Mat dst;
    //resize(edges, dst, Size(), 1, 1, INTER_CUBIC);

    Mat centres = Mat::zeros(edges.size(), CV_8UC1);
    Rect rect(Point(), image.size());

    for(int row = 0; row < edges.rows; ++row) {
      uchar* p = edges.ptr(row);
      for(int col = 0; col < edges.cols; ++col) {
         if (*p++ == 255){
            Point2d rc = screenToReal(Point2d(col,row));
            for (vector<Point>::iterator it = circle.begin(); it != circle.end(); ++it){
                Point2d pt((*it).x*1.44+rc.x,(*it).y*1.44+rc.y);
		if (rect.contains(pt)) centres.at<uchar>(realToScreen(pt))++;
            }
	 } 
    }}

    cout << " For loop: " << (((float)clock() - (float)start) / CLOCKS_PER_SEC) << "\n";
    cout << " Overall: " << (((float)clock() - (float)first_start) / CLOCKS_PER_SEC) << "\n";
    namedWindow( "Display window", WINDOW_AUTOSIZE );// Create a window for display.
    imshow( "Display window", centres );                   // Show our image inside it.

    waitKey(0);                                          // Wait for a keystroke in the window
    return 0;
}
