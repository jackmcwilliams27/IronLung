// MAX POSSIBLE RESPIRATORY RATES AND TIDAL VOLUME
int maxRate = 60;    // upper limit of respiratory rate
int maxVol = 66;    // upper limit tidal volume 

// CURVE FIT EQUATION COEFFICIENTS
float coef1 = 10;     // coefficient
float yInt = 10;     // y-axis intersection

#include <LiquidCrystal.h>
#include <ItemSubMenu.h>
#include <ItemWidget.h>
#include <ItemBack.h>
#include <LcdMenu.h>
#include <MenuScreen.h>
#include <display/LiquidCrystalAdapter.h>
#include <input/ButtonAdapter.h>
#include <renderer/CharacterDisplayRenderer.h>
#include <widget/WidgetRange.h>
#include <ItemValue.h>
#include <Button.h>
#include <ItemLabel.h>


// MENU PINS
#define UP_PIN    49   // assigns pin 49 to "UP" button
#define DOWN_PIN  51   // assigns pin 51 to "DOWN" button
#define ENTER_PIN 53    // assigns pin 53 to "ENTER" button

// PINS TO CHANGE VALUES
int buttonAdd = 47; // pin number for "ADD" button
int buttonSub = 45; // pin number for "SUB" button
int dirPin = 27;    // pin for motor driver
int stepperPin = 25;  // pin for stepper pin signal
int buttonStop = 33;  // pin for stop button

// VALUE VARIABLES
float rateVal = 0; 
float volVal = 0;
float countStep = 0;
float vSensor;

int cal1Steps = 493;
int cal2Steps = 246;
int numbSteps = 0;
float calVolVal;
float calRunVal = 1;
int loadTime = 320; // time between each movement of load screen
int cal1Array[10] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};   // array of volumes tested in calibrate 10-100mL
int cal2Array[10] = {110, 120, 130, 140, 150, 160, 170, 180, 190, 200};   // array of volumes tested in calibrate 110-200mL 
int calibrateRunNumb = 3; // number of times the calibration function will test each volume


// Menus (note the ");" at the end of each)
MENU_SCREEN(respSub, respItems,
  ITEM_VALUE("Rate", rateVal, " %.1f /min"),
  ITEM_BACK("BACK"),
);

MENU_SCREEN(tideSub, tideItems,
  ITEM_VALUE("Vol", volVal, " %.1f mL"),
  ITEM_BACK("BACK"),
);

MENU_SCREEN(runSub, runItems,
//  ITEM_BACK("END PROGRAM"),
  ITEM_VALUE("RR",rateVal,"% .1f /min"),
  ITEM_VALUE("TV",volVal, "% .1f mL"),
);

MENU_SCREEN(simSub, simItems,
	ITEM_SUBMENU("Resp Rate", respSub),
	ITEM_SUBMENU("Tidal Vol", tideSub),
  ITEM_SUBMENU("RUN SIMULATION",runSub),
	ITEM_BACK("BACK"),
);

MENU_SCREEN(cal1RunSub, cal1RunItems,
	ITEM_VALUE("Vol:",calVolVal, " %.1f mL"),
	ITEM_VALUE("Run:",calRunVal, "# %0.1f"),
);

MENU_SCREEN(cal1Sub, cal1Items,
	ITEM_LABEL("ARE VALVES OPEN"),
	ITEM_SUBMENU("yes!", cal1RunSub),
	ITEM_BACK("BACK"),
);

MENU_SCREEN(cal2RunSub, cal2RunItems,
	ITEM_VALUE("Vol:",calVolVal, " %.1f mL"),
	ITEM_VALUE("Run:",calRunVal, "# %0.1f"),
);

MENU_SCREEN(cal2Sub, cal2Items,
	ITEM_LABEL("ARE VALVES SHUT"),
	ITEM_SUBMENU("yes!", cal2RunSub),
	ITEM_BACK("BACK"),
);

MENU_SCREEN(calSub, calItems,
	ITEM_SUBMENU("10mL - 100mL",cal1Sub),
	ITEM_SUBMENU("110mL - 200mL", cal2Sub),
	ITEM_BACK("BACK"),
);

MENU_SCREEN(interceptSub, interceptItems,
  ITEM_VALUE("Intcpt",yInt," %.1f"),
  ITEM_BACK("BACK"),
);

MENU_SCREEN(coefSub, coefItems,
  ITEM_VALUE("Coef",coef1," %.1f"),
  ITEM_BACK("BACK")
);

MENU_SCREEN(settingsSub, settingsItems,
  ITEM_SUBMENU("Coefficient", coefSub),
  ITEM_SUBMENU("Y-intercept", interceptSub),
  ITEM_BACK("BACK"),
);

MENU_SCREEN(mainScreen,mainItems,
	ITEM_SUBMENU("Simulation", simSub),
	ITEM_SUBMENU("Calibrate", calSub),
  ITEM_SUBMENU("SETTINGS", settingsSub),
);

//MENU_SCREEN(simSub, mainItems,
//  ITEM_SUBMENU("Resp Rate", respSub),
//  ITEM_SUBMENU("Tidal Vol", tideSub),
//  ITEM_SUBMENU("START", runSub),
//);

const int rs = 13, en = 12, d4 = 11, d5 = 10, d6 = 9, d7 = 8;     // assigns pins used in LCD
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);     // Tells arduino which pins provide what function to LCD
LiquidCrystalAdapter lcdAdapter(&lcd, 16, 2);   
CharacterDisplayRenderer renderer(&lcdAdapter, 16, 2);
LcdMenu menu(renderer);

// Physical buttons
Button upBtn(UP_PIN);
Button downBtn(DOWN_PIN);
Button enterBtn(ENTER_PIN);

// Adapters that send button presses to the menu
ButtonAdapter upAdapter(&menu, &upBtn, UP);
ButtonAdapter downAdapter(&menu, &downBtn, DOWN);
ButtonAdapter enterAdapter(&menu, &enterBtn, ENTER);


void setup() {
  lcd.begin(16,2);      // starts LCD without "LCD MENU" library
  loadScreen();         // runs "loadScreen" function for load screen (cool factor +100)

  renderer.begin();     // begins rendering menu system on the LCD
  menu.setScreen(mainScreen);   // sets the screen to the mainScreen options

  upBtn.begin();      // begins observing when up button is pressed
  downBtn.begin();    // begins observing when down button is pressed
  enterBtn.begin();   // begins observing when enter button is pressed 

  pinMode(buttonAdd, INPUT_PULLUP);     // declares the add button as an input with internal pullup resistor
  pinMode(buttonSub, INPUT_PULLUP);     // delcares the subtract button as an input with internal pullup resistor
  pinMode(buttonStop, INPUT_PULLUP);    // delcares the stop button as an input with internal pullup resistor

  pinMode(dirPin, OUTPUT);              // delcares the driver pin as an output
  pinMode(stepperPin, OUTPUT);          // delcares the stepper pin as an output
  Serial.begin(9600);                   // sets baud rate to 9600

  maxRate = maxRate - 1;    // idk why this is here but the program works and I am too afraid to remove it... I think it is important but who really knows 
  maxVol = maxVol - 1;      // same thing as the comment above. If i remember corrently it helps prevent an if loop go over the allowed value and allows the program inputs to make more sense but like i said I dont really know anymore and the code works and so therefore I will not remove this.
}

void step(boolean dir, long steps){     // function for driving motor
  digitalWrite(dirPin, dir);
  delay(50);
  for (long i = 0; i < steps; i++){
   digitalWrite(stepperPin, HIGH);
   delayMicroseconds(800);
   digitalWrite(stepperPin, LOW);
   delayMicroseconds(800);
 }
}


void loop() {
  upAdapter.observe();    // intiates using up button for menu
  downAdapter.observe();  // initiates using down button for menu
  enterAdapter.observe();  // initiates using enter button for menu


  // CHANGE RESPIRATORY RATE VALUE
  if (menu.getScreen() == coefSub){      // if the user is on the coefSub menu
    if (digitalRead(buttonAdd)==LOW){  // if the add button is pressed
      coef1 += 0.01;   // add 0.01 to current value
      menu.refresh();  // refresh menu to display current value
      delay(50);      // delay 0.05sec
    }
    if (digitalRead(buttonSub)==LOW){   // if subtract button is pressed
      coef1 -= 0.01;   // subtracts 0.01 from current
      menu.refresh();  // refreshes menu to display new value
      delay(50);  // delay 0.05sec
    }
  }

  if (menu.getScreen() == interceptSub){    // if user is on the interceptSub submenu
    if (digitalRead(buttonAdd)==LOW){     // if user presses add button
      yInt += 0.01; // adds 0.01 to current value
      menu.refresh();   // refreshes menu to display new value
      delay(50);  // delays 0.05s
    }

    if (digitalRead(buttonSub)==LOW) {    // if subtract button is pressed
      yInt -= 0.01;    // subtracts 0.01 from current value 
      menu.refresh();  // refreshes menu from current value
      delay(50);  // delays 0.05seconds
    }
  }

  if (menu.getScreen() == respSub) {      // if the user is on the respiratory settings sub menu
    if (digitalRead(buttonAdd) == LOW) {      // if the add button is pressed
      if (rateVal <= maxRate) {       // checks if the rateVal value is less than the maximum allowed value
        rateVal += 1;       // adds 1 to the current value
        menu.refresh();     // refreshes the menu to display the new value
        delay(200);     // delays 0.2seconds
      }
    }
    if (digitalRead(buttonSub) == LOW) {    // if subtract button is pressed
      if (rateVal > 0) {   // makes sure rateVal is greater than 0 so the value cannot be negative
      rateVal -= 1;       // subtract 1 from the current value
      menu.refresh();       // refresh the menu to display the new value
      delay(200);         // delays 0.2sec
      }
    }
  } 
  
  // CHANGE TIDAL VOLUME VALUE
  if (menu.getScreen() == tideSub) {        // if the user is on the tidal volume sub menu
    if (digitalRead(buttonAdd) == LOW) {      // if the add button is pressed
      if (volVal <= maxVol) {               // if tidal volume is less than or equal to the max allowed value
        volVal += 1;                      // add 1 to the current value
        menu.refresh();   // refreshes the menu to display the new value
        delay(200);       // delays 0.2sec
      }
    }

    if (digitalRead(buttonSub) == LOW) { // if subtract button is pressed
      if (volVal > 0) {      // checks if volume value is greater than 0 to prevent it going negative
        volVal -= 1;          // subtracts 1 from volVal
        menu.refresh();     // refreshes menu to display new value
        delay(200);       // delays 0.2sec
      }
    }
  } 
  

  // RUN STEPPER MOTOR ///
  if (menu.getScreen() == runSub and digitalRead(buttonStop)==HIGH)  {  // determines if user in on the run menu and not pressing the stop button
    step(true,200); 
    step(false,200);
    if (digitalRead(buttonStop)==LOW){   // if the stop button is pressed once the motors at resting location
      menu.setScreen(simSub);             // returns the user to the respirtory simulation menu
    }
  }
  

  if (menu.getScreen() == cal1RunSub) {  // if user is on the 10-100mL calibration screen and has confirmed valves are closed
    int array1Length = sizeof(cal1Array) / sizeof(cal1Array[0]);  // creates variable for the length of the array containing the volumes

    for (int i = 0; i < array1Length; i++){    // for loop for the length of the volume array
      calVolVal = cal1Array[i];  // sets the displayed volume to the value in the array using the 'i' index
      calibrate1();               // calls calibration function for that volume range
    }
    menu.setScreen(calSub);       // once done places user back on the volume range selection menu
  }

  if (menu.getScreen() == cal2RunSub) {         // if the user is on the cal2RunSub menu
    int array2Length = sizeof(cal2Array) / sizeof(cal2Array[0]);    // determines the size of the volume array

    for (int l = 0; l < array2Length; l++){
      calVolVal = cal2Array[l];         // takes the volume from the cal2Array and uses it for that run of the function
      calibrate2();       // runs calibrate 2 function
    } 
    
    menu.setScreen(calSub);   // resets menu to kick program out of motor function
  }
}


void calibrate1(){             // creates calibration function
  for (int j=1; j <= calibrateRunNumb; j++){   // creates for loop so each volume will be tested based on defined value and adds to value after each run
    calRunVal = j;       // defines value displayed on the screen for number of runs
    numbSteps = (calVolVal/10) * cal1Steps;   // determines the number of steps the motor must travel for each volume value
    menu.poll();                       // updates menu to display current volume and run #

    step(true, numbSteps);        // steps motor to expand syringe
    delay(250);                   // delays so there is a clear gap in volumetric flow meter data
    step(false,numbSteps);        // returns motor and syringes to resting state
    delay(1000);                  // delays for clear gap in volumetric flow meter data
  }
}


void calibrate2(){
  for (int k=1; k <= calibrateRunNumb; k++){    // creates for loop so each volume will be tested based on defined value and adds to value after each run
    calRunVal = k;                                    // assigns the current run number to the variable used for display
    numbSteps = 2467 + ((calVolVal/10) * cal2Steps);    // calculates the number of steps to get the volume
    menu.poll();                                      // refreshes the menu to display new volume and run values

    step(true, numbSteps);          // steps motor
    delay(250);                   // delays motor
    step(false, numbSteps);       // steps motor
    delay(1000);                  // delays motor
  }
}


void loadScreen(){          // function for epic loading screen animation (cool factor +100 (again))
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("E");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("HE");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("THE");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(" THE");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   THE");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   THE         R");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   THE        RE");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   THE       REA");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   THE      REAL");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   THE     REAL");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   THE    REAL");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   THE   REAL");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   THE  REAL");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   THE REAL");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   THE REAL");
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   THE REAL");
  lcd.setCursor(0,1);
  lcd.print("G");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   THE REAL");
  lcd.setCursor(0,1);
  lcd.print("NG");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   THE REAL");
  lcd.setCursor(0,1);
  lcd.print("UNG");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   THE REAL");
  lcd.setCursor(0,1);
  lcd.print("LUNG");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   THE REAL");
  lcd.setCursor(0,1);
  lcd.print(" LUNG");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   THE REAL");
  lcd.setCursor(0,1);
  lcd.print("N LUNG");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   THE REAL");
  lcd.setCursor(0,1);
  lcd.print("ON LUNG");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   THE REAL");
  lcd.setCursor(0,1);
  lcd.print("RON LUNG");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   THE REAL");
  lcd.setCursor(0,1);
  lcd.print("IRON LUNG");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   THE REAL");
  lcd.setCursor(0,1);
  lcd.print(" IRON LUNG");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   THE REAL");
  lcd.setCursor(0,1);
  lcd.print("  IRON LUNG");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   THE REAL");
  lcd.setCursor(0,1);
  lcd.print("   IRON LUNG");
  delay(loadTime);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   THE REAL");
  lcd.setCursor(0,1);
  lcd.print("    IRON LUNG");
  delay(2500);
}
