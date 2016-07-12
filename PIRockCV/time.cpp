#include "time.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

#include <cstdlib>

using namespace std;

int frameCount = 0;

Timer::Timer(){
	realTime = clock();
	startTime = clock();
	methodTimer = 0;
	cout.precision(2);
}

void Timer::updateGameTime(){
	frameCount++;
	cout << "Game time: " << frameCount*TIME_STEP << "s" << endl;
}

void Timer::getProgramTime(){
	cout << "Program running time: " << (clock() - startTime)/CLOCKS_PER_SEC << "s" << endl;
}

void Timer::methodTimerStart(){
	methodTimer = clock();
}

void Timer::getMethodTimer(string methodName){
	clock_t methodEndTime = methodTimer;
	methodTimer = 0;
	cout << methodName << " time: " << (float)(clock() - methodEndTime)/CLOCKS_PER_SEC << "s" << endl;
}




