#include "MCodeMotor.hpp"

#include <thread>
#include <chrono>
#include <iostream>
#include <cmath>

using namespace std;

// acceleration of motor 
const unsigned int MCodeMotor::DEFAULT_ACCELERATION = 10000;			
// deceleration of motor // both are kept same for easy calculation purposes	
const unsigned int MCodeMotor::DEFAULT_DECELERATION = 10000;				
const unsigned int MCodeMotor::DEFAULT_INITIAL_VELOCITY = 0;				// initial velocity of motor // kept 0 for easy calculation purposes
const unsigned int MCodeMotor::DEFAULT_MAXIMUM_VELOCITY = 1000;				//maximum velocity attained by the motor
const unsigned int MCodeMotor::DEFAULT_RUN_CURRENT = 80;					// current at run time 
const unsigned int MCodeMotor::DEFAULT_HOLD_CURRENT = 80;					// current at hold time
const unsigned int MCodeMotor::BLOCKING_REFRESH_RATE_MILLISECONDS = 50;		// check for anything blocking the motor rate ///// Time in milliseconds to poll the motor when blocking execution until the motor stops.
const unsigned int MCodeMotor::BLOCKING_DEFAULT_TIMEOUT_MILLISECONDS = 10000;	// maximum time till which motor can sustain block ///// Maximum time to block execution when waiting for the motor to stop.
const unsigned int MCodeMotor::HOME_RETRYS = 5;								// number of tries made by the motor to reach home location before giving up
const unsigned int MCodeMotor::HOME_RETRY_DELAY_MILLISECONDS = 1000;		// time difference between 2 retries of motor to reach home location
const unsigned int MCodeMotor::MOTOR_RESPONSE_TIMEOUT_MILLISECONDS = 500;	// maximum time waiting for the motor to respond to a given command
const unsigned int MCodeMotor::MOTOR_RESPONSE_SLEEP_TIME_MILLISECONDS = 15;	// wait time for motor to respond to the given command
const int MCodeMotor::DEFAULT_POSITION = -1322;								// displacement in terms of encoder counts the new home location with respect to the old one
const unsigned int MCodeMotor::ENCODER_COUNTS_PER_ROTATION = 4000;			// total number of encoder counts present per location
//const unsigned int MCodeMotor::MICRO_STEPS_PER_ROTATION = 51200;			// Total number of microsteps counts per revolutaion

/// creats a new MCodeMotor Code
MCodeMotor::MCodeMotor(const string& ipAddress, const unsigned int port)	
	:	ipAddress(ipAddress)
	,	port(port)
	,	socket() {}

/// connect to motor
bool MCodeMotor::connect()
{
	return is_connected = socket.connectToServer(ipAddress, port);
}

/// sends command to motor
string& MCodeMotor::sendCommand(const string& command)
{
	socket.sendString(command + "\r\n");	// motor recognises the command only if it has \r\n at its end hence appended at last
	this_thread::sleep_for(chrono::milliseconds(MOTOR_RESPONSE_SLEEP_TIME_MILLISECONDS));		// wait for next response
	response = socket.receiveString();		// capture the response from motor

	// Each response from the motor ends with a '?' or '>'.
	// Check to see that a '?' or '>' was recieved before continuing.
	chrono::steady_clock::time_point recieveStartTime = chrono::steady_clock::now(); 				// returns a time point representing the current time 
	while ((getResponse().find('?') == string::npos && getResponse().find('>') == string::npos) &&	// if the response is not '?' == -1 or not '>' == -1 AND
		(chrono::duration_cast<std::chrono::milliseconds>(											// time duration between current and received time is less than timout time 
			chrono::steady_clock::now() - recieveStartTime).count() <=
			MOTOR_RESPONSE_TIMEOUT_MILLISECONDS))
			
	{
		this_thread::sleep_for(chrono::milliseconds(MOTOR_RESPONSE_SLEEP_TIME_MILLISECONDS));		// wait for next response
		response = response + socket.receiveString();												// add new response to old response	
	}


	if (getResponse().find('?') == string::npos && getResponse().find('>') == string::npos)			// again find if the response is not '?' ==-1 or not '>' == -1
	{
		cout << response << endl;
		// The response does not contain a '?' or '>'.
		// The response timedout.
		cerr << "[Error] Timeout: No response from motor at " << ipAddress << ":" 
			<< port << "!" << endl;
		response = "";
	}

	// Get rid of the extra characters on the response.
	if (command.size() + 2 < response.size())
	{
		response = response.substr(command.size() + 2, response.size() - command.size() - 4);
	}
	return getResponse();
}

string& MCodeMotor::getResponse()
{
	return response;
}

bool MCodeMotor::getResponseBool(bool defaultValue)
{
	if (defaultValue)
	{
		// The returns true (1) if the return string does not contain 0
		return getResponse().find('0') == string::npos;
	}
	else
	{
		// The returns false (0) if the return string does not contain 1
		return getResponse().find('1') != string::npos;
	}
}

void MCodeMotor::initializeSettings(const unsigned int acceleration,
	const unsigned int deceleration, const unsigned int initialVelocity,
	const unsigned int maximumVelocity, const unsigned int runCurrent,
	const unsigned int holdCurrent)
{
	sendCommand("ST 0"); // Reset Stall Flag
	sendCommand("EE 1"); // Enable Encoder, all units based on encoder
	setAcceleration(acceleration);
	setDeceleration(deceleration);
	setInitialVelocity(initialVelocity);
	setMaximumVelocity(maximumVelocity);
	setRunCurrent(runCurrent);
	setHoldCurrent(holdCurrent);
}

void MCodeMotor::setAcceleration(const unsigned int acceleration)
{
	sendCommand("A " + to_string(acceleration)); // Set Acceleration
	this->acceleration = acceleration;
}

void MCodeMotor::setDeceleration(const unsigned int deceleration)
{
	sendCommand("D " + to_string(deceleration)); // Set Decleration
	this->deceleration = deceleration;
}

void MCodeMotor::setInitialVelocity(const unsigned int initialVelocity)
{
	sendCommand("VI " + to_string(initialVelocity)); // Set Initial Velocity
	this->initialVelocity = initialVelocity;
}

void MCodeMotor::setMaximumVelocity(const unsigned int maximumVelocity)
{
	sendCommand("VM " + to_string(maximumVelocity)); // Set Maximum Velocity (steps per second)
	this->maximumVelocity = maximumVelocity;
}

void MCodeMotor::setRunCurrent(const unsigned int runCurrent)
{
	sendCommand("RC " + to_string(runCurrent)); // Set Run Current
	this->runCurrent = runCurrent;
}

void MCodeMotor::setHoldCurrent(const unsigned int holdCurrent)
{
	sendCommand("HC " + to_string(holdCurrent)); // Set Hold Current
	this->holdCurrent = holdCurrent;
}

unsigned int MCodeMotor::getAcceleration()
{
	return acceleration;
}

unsigned int MCodeMotor::getDeceleration()
{
	return deceleration;
}

unsigned int MCodeMotor::getInitialVelocity()
{
	return initialVelocity;
}

unsigned int MCodeMotor::getMaximumVelocity()
{
	return maximumVelocity;
}

unsigned int MCodeMotor::getRunCurrent()
{
	return runCurrent;
}

unsigned int MCodeMotor::getHoldCurrent()
{
	return holdCurrent;
}

unsigned int MCodeMotor::getMoveRelativeTime(const double angle)
{
	unsigned int maximumVelocity = getMaximumVelocity();
	unsigned int acceleration = getAcceleration();
	unsigned int deceleration = getDeceleration();

	if (getInitialVelocity() != 0)
	{
		// TODO support non-zero initial velocity
		cerr << "[Error] Move time calculation with non-zero initial velocity "
			<< "not supported." << endl;
	}

	double encoderCounts = abs(ceil(
		angle * ENCODER_COUNTS_PER_ROTATION / 360.0));

	// this uses doubles to prevent overflow and rounding
	double rampUpCounts = 0.5 * maximumVelocity * maximumVelocity /
		acceleration;
	double rampDownCounts = 0.5 * maximumVelocity * maximumVelocity /
		deceleration;
	double rampCounts = rampUpCounts + rampDownCounts;

	bool reachFullSpeed = encoderCounts >= rampCounts;

	if (reachFullSpeed)
	{
		return 1000.0 * encoderCounts / maximumVelocity +
			1000.0 * maximumVelocity * (acceleration + deceleration) /
			(2 * (double)acceleration * deceleration);
	}
	else
	{
		return sqrt(2.0 * 1000 * 1000 *  encoderCounts * (acceleration + deceleration) /
			(acceleration * deceleration));
	}
}

double MCodeMotor::getMoveRelativeAngleAtTime(const double moveAngle, const unsigned int milliseconds)
{
	unsigned int maximumVelocity = getMaximumVelocity();
	unsigned int acceleration = getAcceleration();
	unsigned int deceleration = getDeceleration();

	unsigned int moveTime = getMoveRelativeTime(moveAngle);

	if (getInitialVelocity() != 0)
	{
		// TODO support non-zero initial velocity
		cerr << "[Error] Move time calculation with non-zero initial velocity "
			<< "not supported." << endl;
	}

	double encoderCounts = abs(ceil(
			moveAngle * ENCODER_COUNTS_PER_ROTATION / 360.0));

	// this uses doubles to prevent overflow and rounding
	double rampUpCounts = 0.5 * maximumVelocity * maximumVelocity /
		acceleration;
	double rampUpTime = 1000.0 * maximumVelocity / acceleration;

	double rampDownCounts = 0.5 * maximumVelocity * maximumVelocity /
		deceleration;
	double rampDownTime = 1000.0 * maximumVelocity / deceleration;

	double rampCounts = rampUpCounts + rampDownCounts;

	double fullSpeedCounts = encoderCounts - rampCounts;
	double fullSpeedTime = 1000.0 * fullSpeedCounts / maximumVelocity;

	double fullTime = rampUpTime + rampDownTime + fullSpeedTime;

	bool reachFullSpeed = encoderCounts >= rampCounts;

	if (!reachFullSpeed)
	{
		// TODO support angle calcuation when motor does not reach maximum velocity
		cerr << "[Error] Angle calculation not supported when motor does not "
			<< "reach maximum velocity" << endl;
	}

	double countsTraveled;
	if (milliseconds > moveTime)
	{
		// move complete
		countsTraveled = encoderCounts;
	}
	else if (milliseconds < rampUpTime)
	{
		// ramping up
		countsTraveled = 0.5 * acceleration * milliseconds / 1000.0 * milliseconds / 1000.0;
		
	}
	else if (milliseconds > moveTime - rampDownTime)
	{
		// ramping down
		double millisecondsRemaining = fullTime - milliseconds;
		countsTraveled = encoderCounts - 0.5 * deceleration * millisecondsRemaining / 1000.0 * millisecondsRemaining / 1000.0;
	}
	else
	{
		// full speed
		countsTraveled = rampUpCounts + maximumVelocity * (milliseconds - rampUpTime) / 1000.0;
	}

	return countsTraveled * 360.0 / ENCODER_COUNTS_PER_ROTATION;
}


// move the motor to the new home location aligned parallel with the frame of motor 
bool MCodeMotor::homeToIndex()
{
	bool success = false;	
	for (unsigned int i = 0; i < HOME_RETRYS && !success; i++) 
	{
		if (i != 0)
		{
			cerr << "[Error] Motor at " << ipAddress << ":" << port 
				<< " was unable to home to index. Retrying in " 
				<< HOME_RETRY_DELAY_MILLISECONDS << " milliseconds..." << endl;
			this_thread::sleep_for(chrono::milliseconds(HOME_RETRY_DELAY_MILLISECONDS));
		}
		// Move to the left to make sure the motor is past the index mark
		moveRelative(-20, 2000);
		sendCommand("HI 3"); // Home to Index Mark
		blockWhileMoving(5000);
		detectStall();
		sendCommand("PR I6"); // Read Encoder at Index
		success = getResponseBool(false);
	}

	if (success)
	{
		sendCommand("C2 " + to_string(-1 * DEFAULT_POSITION)); // Reset Encoder Count
		moveAbsolute(0);
	}
	else
	{
		cerr << "[Error] Motor at " << ipAddress << ":" << port 
			<< " was unable to home to index after " << HOME_RETRYS
			<< " trys." << endl;
		
	}
	return success;
}

bool MCodeMotor::moveAngleRelative(const float angle, const unsigned int timeoutMilliseconds)
{
	int encoderCounts = ceil((angle * ENCODER_COUNTS_PER_ROTATION) / 360);
	return moveRelative(encoderCounts, timeoutMilliseconds);
}

bool MCodeMotor::moveAngleAbsolute(const float angle, const unsigned int timeoutMilliseconds)
{
	int encoderCounts = ceil((angle * ENCODER_COUNTS_PER_ROTATION) / 360);
	return moveAbsolute(encoderCounts, timeoutMilliseconds);
}

bool MCodeMotor::moveRelative(const int motorSteps, const unsigned int timeoutMilliseconds)
{
	if (timeoutMilliseconds != 0)
	{
		sendCommand("ST 0"); // Reset Stall Flag
	}

	sendCommand("MR " + to_string(motorSteps));

	if (timeoutMilliseconds == 0)
	{
		return true;
	}

	bool timeout = !blockWhileMoving(timeoutMilliseconds);
	bool stall = detectStall();

	return timeout || stall;
}

bool MCodeMotor::moveAbsolute(const int motorSteps, const unsigned int timeoutMilliseconds)
{
	if (timeoutMilliseconds != 0)
	{
		sendCommand("ST 0"); // Reset Stall Flag
	}

	sendCommand("MA " + to_string(motorSteps));

	if (timeoutMilliseconds == 0)
	{
		return true;
	}

	bool timeout = !blockWhileMoving(timeoutMilliseconds);
	bool stall = detectStall();

	return timeout || stall;
	
}

bool MCodeMotor::blockWhileMoving(const unsigned int timeoutMilliseconds)
{
	chrono::steady_clock::time_point blockStartTime = chrono::steady_clock::now();
	while (isMoving(true) &&
		(chrono::duration_cast<std::chrono::milliseconds>(
			chrono::steady_clock::now() - blockStartTime).count() <=
			timeoutMilliseconds))
	{
		this_thread::sleep_for(chrono::milliseconds(BLOCKING_REFRESH_RATE_MILLISECONDS));
	}

	if (isMoving(true))
	{
		cerr << "[Error] Timeout: Motor at " << ipAddress << ":" << port 
			<< " did not finish moving in " << timeoutMilliseconds << " milliseconds." << endl;
		return false;
	}
	return true;
}

bool MCodeMotor::isMoving(bool defaultValue)
{
	sendCommand("PR MV"); // Check if Moving

	return getResponseBool(defaultValue);
}

bool MCodeMotor::detectStall()
{
	sendCommand("PR ST"); // Read Stall Flag
	bool stall = getResponseBool(false);

	if (stall)
	{
		cerr << "[Error] Stall detected with motor at " << ipAddress << ":" << port << "." << endl;
	}

	sendCommand("ST 0"); // Reset Stall Flag

	return stall;
}

