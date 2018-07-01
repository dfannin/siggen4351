#include <Arduino.h>
#include "adf4351.h"
#include "configEE.h"
#include <OneWireKey.h>
#include <LiquidCrystal_PCF8574.h>
#include <ACE128.h>
#include <ACE128map87654321.h>

#define SWVERSION "1.0s"

//  pinSS, mode, speed, endian
ADF4351  vfo(PIN_SS,SPI_MODE0, 1000000UL , MSBFIRST) ; 

#define OWK_ADDR 0x26
OneWireKey owk = OneWireKey(OWK_ADDR) ; 

#define LCD_ADDR 0x27
LiquidCrystal_PCF8574 lcd(LCD_ADDR);

unsigned long lcdtimer = 0 ; 
byte lcd_backlight_override = false ; 

#define ACE_ADDR 0x3D
ACE128 re((uint8_t) ACE_ADDR, (uint8_t*) encoderMap_87654321 ) ;
uint8_t cupos = 0 ;
uint8_t pupos = 0  ;  
unsigned long retimer = 0 ; 
int reguardtime = 0 ; 

byte ovenready = false ; 

#define PIN_LEDA 3
#define PIN_LEDB 4

unsigned long dtimer = 0 ; 
boolean displaynow = false ; 


int mode = 0  ; 
char ibuf[20];
int ibufidx = 0 ; 
int error= 0 ; 
int decpt = 0 ; // decimal point seen 
int cstep = 0 ; 
uint32_t freq_b = 0 ; 


void eeprom_clear() {
    configee.version = CEE_VERSION ; 
    configee.enable = false ; 
    configee.freq = 1000000000UL ; 
    configee.step = 0 ; 
    configee.saved = false ; 
    writeConfigEE(configee) ; 
}

void eeprom_startup() {
    readConfigEE(configee) ; 
    if (configee.version != CEE_VERSION ) {
        eeprom_clear() ; 
        return ; 
    }
    if ( configee.saved ) {
        cstep = configee.step ; 
        vfo.ChanStep = steps[cstep]; 
        if ( vfo.setf(configee.freq) > 0 )  {  
            error = 501 ; 
        } else { 
            error = 0 ; 
            if ( configee.enable ) 
                vfo.enable() ; 
            else 
                vfo.disable() ; 
        } 
    }
    
}

void eeprom_save() {
     configee.freq = vfo.cfreq ; 
     configee.enable = vfo.enabled ; 
     configee.step = cstep ; 
     configee.saved = true ; 
     writeConfigEE(configee) ; 
}


void chngstep(int change) {
    cstep += change ; 
    if ( cstep > NSTEPS - 1 ) cstep = 0 ; 
    if ( cstep < 0 ) cstep = NSTEPS - 1 ; 
    vfo.ChanStep = steps[cstep] ; 
   if ( vfo.setf(vfo.cfreq) > 0 ) 
      error=302 ;  
   else 
      error=0 ; 
}

void blink_led() {
    digitalWrite(PIN_LEDA,HIGH) ; 
    delay(1000) ; 
    digitalWrite(PIN_LEDA,LOW) ; 
    delay(500) ; 
}

uint32_t  mystrtoul(char str[], uint32_t multi) {
    int len = strlen(str) ; 
    if ( len <= 0 ) return 0UL ; 

    char * pptr = NULL ; 
    char msbuf[12] ; 
    uint32_t  value ; 
    if ( ( pptr = strchr(str,'.')) != NULL )  {
       memcpy(msbuf,str,pptr-str) ;  // copy part before the decpt
       msbuf[pptr-str] = '\0' ; 
       value = strtoul(msbuf,NULL,10) * multi ; 
       strcpy(msbuf,pptr); 

       value = value + (uint32_t) ( ( atof(msbuf) * (float) ( multi ) ))  ; 
       return value ; 
    } 

    return (  strtoul(str,NULL,10) * multi ) ;
}

void fill(char * str){
    int i,sl ; 
    if ( str == NULL ) 
        sl = 0 ; 
    else 
        sl = strlen(str) ; 

    for(i=sl;i<20;i++) str[i]=' '; 
    str[20]= 0 ; 
}

void menu_config(char c) {

    if ( c == 'A') {
       if ( ++vfo.pwrlevel > 3 ) vfo.pwrlevel = 0 ; 
       if ( vfo.setf(vfo.cfreq) > 0 ) 
          error=300 ;  
       else 
          error=0 ; 
    }

    if ( c == 'B') {
        chngstep(1) ;
    }

    if ( c == 'C') {
        if ( vfo.reffreq == 100000000UL ) { // change to 10 mhz
            steps[0] = 2500 ; 
            vfo.BandSelClock = 80 ; 
            vfo.RCounter = 1 ; 
            vfo.ChanStep = steps[0] ; 
            if ( vfo.setrf(10000000UL) > 0 )  
                error= 303 ; 
            else 
                error = 0 ; 
        } else { // change to 100 mhz
            steps[0] = 1000 ; 
            vfo.BandSelClock = 80 ; 
            vfo.RCounter = 25 ; 
            vfo.ChanStep = steps[0] ; 
            if ( vfo.setrf(100000000UL) > 0 )  
                error=304 ; 
            else
                error=0 ; 
        }

       if ( error == 0 ) {
           if ( vfo.setf(vfo.cfreq) > 0 ) 
              error=301 ;  
           else 
              error=0 ; 
        }
    }


    if ( c == '#' ) { // cancel and return 
        mode = 0 ; 
    }

}
void menu_debug(char c) {
    mode = 0 ; 
    return ; 
}

void menu_sweep(char c) {

    if ( c == '#' ) { // cancel and return 
        mode = 0 ; 
    }
}

int chstate = 0 ; 

void menu_chan(char c) {
    uint32_t newfreq ; 


    if ( c == 'A') {
             newfreq = channels[chstate] ;   
             if ( vfo.setf(newfreq) > 0 ) 
                 error=400 ;  
             else 
                 error=0 ; 
             mode = 0 ; 
             chstate = 0 ; 
    }

    if ( c == 'B') {
             if ( --chstate < 0 ) chstate = NCHANS - 1 ; 
    }

    if ( c == 'C') {
            if (  ++chstate > NCHANS - 1 ) chstate = 0  ; 
    }


    if ( c == '#' ) { // cancel and return 
        chstate = 0 ; 
        mode = 0 ; 
    }

    return ; 

}

void menu_freq(char c) {
    static int state = 0 ; 
    uint32_t newfreq ; 

    if (state == 0 ) { 
        ibuf[0] = '\0' ; 
        ibufidx = 0 ; 
        decpt = 0 ; 

    }

    if ( ibufidx > 11 ) {
         return ; 
    }

    if ( ( c >= '0' && c <= '9' )) {
        ibuf[ibufidx++] = c ; 
        ibuf[ibufidx] = '\0' ; 
        state = 1 ; 
    }

    if ( c == '*' && decpt == 0 ) {
        ibuf[ibufidx++] = '.' ; 
        ibuf[ibufidx] = '\0' ; 
        state = 1 ; 
        decpt = 1 ; 
    }

    if ( c == 'A') {
             newfreq = strtoul(ibuf,NULL,10) ;  
             if ( vfo.setf(newfreq) > 0 ) 
                 error=100 ;  
             else 
                 error=0 ; 
             mode = 0 ; 
             state = 0 ; 
    }


    if ( c == 'B') {
             newfreq = mystrtoul(ibuf,1000000000UL) ;  
             if ( vfo.setf(newfreq) > 0 ) 
                 error=101 ;  
             else 
                 error=0 ; 
             mode = 0 ; 
             state = 0 ; 
    }

    if ( c == 'C') {
             newfreq = mystrtoul(ibuf,1000000UL)  ;  
             if ( vfo.setf(newfreq) > 0 ) 
                 error=102 ;  
             else 
                 error=0 ; 
             mode = 0 ; 
             state = 0 ; 
    }

    if ( c == 'D') {
             newfreq = mystrtoul(ibuf, 1000) ;  
             if ( vfo.setf(newfreq) > 0 ) 
                 error=103 ;  
             else 
                 error=0 ; 
             mode = 0 ; 
             state = 0 ; 
    }

    if ( c == '#' ) { // cancel and return 
        state = 0 ; 
        mode = 0 ; 
    }

    return ; 

}

// top menu
void gotkey(char c, uint8_t ptype) {

    displaynow = true ; 
    lcdtimer = millis() ; 

    switch(mode) {
        case 1: menu_freq(c) ; 
                return ; 
                break ; 
        case 2: menu_chan(c) ; 
                return ; 
                break ; 
        case 3: menu_config(c) ; 
                return ; 
                break ; 
        case 4: menu_debug(c) ; 
                return ; 
                break ; 
        case 5: menu_sweep(c) ; 
                return ; 
                break ; 
    }


    if ( c == 'A' && ptype == OWK_PRESSED)  { 
        mode = 1 ; 
        ibuf[0] = '\0' ; 
        ibufidx = 0 ;
    } 
    if ( c == 'A' && ptype >= OWK_HOLD)  { 
        mode = 5 ; 
        ibuf[0] = '\0' ; 
        ibufidx = 0 ;
    } 
    if ( c == 'B' && ptype == OWK_PRESSED) { 
        mode = 2 ; 
        ibuf[0] = '\0' ; 
        ibufidx = 0 ;
    } 
    if ( c == 'C' && ptype == OWK_PRESSED)  { 
        mode = 3 ; 
        ibuf[0] = '\0' ; 
        ibufidx = 0 ;
    } 
    if ( c == 'D' && ptype == OWK_PRESSED)  { 
        mode = 4 ; 
        ibuf[0] = '\0' ; 
        ibufidx = 0 ;
    } 

    // save registers for current settings
    if ( c == '1' && ptype == OWK_PRESSED) {
        eeprom_save() ; 
    }

    if ( c == '1' && ptype >= OWK_HOLD) {
        eeprom_clear() ; 
    }

    if ( c == '4') {
          chngstep(1) ;  
    }

    if ( c == '9') {
          lcd_backlight_override = ! lcd_backlight_override ; 
    }


    if ( c == '5') {
       if ( ++vfo.pwrlevel > 3 ) vfo.pwrlevel = 0 ; 
       if ( vfo.setf(vfo.cfreq) > 0 ) 
          error=200 ;  
       else 
          error=0 ; 
    }

    if ( c == '0') {
        if (vfo.enabled)
            vfo.disable(); 
        else 
            vfo.enable(); 
    }

    if ( c == '*') {
    // decrement freq by chanstep
       uint32_t newfreq  ;  
        if ( vfo.cfreq - ADF_FREQ_MIN >= vfo.ChanStep ) 
            newfreq = vfo.cfreq - vfo.ChanStep ; 
        else 
            newfreq = vfo.cfreq;

       if ( vfo.setf(newfreq) > 0 ) 
          error=201 ;  
       else 
          error=0 ; 
    }

    if ( c == '#') {
    //increment freq by chanstep
        uint32_t newfreq ;
        if ( ADF_FREQ_MAX - vfo.cfreq >= vfo.ChanStep ) 
            newfreq = vfo.cfreq + vfo.ChanStep ; 
        else 
            newfreq = vfo.cfreq ; 

       if ( vfo.setf(newfreq) > 0 ) 
          error=202 ;  
       else 
          error=0 ; 
    }

}

int checkI2C(byte addr) {
    int error = 1 ; 

    int cnt = 500 ; 
     while(error != 0 ) {
        Wire.beginTransmission(addr) ;
        error = Wire.endTransmission() ;
        if ( cnt-- < 0 ) return 1 ;  
    }

    return 0 ; 
}

#define MAXCMD 13 

char indata[MAXCMD+1] ; 
int inindex = 0 ; 

void clr_cmd_buf() {
    inindex = 0 ; 
    indata[0] = '\0' ;  
}

int parse_cmd(char c) {
    if (inindex >= MAXCMD - 1 ) {
       clr_cmd_buf() ;  
       return(-1) ;
    }
    if (inindex == 0 && c == ' ') return(-1) ;
    if ( c == '\n' || c == '\r' ) {
        switch(toupper(indata[0])) {
            case 'F': // set frequency 
                if (inindex >=  8) {  
                    uint32_t nfreq = strtoul(indata+1,NULL,10) ; 
                    if ( vfo.setf(nfreq)  > 0 )  
                        Serial.println(F("F-")) ; 
                    else 
                        Serial.println(F("F+")) ; 
                }
                break ;
            case 'S': // set step
                if (inindex >=  2) {  
                    int nstep = atoi(indata+1) ; 
                    if ( nstep >  0  && nstep < NSTEPS ) { 
                        vfo.ChanStep = steps[nstep] ; 
                        Serial.println("S+") ; 
                    } else {
                        Serial.println("S-") ; 
                    }
                }
                break ;
            case 'E': // enable
                vfo.enable() ; 
                Serial.println(F("E+")) ; 
                break ;
            case 'D':  // disable
                vfo.disable() ; 
                Serial.println(F("D+")) ; 
                break ;
            case 'C':  // save current config
                if ( inindex > 1 ) { 
                    if ( indata[1] == '0' ) {
                    eeprom_clear() ; 
                    Serial.println(F("C0+")) ; 
                    }
                } else {
                    eeprom_save() ; 
                    Serial.println(F("C+")) ; 
                }
                break ;
            case 'B':  // toggle backlight override
                lcd_backlight_override = ! lcd_backlight_override ; 
                Serial.println(F("B+")) ; 
                break ; 
            case '?':  // current settings
                char obuf[25] ; 
                sprintf(obuf,"F%10lu S%7lu %1d\n",vfo.cfreq,vfo.ChanStep,vfo.enabled) ;  
                Serial.println(obuf) ; 
                break ;
        }
        clr_cmd_buf() ; 
        return 0 ; 
    }

    indata[inindex++] = c ; 
    indata[inindex] = '\0' ; 
    return 0 ; 

}



void setup() {
    Serial.begin(9600) ;
    Serial.print(F("Hello adf4351 v")) ; 
    Serial.println(SWVERSION) ;
    Wire.begin() ; 
    pinMode(PIN_LEDA, OUTPUT) ;
    digitalWrite(PIN_LEDA,LOW) ; 
    pinMode(PIN_LEDB, OUTPUT) ;
    digitalWrite(PIN_LEDB,LOW) ; 

    checkI2C(LCD_ADDR);
    lcd.begin(20,4);
    lcd.setBacklight(255);
    lcd.home() ; lcd.clear() ;
    lcd.print(F("SigGen ADF4351 v")) ;
    lcd.print(SWVERSION) ;

    if ( checkI2C(OWK_ADDR) > 0 ) {
        lcd.setCursor(0,1) ; 
        lcd.print(F("OWK not found")) ; 
        while(1) blink_led() ;  
    } ;
    owk.init() ;
    owk.addEventListener(gotkey) ; 

    if ( checkI2C(ACE_ADDR) > 0 ) {
        lcd.setCursor(0,1) ; 
        lcd.print(F("ACE not found")) ; 
        while(1) blink_led() ;  
    }

    re.begin() ; 
    re.setMpos(0) ;

    lcd.setCursor(0,1) ;
    lcd.print(F("Checks Passed")) ; 
    vfo.init() ; 
    vfo.enable() ; 
    blink_led() ; 
    blink_led() ; 
    blink_led() ; 
    vfo.ChanStep = steps[cstep] ; 
    ibuf[0] = '\0' ; 
    lcd.home() ; lcd.clear() ; 
    if ( vfo.setf(1000000000) > 0 ) 
        error = 110 ; 
    else 
        error = 0 ; 
    eeprom_startup() ; 
}


char kp ; 

void loop() {

    char buf[22] ; 

    while (Serial.available() > 0) parse_cmd(Serial.read()); 

    kp = owk.get_key() ;

    if (millis() - retimer > 150) {
        retimer = millis() ; 
        cupos = re.upos() ;

        if ( reguardtime > 0 ) {
            reguardtime-- ; 
            pupos = cupos ; 
            return ; 
        }

        if (cupos != pupos ) {
            int8_t diff = 0 ; 

            lcdtimer = millis(); 

            // 63 = 127/2 
            if ( abs( (int) ( cupos - pupos) ) <=  63 ) {  // check for zero crossing
                diff = cupos - pupos  ; 
            } else if ( pupos <= 63 ) { // zero crossing, descreasing
                diff = - ( ( 127-cupos )  + pupos )  ; 
            } else { // zero cross, increasing
                diff = (127-pupos) +  cupos ; 
            }


            if ( mode == 0 || mode == 4) { // only change freq for main and debug menus
                uint32_t multi = 1 + abs(diff)/5   ; // heristic for adding velocity
                uint32_t newfreq  ;  
                if ( diff > 0 ) {
                    if ( ADF_FREQ_MAX - vfo.cfreq >= ( vfo.ChanStep * multi)  ) 
                        newfreq = vfo.cfreq + ( vfo.ChanStep * multi) ; 
                    else 
                        newfreq = vfo.cfreq ; 
                } else {
                    if ( vfo.cfreq - ADF_FREQ_MIN >= ( vfo.ChanStep * multi) ) 
                        newfreq = vfo.cfreq - ( vfo.ChanStep * multi)  ; 
                    else 
                        newfreq = vfo.cfreq;
                }

               if ( vfo.setf(newfreq) > 0 ) 
                  error=201 ;  
               else 
                  error=0 ; 

               reguardtime = 1 ; 
            } 

            pupos = cupos ; 
        }

    }

    if (millis() - dtimer > 1000 || displaynow) {
        dtimer = millis() ;
        displaynow = false ;

        if ( digitalRead(PIN_LD) ) {
            digitalWrite(PIN_LEDA, HIGH) ; 
        } else {
            digitalWrite(PIN_LEDA, LOW) ; 
        }

        int i; 

        lcd.setCursor(0,0) ;
        // freq,enabled,pwrlevel (20 char) 
        sprintf(buf,"%13lu %-3s P%1d",vfo.cfreq, vfo.enabled? "On" : "Off", vfo.pwrlevel) ;  

        buf[0] = buf[3];  
        if ( buf[0] != ' ') buf[1] = ',' ; 
        for(i=0;i<3;i++) buf[i+2] = buf[i+4] ;
        buf[5] = ',' ; 
        for(i=0;i<3;i++) buf[i+6] = buf[i+7] ;
        buf[9] = ',' ; 
        lcd.print(buf); 

        lcd.setCursor(0,1) ; 
        // error code, step , mode (20 char)

        // convert chanstep for display
        char csbuf[9] ;  
        int  csdig; 
        char csunit;
        if ( vfo.ChanStep >=  999999UL ) { 
            csdig = vfo.ChanStep / 1000000UL ; 
            csunit = 'M' ; 
        } else {  
            csdig = vfo.ChanStep / 1000UL ; 
            csunit = 'k' ; 
        }   
        sprintf(csbuf,"%3d %c", csdig, csunit) ;  

        sprintf(buf,"E%3d S%5s %5s %c", error,csbuf, 
                ( vfo.Frac == 0 ) ? "Int-N": "FracN",
                ( configee.saved == 1) ? '@' : ' ' ) ;  
        lcd.print(buf) ; 

         



         switch(mode) { 

         case 1: // freq mode 
            lcd.setCursor(0,2) ; 
            strcpy(buf,"Enter Frequency") ; 
            fill(buf) ;
            lcd.print(buf) ; 

            lcd.setCursor(0,3) ; 
            sprintf(buf," %-10s", ibuf) ;  
            fill(buf) ;
            lcd.print(buf) ; 
            break ; 

         case 2: // channel mode 
            lcd.setCursor(0,2) ; 
            strcpy(buf,"Select Channel") ; 
            fill(buf) ;
            lcd.print(buf) ; 

            lcd.setCursor(0,3) ; 
            sprintf(buf," %-10lu", channels[chstate]) ;  
            fill(buf) ;
            lcd.print(buf) ; 
            break ; 

         case 3: // config mode 
            lcd.setCursor(0,2) ; 
            strcpy(buf,"Config Mode") ; 
            fill(buf) ;
            lcd.print(buf) ; 

            lcd.setCursor(0,3) ; 

            // convert chanstep for display
            char csbuf[9] ;  
            int  csdig; 
            char csunit;
            if ( vfo.ChanStep >=  999999UL ) { 
                csdig = vfo.ChanStep / 1000000UL ; 
                csunit = 'M' ; 
            } else {  
                csdig = vfo.ChanStep / 1000UL ; 
                csunit = 'k' ; 
            }   
            sprintf(csbuf,"%3d %c", csdig, csunit) ;  


            sprintf(buf,"AP%1d BS%7lu CR%3lu",vfo.pwrlevel,vfo.ChanStep,vfo.reffreq/1000000UL ) ;  
            fill(buf) ;
            lcd.print(buf) ; 
            break ; 

         case 4:  // debug mode
            lcd.setCursor(0,2);
            char pfdstr[8] ; 
            dtostrf(vfo.PFDFreq/1000000.0,7,3,pfdstr) ; 
            sprintf(buf,"F%4d D%2d PF%7s", vfo.Frac,vfo.outdiv,pfdstr) ;  
            fill(buf) ;
            lcd.print(buf) ; 

            lcd.setCursor(0,3);
            sprintf(buf,"M%4lu N%5u", vfo.Mod, vfo.N_Int) ;  
            fill(buf) ;
            lcd.print(buf) ; 
            break ; 

         case 5:  // sweep mode
            lcd.setCursor(0,2);
            sprintf(buf,"Sweep:%-10lu",freq_b) ;  
            fill(buf) ;
            lcd.print(buf) ; 

            lcd.setCursor(0,3);
            strcpy(buf,"Not Imp") ; 
            fill(buf) ;
            lcd.print(buf) ; 
            break ; 

         case 0: 
         default: 
             lcd.setCursor(0,2);
             strcpy(buf," ") ; 
             fill(buf) ;
             lcd.print(buf) ; 

             if ( ! ovenready &&  ( millis() > 600000UL ) ) ovenready = true ; 

             if ( ovenready ) {
                strcpy(buf," ") ; 
             } else {
                sprintf(buf,"Oven Cold %2d min", (int) (millis() / 60000 )  ) ; 
             } 
             fill(buf) ;
             lcd.setCursor(0,3);
             lcd.print(buf) ; 
             break ; 
         }

         if ( ! lcd_backlight_override ) {
             if ( millis() - lcdtimer > 120000 ) {
                 lcd.setBacklight(0);
             } else {
                 lcd.setBacklight(255);
             }
         } else {
                 lcd.setBacklight(255);
         } 

    }


}
