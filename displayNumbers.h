/* 
 * File:   displayNumbers.h
 * Author: Robert
 *
 * Created on October 8, 2017, 10:04 PM
 */

#ifndef DISPLAYNUMBERS_H
#define	DISPLAYNUMBERS_H

#include "./lookupTable.h"

#ifdef	__cplusplus
extern "C" {
#endif

    static bit cursor = 0b0; //Is the cursor on? On if 1
    
    static unsigned long Vout = 0;
    static unsigned long Iout = 0;
    static unsigned long Vlim = 0;    
    
    static int VOutDigits[4] = {0,0,0,0};
    static int IOutDigits[4] = {0,0,0,0};
    
    static int VLimDigits[4] = {0,0,0,0};
    static int ILimDigits[4] = {0,0,0,0};
    
    static int VStopDigits[4] = {0,0,0,0};
    static int IStopDigits[4] = {0,0,0,0};
    static unsigned long Ilim = 0;
    
    
    
    uint _GetNumberDigit(unsigned long in, unsigned long fact)
    {
        uint extract = 0;
        for (int i = 0; in >= fact; ++i)
        {
            in -= fact;
            extract++;
        }
        return extract;
    }
    
    uint _GetDoubleDigit(double in, double fact)
    {
        uint extract = 0;
        for (int i = 0; in >= fact; ++i)
        {
            in -= fact;
            extract++;
        }
        return extract;
    }
    
    uint writeLargeNumber(uint line, uint linePos, unsigned long number)
    {
        //Cursor Position: 0x20 * line + linePos
        uint addr = 0x20 * line + linePos;
        uint cmd = addr | 0x80;
        
        uchar thousands = _GetNumberDigit(number, 1000);
        number -= thousands * 1000;
        
        uchar hundreds = _GetNumberDigit(number, 100);
        number -= hundreds * 100;
        
        uchar tens = _GetNumberDigit(number, 10);
        number -= tens * 10;
        
        uchar ones = _GetNumberDigit(number, 1);
        number -= ones;
        
        thousands += NumbersBase;
        hundreds += NumbersBase;
        tens += NumbersBase;
        ones += NumbersBase;
        
        OutputBuffer[linePos] = thousands;
        OutputBuffer[linePos + 1] = hundreds;
        OutputBuffer[linePos + 2] = tens;
        OutputBuffer[linePos + 3] = ones;
        
        return (linePos + 4);
    }


#ifdef	__cplusplus
}
#endif

#endif	/* DISPLAYNUMBERS_H */

