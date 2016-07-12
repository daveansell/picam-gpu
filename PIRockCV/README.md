This program is used to track the rocks in the scene.

# Building

    cmake . && make
    
You need OpenCV 3, or possibly 2.4.9 if you're looking at an older
version (they are different). I might recommend looking into Nix if
you find the need to switch between these:

https://nixos.org/nix/

# old-state-shot-detect

There is an old branch called old-state-shot-detect which attempted to
generate the logs in real time on every frame with no history by
detecting when a shot enters the scene after rocks have settled and
then using this as a delimeter for turns. Unfortunately this
encountered problems due to the rock detection not always detecting
rocks, which made it difficult to tell when shots actually occurred
sometimes. In particular if we don't detect a rock until it's almost
all of the way down the sheet then it's really difficult to tell if
it's a new rock entering the scene, or some single frame false
positive (such as a balding head).

In order to decrease the chances of these false positives this version
tried to detect shots with only the colour of the rock that we were
expecting. Unfortunately this can also cause problems because if you
just don't see the rock, then everything gets out of sync.

I believe this version depends upon OpenCV 2.4.9 as it was before I
transfered everything over to OpenCV 3.

# master

The current version does away with trying to identify the logs in real
time for now, and instead is focusing on the rock tracking. This
version has been moved to OpenCV3, but not tested very extensively
yet.

# main.cpp

This is a bit of a nightmare because this program was adapted to
handle a bunch of different things while under development. There are
a number of cases for what it does depending on the arguments
given. You want the last ones which run on a directory of a game. E.g.,

    ./rock-track end ~/game-2016-03-06/pi2/background.jpg ~/game-2016-03-06/pi2/end4/ output

Make sure you create the output directory first.

It's also important to note that main sets up the camera matrices (for
perspective correction, and such). I was simply replacing the screen_corners variable with a vector of the screen space coordinates (of a distorted, straight from the camera image) for points of interest. These correspond to the "real-space" coordinates:

    static vector<Point2d> real_corners { Point2d(BACK_LINE, -BACK_LINE_WIDTH/2),
                                          Point2d(HOG_LINE, -HOG_LINE_WIDTH/2),
                                          Point2d(BACK_LINE, BACK_LINE_WIDTH/2),
                                          Point2d(HOG_LINE, HOG_LINE_WIDTH/2)
                                        };
                                        
I.e., the backline with -y, hogline with -y, backline with +y, and hogline with +y.

# camera_constants.cpp

This sets up some of the important values for our camera for
distortion correction, perspective correction, and transformations
between screen space and real space.

The camera matrix and distortion coefficients were computed using
this:

http://docs.opencv.org/3.1.0/d4/d94/tutorial_camera_calibration.html#gsc.tab=0

It should be the same for all cameras, as long as we have the same
lenses, and image sensors with the same size.

# rocks.cpp

## findRocks

There are currently two methods for detecting rocks in a scene. There
is a complete hodge-podge method called findRocks which performs
background subtraction, and colour based thresholding. After this
contours are found and we simply check for ones that are vaguely
circular, and roughly about the right size. This could be improved by
a more robust notion of "circular". This method is much faster than
findRocksCircles, but it has a bit lower accuracy, and produces more
false positives.

## findRocksCircles

findRocksCircles is more accurate. It performs a perspective
correction, and undistortion of the full image, and then uses this to
find circles of the appropriate size using a Hough circle transform on
background subtracted / colour thresholded versions of the perspective
corrected image. Unfortunately these operations are quite slow,
especially on the Raspberry Pi, which makes it not particularly viable
for real time use.

We were going to implement our own Hough circle transform which used a
large cache of circles which have perspective / distortion applied to
them. There might still be some code from this left in the codebase
for generating all such circles, but there were a couple of problems
with this approach.

    1) This would take up a fairly big chunk of disk space, you can
       alleviate some of this by only making a new circle for every
       couple of pixels...
    2) The Canny Edge detector is much to slow on the Raspberry Pi,
       taking almost a full second to run.
    3) I'm relatively certain that our code would not have been fast enough anyway.
    
test.cpp has to do with this. If you end up looking at this make sure you read:

https://en.m.wikipedia.org/wiki/Circle_Hough_Transform

What we can potentially do to improve this is to note that we're
mostly going to have rocks where the image differs from the
background, and where we see colours in the expected ranges in the
images. So we could perform the perspective / distortion correction
only on these chunks of the image, and then perform circle detection
on these smaller chunks. You'll have to perspective correct /
undistort the coordinates of the corners of these regions in order to
know what this would correspond to in the undistorted / perspective
corrected image. There are functions for that in camera_transforms.cpp

## Possibilities for improvement in general

One avenue for improvement is to focus detection on the areas where we
expect rocks. For example we should look for incoming rocks near the
hogline. Unfortunately it seems that some teams may have the rock
covered almost entirely all of the way to the house. Thus looking for
new rocks only near the hogline is actually not a particularly stable
approach. For this reason I really believe that you do want to check
over the full image first.

Perhaps it makes sense to try rock detection on a much smaller image,
e.g., 640x480, and then when a possible rock is encountered perform
the more accurate rock detection on the higher resolution version of
the frame in the specific area. I suspect this is the most reasonable
approach.


# kalman.cpp (Kalman Filters)

In order to keep track of the rocks we use Kalman filters. This may be
a useful reference:

http://www.bzarg.com/p/how-a-kalman-filter-works-in-pictures/

(The code might look a bit different than what was there because our
sensor matrix is just an identity matrix, so I simplified based on
that)

The basic idea is to have a guassian distribution around your point
representing where it *might* be. There are uncertainties in what we
detect from sensors, and we might not detect the rocks in every frame,
so our certainty about where the rock is decreases.

I elected to make the switch to Kalman filters (instead of the
slightly simpler approach of just detecting rocks and counting how
many frames you see them) for a couple of reasons. The Kalman filters
are simply the right solution for the problem, and I was encountering
some problems with what I had written before. The Kalman filters make
it much nicer to determine the associations of rocks between frames
because they actually give you a probability distribution, and account
for velocities. Without this it's difficult to determine when rocks
should be associated with an existing KalmanRock (a kalman filter for
a rock), since if it was moving at a high speed, or if you didn't see
it for a few frames it might be a significant distance from its
previous location. So it's hard to tell if you're seeing a new rock,
or an old one that just moved more than your distance cut-off for
pairing the rocks in a new frame to previously seen rocks. So,
refactoring the code to use Kalman filters helped to solve these
problems. It also makes dealing with obstructions, and rocks that have
gone out of play a bit easier because the Kalman filters use a basic
model of the physics of the rocks so you can tell where they *would*
roughly be if they keep a constant velocity, even if they're obscured.

The rocks are paired with rocks from the previous frame based on the
largest eigenvalue of the covariance matrix of the KalmanRock if it
were to be updated using the currently seen rock. I haven't tuned this
parameter much, but there is also a threshold for when the minimum of
these has a covariance matrix eigenvalue that's too large. If this
happens then we consider it a new rock because it is unlikely to be
associated with any of the existing KalmanRocks

Currently this makes no attempt to remove false positives, but their
covariance matrices after a few frames will have very large maximum
eigenvalues. If the covariance matrix has an eigenvalue that is very
large, then the rock is probably out of play, or not a real rock.

This is useful information for building logs, because a rock that has
only been seen for a few frames, and has a covariance matrix with
massive eigenvalues is very likely a false positive and can be
ignored, so when generating logs you can weed these out.

The uncertainty matrices for the Kalman filters probably needs some
tuning as well. For instance we are more uncertain of velocities than
we are of positions in sensor readings, and I believe some of the
uncertainty matrix values + incorrect associations of rocks with
kalman filters can cause the rocks to shift a bit from their actual
position.

Also it might be a good idea to look into multiple hypothesis tracking
for reference. Kalman filters are only one part of the rock tracker,
and they only work if you associate them with a particular rock.

# ends.cpp

This includes some stuff that is used for keeping track of rocks in an
end. Basically it keeps track of the state of all KalmanRocks that we
have currently seen, performs a time update on them, and if it's
associated with another rock it updates it with the rocks location
(this is considered a "sensor reading").

The associations with rocks are done by greedily selecting the freshly
seen rock and old KalmanRock which minimizes some metric (in this case
maximum covariance eigenvalue, as discussed in the kalman.cpp section,
but you could also swap this out for distance, etc.).

# Generating logs

Since shifting things over from generating logs on the fly, to just
keeping track of the rocks individually (which simplified a lot of
things), there is currently not a way to generate logs. The plan was
to generate the logs using a method in EndState. This would traverse
through the history of the KalmanRocks (saved each frame), and attempt
to detect turns by looking for stability over a few frames, and seeing
if changes start to occur with rocks that are seen for more than a few
frames, with reasonable covariance matrices. Snapshots of these logs
could be saved to start at a later point and save time, but I'm not
sure if it will be necessary yet.

With this implemented and some tuning (probably to the initial Kalman
matrices, the rock detector), this should be able to produce logs of
the desired format pretty well, and hopefully by detecting how many
turns have been completed you should be able to tell when an end has
finished. In order to deal with people releasing rocks on one end of
the ice it would probably make sense to keep track of rocks with
velocities that seem to be towards the hogline, which means people are
probably shooting from that end. This won't be perfect, though,
because rocks are difficult to see during this. Might have to detect
people / communicate between the pis to see which one is most certain
that they have the active house.
