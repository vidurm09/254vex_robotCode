bool lcdRun = true;
string mainBattery, backupBattery;
const short leftButton = 1;
const short centerButton = 2;
const short rightButton = 4;

const int batteryButton = 0;
const int autoPick = 1;
const int bluePick = 2;
const int redPick = 3;

void backLightFlash() {
	for(int i = 0; i < 5; i++) {
		// Turns the backlight on (1)
		bLCDBacklight = 1;

		//Keeps the light on for 500 milliseconds
		wait1MSec(100);

		//Turns the backlight off (0)
		bLCDBacklight = 0;

		//Keeps the light off for 500 milliseconds
		wait1MSec(100);		
	}
}

void clearLines(){
	clearLCDLine(0);
	clearLCDLine(1);
}

void batteryVoltagle(int buttonPress) {
	if(buttonPress == batteryButton) {	
		clearLCDLine(0);											// Clear line 1 (0) of the LCD
		clearLCDLine(1);											// Clear line 2 (1) of the LCD

		//Display the Primary Robot battery voltage
		displayLCDString(0, 0, "Primary: ");
		sprintf(mainBattery, "%1.2f%c", nImmediateBatteryLevel/1000.0,'V'); //Build the value to be displayed
		displayNextLCDString(mainBattery);
		if(nImmediateBatteryLevel < 6.2) {
			backLightFlash();
		}
		//Display the Backup battery voltage
		displayLCDString(1, 0, "Backup: ");
		sprintf(backupBattery, "%1.2f%c", BackupBatteryLevel/1000.0, 'V');	//Build the value to be displayed
		displayNextLCDString(backupBattery);
		wait1Msec(100);
	}	
}

void displayLCD(string first, string second) {
	displayLCDString(0,0, first);
	displayLCDString(1,0, second);
}

void waitForPress()
{
	while(nLCDButtons == 0){}
	wait1Msec(5);
}

int titles(int title) {
	switch(title){
	case 0:
		break;
	case 1:
		string first = "Auto Pick: ";
		string second = "";
		displayLCD(first, second);
		waitForPress();
		if(nLCDButtons == leftButton)
		{
			return redPick;
		} 
		else if(nLCDButtons == rightButton)
		{
			return bluePick;
		}
		else if(nLCDButtons == centerButton)
		{
			return batteryButton;
		}
		break;
	case 2:
		waitForPress();
		if(nLCDButtons == centerButton)
		{
			return autoPick;
		}
		break;
	default:
		return -1;

	}
}

int updateTitle(int buttonPress) {
	if(buttonPress == autoPick) {
		return 1;	
		} else if( buttonPress == batteryButton) {
		return 2;
		} else {
		return -1;
	}
}

void updateAuto(int buttonPress) {
	if(buttonPress == redPick) {
		backLightFlash();
		//set to red
		} else if (buttonPress == bluePick) {
		backLightFlash();
		//set to blue
	}
}

task LCD_ButtonPress() {
	int buttonPress = 0;
	int title = 2;
	while(lcdRun) {
		buttonPress = titles(title);	
		title = updateTitle(buttonPress);
		batteryVoltagle(buttonPress);
		updateAuto(buttonPress);
	}
}
