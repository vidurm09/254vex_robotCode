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


bool isRampedUp = false;
int outakeSpeed = 0;
bool isRamping = false;
bool isSlowing = false;
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
void outake(int power)
{
	motor[leftBackOut] = power;
	motor[leftFrontOut] = power;
	motor[rightBackOut] = power;
	motor[rightFrontOut]= power;

}
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

void rampDown( int time )
{
	for(int i = outakeSpeed; i > 0 ; i--)
	{
		outakeSpeed = i;
		outake(outakeSpeed);
		wait1Msec( time );
	}
	isRampedUp = false;
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

		if( vexRT[Btn8D] && !isRampedUp)
		{
			rampUp( 10 );
		}
		else if ( vexRT[Btn8D] && isRampedUp )
		{

			rampDown( 30 );
		}

		if( vexRT[Btn7U] && time1[T1] > 250)
		{
			if( outakeSpeed < 80)
			{
				outakeSpeed = outakeSpeed + 3;
				outake(outakeSpeed);

			}
			clearTimer(T1);
		}
		else if (vexRT[Btn7D] && time1[T1] > 250)
		{
			if( outakeSpeed > 65)
			{
				outakeSpeed = outakeSpeed - 3;
				outake(outakeSpeed);
			}
			clearTimer(T1);
		}
	 	/*
		if( vexRT[Btn8R] )
		{
			motor[rightFrontOut] = 60;
			motor[rightBackOut] = 60;
		}
		else
		{
			motor[rightFrontOut] = 0;
			motor[rightBackOut] = 0;
		}

		if( vexRT[Btn8L] )
		{
			motor[leftFrontOut] = 60;
			motor[leftBackOut] = 60;
		}
		else
		{
			motor[leftFrontOut] = 0;
			motor[leftBackOut] = 0;
		}
		*/

	}
}