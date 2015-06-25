#pragma systemFile
float liftkp = 10;
float liftdownkp = 5;
float liftkd = 0;
float liftki = 0;
int liftSetPt = 0;
int prevSetPt = 0;
bool armLoop = false;

void setLiftRight(int power) {
	SetMotor(liftRA, power);
	SetMotor(liftRB, power);
	SetMotor(liftRC, power);
}

void setLiftLeft(int power) {
	SetMotor(liftLA, power);
	SetMotor(liftLB, power);
	SetMotor(liftLC, power);
}

void setLift(int power) {
	setLiftRight(power);
	setLiftLeft(power);
}

bool cubeIsOpen = false;
bool skyIsOpen = false;
bool cubePress = false;
bool skyPress = false;

void cubeControl(bool control) {
	//if(control)
	//{
	//	if(cubeIsOpen && !cubePress)
	//	{
	//		SensorValue[dumpSolenoid] = 1;
	//		cubePress = true;
	//	}
	//	else if(!cubeIsOpen && !cubePress)
	//	{
	//		SensorValue[dumpSolenoid] = 0;
	//		cubePress = true;
	//	}
	//}
	//else
	//{
	//	cubePress = false;
	//}
	if(control && cubeIsOpen && !cubePress) {
		cubePress = true;
		SensorValue[dumpSolenoid] = 1;
		cubeIsOpen = false;
	} else if(control && !cubeIsOpen && !cubePress) {
		cubePress = true;
		SensorValue[dumpSolenoid] = 0;
		cubeIsOpen = true;
	} else if(!control)
	{
		cubePress = false;
	}
}

void skyControl(bool control) {
	if(control && skyIsOpen && !skyPress) {
		skyPress = true;
		SensorValue[skySolenoid] = 1;
		skyIsOpen = false;
	} else if(control && !skyIsOpen && !skyPress) {
		skyPress = true;
		SensorValue[skySolenoid] = 0;
		skyIsOpen = true;
	} else if(!control)
	{
		skyPress = false;
	}
}

task solenoidControl() {
	while(true) {
		cubeControl(vexRT[Btn7D]);
		skyControl(vexRT[Btn8D]);
	}
}

float leftLift() { return -SensorValue[lLiftEncoder]; }

float rightLift() { return -SensorValue[rLiftEncoder]; }

float liftAvg() { return ((leftLift() + rightLift())/2.0);}

float konstant = 1;
float deadzone = 10;

task liftPID() {
	float lError = 0;
	float lPrevError = 0;
	float lIntegral = 0;
	float lDerivative = 0;
	float rError = 0;
	float rPrevError = 0;
	float rIntegral = 0;
	float rDerivative = 0;
	int counter = 0;
	while(true) {
		lError = liftSetPt - leftLift();
		rError = liftSetPt - rightLift();
		lIntegral += lError;
		rIntegral += rError;
		/*if(counter>=20) {
			lDerivative = lError - lPrevError;
			rDerivative = rError - rPrevError;
			counter = 0;
		}
		float realkp = liftkp;
		if(lError<0) {
			realkp = liftdownkp;
		}*/
		float lPower = (liftkp*lError)+(liftki*lIntegral)+(liftkd*lDerivative);
	/*	realkp = liftkp;
		if(rError<0) {
			realkp = liftdownkp;
		}*/
		float rPower = (liftkp*rError)+(liftki*rIntegral)+(liftkd*rDerivative);
		setLiftLeft(lPower);
		setLiftRight(rPower);
	/*	if(counter==0) {
			lPrevError = lError;
			rPrevError = rError;
		}
		counter++;*/
	}
}

void startLiftPID(float kp, float kd = 0, float ki = 0) {
	liftkp = kp;
	liftdownkp = kp;
	liftkd = kd;//100;
	liftki = ki;//0.000001;
	startTask(liftPID);
}

void moveLift(int change) {
	liftSetPt += change;
}
int prev = 0;
void userControlArmPID() {
	if(vexRT[Btn5D]) {
		liftSetPt = liftAvg() - (127.0/liftkp);
		prev = -1;
		armLoop = false;
	} else if(vexRT[Btn5U]) {
		liftSetPt = liftAvg() + 127.0/liftkp;
		prev = 1;
		armLoop = false;
	} else {
		if(!armLoop) {
			if(prev==1) {
				liftSetPt = liftAvg()+5;
			}
			else if(prev==-1){
				liftSetPt = liftAvg()-5;
			}
			else {
				liftSetPt = liftAvg();
			}
			writeDebugStreamLine("Set PID to: %d, avg is at %d", liftSetPt, liftAvg());
		}
		armLoop = true;
	}
}

void userControlArmNoPID() {
	if(vexRT[Btn6D]) {
		setLift(-127);
	} else if(vexRT[Btn6U]) {
		setLift(127);
	} else {
		setLift(0);
	}
}
