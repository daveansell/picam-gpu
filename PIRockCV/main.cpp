#include "camera_constants.hpp"
#include "camera_transforms.hpp"
#include "rocks.hpp"
#include "ends.hpp"
#include "shot.hpp"
#include "time.hpp"

 
#include "gpu/gpu.hpp"



#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <time.h>
#include <math.h>
#include <fstream>
#include <iomanip>
#include <dirent.h>
#include <thread>
#include <omp.h>
#include <unistd.h>

using namespace cv;
using namespace std;

//Timer class instatiation
Timer mainTimer;

//GPU
bool enableGPU = false;
GPU gpu;



//Class made to write the output
class MyWriter {
protected:
    String debug_file;
    unsigned int current_frame;
    vector<int> image_params;
public:
    MyWriter(String debug_file) {
        this->debug_file = debug_file;
        this->current_frame = 0;
    }
    
	//Method that writes the images
    void write(Mat image) {
    	
        ostringstream frame_string;

        /* Get file name with leading 0s */
        frame_string << setw(10) << setfill('0') << current_frame;
        String file_name = debug_file + "-" + frame_string.str() + ".jpg";

        /* Resize if necessary */
        Mat smaller;
        resize(image, smaller, Size(), DEBUG_SIZE, DEBUG_SIZE, INTER_LINEAR);
        
        /* Write image to video file, and increment our frame number */
        imwrite(file_name, smaller, image_params);
        current_frame++;

        smaller.release();
    }
};

//Debug log generation stuff 
class LogData {
public:
    string team1, team2;
    int score1, score2;
    int endnumber, shotnumber;
    int calls; 
    int imagesPerShot = 3;

    //ENABLES THIS CLASS
    bool enabled = false;

    bool isRedFirst;
    LogData(){};
    LogData(string Team1, string Team2, bool IsRedFirst, int End){
        team1 = Team1;
        team2 = Team2;
        isRedFirst = IsRedFirst;
        score1 = 0;
        score2 = 0;
        shotnumber = 1;
        calls = 0;
        endnumber = End;
    }
    string step(){
        ostringstream buffer;
        calls++;
        buffer << "";
        if (calls % imagesPerShot == 0)
        {
            shotnumber++;
            buffer << endl;
            return buffer.str();
        }
        return buffer.str();
    }
};

clock_t start_clock;

clock_t prev_time;
const float FPS = 10;

//Main functions prototypes
void runCamera(char *background_file, const String &debug_file, bool debug);
void runImage(char *background_file, char *input, char *output, const String &debug_file, bool debug);
void runEnd(char *background_file, char *input_dir, char *output_dir, const String &debug_file, bool debug);

void processFrame(Mat background, Mat image, ShotDetector *shotDetector, MyWriter *vid_writer, vector<Rock> &red_rocks, vector<Rock> &yellow_rocks, String output_name="flat_perspective.jpg");

int main(int argc, char **argv)
{
    //These need to be adjusted before running into different pictures, the camera_setup
    //Method has more details

    /* End 2 */
    /*vector<Point2d> screen_corners { Point2d(374, 442 ),
                                     Point2d(2445, 438 ),
                                     Point2d(488, 1762),
                                     Point2d(2518, 1421)
                                   };*/
    //My image parameters
    vector<Point2d> screen_corners { Point2d(184, 288 ),
                                     Point2d(2352, 544 ),
                                     Point2d(144, 1680),
                                     Point2d(2308, 1564)
                                   };

    //Ryan game, cam 252
    /*vector<Point2d> screen_corners { Point2d(192, 156 ),
                                     Point2d(2324, 484 ),
                                     Point2d(108, 1540),
                                     Point2d(2252, 1496)
                                   };*/

    //Ryan game,  cam 67
    /*vector<Point2d> screen_corners { 
                                     Point2d(2460, 1504),
                                     Point2d(300, 1260),
                                     Point2d(2496, 148 ),
                                     Point2d(348, 236 )
                                   };
*/
    /* End 1 */
    //vector<Point2d> screen_corners {Point2d(268, 130), Point2d(2414, 438), Point2d(190, 1538), Point2d(2356, 1468)};
    
    //Change according to dataset of pictures you are using
    camera_setup(Size(2592, 1944), screen_corners);
    setupBasicCircle();
    
    //Timing stuff
    start_clock = clock();


    if (argc == 3 && 0 == strcmp(argv[1], "camera")) {
        runCamera(argv[2], "", false);
    }
    else if (argc == 4 && 0 == strcmp(argv[1], "camera")) {
        runCamera(argv[2], argv[3], true);
    }
    else if (argc == 5 && 0 == strcmp(argv[1], "image")) {
        runImage(argv[2], argv[3], argv[4], "", false);
    }
    else if (argc == 5 && 0 == strcmp(argv[1], "end")) {
        //"release" run
        runEnd(argv[2], argv[3], argv[4], "", false);
    }
    else if (argc == 6 && 0 == strcmp(argv[1], "image")) {
        runImage(argv[2], argv[3], argv[4], argv[5], true);
    }
    else if (argc == 6 && 0 == strcmp(argv[1], "end")) {
        //Debug run
        runEnd(argv[2], argv[3], argv[4], argv[5], true);
    } 
    else if (argc == 6 && 0 == strcmp(argv[1], "gpu")){
        enableGPU = true;
        //Debug run
        runEnd(argv[2], argv[3], argv[4], argv[5], true);
    }else if (argc == 5 && 0 == strcmp(argv[1], "gpu")) {
        enableGPU = true;
        //release run
         runEnd(argv[2], argv[3], argv[4], "", false);
    }
    else {
        printf("Camera usage: %s camera <background_image>\n", argv[0]);
        printf("Camera usage with debug: %s camera <background_image> <debug_root>\n", argv[0]);
        printf("Image usage: %s image <background_image> <input_image> <output_image>\n", argv[0]);
        printf("Image usage debug: %s image <background_image> <input_image> <output_image> <debug_root>\n", argv[0]);
        printf("Directory usage: %s end <background_image> <input_dir> <output_dir>\n", argv[0]);
        printf("Directory usage debug output: %s end <background_image> <input_dir> <output_dir> <debug_root>\n", argv[0]);
        printf("The part that Diedre worked with is the ´end and gpu´ run, that runs in a whole directory of pictures using (gpu) or not using (end) the GPU for blur.\n");
        return -1;
    }

    return 0;
}

//Utility to check if an string ends with other string
bool ends_with(const std::string &suffix, const std::string &str)
{
    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

//Main method being developed
void runEnd(char *background_file, char *input_dir, char *output_dir, const String &debug_file, bool debug)
{
    if (debug) cout << endl << "DEBUG MODE IS ENABLED!" << endl << endl;
    else cout << endl << "DEBUG MODE IS DISABLED!" << endl << endl;
    sleep(1);


    MyWriter vid_writer(debug_file);
    //Initialize debug stuff
    ofstream debug_text;
    if (debug) {
        debug_text.open(debug_file + ".txt");
    }

    //Initialize background image
    Mat background = imread(background_file, 1);
    ShotDetector shotDetector(background, debug);
    //

    //Initialize GPU if used
    if (enableGPU){
        gpu = GPU(background_file);
    }


    //Array of file names
    vector<String> all_files;
    //Pointer to input directory
    DIR *dir_ptr = opendir(input_dir);

    while (true) {
        dirent *dir_ent = readdir(dir_ptr);

        if (NULL == dir_ent) {
            break;
        }

        all_files.push_back(string(dir_ent->d_name));
    }
    closedir(dir_ptr);

    //Sort file names 
    sort(all_files.begin(), all_files.end());

    vector<String> image_files;
    for (String file : all_files) {
        if (ends_with(".jpg", file)) {
            image_files.push_back(file);
        }
    }
    LogData debugLog;
     
    if (debugLog.enabled) {
        cout << endl << "DEBUGLOG GENERATION IS ENABLED! be careful this has to be manually checked." << endl << endl;
        string Team1, Team2;
        bool IsRedFirst;
        int End;
        cout << "Type the team name of the first team to shoot and the second." << endl;
        cin >> Team1;
        cin >> Team2;
        cout << "Is red the first team's rock? (1 for yes 0 for no)" << endl;
        cin >> IsRedFirst;
        cout << "End Number?" << endl;
        cin >> End;
        debugLog = LogData(Team1, Team2, IsRedFirst, End);
        debug_text << "Debug game log" << endl;
        
    } else {
        cout << endl << "DEBUGLOG GENERATION IS DISABLED! Change bool in LogData class to enable." << endl << endl;
        sleep(1);
    }
    //
    EndState myState;
    for (String file : image_files) {
	Mat image;	
        image = imread(string(input_dir) + "/" + file, 1);

        vector<Rock> red_rocks, yellow_rocks;
        String out_file = string(output_dir) + "/" + file;
        if (debug) {
            processFrame(background, image,  &shotDetector, &vid_writer, red_rocks, yellow_rocks, out_file);
        }
        else {
            processFrame(background, image, &shotDetector, NULL, red_rocks, yellow_rocks, out_file);
        }


        myState.advanceFrame(1, red_rocks, yellow_rocks);
        cout << "myState: " << myState.toString(true) << endl << endl;


        //Debug log builder, not very efficient
        if (debugLog.enabled){
            unsigned int i, j;
            i = 0;
            j = 0;

            debug_text << debugLog.team1 << "," << debugLog.team2 << "|" << debugLog.score1 << "," << debugLog.score2 << "|" << debugLog.endnumber << "," << debugLog.shotnumber << "|";

            if (debugLog.isRedFirst){ 
                for (Rock rock : red_rocks) {
                    debug_text << rock.center.x << "," << rock.center.y;
                    if (++i < red_rocks.size()) debug_text << ":";
                }
                debug_text << "|";
                for (Rock rock : yellow_rocks) {
                    debug_text << rock.center.x << "," << rock.center.y;
                    if (++j < yellow_rocks.size()) debug_text << ":";
                }
            } else {
                for (Rock rock : yellow_rocks) {
                    debug_text << rock.center.x << "," << rock.center.y << ":";
                    if (++i < yellow_rocks.size()) debug_text << ":";
                }
                debug_text << "|";
                for (Rock rock : red_rocks) {
                    debug_text << rock.center.x << "," << rock.center.y << ":";
                    if (++j < yellow_rocks.size()) debug_text << ":";
                }
                
            }
            debug_text << " filename: " << file << endl;
            debug_text <<  debugLog.step();

        }
        //Original Debug output, rock positions/colors.
        else if (debug) {
            debug_text << "T:" << ((float)(clock() - start_clock)) / CLOCKS_PER_SEC << endl;

            debug_text << "Reds:" << endl;
            for (Rock rock : red_rocks) {
                debug_text << rock.radius << " | " << rock.center << endl;
            }

            debug_text << "Yellows:" << endl;
            for (Rock rock : yellow_rocks) {
                debug_text << rock.radius << " | " << rock.center << endl;
            }
            debug_text << endl;

            
        }

        mainTimer.updateGameTime();
        mainTimer.getProgramTime();
    }
}

//Run in camera feed.
void runCamera(char *background_file, const String &debug_file, bool debug)
{
    //load our background image 
    Mat background = imread(background_file, 1);
    //Capture from the camera 
    VideoCapture camera(0);

    //Make sure we have the desired width / height. Default is different. 
    camera.set(CV_CAP_PROP_FRAME_WIDTH, STILL_WIDTH);
    camera.set(CV_CAP_PROP_FRAME_HEIGHT, STILL_HEIGHT);

    MyWriter vid_writer(debug_file);

    Mat image;

    ofstream debug_text;
    if (debug) {
        debug_text.open(debug_file + ".txt");
    }

    while (1) {
        prev_time = clock();
        while (((float)clock() - (float)prev_time) / CLOCKS_PER_SEC < 1/FPS) { }

        // Grab a frame from the camera 
        camera.read(image);

        // Process the frame to find rocks. 
        vector<Rock> red_rocks, yellow_rocks;
        
        if (debug) {
            processFrame(background, image,  NULL, &vid_writer, red_rocks, yellow_rocks);
        }
        else {
            processFrame(background, image, NULL, NULL, red_rocks, yellow_rocks);
        }

        if (debug) {
            debug_text << "T:" << ((float)(clock() - start_clock)) / CLOCKS_PER_SEC << endl;

            debug_text << "Reds:" << endl;
            for (Rock rock : red_rocks) {
                debug_text << rock.radius << " | " << rock.center << endl;
            }

            debug_text << "Yellows:" << endl;
            for (Rock rock : yellow_rocks) {
                debug_text << rock.radius << " | " << rock.center << endl;
            }
        }

        // Free image
        image.release();
    }

    if (debug) {
        debug_text.close();
    }
}

//Run in one image
void runImage(char *background_file, char *input, char *output, const String &debug_file, bool debug)
{
    MyWriter vid_writer(debug_file);
    ofstream debug_text;
    if (debug) {
        debug_text.open(debug_file + ".txt");
    }

    Mat image = imread(input, 1);
    Mat background = imread(background_file, 1);

    Mat undistorted;
    undistort(image, undistorted, CAMERA_MATRIX, DIST_COEFFS);

    vector<Rock> red_rocks, yellow_rocks;

    if (debug) {
        processFrame(background, image,  NULL, &vid_writer, red_rocks, yellow_rocks);
    }
    else {
        processFrame(background, image, NULL, NULL, red_rocks, yellow_rocks);
    }

    if (debug) {
        debug_text << "T:" << ((float)(clock() - start_clock)) / CLOCKS_PER_SEC << endl;

        debug_text << "Reds:" << endl;
        for (Rock rock : red_rocks) {
            debug_text << rock.radius << " | " << rock.center << endl;
        }

        debug_text << "Yellows:" << endl;
        for (Rock rock : yellow_rocks) {
            debug_text << rock.radius << " | " << rock.center << endl;
        }
    }
    
    image.release();
}

void processFrame(Mat background, Mat image, ShotDetector *shotDetector, MyWriter *vid_writer, vector<Rock> &red_rocks, vector<Rock> &yellow_rocks, String output_name)
{
    if (NULL != vid_writer) {
        //vid_writer->write(image);
    }
    mainTimer.methodTimerStart();

    #pragma omp parallel sections
    { 
        /* Find our rocks */
        #pragma omp section
            findRocksCircles(background, image, red_rocks, yellow_rocks,  enableGPU, true, output_name);
        /* Detect Shots */
        #pragma omp section
            if (shotDetector != NULL) shotDetector -> analyzeFrame(image);
    }

    mainTimer.getMethodTimer("whole frame");
    
}


