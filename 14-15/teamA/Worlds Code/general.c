#pragma systemFile
bool hasInit = false;
void init() {
	if(!hasInit) {
		SmartMotorsInit();
		//SmartMotorsAddPowerExtender(liftLB, liftLC, liftRB, liftRC);
		//SmartMotorSetPowerExpanderStatusPort(powerExpander);
		SmartMotorLinkMotors(driveLF, driveLB);
		SmartMotorLinkMotors(driveRF, driveRB);
		SmartMotorLinkMotors(liftLA, liftLB);
		SmartMotorLinkMotors(liftLA, liftLC);
		SmartMotorLinkMotors(liftRA, liftRB);
		SmartMotorLinkMotors(liftRA, liftRC);
		//SmartMotorCurrentMonitorEnable();
		SmartMotorsSetEncoderGearing(liftLA, 0.04);
		SmartMotorsSetEncoderGearing(liftLB, 0.04);
		SmartMotorsSetEncoderGearing(liftLC, 0.04);
		SmartMotorsSetEncoderGearing(liftRA, 0.04);
		SmartMotorsSetEncoderGearing(liftRB, 0.04);
		SmartMotorsSetEncoderGearing(liftRC, 0.04);
		SmartMotorRun();
		startLiftPID(2.8);
		hasInit = true;
		startTask(solenoidControl);
	}
}

void userControl(bool isArcade, bool isArmPID) {
	stopTask(drivePID);
	if(isArcade) {
		arcade();
	} else {
		tank();
	}
	if(isArmPID) {
		userControlArmPID();
	} else {
		stopTask(liftPID);
		userControlArmNoPID();
	}

}
