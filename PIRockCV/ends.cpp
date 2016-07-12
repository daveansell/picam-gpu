#include "ends.hpp"

#include <fstream>
#include <iomanip>
#include <cmath>

using namespace std;

vector< tuple<size_t, size_t, double> > kalmanPairings(vector<KalmanRock> kalmans, vector<Rock> rocks, double time_step, vector<size_t> &rock_indices);


void KalmanFrame :: timeUpdate(double time_step)
{
    for (KalmanRock kalman : red_kalmans) {
        kalman.timeUpdate(time_step);
    }

    for (KalmanRock kalman : yellow_kalmans) {
        kalman.timeUpdate(time_step);
    }
}


static void trackUpdate(vector<KalmanRock> &kalmans, vector<Rock> &rocks, double time_step, size_t frame)
{
    /* Create associations */
    /* Find min prob kalman rock pairs greedily. */
    /* If this is above probability threshold update kalman with rock, otherwise don't */
    vector<size_t> remaining_rocks;
    vector< tuple<size_t, size_t, double> > pairings = kalmanPairings(kalmans, rocks, time_step, remaining_rocks);

    for (tuple<size_t, size_t, double> pair : pairings) {
        size_t kalman_ind = get<0>(pair);
        size_t rock_ind = get<1>(pair);
        double metric_val = get<2>(pair);

        if (metric_val < COVAR_THRESH) {
            kalmans[kalman_ind].rockUpdate(rocks[rock_ind], time_step);
        }
        else {
            KalmanRock new_kalman(rocks[rock_ind], frame);
            kalmans.push_back(new_kalman);            
        }
    }

    /* Each rock that is not associated with a Kalman filter now becomes one */
    for (size_t rock_ind : remaining_rocks) {
        KalmanRock new_kalman(rocks[rock_ind], frame);
        kalmans.push_back(new_kalman);
    }
}


void KalmanFrame :: rockUpdate(vector<Rock> &red_rocks, vector<Rock> &yellow_rocks, double time_step, size_t frame)
{
    trackUpdate(red_kalmans, red_rocks, time_step, frame);
    trackUpdate(yellow_kalmans, yellow_rocks, time_step, frame);
}


void EndState :: advanceFrame(double time_step, vector<Rock> &red_rocks, vector<Rock> &yellow_rocks)
{
    /* Save frame */
    frame_times.push_back(time_step);

    /* Create next frame of KalmanRocks... */
    KalmanFrame next_frame;
    if (frames.size() > 0) {
        next_frame = frames.back();
    }

    next_frame.timeUpdate(time_step);
    
    /* Get KalmanRocks for each Rock we've seen this frame */
    /* Associate + create new tracks if necessary */
    next_frame.rockUpdate(red_rocks, yellow_rocks, time_step, frames.size());

    /* Now that we have KalmanRocks, save the frame */
    frames.push_back(next_frame);
}


/* Kalman, rock, prob */
tuple<ssize_t, ssize_t, double> minKalmanRockPair(vector<KalmanRock> &kalmans, vector<Rock> &rocks, double time_step, double metric(KalmanRock, Rock, double))
{
    if (kalmans.size() == 0 || rocks.size() == 0) {
        tuple<ssize_t, ssize_t, double> new_pair(-1, -1, -1);

        return new_pair;
    }

    double min = metric(kalmans[0], rocks[0], time_step);
    ssize_t min_rock_ind = 0;
    ssize_t min_kalman_ind = 0;
    for (size_t rock_ind = 0; rock_ind < rocks.size(); ++rock_ind) {
        for (size_t kalman_ind = 0; kalman_ind < kalmans.size(); ++kalman_ind) {
            double val = metric(kalmans[kalman_ind], rocks[rock_ind], time_step);

            if (val < min) {
                min_rock_ind = rock_ind;
                min_kalman_ind = kalman_ind;
                min = val;
            }
        }
    }

    tuple<ssize_t, ssize_t, double> new_pair(min_kalman_ind, min_rock_ind, min);
    return new_pair;
}

//Not used
/*static double distMetric(KalmanRock kalman, Rock rock, double time_step)
{
    return rockDist(kalman.rock, rock);
}*/


static double probMetric(KalmanRock kalman, Rock rock, double time_step)
{
    return kalman.kalmanCovarianceSize(rock, time_step);
}


/* Kalman, rock, prob */
vector< tuple<size_t, size_t, double> > kalmanPairings(vector<KalmanRock> kalmans, vector<Rock> rocks, double time_step, vector<size_t> &rock_indices)
{
    /* Index vectors */
    vector<size_t> kalman_indices;

    for (size_t i = 0; i < kalmans.size(); ++i) {
        kalman_indices.push_back(i);
    }

    for (size_t i = 0; i < rocks.size(); ++i) {
        rock_indices.push_back(i);
    }
    
    /* Pair rock with nearest kalman rock */
    vector< tuple<size_t, size_t, double> > pairs;
    while (1) {
        tuple<ssize_t, ssize_t, double> min_pair = minKalmanRockPair(kalmans, rocks, time_step, probMetric);

        ssize_t min_kalman_ind = get<0>(min_pair);
        ssize_t min_rock_ind = get<1>(min_pair);
        ssize_t min_val = get<2>(min_pair);

        if (min_kalman_ind == -1) {
            break;
        }

        tuple<size_t, size_t, double> new_pair(kalman_indices[min_kalman_ind], rock_indices[min_rock_ind], min_val);
        pairs.push_back(new_pair);

        /* Get rid of used indices */
        kalmans.erase(kalmans.begin() + min_kalman_ind);
        kalman_indices.erase(kalman_indices.begin() + min_kalman_ind);

        rocks.erase(rocks.begin() + min_rock_ind);
        rock_indices.erase(rock_indices.begin() + min_rock_ind);
    }

    return pairs;
}


String KalmanFrame :: toString(bool redFirst)
{
    ostringstream concat;
    if (redFirst) concat << toLogString(red_kalmans) << " | " << toLogString(yellow_kalmans);
    else concat << toLogString(yellow_kalmans) << " | " << toLogString(red_kalmans);
    return concat.str();
}


String EndState :: toString(bool redFirst)
{
    if (frames.size() > 0) {
        ostringstream concat;
        //concat << "(" << frames.size() << ")" << frames.back().toString(redFirst);
        concat << frames.back().toString(redFirst);
        return concat.str();
    }

    return "";
}
