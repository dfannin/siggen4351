#include <Arduino.h>
#include "OneWireKey.h"

#define OWK_ADDR 0x26

OneWireKey owk = OneWireKey(OWK_ADDR) ;

void gotkey(char c, uint8_t ptype) {
    Serial.print("char is ") ;
    Serial.print(c) ;
    Serial.print(" type is ") ;
    Serial.println(ptype) ;
}

void setup() {

    int error;

    Wire.begin() ;
    Serial.begin(9600) ;
    Serial.println(F("one wire key example start")) ;

  
    Serial.println(F("looking for Keypad...")) ; 
    error = 1 ; 
    while (error != 0 ) {
        Wire.beginTransmission(OWK_ADDR) ;
        error = Wire.endTransmission() ;
    }
    Serial.println(F("keypad found.")) ; 
    // setup owk 
    owk.init() ;
    owk.addEventListener(gotkey) ; 
}



char prevc = 0 ; 
char c ; 

void loop() {


    c = owk.get_key() ; 

    if ( c != prevc ) {
        prevc = c ; 
        if ( c != 0 ) {
            Serial.print(F("press: ")) ; 
            Serial.print(c) ;
            Serial.print(" press type is ") ; 
            Serial.println(owk.lastpresstype) ; 
        }
    }


}
