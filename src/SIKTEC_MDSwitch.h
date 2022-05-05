/******************************************************************************/
// Created by: SIKTEC.
// Release Version : 1.0.1
// Creation Date: 2022-04-12
// Copyright 2022, SIKTEC.
// 
/******************************************************************************/
/*****************************      NOTES       *******************************
    -> UC8276 -> IL0398 SIKTEC_EPD_G4
    -> UC8276 -> SIKTEC_EPD_3CU
    -> SSD1619 -> SIKTEC_EPD_3CS
*******************************************************************************/
/*****************************      Changelog       ****************************
1.0.1:
    -> initial release.
    -> adafruit gfx compatible.
    -> 3 drivers allready implemented.
    -> SRAM support - 23K256-I/SN implements SIKTEC_SRAM Library.
    -> MONO, TRICOLOR, 4GRAY Modes support.
*******************************************************************************/

#pragma once

//------------------------------------------------------------------------//
// INCLUDES:
//------------------------------------------------------------------------//

#include <Arduino.h>

namespace SIKtec {

//------------------------------------------------------------------------//
// DEFAULTS:
//------------------------------------------------------------------------//

#define SIKTEC_MDS_DEFAULT_ACTIVE       true        // active or disable callback by default.
#define SIKTEC_MDS_DEBOUNCE_DELAY       80          // switch debounce delay.
#define SIKTEC_MDS_DEFAULT_MODES        1           // default modes we are going to allocate memory for.

#ifdef ESP32
    #define PUT_IN_RAM IRAM_ATTR 
    #define DEFINE_MUTEX static portMUX_TYPE switch_mutex
    #define DECLARE_MUTEX portMUX_TYPE MDSwitch::switch_mutex = portMUX_INITIALIZER_UNLOCKED
    #define DISABLE_INTERRUPS portENTER_CRITICAL(&MDSwitch::switch_mutex)
    #define ENABLE_INTERRUPS  portEXIT_CRITICAL(&MDSwitch::switch_mutex)
#else
    #define PUT_IN_RAM  
    #define DEFINE_MUTEX
    #define DECLARE_MUTEX
    #define DISABLE_INTERRUPS noInterrupts()
    #define ENABLE_INTERRUPS  interrupts()
#endif

//------------------------------------------------------------------------//
// TYPES and ENUMS:
//------------------------------------------------------------------------//
/** @brief naming on the switch keys/buttons: */
enum MDS_KEYS {
    PUSH,
    CW,
    CCW,
    ANY,
    NONE,
    NUMBER_OF_KEYS // 5
};

/** @brief a small struct to store the pins */
typedef struct MDSwitchPins {
    uint8_t push; 
    uint8_t ccw; 
    uint8_t cw; 
    uint8_t inter; 
} MDSwitchPins_t;

/** @brief the cb function pointer that can be attached as a callback */
typedef void (*MDSCallback_f)(const int mode, const MDS_KEYS key);

//------------------------------------------------------------------------//
// MDSwitch Class:
//------------------------------------------------------------------------//

class MDSwitch {

private:

    // a mutex only used with ESP boards
    DEFINE_MUTEX;

    // Internal struct of the pins used:
    static MDSwitchPins_t pins;

    // Stores the last time in milli seconds a keypress triggered interupt. 
    static volatile int32_t lastDebounceTime;
    
    // Defines the debounce delay to be use
    static volatile int32_t debounceDelay;

    // a flag to enable / disable callbacks execution.
    static volatile bool active;

    // will store the captured key event.
    static volatile MDS_KEYS consume;

    //a container for the callbacks pointers in a 2D format -> mode - callbacks
    MDSCallback_f **eventCallbacks;
    
    // How many modes we are using:
    int modes        = 0;

    //How many call backs are available:
    int callbacks    = MDS_KEYS::NUMBER_OF_KEYS;

    //Internal mode flag: 
    int using_mode   = 0;

public:

    /** @brief the constructor - will only set the time and store the the pin definition */
    MDSwitch(const uint8_t pushPin, const uint8_t ccwPin, const uint8_t cwPin, const uint8_t interPin = -1);
    
    /** @brief the destructor - will free the eventcallbacks and release the interrupt */
    ~MDSwitch();

    /** @brief allocates the 2d array for callbacks and sets the pins + interrupt */
    void init(const int _modes = SIKTEC_MDS_DEFAULT_MODES);

    /** @brief get the current used mode */
    size_t mode();

    /** @brief sets the mode - if the number is greater or smaller then available mode will reset to 0 */
    bool mode(const int mode);

    /** @brief enables the callbacks execution */
    void enable();

    /** @brief disables the callbacks execution */
    void disable();

    /** @brief attaches a cb function to a specific key in a specific mode */
    void attach(const int mode, const MDS_KEYS key, MDSCallback_f cb);

    /** @brief detaches a cb of a key in a specific mode */
    void detach(const int mode, const MDS_KEYS key);

    /** @brief when called check wether an event was captured and executes relevent callbacks */
    bool tick();

    /** @brief reading all pins to determine which key of the switch was used */
    static MDS_KEYS PUT_IN_RAM read();

private:

    /** @brief sets pin modes which were set by the constructor */
    void setPinModes();

    /** @brief internal interupt method used to set the correct captured key and handle the debouncing logic */
    static void PUT_IN_RAM isr();

};

}

