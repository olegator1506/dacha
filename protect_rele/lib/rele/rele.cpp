#include <Arduino.h>
#include "rele.h"
#include "../../include/dbg.h"
#include "../../include/_config.h"

#ifdef PIO_UNIT_TESTING 
    bool _emulateMotion = false;
    bool _emulateU = false;
#endif

Rele::Rele(void) {
    DBG(__FUNCTION__);
    pinMode(LED_PIN, OUTPUT);
    pinMode(PIN_OUTPUT, OUTPUT);
    pinMode(PIN_PIR, INPUT);
    pinMode(PIN_U, INPUT);
    _led(false,true);
    _setOut(false, true);
    _stateChanged(true);
}

void Rele::_led(bool state, bool force) {
    DBG(__FUNCTION__);
    if((_ledState == state) && !force) return;
    _ledState = state;
    DBG("Led %s",_ledState ? "On":"Off");
    digitalWrite(LED_PIN,!_ledState);
}

void Rele::_delayedOn() {
    DBG(__FUNCTION__);
    _timerOff = 0;
    _timerOn = millis() + DELAY_ON;
}

void Rele::_delayedOff() {
    DBG(__FUNCTION__);
    _timerOn = 0;
    _timerOff = millis() + DELAY_OFF;
}

void Rele::_stateChanged(bool force) {
    DBG(__FUNCTION__);
    _timerOn = 0;
    _timerOff = 0;
    DBG("_motionState %s, _uState %s", _motionState ? "ON":"OFF",_uState ? "ON":"OFF");
    bool state = !_motionState && !_uState;
    if((_mainState == state) && !force) return;
    _mainState = state;
    if(_mainState && !_outState) 
        _delayedOn();
    else if(!_mainState && _outState) 
        _delayedOff();    
}


void Rele::ledSwitch(void) {
    DBG(__FUNCTION__);
    bool state = !_ledState;
    _led(state);
}
void Rele::_resetTimers(void) {
    DBG(__FUNCTION__);
    _timerOff = 0;
    _timerOn = 0;
}

void Rele::_pollSensors(void) {
    bool motion,u, changed = false; 
//    DBG(__FUNCTION__);
#ifdef PIO_UNIT_TESTING 
    motion = _emulateMotion;
    u = _emulateU;
#else
    motion = digitalRead(PIN_PIR);
    u = digitalRead(PIN_U);
#endif
    if(motion != _motionState) {
        _motionState = motion;
        changed = true;
    }
    if(u != _uState) {
        _uState = u;
        changed = true;
    }
    if(changed) _stateChanged();
}

void Rele::_setOut(bool state, bool force) {
    DBG(__FUNCTION__);
    if((state == _outState) && !force) return;
    _outState = state;
    digitalWrite(PIN_OUTPUT, _outState);
    _led(state);
}

void Rele::loop(void) {
    unsigned long t = millis();
    delay(1);
//    DBG(__FUNCTION__);
    if(_timerOn && (t > _timerOn)) {
        _setOut(true);
        _resetTimers();
    }
    else if(_timerOff && (t > _timerOff)){
       _setOut(false);
       _resetTimers();
    } else {
        _pollSensors();
    }
}

