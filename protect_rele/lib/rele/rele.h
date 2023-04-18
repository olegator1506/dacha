#include <Arduino.h>
#ifndef __RELE_H__
#define __RELE_H__

class Rele {
    protected:
     bool _ledState = false; 
     bool _motionState = false; // датчик движения
     bool _uState = false; // датчик напряжения
     bool _mainState = false;   
     bool _outState = false;

     unsigned long _timerOn = 0;
     unsigned long _timerOff = 0;   
     void _led(bool state, bool force = false); 
     void _stateChanged(bool force = false);
     void _delayedOn(void);
     void _delayedOff(void);
     void _resetTimers();
     void _pollSensors(void);
     void _setOut(bool state, bool force = false);   
    public:
        Rele(void);
        inline void ledOn(void) {_led(true);}
        inline void ledOff(void) {_led(false);}
        void ledSwitch(void);
        void loop();
        inline bool ledState() {return _ledState;}
        inline bool uState(void) {return _uState;}
        inline bool motionState(void) {return _motionState;}
        inline bool outState(void) {return _outState;}
        inline bool mainState(void) {return _mainState;}
        inline unsigned long timerOn(void) {return _timerOn;}
        inline unsigned long timerOff(void) {return _timerOff;}
};
#endif