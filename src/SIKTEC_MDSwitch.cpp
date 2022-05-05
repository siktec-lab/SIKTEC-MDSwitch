
#include "SIKTEC_MDSwitch.h"

namespace SIKtec {


DECLARE_MUTEX;
MDSwitchPins_t MDSwitch::pins;
volatile int32_t MDSwitch::debounceDelay = SIKTEC_MDS_DEBOUNCE_DELAY;
volatile int32_t MDSwitch::lastDebounceTime = 0;
volatile bool MDSwitch::active = SIKTEC_MDS_DEFAULT_ACTIVE;
volatile MDS_KEYS MDSwitch::consume = MDS_KEYS::NONE;

/**
 * @brief Construct a new MDSwitch::MDSwitch object
 * 
 * @param pushPin   - the middle push button pin
 * @param ccwPin    - the ccw switch button pin
 * @param cwPin     - the cw switch button pin
 * @param interPin  - shared switch interrupt pin
 */
MDSwitch::MDSwitch(const uint8_t pushPin, const uint8_t ccwPin, const uint8_t cwPin, const uint8_t interPin) {

    MDSwitch::pins.push         = pushPin;
    MDSwitch::pins.ccw          = ccwPin;
    MDSwitch::pins.cw           = cwPin;
    MDSwitch::pins.inter        = interPin;
    MDSwitch::lastDebounceTime  = millis();
    this->setPinModes();
}

/**
 * @brief Destroy the MDSwitch::MDSwitch object
 *        will free the eventcallbacks and release the interrupt
 */
MDSwitch::~MDSwitch() {

    //Free allocated memory:
    for (int i = 0; i < this->modes; ++i) {
        delete[] this->eventCallbacks[i];
    }
    delete[] this->eventCallbacks;

    //Release interrupt:
    if (MDSwitch::pins.inter != -1) {
       detachInterrupt(digitalPinToInterrupt(this->pins.inter));
    }
}

/**
 * @brief allocates the 2d array for callbacks and sets the pins + interrupt
 * 
 * @param _modes - how many modes to define? 
 * 
 * @returns void
 */
void MDSwitch::init(const int _modes) {

    //Define timer and pins:
    MDSwitch::lastDebounceTime  = millis();
    this->setPinModes();
    
    //Allocate callbacks arrays:
    this->modes = _modes;
    this->eventCallbacks = new MDSCallback_f *[_modes];
    for (int i = 0; i < _modes; ++i) {
        this->eventCallbacks[i] = new MDSCallback_f[this->callbacks];
    }
    //Set all to null:
    for(int m = 0; m < _modes; ++m) {
        for(int c = 0; c < this->callbacks; ++c) {
            this->eventCallbacks[m][c] = nullptr;
        }
    }
}

/**
 * @brief get the current used mode
 * 
 * @return size_t - the mode we are currently using
 */
size_t MDSwitch::mode() {
    return this->using_mode;
}

/**
 * @brief sets the mode - if the number is greater or smaller then available mode will reset to 0
 * 
 * @param mode - the mode index to use starting from 0
 * @return true - mode is set.
 * @return false - mode not defined.
 */
bool MDSwitch::mode(const int mode) {
    if (mode >= 0 && mode < this->modes) {
        this->using_mode = mode;
        return true;
    }
    return false;
}

/**
 * @brief sets pin modes which were set by the constructor
 * 
 * @returns void
 */
void MDSwitch::setPinModes() {

    //define pin modes
    pinMode(MDSwitch::pins.push,  INPUT);
    pinMode(MDSwitch::pins.ccw,   INPUT);
    pinMode(MDSwitch::pins.cw,    INPUT);
    pinMode(MDSwitch::pins.inter, INPUT);
    if (MDSwitch::pins.inter != -1) {
       attachInterrupt(digitalPinToInterrupt(this->pins.inter), MDSwitch::isr, RISING);
    }
    
}

/**
 * @brief enables the callbacks execution
 * 
 * @returns void
 */
void MDSwitch::enable() {
    //No lock needed as its a byte....
	MDSwitch::active = 1;
}

/**
 * @brief disables the callbacks execution
 * 
 * @returns void
 */
void MDSwitch::disable() {
    //No lock needed as its a byte....
	MDSwitch::active = 0;
}

/**
 * @brief attaches a cb function to a specific key in a specific mode
 * 
 * @param mode - which mode index?
 * @param key  - which key/button?
 * @param cb   - teh function pointer
 * 
 * @returns void
 */
void MDSwitch::attach(const int mode, const MDS_KEYS key, MDSCallback_f cb) {
    if (mode >= 0 && mode < this->modes) {
        this->eventCallbacks[mode][key] = cb;
    } else if (mode == -1) {
        for(int m = 0; m < this->modes; ++m) {
            this->eventCallbacks[m][key] = cb;
        }
    }
}

/**
 * @brief detaches a cb of a key in a specific mode
 * 
 * @param mode  - which mode index?
 * @param key   - which key/button?
 * 
 * @returns void
 */
void MDSwitch::detach(const int mode, const MDS_KEYS key) {
    if (mode >= 0 && mode < this->modes) {
        this->eventCallbacks[mode][key] = nullptr;
    } else if (mode == -1) {
        for(int m = 0; m < this->modes; ++m) {
            this->eventCallbacks[m][key] = nullptr;
        }
    }
}

/**
 * @brief when called check if an event was captured and executes relevent callbacks
 * 
 * @return true - executed a callback.
 * @return false - no callback was executed.
 */
bool MDSwitch::tick() {
    bool executed = false;
    if (MDSwitch::consume != MDS_KEYS::NONE) {
        if (this->eventCallbacks[this->using_mode][MDSwitch::consume] != nullptr) {
            (this->eventCallbacks[this->using_mode][MDSwitch::consume])(this->using_mode, MDSwitch::consume);
            executed = true;
        }
        if (this->eventCallbacks[this->using_mode][MDS_KEYS::ANY] != nullptr) {
            (this->eventCallbacks[this->using_mode][MDS_KEYS::ANY])(this->using_mode, MDSwitch::consume);
            executed = true;
        }
        MDSwitch::consume = MDS_KEYS::NONE;
    }
    return executed;
}

/**
 * @brief reading all pins to determine which key of the switch was used
 * 
 * @return MDS_KEYS - the key/button
 */
MDS_KEYS PUT_IN_RAM MDSwitch::read() {
    if (digitalRead(MDSwitch::pins.push)) return MDS_KEYS::PUSH;
    if (digitalRead(MDSwitch::pins.ccw)) return MDS_KEYS::CCW;
    if (digitalRead(MDSwitch::pins.cw)) return MDS_KEYS::CW;
    return MDS_KEYS::NONE;
}

/**
 * @brief internal interupt method used to set the correct captured key and handle the debouncing logic
 * 
 * @returns void
 */
void PUT_IN_RAM MDSwitch::isr() {

    DISABLE_INTERRUPS;
    
    int32_t current = millis();
    if ((current - MDSwitch::lastDebounceTime) > MDSwitch::debounceDelay) {
        //Update time:
        MDSwitch::lastDebounceTime = current;
        //Read pins:
        MDS_KEYS got = MDSwitch::read();
        //Set key to consume:
        if (MDSwitch::active && got != MDS_KEYS::NONE) {
            MDSwitch::consume = got;
        }
    }

    ENABLE_INTERRUPS;
}

}