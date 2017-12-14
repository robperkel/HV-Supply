/* 
 * File:   Utility.h
 * Author: Robert Perkel
 *
 * Created on December 13, 2017,
 */

#ifndef UTILITY_H
#define	UTILITY_H

#ifdef	__cplusplus
extern "C" {
#endif
    
    inline int setSPIchannel(int channel)
    {
        switch (channel)
        {
            case 0:
                PORTA = 0x0; break;
            case 1:
                PORTA = 0x40; break;
            case 2:
                PORTA = 0x80; break;
            case 3:
                PORTA = 0xC0; break;
            case 4:
                PORTA = 0x20; break;
            case 5:
                PORTA = 0x60; break;
            case 6:
                PORTA = 0xA0; break;
            case 7:
                PORTA = 0xE0; break;
            default: return -1;
        }
        return 0;
    }
    
    inline void revertSPIchannel()
    {
        PORTA = 0xE0;
    }

#ifdef	__cplusplus
}
#endif

#endif	/* UTILITY_H */

