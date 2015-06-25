#pragma systemFile
float precisionMode = 1;
const int skyrise[7] = {1, 2, 3, 4, 5, 6, 7}; //skyrise deg values Need to set
const int post[3] = {1, 2, 3};
int skyriseIndex = 0;
bool toSet = true;
//PID code
bool toArmStream = false;
float rAPrevError = 0;
float rAIntegral = 0;
float lAPrevError = 0;
float lAIntegral = 0;
float rAError;

float lAError;
float lastrAError = 0;
float lastlAError = 0;
float lADerivative;
float rADerivative;
float liftSetPt = 0;
int lLiftModifier = -1;
float lastLiftSetPt = 0;
int previousLiftSetPt = 0;
bool armLoop = false;
float ignoreError = 10;
float armMovePo = 127;
task armPID() {
	float armKp = 20.0; //Need to set
	float armKi = 0.0; //Ned to set
	float armKd = 0.0; //Neeed to set
	while(true) {
		rAError = liftSetPt + SensorValue[rLiftEncoder];
		lAError = liftSetPt - SensorValue[lLiftEncoder] + 3;
		lAIntegral += lAError;
		rAIntegral += rAError;
		rADerivative = rAError - rAPrevError;
		lADerivative = lAError - lAPrevError;
		motor[liftLB] = (((armKp*lAError) + (armKi*lAIntegral) + (armKd*lADerivative))) * -1;
		motor[liftLT] = (((armKp*lAError) + (armKi*lAIntegral) + (armKd*lADerivative))) * -1;
		motor[liftRB] = (((armKp*rAError) + (armKi*rAIntegral) + (armKd*rADerivative))) * -1;
		motor[liftRT] = (((armKp*rAError) + (armKi*rAIntegral) + (armKd*rADerivative))) * -1;
		if(toArmStream) {
			if(lastlAError != lAError || lastrAError != rAError) {
				writeDebugStreamLine("%f, %f, %f",liftSetPt, SensorValue[lLiftEncoder], -1 * SensorValue[rLiftEncoder]);
				lastlAError = lAError;
				lastrAError = rAError;
			}
		}
	}
}

void intakeSet(float po) {
	motor[intakeL] = -po;
	motor[intakeR] = po;
}

void intakeControl() {
	SensorValue[dumpSolenoid] = vexRT[Btn8L] ? 1 : 0;
	if(vexRT[Btn5U]) {
		intakeSet(127);
	}
	else if(vexRT[Btn5D]) {
		intakeSet(-127);
	}
	else if(vexRT[Btn8L]) {

		intakeSet(-127);
	}
	else {
		intakeSet(0);
	}
}

task dump() {
	clearTimer(T1);
	intakeSet(-127);
	SensorValue[dumpSolenoid] = 1;
	while(time1[T1] < 3000) {}
	SensorValue[dumpSolenoid] = 0;
	intakeSet(0);
	stopTask(dump);

}

void dumpControl() {
	if(vexRT[Btn7L])
		//startTask(dump);
		SensorValue[dumpSolenoid] = vexRT[Btn7L] ? 1 : 0;
		//intakeSet(-127);
	else
		intakeSet(0);
		//if(vexRT[Btn7L])
			//intakeSet(-127);
}

float liftHeight() {
	return (float)(abs((SensorValue[lLiftEncoder])));
}

void armPow(int po) {
	motor[liftLB] = po;
	motor[liftLT] = po;
	motor[liftRB] = po;
	motor[liftRT] = po;
}

bool armStop() {
	return (liftDetectLeft || liftDetectRight);
}

task armController() {
	while(true) {
		while(abs(liftSetPt - liftHeight()) > ignoreError) {
			if(liftSetPt > liftHeight())
				armPow(armMovePo);
			else if(liftSetPt < liftHeight())
				armPow(-60);
		}
		armPow(15);
	}
}

void setArm(float deg) {
	liftSetPt = deg;
}

void zeroArm(float po = -100) {
	while(!armStop())
		armPow(po);
}

void armControl() {
	precisionMode = vexRT[Btn8D] ? 1 : 1;
	if(vexRT[Btn6D]) {
		liftSetPt = SensorValue[lLiftEncoder] - 40;
	} else if(vexRT[Btn6U]) {
		liftSetPt = SensorValue[lLiftEncoder] + 40;
	} /*else if(vexRT[Btn8DXmtr2]) {
		setArm(post[0]);
	} else if(vexRT[Btn8LXmtr2]) {
		setArm(post[1]);
	} else if(vexRT[Btn8RXmtr2]) {
		setArm(post[2]);
	} else if(vexRT[Btn5UXmtr2]) {
		setArm(skyrise[skyriseIndex]);
		skyriseIndex++;
  }*/ else {
		liftSetPt = SensorValue[lLiftEncoder] + 3;
	}
}
void armSmartControl() {
	if(vexRT[Btn6U] /*&& (!liftDetectLeft || !liftDetectRight)*/) {
		stopTask(armPID);
		motor[liftLB] = 127;
		motor[liftLT] = 127;
		motor[liftRB] = 127;
		motor[liftRT] = 127;
		armLoop = false;
	}
	else if(vexRT[Btn6D] /*&& (!liftDetectLeft || !liftDetectRight)*/) {
		stopTask(armPID);
		motor[liftLB] = -127;
		motor[liftLT] = -127;
		motor[liftRB] = -127;
		motor[liftRT] = -127;
		armLoop = false;
	}
	else {
		startTask(armPID);
		if(!armLoop) {
			liftSetPt = SensorValue[lLiftEncoder];
		}
		armLoop = true;
	}
}

int preset = 0;
void armDumbControl() {
	ignoreError = 3;
	if(vexRT[Btn6U]) {
		//armPow(127);
		preset = 0;
		liftSetPt = SensorValue[lLiftEncoder] + 40;
	}
	else if(vexRT[Btn6D]) {
		//armPow(-60);
		preset = 0;
		liftSetPt = SensorValue[lLiftEncoder] - 40;
	}
	else if(vexRT[Btn7U] || preset == 1) {
		preset = 1;
		liftSetPt = 153;
	}
	else if(vexRT[Btn7L] || preset == 2) {
		preset = 2;
		liftSetPt = 101;
	}
	else if(vexRT[Btn7D] || preset == 3) {
		preset = 3;
		liftSetPt = 66;
	}
	else if(true)
		liftSetPt = SensorValue[lLiftEncoder];
}
