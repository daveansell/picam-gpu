#include "kalman.hpp"
#include <math.h>
#include <opencv2/core/core.hpp>


Point2d DEFAULT_VELOCITY(2000, 0);
using namespace std;


Mat stateFromRock(Rock rock, Point2d velocity)
{
    Mat state = Mat::eye(4, 1, CV_64F);

    state.at<double>(0,0) = rock.center.x;
    state.at<double>(1,0) = rock.center.y;
    state.at<double>(2,0) = velocity.x;
    state.at<double>(3,0) = velocity.y;

    return state;
}


Mat KalmanRock :: getState()
{
    return stateFromRock(rock, velocity);
}


void KalmanRock :: setState(Mat state)
{
    rock.center.x = state.at<double>(0,0);
    rock.center.y = state.at<double>(1,0);
    velocity.x = state.at<double>(2,0);
    velocity.y = state.at<double>(3,0);
}


/* Super unthreadsafe, the returned matrix does not copy update_M -_- */
Mat KalmanRock :: getUpdateMatrix(double time_step)
{
    static double update_M[4][4] = {
        {1, 0, time_step, 0},
        {0, 1, 0, time_step},
        {0, 0, 1, 0},
        {0, 0, 0, 1},
    };

    return Mat(4, 4, CV_64F, update_M);
}


void KalmanRock :: timeUpdate(double time_step) {
     /* Another frame of updates */
    ++total_frames;

    /* If no rocks correspond we can still make a guess, but our variance will be larger. */
    Mat update_matrix = getUpdateMatrix(time_step);
    Mat update_matrix_trans = update_matrix.t();

    Mat old_state = getState();
    Mat new_state = update_matrix * old_state;

    /* Update the covariance matrix */
    covariance = update_matrix * covariance * update_matrix_trans
        + environmental_uncertainty;

    covariance_inv = covariance.inv();
    setState(new_state);    
}

void KalmanRock :: rockUpdate(Rock rock, double time_step)
{
    /* We've seen the rock in another frame! */
    ++frames_seen;

    Mat K = covariance * (covariance + sensor_uncertainty).inv();

    double vx = (rock.center.x - this->rock.center.x) / time_step;
    double vy = (rock.center.y - this->rock.center.y) / time_step;
    Point2d rock_velocity(vx, vy);

    Mat cam_state = stateFromRock(rock, rock_velocity);

    Mat new_state = getState();
    new_state = new_state + K * (cam_state - new_state);
    covariance = covariance - K * covariance;

    covariance_inv = covariance.inv();
    setState(new_state);
}


double KalmanRock :: rockProbability(Rock rock)
{
    /* We will just assume that the rock has the same velocity as our
       filter, since we can't directly measure it. */
    Mat rock_state = stateFromRock(rock, this->velocity);
    Mat kalman_state = getState();

    /* We will assume a normal distribution */
    Mat mean_diff = rock_state - kalman_state;
    Mat mean_diff_trans = mean_diff.t();
    
    Mat exp_mat = (mean_diff_trans * covariance_inv) * mean_diff;
    double exponent = -exp_mat.at<double>(0,0)/2;
    double power = pow(M_E, exponent);

    double det = determinant(covariance);
    double prob = power / sqrt(pow(2 * M_PI, 4) * det);

    return prob;
}


double KalmanRock :: kalmanCovarianceSize(Rock rock, double time_step)
{
    KalmanRock copy = *this;
    copy.rockUpdate(rock, time_step);

    return copy.covarianceSize();
}


void KalmanRock :: resetFrames()
{
    frame_appeared = 0;
    total_frames = 1;
    frames_seen = 1;
}


double KalmanRock :: covarianceSize()
{
    Mat eigen_values;
    eigen(covariance, eigen_values);

    double max = *eigen_values.begin<double>();
    for (MatIterator_<double> it = eigen_values.begin<double>(); it != eigen_values.end<double>(); ++it) {
        if (*it > max) {
            max = *it;
        }
    }

    return max;
}


double KalmanRock :: speed()
{
    double vx = velocity.x * velocity.x;
    double vy = velocity.y * velocity.y;

    double speed = sqrt(vx + vy);

    return speed;
}


String toLogString(vector<KalmanRock> &kalmans)
{
    ostringstream log_concat;
    
    for (size_t i = 0; i < kalmans.size(); ++i) {
        KalmanRock kalman_rock = kalmans[i];
        Rock rock = kalman_rock.rock;

        log_concat << rock.center.x << "," << rock.center.y; //<< "," << kalman_rock.covarianceSize();

        if (i+1 < kalmans.size()) {
            log_concat << ":";
        }
    }

    return log_concat.str();
}
