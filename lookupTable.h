/* 
 * File:   lookupTable.h
 * Author: Robert Perkel
 *
 * Created on October 8, 2017
 */

#ifndef LOOKUPTABLE_H
#define	LOOKUPTABLE_H

#ifdef	__cplusplus
extern "C" {
#endif

//Designed for ROM Set C
#define BLANK 0x20

#define UP 0x05
#define DOWN 0x06
#define RIGHT 0x07
#define LEFT 0x08

#define OHMS 0x1E

#define LEFT_PAR 0x28
#define RIGHT_PAR 0x29

#define COLON 0x3A
#define COMMA 0x2C

#define PERIOD 0x2E

#define PLUS 0x2B
#define MINUS 0x2D

#define ULB 0x41 //Uppercase Letters Base / Offset
#define LLB 0x61 //Lowercase Letters Base / Offset
#define NumbersBase 0x30 //Base offset for Numbers
    
#define CMD_CLEAR 0x01
#define CMD_GOHOME 0x02
#define CMD_CURSOR_ON 0x0D
#define CMD_CURSOR_OFF 0x0C

#ifdef	__cplusplus
}
#endif

#endif	/* LOOKUPTABLE_H */

