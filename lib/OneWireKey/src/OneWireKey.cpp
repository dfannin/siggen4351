#include "OneWireKey.h"

const char keymap[4][4] =
{ {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

// 
// defines the row bit mask pattern
//
#define ROWPAT(R)  (uint8_t) ( ~(1U << R))  

uint8_t row_mask[4] = { ROWPAT(R0), ROWPAT(R1), ROWPAT(R2), ROWPAT(R3)  };


/*
 *  CONSTRUCTORS
 */

OneWireKey::OneWireKey(int i2c_addr)
{
  addr = i2c_addr ;
}

/*
 *  PUBLIC METHODS
 */

void OneWireKey::init()
{
  lastkey = 0 ; 
  timer = 0UL ; 

  presstype = OWK_NONE ; 
  holdtime = HOLDTIME ; 
  longholdtime = LONGHOLDTIME ; 
  countup = 0 ; 

  Wire.begin() ;
  write(0xff); // set all pins high

}


char  OneWireKey::get_key()
{
 
 int r, c ;  
 uint8_t data ; 
 char newkey ; 
 uint8_t newpresstype ; 

  if ( millis() - timer >  KP_INTERVAL ) { 
      ;
  } else {
      return 0 ; 
  }

  newkey = 0 ; 
  newpresstype = OWK_NONE ; 

  //  loop through until a match is found
  for(r=0;r<NUMR;r++)  {

      // set the row mask and then read the row data
      write(row_mask[r]); 
      data = read() ; 
      // check the columns for a match
      for (c=0;c<NUMC;c++) {
          // if bit is zero for a column, then key is pressed
          if ( ( (data >> c ) & 1u )  == 0 ) { 
              newpresstype = OWK_PRESSED ; 
              newkey = keymap[r][c] ;  
              goto found ; // agh!! first time I used a goto in 20 years!
          } 

      }
  }

  found: 
  // reset 
  timer = millis() ; 
  write(0xFF);

  // nothing pressed
  if ( presstype == OWK_NONE  && newpresstype == OWK_NONE) {
      // no key found
      return 0 ; 
  }     


  // key pressed started
  if ( presstype == OWK_NONE && newpresstype == OWK_PRESSED ) {
     presstype = OWK_PRESSED ; 
     lastkey = newkey ; 
     countup = 1 ; 
     return 0 ; 
  }

  // key released 
  if ( presstype == OWK_PRESSED && newpresstype == OWK_NONE ) {
       countup++; 
       presstype = OWK_NONE ; 

       // check the hold time
       int ptype = OWK_PRESSED ; 
       if (countup >= holdtime )  ptype = OWK_HOLD ; 
       if (countup >= longholdtime )  ptype = OWK_LONGHOLD ; 
       countup = 0 ; 
       lastpresstype = ptype ; 

       // callback
       if ( eventlistener != NULL) {
           eventlistener(lastkey,ptype) ; 
       }
       return lastkey ; 
  }


  // count while button held
  if ( presstype == OWK_PRESSED && newpresstype == OWK_PRESSED ) {
     countup++ ; 
     return 0 ; 
  }


  return 0 ; 
}


void OneWireKey::write(uint8_t value)
{
  Wire.beginTransmission(addr);
  Wire.write(value);
  Wire.endTransmission();
}

uint8_t OneWireKey::read()
{
  Wire.requestFrom(addr, 1);
  return Wire.read();
}

void OneWireKey::setholdtime(uint8_t hold)
{
    holdtime = hold ; 
}

void OneWireKey::setlongholdtime(uint8_t longhold)
{
    longholdtime = longhold ; 
}

void OneWireKey::addEventListener( void (*listener) (char,uint8_t) ) 
{
    eventlistener = listener ; 
}
