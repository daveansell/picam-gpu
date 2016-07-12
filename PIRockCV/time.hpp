#pragma once


#include <time.h>
#include <string>

const float TIME_STEP = 0.5;


class Timer {
	private:
	    clock_t startTime;
	    clock_t realTime;
	    clock_t methodTimer;
	public:
		Timer();
		void updateGameTime();
		void getProgramTime();
		void methodTimerStart();
		void getMethodTimer(std::string methodName);
};
