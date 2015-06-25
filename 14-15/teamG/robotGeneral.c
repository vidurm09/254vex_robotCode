#pragma systemFile
//Clear all encoder values
void resetEncoder() {
	SensorValue[rLiftEncoder] = 0;
	SensorValue[lLiftEncoder] = 0;
	SensorValue[rDriveEncoder] = 0;
	SensorValue[lDriveEncoder] = 0;
}
//All code that will run in user control
void drive() {
	driveControl(1);//tank - 0, arcade - 1
	//armSmartControl();
	armDumbControl();
	intakeControl();
	//dumpControl();
}
