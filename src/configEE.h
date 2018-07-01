#ifndef CONFIGEE_H
#define CONFIGEE_H

#include <Arduino.h>
#include <EEPROM.h>

#define CEE_VERSION   1

template <class T> int writeConfigEE(const T &value) {
    int ee = 0 ;  
    const byte *p = (const byte*)(const void*) &value ; 
    unsigned int i ; 
    for ( i=0; i< sizeof(value); i++) 
        EEPROM.write(ee++,*p++) ; 
    return i ; 
}

template <class T> int readConfigEE(T &value) {
    int ee = 0 ;  
    byte *p = (byte*)(void*)&value ; 
    unsigned int i ; 
    for( i=0; i< sizeof(value); i++) 
        *p++ = EEPROM.read(ee++) ;
    return i; 
}


struct ConfigEE {
    uint32_t freq ; 
    byte step ; 
    byte enable ; 
    byte saved ; 
    byte version;  // version of eeprom format
} ;


ConfigEE configee ; 

#endif
