#include "EasyNextionLibrary.h"  // Include EasyNextionLibrary

EasyNex myNex(Serial); // Create an object of EasyNex class with the name < myNex >
                       // Set as parameter the Hardware Serial you are going to use

#define GET_DATA_EVERY 500 // DHT sensors can give us measurements every 2 seconds,
                            // as Adafruit suggests
                            
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
//---------------------------------- 
// Change Page Initialization
//---------------------------------- 

/* In order to update currentPageId variable with the current Id of the page, 
 * you must write at the Preinitialize Event of every page: `printh 23 02 50 XX` , where `XX` the id of the page in HEX.  
 *      For page0: `printh 23 02 50 00`
 *      For page9: `printh23 02 50 09`
 *      For page10: `printh 23 02 50 0A`
 */
#define DATA_REFRESH_RATE 1000 // The time between each Data refresh of the page
                               // Depending on the needs of the project, the DATA_REFRESH_RATE can be set
                               // to 50ms or 100ms without a problem. In this example, we use 1000ms, 
                               // as DHT sensor is a slow sensor and gives measurements every 2 seconds

unsigned long pageRefreshTimer = millis(); // Timer for DATA_REFRESH_RATE

bool newPageLoaded = false; // true when the page is first loaded ( lastCurrentPageId != currentPageId )

 
void setup(){

  myNex.begin(9600);
  
  delay(500);               // give Nextion some time to finish initialize
  myNex.writeStr("page 0"); // For synchronizing Nextion page in case of reset to Arduino
  delay(50);
  myNex.lastCurrentPageId = 1; // At the first run of the loop, the currentPageId and the lastCurrentPageId
                               // must have different values, due to run the function firstRefresh()
}

void loop(){
  readSensorValues();
  
  refereshCurrentPage();
  
  firstRefresh();
}

void firstRefresh() {

  if(myNex.currentPageId != myNex.lastCurrentPageId){ // If the two variables are different, means a new page is loaded.
    
    newPageLoaded = true;    // A new page is loaded
                             // This variable is used as an argument at the if() statement on the refreshPageXX() voids, 
                             // in order when is true to update all the values on the page with their current values
                             // with out run a comparison with the last value.
    
    switch(myNex.currentPageId){
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
                            // Now the values updated ONLY if the new value is different from the last Sent value.
                            // See void refreshPage0()
    
    myNex.lastCurrentPageId = myNex.currentPageId; // Afer the refresh of the new page We make them equal,
                                                   // in order to identify the next page change.
  }
}

void readSensorValues() {

  if((millis() - getDataTimer) > GET_DATA_EVERY){
    speed += 1;
  }
}

void refereshCurrentPage() {
// In this function we refresh the page currently loaded every DATA_REFRESH_RATE
  if((millis() - pageRefreshTimer) > DATA_REFRESH_RATE){
    switch(myNex.currentPageId){
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
  if(charge != chargeLast || newPageLoaded == true){
    String chargeString = String(charge, 1);
    myNex.writeStr("charge.value", chargeString);
  }

  if(speed != speedLast || newPageLoaded == true){
    String speedString = String(speed, 1);
    myNex.writeStr("speed.txt", speedString);
  }

  if(newPageLoaded == true){
    myNex.writeStr("warning.txt", warning);
  }
}

void refreshPage1() {
  if(pwrIn != pwrInLast || newPageLoaded == true){
    String pwrInString = String(pwrIn, 1);
    myNex.writeStr("powerIn.txt", pwrInString);
  }

  if(pwrOut != pwrOutLast || newPageLoaded == true){
    String pwrOutString = String(pwrOut, 1);
    myNex.writeStr("powerOut.txt", pwrOutString);
  }
}

void refreshPage2() {
  if(battTemp != battTempLast || newPageLoaded == true){
    String battTempString = String(battTemp, 1);
    myNex.writeStr("battTemp.txt", battTempString);
  }

  if(solarTemp != solarTempLast || newPageLoaded == true){
    String solarTempString = String(solarTemp, 1);
    myNex.writeStr("solarTemp.txt", solarTempString);
  }

  if(motorTemp != motorTempLast || newPageLoaded == true){
    String motorTempString = String(motorTemp, 1);
    myNex.writeStr("motorTemp.txt", motorTempString);
  }

  if(brakeTemp != brakeTempLast || newPageLoaded == true){
    String brakeTempString = String(brakeTemp, 1);
    myNex.writeStr("brakeTemp.txt", brakeTempString);
  }
}