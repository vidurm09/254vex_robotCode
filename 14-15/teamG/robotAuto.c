#pragma systemFile
//All code that will run in Auto

void redSkyrise() {
		setArm(20);
		intakeSet(127);
		wait1Msec(750);
		setArm(5);
		wait1Msec(500);
		driveStraight(550);
		setArm(120);
		wait1Msec(3000);
		setRightTicks(230);
		setRightPo(127);
		setLeftPo(127);
		wait1Msec(1500);
		setRightPo(0);
		setLeftPo(0);
		SensorValue[lDriveEncoder] = 0;
		SensorValue[rDriveEncoder] = 0;
		driveStraight(-390);
		setArm(95);
		wait1Msec(1000);
		intakeSet(-127);
		SensorValue[dumpSolenoid] = 1;
		wait1Msec(1500);
		SensorValue[dumpSolenoid] = 0;
		SensorValue[lDriveEncoder] = 0;
		SensorValue[rDriveEncoder] = 0;
		driveStraight(-180);
		wait1Msec(500);
		setArm(5);
}

void redPost() {
	setArm(20);
	intakeSet(127);
	wait1Msec(750);
	setArm(5);
	wait1Msec(500);
	driveStraight(550);
	setArm(120);
	wait1Msec(3000);
	setLeftTicks(685);
	SensorValue[lDriveEncoder] = 0;
	SensorValue[rDriveEncoder] = 0;
	setRightPo(127);
	setLeftPo(127);
	wait1Msec(1500);
	setRightPo(0);
	setLeftPo(0);
	SensorValue[lDriveEncoder] = 0;
	SensorValue[rDriveEncoder] = 0;
	driveStraight(-390);
	setArm(95);
	wait1Msec(1000);
	intakeSet(-127);
	SensorValue[dumpSolenoid] = 1;
	wait1Msec(1500);
	SensorValue[dumpSolenoid] = 0;
	SensorValue[lDriveEncoder] = 0;
	SensorValue[rDriveEncoder] = 0;
	driveStraight(-180);
	wait1Msec(500);
	setArm(5);
}

void blueSkyrise() {

}

void bluePost() {

}

void auto() {
	resetEncoder();
	startTask(armController);
	//startTask(drivePID);
	//redSkyrise is left;
	//redPost is right
	redSkyrise();
	//stopTask(drivePID);
}
