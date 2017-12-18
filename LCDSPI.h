/*
 * Robert Perkel, 2017
 * lookup_table.h
 */

#ifndef LCDSPI_H
#define	LCDSPI_H

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h> // include processor files - each processor file is guarded.  
#include <pic18f25k40.h>

#include "./lookupTable.h"
#include "./Utility.h"

//Found Syntax Help from pic18f25k40.h

const unsigned int chars_per_line = 20;
const unsigned int c_rows = 4;

typedef unsigned char uchar;
typedef unsigned int uint;

static unsigned int state;

typedef enum Status{
    NOT_READY = 0, COMMAND = 1, DATA_WRITE = 2
};

void InitDisplay(); //Inits the Display
inline void SayHelloWrite(); //Write the Start Byte for Writing Data to RAM
inline void SayHelloCommand(); //Write the Start Byte for Commanding the Processor
inline void CloseLCD();

inline void _WriteBit();
void WriteLine(); //Writes the line of text stored in [buffer] to the LCD. Must be PRE-CONFIGURED!
inline void Write(uchar data); //Write a command or data to the processor

void AdvanceLine();
void SetLine(uint line);

static uint MemAddr; //The Last Line Mem Address.
static uchar OutputBuffer[20]; //Static Buffer of Text to be Rendered
static uint mask = 0x80;
static bit out = 0b0;

static char conv_array[6];

#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */
    
    void InitDisplay()
    {
        state = NOT_READY;
        MemAddr = 0x0;
        uchar on = 0x0C; //Turn on the display. Base Command = 0b00001, with parms Display On (1), Cursor On (0), Cursor Blink (0)
        uchar setup_re = 0x2A; //Command: Function Set. Base: 0x2, with parms number of line-display (1), Double Height Font (0), RE-set (1), IS-set (0))
        uchar setup_font = 0x09; //Command: Extended Function Set, RE = 1. Base: 0b00001, with parms font-width (0), cursor invert (0), 2 or 4 line mode. (1)
        uchar rev = 0x06; //Command: Entry Mode Set. Fix Reversed Characters, RE = 1. Base: 0b000001, BDC (1), BGC (0))
        uchar disable_re = 0b00101000; //Command: Function Set.
        uchar goHome = 0x02; //Move the cursor to 0h0. Doesn't affect RAM
        
        
        //CloseLCD();
        SayHelloWrite(); //IDK why this works...
        SayHelloCommand(); //Open Command Mode
        
      //  Write(0x0); //Turn off the display
        //Clear any RAM
        Write(CMD_CLEAR);
        Write(CMD_CURSOR_OFF);
        //Move the cursor to the top.
        Write(goHome);
        //Turn on the Display
        Write(on);
        //Enable RE for Font and Line Setup
        Write(setup_re);
        //Set BDC, BGC
        Write(rev);
        //Setup Font and Line
        //0b 0000 1011
        Write(setup_font);
        //Disable RE
        //0b 0010 1000
        Write(disable_re);

        CloseLCD();
    }
    
    inline void SayHelloWrite()
    {
        //Chip Select LOW (60ns Setup, 20ns HOLD)
        PORTBbits.RB2 = 0;
        PORTBbits.RB0 = 0;
        //Assert CS
        setSPIchannel(6);
        state = DATA_WRITE;
        //0b11111010;
        out = 0b1;
        _WriteBit();
        _WriteBit();
        _WriteBit();
        _WriteBit();
        _WriteBit();
        out = 0b0;
        _WriteBit();
        out = 0b1;
        _WriteBit();
        out = 0b0;
        _WriteBit();
    }
    
    inline void SayHelloCommand()
    {
        PORTBbits.RB2 = 0;
        PORTBbits.RB0 = 0;
        //Chip Select LOW (60ns Setup, 20ns HOLD)
        setSPIchannel(6);
        state = COMMAND;
        //0b11111000
        out = 0b1;
        _WriteBit();
        _WriteBit();
        _WriteBit();
        _WriteBit();
        _WriteBit();
        out = 0b0;
        _WriteBit();
        _WriteBit();
        _WriteBit();
    }
    
    inline void CloseLCD()
    {
        //Deassert CS
        revertSPIchannel();
        PORTBbits.RB0 = 0;
        PORTBbits.RB2 = 0;
        state = NOT_READY;
    }
    
    inline void _WriteBit()
    {   //Serial Setup time = 200nS; Serial Hold time = TBA (assume 200ns);
        PORTBbits.RB2 = out;
        PORTBbits.RB0 = 1; //Clock On
        _delay(6); //Note: Originally 8
        PORTBbits.RB0 = 0; //Clock Off
        _delay(6);
    }
    
    void WriteLine()
    {
        for (uint i = 0; i < 20; i++)
        {
            Write(OutputBuffer[i]);
        }
        //CloseLCD();
        //AdvanceLine();
    }
    
    inline void Write(uchar cmd)
    {
        //Write output
        for (uint i = 0; i < 4; ++i)
        {
            out = (cmd & 0b1 > 0 ? 1 : 0);
            _WriteBit();
            cmd = cmd >> 1;
        }
        out = 0b0;
        _WriteBit();
        _WriteBit();
        _WriteBit();
        _WriteBit();
        for (uint i = 0; i < 4; ++i)
        {
           out = (cmd & 0b1 > 0 ? 1 : 0);
           _WriteBit();
           cmd = cmd >> 1;
        }
        out = 0b0;
        _WriteBit();
        _WriteBit();
        _WriteBit();
        _WriteBit();
        return;
    }
    
    /*void AdvanceLine()
    {
        if (MemAddr == 0x80)
        {
            MemAddr = 0x0;
        }
        else
        {
            MemAddr += 0x20;
        }
        uchar command = 0x80;
        command |= MemAddr;
        SayHelloCommand();
        Write(command);
        CloseLCD();
    }*/
    
    /*void SetLine(uint line)
    {
        uchar command = 0x80;
        uint pos = line * 0x20;
        command |= pos;
        SayHelloCommand();
        Write(command);
        CloseLCD();
    }*/
    
    void ClearBuffer()
    {
        for (uint i = 0; i < 20; ++i)
        {
            OutputBuffer[i] = BLANK;
        }
    }

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif	/* XC_HEADER_TEMPLATE_H */

