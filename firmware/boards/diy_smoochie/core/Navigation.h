#pragma once

#include <Arduino.h>
#include <functional>

enum NavigationAction {
    NAV_SEL,
    NAV_UP,
    NAV_DW,
    NAV_BACK,
    NAV_NONE
    };
   

enum NavigationMode {
    MODE_DEFAULT,
    MODE_ENCODER
};

class Navigation {
public:
    Navigation();
    void begin();
    void update();
    NavigationAction getAction();
    bool isPressed(NavigationAction action);
    void setMode(NavigationMode mode);
    NavigationMode getMode();
    void resetMode(); // Hold select for 3 seconds to reset

private:
    NavigationMode _mode;
    NavigationAction _lastAction;
    unsigned long _lastDebounceTime;
    unsigned long _selectPressStart;
    bool _selectHeld;
    
   /* GPIO pins for DIY Smoochie
    static const uint8_t SEL_BTN = 0;
    static const uint8_t UP_BTN = 41;
    static const uint8_t DW_BTN = 40;*/
    
    // Button states
    bool _selState;
    bool _upState;
    bool _dwState;
    bool _lastSelState;
    bool _lastUpState;
    bool _lastDwState;
    
    void readButtons();
    NavigationAction decodeAction();
    void checkResetMode();
};
