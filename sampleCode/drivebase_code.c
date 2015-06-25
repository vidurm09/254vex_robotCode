void tank(){
	motor[RB] = vexRT[Ch2];
	motor[RF] = vexRT[Ch2];
	motor[LF] = motor[LB] = vexRT[Ch3];
}

void arcade() {
	motor[RB] = motor[RF] = (vexRT[Ch3] - vexRT[Ch1]);
	motor[LB] = motor[LF] = (vexRT[Ch3] + vexRT[Ch1]);
}

void hDrive() {
	tank();
	motor[s] = (vexRT[Btn6U] - vexRT[Btn5U])*127;
}

void mecanum() {
	motor[RF] = vexRT[Ch3] - vexRT[Ch1] - vexRT[Ch4];
	motor[RB] =  vexRT[Ch3] - vexRT[Ch1] + vexRT[Ch4];
	motor[LF] = vexRT[Ch3] + vexRT[Ch1] + vexRT[Ch4];
	motor[LB] =  vexRT[Ch3] + vexRT[Ch1] - vexRT[Ch4];
}

void drive(bool isTank) {
		if(isTank) {
			tank();
		} else {
			arcade();
		}
}
