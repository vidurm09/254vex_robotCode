#pragma config(Sensor, dgtl1,  flyEncoder,     sensorQuadEncoder)
#pragma config(Sensor, dgtl3,  led1,           sensorDigitalOut)
#pragma config(Sensor, dgtl4,  led2,           sensorDigitalOut)
#pragma config(Motor,  port2,           rightDrive,    tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port3,           leftDrive,     tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port4,           roll,          tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port5,           triangle,      tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port6,           rightFrontOut, tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port7,           rightBackOut,  tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port8,           leftFrontOut,  tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port9,           leftBackOut,   tmotorVex393_MC29, openLoop)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

#pragma platform(VEX)

//Competition Control and Duration Settings
#pragma competitionControl(Competition)
#pragma autonomousDuration(20)
#pragma userControlDuration(120)

#include "Vex_Competition_Includes.c"   //Main competition background code...do not modify!

struct pidValues // an "array" for storing changing PID values
{
	float target;
	float integralSum;
	float currentDistance;
	float lastDistance;
	float derivative;
}

struct pidValues flyPID;
bool isRampedUp = false;
float outakeSpeed = 0;
bool isRamping = false;
bool isSlowing = false;
int sampleTime = 30; // in milliseconds, amount of delay between PID/RPM calculations
float proportionalMod = 18; // modifiers of each section of PID
float integralMod = 0.003;
float derivativeMod = 1;
float wheelRPM;
float lastEncoder;

void outake(int power)
{
	motor[leftBackOut] = power;
	motor[leftFrontOut] = power;
	motor[rightBackOut] = power;
	motor[rightFrontOut]= power;

}
////////////////////////////////////////////////////////////////////////////////

// Update inteval (in mS) for the flywheel control loop
#define FW_LOOP_SPEED              25

// Maximum power we want to send to the flywheel motors
#define FW_MAX_POWER              127

// encoder counts per revolution depending on motor
#define MOTOR_TPR_269           240.448
#define MOTOR_TPR_393R          261.333
#define MOTOR_TPR_393S          392
#define MOTOR_TPR_393T          627.2
#define MOTOR_TPR_QUAD          360.0

// Structure to gather all the flywheel ralated data
typedef struct _fw_controller {
    long            counter;                ///< loop counter used for debug

    // encoder tick per revolution
    float           ticks_per_rev;          ///< encoder ticks per revolution

    // Encoder
    long            e_current;              ///< current encoder count
    long            e_last;                 ///< current encoder count

    // velocity measurement
    float           v_current;              ///< current velocity in rpm
    long            v_time;                 ///< Time of last velocity calculation

    // TBH control algorithm variables
    long            target;                 ///< target velocity
    long            current;                ///< current velocity
    long            last;                   ///< last velocity
    float           error;                  ///< error between actual and target velocities
    float           last_error;             ///< error last time update called
    float           gain;                   ///< gain
    float           drive;                  ///< final drive out of TBH (0.0 to 1.0)
    float           drive_at_zero;          ///< drive at last zero crossing
    long            first_cross;            ///< flag indicating first zero crossing
    float           drive_approx;           ///< estimated open loop drive

    // final motor drive
    long            motor_drive;            ///< final motor control value
    } fw_controller;

// Make the controller global for easy debugging
static  fw_controller   flywheel;

/*-----------------------------------------------------------------------------*/
/** @brief      Set the flywheen motors                                        */
/** @param[in]  value motor control value                                      */
/*-----------------------------------------------------------------------------*/
void
FwMotorSet( int value )
{
    outake(value);
}

/*-----------------------------------------------------------------------------*/
/** @brief      Get the flywheen motor encoder count                           */
/*-----------------------------------------------------------------------------*/
long
FwMotorEncoderGet()
{
    return( SensorValue[flyEncoder]/12 );
}

/*-----------------------------------------------------------------------------*/
/** @brief      Set the controller position                                    */
/** @param[in]  fw pointer to flywheel controller structure                    */
/** @param[in]  desired velocity                                               */
/** @param[in]  predicted_drive estimated open loop motor drive                */
/*-----------------------------------------------------------------------------*/
void
FwVelocitySet( fw_controller *fw, int velocity, float predicted_drive )
{
    // set target velocity (motor rpm)
    fw->target        = velocity;

    // Set error so zero crossing is correctly detected
    fw->error         = fw->target - fw->current;
    fw->last_error    = fw->error;

    // Set predicted open loop drive value
    fw->drive_approx  = predicted_drive;
    // Set flag to detect first zero crossing
    fw->first_cross   = 1;
    // clear tbh variable
    fw->drive_at_zero = 0;
}

/*-----------------------------------------------------------------------------*/
/** @brief      Calculate the current flywheel motor velocity                  */
/** @param[in]  fw pointer to flywheel controller structure                    */
/*-----------------------------------------------------------------------------*/
void
FwCalculateSpeed( fw_controller *fw )
{
    int     delta_ms;
    int     delta_enc;

    // Get current encoder value
    fw->e_current = FwMotorEncoderGet();

    // This is just used so we don't need to know how often we are called
    // how many mS since we were last here
    delta_ms   = nSysTime - fw->v_time;
    fw->v_time = nSysTime;

    // Change in encoder count
    delta_enc = (fw->e_current - fw->e_last);

    // save last position
    fw->e_last = fw->e_current;

    // Calculate velocity in rpm
    fw->v_current = (1000.0 / delta_ms) * delta_enc * 60.0 / fw->ticks_per_rev;
}

/*-----------------------------------------------------------------------------*/
/** @brief      Update the velocity tbh controller variables                   */
/** @param[in]  fw pointer to flywheel controller structure                    */
/*-----------------------------------------------------------------------------*/
void
FwControlUpdateVelocityTbh( fw_controller *fw )
{
    // calculate error in velocity
    // target is desired velocity
    // current is measured velocity
    fw->error = fw->target - fw->current;

    // Use Kp as gain
    fw->drive =  fw->drive + (fw->error * fw->gain);

    // Clip - we are only going forwards
    if( fw->drive > 1 )
          fw->drive = 1;
    if( fw->drive < 0 )
          fw->drive = 0;

    // Check for zero crossing
    if( sgn(fw->error) != sgn(fw->last_error) ) {
        // First zero crossing after a new set velocity command
        if( fw->first_cross ) {
            // Set drive to the open loop approximation
            fw->drive = fw->drive_approx;
            fw->first_cross = 0;
        }
        else
            fw->drive = 0.5 * ( fw->drive + fw->drive_at_zero );

        // Save this drive value in the "tbh" variable
        fw->drive_at_zero = fw->drive;
    }

    // Save last error
    fw->last_error = fw->error;
}

/*-----------------------------------------------------------------------------*/
/** @brief     Task to control the velocity of the flywheel                    */
/*-----------------------------------------------------------------------------*/
task
FwControlTask()
{
    fw_controller *fw = &flywheel;

    // Set the gain
    fw->gain = 0.00025;

    // We are using Speed geared motors
    // Set the encoder ticks per revolution
    fw->ticks_per_rev = MOTOR_TPR_QUAD;

    while(1)
        {
        // debug counter
        fw->counter++;

        // Calculate velocity
        FwCalculateSpeed( fw );

        // Set current speed for the tbh calculation code
        fw->current = fw->v_current;

        // Do the velocity TBH calculations
        FwControlUpdateVelocityTbh( fw ) ;

        // Scale drive into the range the motors need
        fw->motor_drive  = (fw->drive * FW_MAX_POWER) + 0.5;

        // Final Limit of motor values - don't really need this
        if( fw->motor_drive >  127 ) fw->motor_drive =  127;
        if( fw->motor_drive < -127 ) fw->motor_drive = -127;


        // and finally set the motor control value
        FwMotorSet( fw->motor_drive );

        // Run at somewhere between 20 and 50mS
        wait1Msec( FW_LOOP_SPEED );
        }
}




/////////////////////////////////////////////////////////////////////////////////////

void clearPID(struct pidValues* values) // clear a PID's values
{
	values->target = 0;
	values->integralSum = 0;
	values->currentDistance = 0;
	values->lastDistance = 0;
	values->derivative = 0;
}


float calcPID(float encoder, struct pidValues* values) // calculates PID and updates values
{
	values->integralSum += encoder; // adds encoder value to the integral sum every calculation

	float output = (values->target-encoder)*proportionalMod + // "P" of PID, target subtracted from current distance
		(values->integralSum)*integralMod + // "I" of PID, calculated above
		(encoder-values->lastDistance)/sampleTime*derivativeMod; // "D" of PID, current distance minus last distance over time between last calculation

	values->lastDistance = values->currentDistance; // setting changing values in the "array"
	values->currentDistance = encoder;
	values->derivative = (encoder-values->lastDistance)/sampleTime;

	if (output > 127) // clamping PID output to -127 or 127, maximum motor values
		return 127;
	else if (output < -127)
		return -127;
	else
		return output;
}

task statusRPM() // updates the flywheel RPM constantly
{
	while(true)
	{
		wheelRPM = (SensorValue[flyEncoder]-lastEncoder) / sampleTime * 1000 * 60;
		lastEncoder = SensorValue[flyEncoder];
		wait1Msec(sampleTime);
	}
}

task pidRPM() // started in flywheelAdjust()
{
	while(true)
	{
		outake(outakeSpeed + calcPID(wheelRPM, flyPID)); // target is a set speed, so output is added/subtracted to the speed
		wait1Msec(sampleTime);
	}
}

void flywheelAdjust(float targetRPM) // sets PID target for flywheel to reach and starts pidRPM()
{
	flyPID.target = targetRPM;
	startTask(pidRPM);
	isRampedUp = true;
}

void rampDown( int time )
{
	stopTask(pidRPM); // rampDown is assumed to be called only after flywheelAdjust()
	for(int i = outakeSpeed; i > 0 ; i--)
	{
		outakeSpeed = i;
		outake(outakeSpeed);
		wait1Msec( time );
	}
	isRampedUp = false;
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//                          Pre-Autonomous Functions
//
// You may want to perform some actions before the competition starts. Do them in the
// following function.
//
/////////////////////////////////////////////////////////////////////////////////////////

void pre_auton()
{
  // Set bStopTasksBetweenModes to false if you want to keep user created tasks running between
  // Autonomous and Tele-Op modes. You will need to manage all user created tasks if set to false.
  bStopTasksBetweenModes = true;
	startTask(statusRPM);
	// All activities that occur before the competition starts
	// Example: clearing encoders, setting servo positions, ...
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//                                 Autonomous Task
//
// This task is used to control your robot during the autonomous phase of a VEX Competition.
// You must modify the code to add your own robot specific commands here.
//
/////////////////////////////////////////////////////////////////////////////////////////

task autonomous()
{
  // .....................................................................................
  // Insert user code here.
  // .....................................................................................

	AutonomousCodePlaceholderForTesting();  // Remove this function call once you have "real" code.
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//                                 User Control Task
//
// This task is used to control your robot during the user control phase of a VEX Competition.
// You must modify the code to add your own robot specific commands here.
//
/////////////////////////////////////////////////////////////////////////////////////////

void rampUp( int time )
{
	for(int i = 0; i <= 80; i++)
	{
		outakeSpeed = i;
		outake(outakeSpeed);
		wait1Msec( time );

	}
	isRampedUp = true;

}

void slowDown( int time )
{
	int newSpeed = outakeSpeed - 10;
	for( int i = outakeSpeed; i > newSpeed; --i)
	{
		outakeSpeed = i;
		outake(outakeSpeed);
		wait1Msec( time );
	}
}





void outakeStop()
{
	motor[leftBackOut] = 0;
	motor[leftFrontOut] = 0;
	motor[rightBackOut] = 0;
	motor[rightFrontOut]= 0;
}

task usercontrol()
{
	// User control code here, inside the loop
	// Start the flywheel control task
 	startTask( FwControlTask );
 	startTask(statusRPM);


	while (true)
	{
	 	motor[leftDrive] = vexRT[Ch3];
	 	motor[rightDrive] = vexRT[Ch2];

	 	if( vexRT[Btn6U] )
		{
			motor[roll] = 127;

		}
		else if( vexRT[Btn6D] )
		{
			motor[roll] = -127;
		}
		else
		{
			motor[roll] = 0;

		}

		if( vexRT[Btn5U] )
		{
			motor[triangle] = 80;
		}
		else if(vexRT[Btn5D])
		{
			motor[triangle] = -80;
		}
		else
		{
			motor[triangle] = 0;
		}
		/*
		if( vexRT[Btn8D] && !isRampedUp)
		{
			rampUp( 10 );
		}
		else if ( vexRT[Btn8D] && isRampedUp )
		{

			rampDown( 30 );
		}

		if( vexRT[Btn8R] && time1[T1] > 250)
		{
			if( outakeSpeed < 110)
			{
				outakeSpeed = outakeSpeed + 3;
				outake(outakeSpeed);

			}
			clearTimer(T1);
		}
		else if (vexRT[Btn8L] && time1[T1] > 250)
		{
			if( outakeSpeed > 65)
			{
				outakeSpeed = outakeSpeed - 3;
				outake(outakeSpeed);
			}
			clearTimer(T1);
		}
		if( vexRT[ Btn7L ] == 1 )
    	FwVelocitySet( &flywheel, 144, 0.55 );
    if( vexRT[ Btn7U ] == 1 )
      FwVelocitySet( &flywheel, 120, 0.38 );
    if( vexRT[ Btn7R ] == 1 )
      FwVelocitySet( &flywheel, 50, 0.2 );
    if( vexRT[ Btn7D ] == 1 )
      FwVelocitySet( &flywheel, 00, 0 );
    */



	}
}