/*
	TODO:
	Add Holonomic code
*/

void tank(){
	motor[RB] = motor[RF] = vexRT[Ch2]; //Right side of the drive
	motor[LF] = motor[LB] = vexRT[Ch3]; //Left side of the drive
}

void arcade() {
	motor[RB] = motor[RF] = (vexRT[Ch3] - vexRT[Ch1]); //Right side of the drive
	motor[LB] = motor[LF] = (vexRT[Ch3] + vexRT[Ch1]); //Left side of the drive
}

void hDrive() {
	tank();
	motor[s] = (vexRT[Btn6U] - vexRT[Btn5U])*127; //Strafe Wheel controlled by bumpers
	/*
		Equivalent code:
		if(vexRT[Btn6U]){
			motor[s] = 127;
		} else if(vexRT[Btn5U]) {
			motor[s] = 127;
		} else {
			motor[s] = 0;
		}
	*/
}

void mecanum() {
	motor[RF] = vexRT[Ch3] - vexRT[Ch1] - vexRT[Ch4];
	motor[RB] =  vexRT[Ch3] - vexRT[Ch1] + vexRT[Ch4];
	motor[LF] = vexRT[Ch3] + vexRT[Ch1] + vexRT[Ch4];
	motor[LB] =  vexRT[Ch3] + vexRT[Ch1] - vexRT[Ch4];
}

void drive(bool isTank) { //one drive function that can toggle between drives
		if(isTank) {
			tank();
		} else {
			arcade();
		}
}
