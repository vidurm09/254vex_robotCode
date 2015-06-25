#pragma systemFile
int drivekp = 10;
int drivekd = 0;
int driveki = 0;
int driveLSetPt = 0;
int driveRSetPt = 0;

void setDriveRight(int power) {
	SetMotor(driveRF, power);
	SetMotor(driveRB, power);
}

void setDriveLeft(int power) {
	SetMotor(driveLF, -power);
	SetMotor(driveLB, -power);
}

void setDrive(int power) {
	setDriveRight(power);
	setDriveLeft(power);
}

float leftDrive() { return -SensorValue[lDriveEncoder]; }

float rightDrive() { return SensorValue[rDriveEncoder]; }

task drivePID() {
	float lError = 0;
	float lPrevError = 0;
	float lIntegral = 0;
	float lDerivative = 0;
	float rError = 0;
	float rPrevError = 0;
	float rIntegral = 0;
	float rDerivative = 0;
	while(true) {
		lError = driveLSetPt - leftDrive();
		rError = driveRSetPt - rightDrive();
		lIntegral += lError;
		rIntegral += rError;
		lDerivative = lError - lPrevError;
		rDerivative = rError - rPrevError;
		setDriveLeft(1*((drivekp*lError)+(driveki*lIntegral)+(drivekd*lDerivative)));
		setDriveRight(1*((drivekp*rError)+(driveki*rIntegral)+(drivekd*rDerivative)));
		lPrevError = lError;
		rPrevError = rError;
		writeDebugStreamLine("drive: %f, %f", lError, rError);
	}
}

void startDrivePID(int kp, int kd = 0, int ki = 0) {
	drivekp = kp;
	drivekd = kd;
	driveki = ki;
	startTask(drivePID);
}

void changeLDrive(int change) {
	driveLSetPt += change;
}

void changeRDrive(int change) {
	driveRSetPt += change;
}

void changeDrive(int change) {
	changeLDrive(change);
	changeRDrive(change);
}

void changeDrive(int changeLeft, int changeRight) {
	changeLDrive(changeLeft);
	changeRDrive(changeRight);
}

void arcade() {
	motor[driveRB] = motor[driveRF] = (vexRT[Ch3] - vexRT[Ch1]);
	motor[driveLB] = motor[driveLF] = (vexRT[Ch3] + vexRT[Ch1]);
}
void tank() {
	motor[driveRB] = motor[driveRF] = vexRT[Ch2];
	motor[driveLB] = motor[driveLF] = vexRT[Ch3];
}
