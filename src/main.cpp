#include "EasyNextionLibrary.h"

EasyNex myNex(Serial1); // Create an object of EasyNex class with the name <myNex>

#define GET_DATA_EVERY 500 // DHT sensors can give us measurements every 2 seconds, as Adafruit suggests

unsigned long getDataTimer = millis(); // Timer for GET_DATA_EVERY 

int charge = 0;     
int speed = 0; 
int battTemp = 0;     
int solarTemp = 0; 
int motorTemp = 0; 
int brakeTemp = 0;
int pwrIn = 0;
int pwrOut = 0;
String warning = "Hi";

int chargeLast = -1;     
int speedLast = -1; 
int battTempLast = -1;     
int solarTempLast = -1; 
int motorTempLast = -1; 
int brakeTempLast = -1;
int pwrInLast = -1;
int pwrOutLast = -1;

void firstRefresh();
void refereshCurrentPage();
void readSensorValues();
void refreshPage0();
void refreshPage1();
void refreshPage2();

// Change Page Initialization
#define DATA_REFRESH_RATE 1000 // The time between each Data refresh of the page

unsigned long pageRefreshTimer = millis(); // Timer for DATA_REFRESH_RATE
bool newPageLoaded = false; // true when the page is first loaded (lastCurrentPageId != currentPageId)

void setup() {
  Serial.begin(9600); // Start hardware serial for debugging
  Serial1.begin(9600); // Start Serial1 for Nextion display

  delay(500);               // give Nextion some time to finish initialize
  myNex.writeStr("page 0"); // For synchronizing Nextion page in case of reset to Arduino
  delay(50);
  myNex.lastCurrentPageId = 1; // At the first run of the loop, the currentPageId and the lastCurrentPageId must have different values, due to run the function firstRefresh()
  Serial.println("Setup complete, starting loop");
}

void loop() {
  readSensorValues();
  refereshCurrentPage();
  firstRefresh();
}

void firstRefresh() {
  if (myNex.currentPageId != myNex.lastCurrentPageId) { // If the two variables are different, means a new page is loaded.
    newPageLoaded = true;    // A new page is loaded

    Serial.print("New page loaded: ");
    Serial.println(myNex.currentPageId);

    switch (myNex.currentPageId) {
      case 0:
        refreshPage0();
        break;

      case 1:
        refreshPage1();
        break;

      case 2:
        refreshPage2();
        break;
    }

    newPageLoaded = false;  // After we have updated the new page for the first time, we update the variable to false.
    myNex.lastCurrentPageId = myNex.currentPageId; // After the refresh of the new page We make them equal, in order to identify the next page change.
  }
}

void readSensorValues() {
  if ((millis() - getDataTimer) > GET_DATA_EVERY) {
    speed += 1; // Example increment, replace with actual sensor reading
    Serial.print("Speed: ");
    Serial.println(speed);
    getDataTimer = millis(); // Reset the timer
  }
}

void refereshCurrentPage() {
  // In this function we refresh the page currently loaded every DATA_REFRESH_RATE
  if ((millis() - pageRefreshTimer) > DATA_REFRESH_RATE) {
    Serial.print("Refreshing page: ");
    Serial.println(myNex.currentPageId);
    switch (myNex.currentPageId) {
      case 0:
        refreshPage0();
        break;

      case 1:
        refreshPage1();
        break;

      case 2:
        refreshPage2();
        break;
    }
    pageRefreshTimer = millis();
  }
}

void refreshPage0() {
  if (charge != chargeLast || newPageLoaded == true) {
    String chargeString = String(charge);
    myNex.writeStr("charge.txt", chargeString);
    chargeLast = charge;
    Serial.print("Charge updated: ");
    Serial.println(charge);
  }

  if (speed != speedLast || newPageLoaded == true) {
    String speedString = String(speed);
    myNex.writeStr("speed.txt", speedString); // Ensure that speed is a text component on Nextion
    Serial.print("Writing to Nextion: speed.txt=");
    Serial.println(speedString);
    speedLast = speed;
    Serial.print("Speed updated: ");
    Serial.println(speed);
  }

  if (newPageLoaded == true) {
    myNex.writeStr("warning.txt", warning);
    Serial.print("Warning updated: ");
    Serial.println(warning);
  }
}

void refreshPage1() {
  if (pwrIn != pwrInLast || newPageLoaded == true) {
    String pwrInString = String(pwrIn);
    myNex.writeStr("powerIn.txt", pwrInString);
    pwrInLast = pwrIn;
    Serial.print("Power In updated: ");
    Serial.println(pwrIn);
  }

  if (pwrOut != pwrOutLast || newPageLoaded == true) {
    String pwrOutString = String(pwrOut);
    myNex.writeStr("powerOut.txt", pwrOutString);
    pwrOutLast = pwrOut;
    Serial.print("Power Out updated: ");
    Serial.println(pwrOut);
  }
}

void refreshPage2() {
  if (battTemp != battTempLast || newPageLoaded == true) {
    String battTempString = String(battTemp);
    myNex.writeStr("battTemp.txt", battTempString);
    battTempLast = battTemp;
    Serial.print("Battery Temp updated: ");
    Serial.println(battTemp);
  }

  if (solarTemp != solarTempLast || newPageLoaded == true) {
    String solarTempString = String(solarTemp);
    myNex.writeStr("solarTemp.txt", solarTempString);
    solarTempLast = solarTemp;
    Serial.print("Solar Temp updated: ");
    Serial.println(solarTemp);
  }

  if (motorTemp != motorTempLast || newPageLoaded == true) {
    String motorTempString = String(motorTemp);
    myNex.writeStr("motorTemp.txt", motorTempString);
    motorTempLast = motorTemp;
    Serial.print("Motor Temp updated: ");
    Serial.println(motorTemp);
  }

  if (brakeTemp != brakeTempLast || newPageLoaded == true) {
    String brakeTempString = String(brakeTemp);
    myNex.writeStr("brakeTemp.txt", brakeTempString);
    brakeTempLast = brakeTemp;
    Serial.print("Brake Temp updated: ");
    Serial.println(brakeTemp);
  }
}
