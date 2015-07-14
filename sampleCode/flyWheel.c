// Author: Lucas 
// Date: 6/26/2015
// This is the flywheel code that has been run and tested 
// on Zach's bot during summer camp. It is only a snippet 
// of the entire code. 

// The robot used a 4 motor flywheel with small omnis on top 
// and large omnis on the bottom. The gear ratio of the flywheel 
// is 21:1 on top and 35:1 on the bottom to help with backspin.  


// global variables for tracking speed of flywheel 
bool isRampedUp = false;
int outakeSpeed = 0;
int maxSpeed = 80; //The maximum speed of the flywheel (max value is 127)
int minSpeed = 65;

// sets the speed of the flywheel **Change the motor names to your own!**
void outake(int power)
{
	motor[leftBackOut] = power;
	motor[leftFrontOut] = power;
	motor[rightBackOut] = power;
	motor[rightFrontOut]= power;
}

// raises the flywheel up to a speed of maxSpeed in maxSpeed*time milliseconds 
void rampUp( int time )
{
	for(int i = outakeSpeed; i <= maxSpeed; i++)
	{
		outakeSpeed = i;
		outake(outakeSpeed);
		wait1Msec( time );

	}
	isRampedUp = true;

}

// slows the flywheel down; adjust time for faster/slower slowdown
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



task usercontrol()
{

	while (true)
	{
		// ramp up if button is pressed and flywheel isn't ramped up 
		if( vexRT[Btn8D] && !isRampedUp)
		{
			rampUp( 10 );
		}
		// ramp down if button is pressed and flyweel is ramped up 
		else if ( vexRT[Btn8D] && isRampedUp )
		{

			rampDown( 30 );
		}

		// raise speed in increments of 3 
		if( vexRT[Btn7U] && time1[T1] > 250)
		{
			if( outakeSpeed < maxSpeed)
			{
				outakeSpeed = outakeSpeed + 3;
				outake(outakeSpeed);

			}
			clearTimer(T1);
		}
		
		// slow speed in increments of 3 
		else if (vexRT[Btn7D] && time1[T1] > 250)
		{
			if( outakeSpeed > minSpeed)
			{
				outakeSpeed = outakeSpeed - 3;
				outake(outakeSpeed);
			}
			clearTimer(T1);
		}

	}
}
