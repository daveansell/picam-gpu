#pragma once

#include "rocks.hpp"
#include "kalman.hpp"


/*
  A single frame of Kalman rocks. Each KalmanRock represents a track
  in our multiple object tracker.
 */
class KalmanFrame {
    vector<KalmanRock> red_kalmans;
    vector<KalmanRock> yellow_kalmans;

public:
    KalmanFrame()
    {

    }

    KalmanFrame(vector<KalmanRock> red_kalmans, vector<KalmanRock> yellow_kalmans)
    {
        this->red_kalmans = red_kalmans;
        this->yellow_kalmans = yellow_kalmans;
    }

    void timeUpdate(double time_step);
    void rockUpdate(vector<Rock> &red_rocks, vector<Rock> &yellow_rocks, double time_step, size_t frame);
    String toString(bool redFirst);
};



/*
  Keep track of the state of a curling end.

  This will let us construct a log of the events.

  This does not immediately break things up into turns, but instead
  keeps a log of expected rock positions.
 */
class EndState {
    vector<KalmanFrame> frames;
    vector<double> frame_times;

public:
    EndState()
    {
    }


    /* Take another frame into account for the end.

       This function will remember obscured rocks, and try to smooth
       out stationary rocks by using Kalman filters.
     */

    void advanceFrame(double time_step, vector<Rock> &red_rocks, vector<Rock> &yellow_rocks);

    String toString(bool redFirst);
};
