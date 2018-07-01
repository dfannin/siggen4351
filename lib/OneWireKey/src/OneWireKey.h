#ifndef ONEWIREKEY_H
#define ONEWIREKEY_H

#include <Arduino.h>
#include <stdint.h>
#include <Wire.h>

/*
 *  PIN MAPPING
 *  Here you can change your wire mapping between your keypad and PCF8574
 */


// maps to pcf8574 pin

#define C0  0  
#define C1  1  
#define C2  2  
#define C3  3  
#define R0  4  // miswired on board 
#define R1  5  
#define R2  6  // miswired on board
#define R3  7  

// 
#define NUMR  4
#define NUMC  4

// minimal interval between get_key checks
//
#define KP_INTERVAL 100
// hold time 10 x 100 = 1000 ms
#define HOLDTIME 10
// long hold time 20 x 100 = 1000 ms
#define LONGHOLDTIME 20

#define OWK_NONE     0x0 
#define OWK_PRESSED  0x1 
#define OWK_HOLD     0x2 
#define OWK_LONGHOLD 0x3 

class OneWireKey {
    public:
          OneWireKey(int i2c_addr);
          char get_key();
          void init();
          void write(uint8_t value);
          uint8_t read();
          void addEventListener( void(*listener) (char,uint8_t)) ; 
          void setholdtime(uint8_t hold) ;  // number of KP_INTERVALS
          void setlongholdtime(uint8_t longhold); // number of KP_INTERVALS (must be greater than holdtime) ;
          int  addr ;
          void (*eventlistener)(char,uint8_t)  ; 
          char lastkey ; 
          uint8_t  lastpresstype ; 
          uint8_t  presstype ; 
          unsigned long timer ; 
          uint8_t  holdtime ; 
          uint8_t  longholdtime ;
          uint8_t  countup ; 
           
};

#endif
