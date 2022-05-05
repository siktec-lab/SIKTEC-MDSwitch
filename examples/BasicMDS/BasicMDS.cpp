/******************************************************************************/
// Created by: SIKTEC.
// Release Version : 1.0.1
// Creation Date: 2022-04-12
// Copyright 2022, SIKTEC.
/******************************************************************************/
/*****************************      NOTES       *******************************
    -> This prints to serial and increments a counter.
    -> I uses 'modes' and the PUSH key is changing the mode
    -> CW, CCW are incrementing and decrementing the counter ONLY in mode 0.
    -> ANY callback is printing the counter in MODES 0 , 1.
    -> The Multi directional switch is triggering an interrupt all managed by the lib.
*******************************************************************************/
/*****************************      Changelog       ****************************
1.0.1:
    -> initial release.
*******************************************************************************/

//------------------------------------------------------------------------//
// INCLUDES:
//------------------------------------------------------------------------//

#include <Arduino.h>
#include <SIKTEC_MDSwitch.h>     

//------------------------------------------------------------------------//
// PIN DEFINITION and define literals:
//------------------------------------------------------------------------//

#define CONT_SERAIL_BAUD    115200

//Declare your own pins:
#ifdef ESP32
    #define SW_INT  35
    #define SW_PUSH 34
    #define SW_CCW  36
    #define SW_CW   39
#else
    #define SW_INT  3
    #define SW_PUSH 4
    #define SW_CCW  5
    #define SW_CW   6
#endif

using namespace SIKtec;


//The switch object"
MDSwitch mdswitch(SW_PUSH, SW_CCW, SW_CW, SW_INT);


//Some variables to store the mode and the counter:
static int usemode = 0;
static int counter = 0;

void printModeKey(const int, const char *key); // Simple helper function declared later

void printModeKey(const int mode, const char *key) {
    Serial.print(" -> MODE:");
    Serial.print(mode);
    Serial.print(" - KEY:");
    Serial.println(key);
}
//------------------------------------------------------------------------//
// CALLBACK functions that will be attached in the setup loop
// all callbacks have the form of `void (const int, const MDS_KEYS)`
// No need to declare volatile variables or IRAM_ATTR as those callbacks are not directly called fro the ISR. 
//------------------------------------------------------------------------//
void cb_push(const int mode, const MDS_KEYS key) {
    printModeKey(mode, "PUSH");
    if (!mdswitch.mode(++usemode)) {
        usemode = 0;
        mdswitch.mode(0);
    }
    Serial.print("    * switched MODE to ");
    Serial.println(usemode);
}
void cb_ccw(const int mode, const MDS_KEYS key) {
    counter++;
    printModeKey(mode, "CCW");
}
void cb_cw(const int mode, const MDS_KEYS key) {
    counter--;
    printModeKey(mode, "CW");
}
void cb_any(const int mode, const MDS_KEYS key) {
    printModeKey(mode, "ANY");
    Serial.print("    * counter is ");
    Serial.println(counter);
}

//------------------------------------------------------------------------//
// SETUP: run once
//------------------------------------------------------------------------//
void setup() {

    //Initiate serial port:
    Serial.begin(CONT_SERAIL_BAUD);
    while (!Serial) { ; }
    
    //TODO: remove this later its just for debugging
    #ifdef ESP32 
        Serial.println("ESP32 is the micro");
    #else
        Serial.println("ARDUINO is the micro");
    #endif

    //Initialize and set the switch object:
    mdswitch.init(3 /* how many modes */); ///< each mode can take a set of callbacks.
    mdswitch.mode(usemode); ///< set start mode:

    //Attach th callbacks - (mode, key, &fn):

    mdswitch.attach(-1, MDS_KEYS::PUSH, &cb_push);  ///< -1 means all modes - 0,1,2.
    mdswitch.attach(0, MDS_KEYS::CCW,  &cb_ccw);    ///< only in mode 0.
    mdswitch.attach(0, MDS_KEYS::CW,   &cb_cw);     ///< only in mode 0.
    mdswitch.attach(0, MDS_KEYS::ANY,  &cb_any);    ///< any will be called in mode 0 + 1
    mdswitch.attach(1, MDS_KEYS::ANY,  &cb_any);

    delay(1000);
}

void loop() {

    //This is the tick - extremely fast just checks if a key was recorded.  
    mdswitch.tick();

    delay(20);
}