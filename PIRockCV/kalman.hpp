#pragma once

#include "rocks.hpp"

/* mm / s */
extern Point2d DEFAULT_VELOCITY;
const double PROB_THRESH = 0.6;
const double COVAR_THRESH = 2;


/* Rock + Kalman Filter */
class KalmanRock {
public:
    /* State */
    Rock rock;

protected:
    Point2d velocity;

    /* Update matrices */
    Mat covariance;
    Mat covariance_inv;
    Mat sensor_uncertainty;
    Mat environmental_uncertainty;

public:
    /* Frame information */
    size_t frame_appeared;
    size_t total_frames;
    size_t frames_seen;

    KalmanRock(Rock rock, Point2d velocity, size_t frame)
    {
        this->rock = rock;
        this->velocity = velocity;

        this->frame_appeared = frame;
        frames_seen = 1;
        total_frames = 1;

        covariance = Mat::eye(4, 4, CV_64F);
        covariance_inv = covariance.inv();
        sensor_uncertainty = Mat::eye(4, 4, CV_64F);
        environmental_uncertainty = 2 * Mat::ones(4, 4, CV_64F);
    }


    KalmanRock(Rock rock, size_t frame) : KalmanRock(rock, DEFAULT_VELOCITY, frame)
    {
    }


    /* Update the Kalman filter with a time step.

     */

    void timeUpdate(double time_step);


    /* Update the Kalman filter based on a freshly seen rock.

     */
    void rockUpdate(Rock rock, double time_step);

    
    /* Probability that a rock is associated with this Kalman filter rock.

     */
    double rockProbability(Rock rock);

    
    double kalmanCovarianceSize(Rock rock, double time_step);

    /* Returns the largest eigenvalue of the covariance matrix.

     */
    double covarianceSize();


    Mat getState();
    void setState(Mat state);
    Mat getUpdateMatrix(double time_step);
    void resetFrames();
    double speed();
};


String toLogString(vector<KalmanRock> &kalmans);
