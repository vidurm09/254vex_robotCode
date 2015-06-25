#pragma systemFile

int skyHeight = 150;
int postDist = 1000;
int skyNum = 1;

void auton()
{
	//DRIVE PID TEST
	//startDrivePID(2);
	//changeDrive(1000);
	//
	//INSPECTION AUTON
	//motor[driveRB] = motor[driveRF] = motor[driveLB] = motor[driveLF] = 127;
	//
	////SKYRISE AUTON
	////
	////Build 4 or 5 skyrises
	////Start at skyrise square
	////Start PID
	//startDrivePID(2);
	//startLiftPID(3.3);

	////1st skyrise
	////
	////Grab skyrise
	//skyControl(true);
	//skyControl(false);
	////Lift skyrise out of holder
	//moveLift(skyHeight);
	////Drive back to skyrise post
	//changeDrive(-postDist);
	////Lower arm down to post
	//moveLift(-skyHeight);
	////Release skyrise
	//skyControl(true);
	//skyControl(false);
	////1st skyrise done
	////
	////Start loop
	//for(int n = 0; n < 4; n++)
	//{
	//	//nth skyrise
	//	//
	//	//Lift arm above post
	//	moveLift(skyHeight*skyNum);
	//	//skyNum = n;
	//	//Drive back halfway to skyrise holder
	//	changeDrive(postDist/2);
	//	//Lower arm down to skyrise holder
	//	moveLift(-skyHeight*(skyNum-1));
	//	//Set skyNum to next skyrise number
	//	skyNum = n;
	//	//Drive to skyrise holder
	//	changeDrive(postDist/2);
	//	//Grab skyrise
	//	skyControl(true);
	//	skyControl(false);
	//	//Lift skyrise out of holder
	//	moveLift(skyHeight*skyNum);
	//	//Drive back to skyrise post
	//	changeDrive(-postDist);
	//	//Lower arm down to post
	//	moveLift(-skyHeight);
	//	//Release skyrise
	//	skyControl(true);
	//	skyControl(false);
	//	//nth skyrise done
	//}
	//
	////CUBE AUTON
	////
	////Grab cube in front and score on post next to it
	//startDrivePID(2)
	//startLiftPID(3.3);
	//moveLift(300);
	//changeDrive(1000);
	//moveLift(-300);
	//changeDrive(400);
	//changeDrive(400, -400);
	//moveLift(1000);
	//changeDrive(300);
	//moveLift(-200);
	//
	//SKETCH CUBE AUTON
	//startDrivePID(2);
	//startLiftPID(3.3);
	//moveLift(200);
	//changeDrive(-500);
	setDrive(-127);
	wait1Msec(1000);
	setDrive(0);
}
