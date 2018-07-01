#ifndef ADF4351_H
#define ADF4351_H
#include <Arduino.h>
#include <SPI.h>
#include <stdint.h>
#include <BigNumber.h>


extern uint32_t steps[];  
extern uint32_t channels[]; 

// update adf4351.cpp 
#define NSTEPS 7
#define NCHANS 11

// need to use max unsigned long on arduino , oh well
#define ADF_FREQ_MAX  4294967295UL 
#define ADF_FREQ_MIN  34385000UL
#define ADF_PFD_MAX   32000000.0
#define ADF_PFD_MIN   125000.0
#define ADF_REFIN_MAX   250000000UL
#define REF_FREQ_DEFAULT 100000000UL


#define PIN_CE   2
#define PIN_LD   8
#define PIN_SS   9
#define PIN_MOSI  11
#define PIN_MISO  12
#define PIN_SCK  13


class Reg
{
    public:
        Reg();
        uint32_t get()  ;  // get the 32 bit register value = getbf(0,32) 
        void set(uint32_t value);  // set the 32 bit register value  = setbf(0,32,value) 
        uint32_t whole ;  
        // set the register bitfield
        // start = starting bit number to set, index 0 
        // len = number of bits to set
        // value = value to set (note the value is truncated if larger than len bits) 
        void setbf(uint8_t start, uint8_t len , uint32_t value) ; 
        // returns the register bit field
        // start = starting bit number to read, index 0 
        // len = number of bits to get
        uint32_t getbf(uint8_t start, uint8_t len) ; 
        
};


class ADF4351 {
    public:
        ADF4351(byte pin, uint8_t mode, unsigned long  speed, uint8_t order ) ;
        void init() ;
        int setf(uint32_t freq) ; // set freq 
        int setrf(uint32_t f) ;  // set reference freq
        void enable(); // enable output (CD pin) 
        void disable();  // disable output (CE pin) 
        void writeDev(int n, Reg r) ; 
        uint32_t getReg(int n) ; 
        uint32_t gcd_iter(uint32_t u, uint32_t v) ; 
        SPISettings spi_settings; 
        uint8_t pinSS ; 
        Reg R[6] ; 
        uint32_t reffreq; 
        byte enabled ; 
        uint32_t cfreq ; 
        uint16_t N_Int ; 
        int Frac ; 
        uint32_t Mod ; 
        float PFDFreq ; 
        uint32_t ChanStep; 
        int outdiv ; 
        uint8_t RD2refdouble ;
        int RCounter ;
        uint8_t RD1Rdiv2 ;
        uint8_t BandSelClock ;
        int ClkDiv ;
        uint8_t Prescaler ;
        byte pwrlevel ; 

};


#endif
