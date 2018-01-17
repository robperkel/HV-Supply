#include "./LCDSPI.h"
#include "./lookupTable.h"
#include "./displayNumbers.h"

// CONFIG1L
#pragma config FEXTOSC = OFF    // External Oscillator mode Selection bits (Oscillator not enabled)
#pragma config RSTOSC = HFINTOSC_1MHZ// Power-up default value for COSC bits (HFINTOSC with HFFRQ = 4 MHz and CDIV = 4:1)

// CONFIG1H
#pragma config CLKOUTEN = OFF    // Clock Out Enable bit (CLKOUT function is enabled)
#pragma config CSWEN = ON       // Clock Switch Enable bit (Writing to NOSC and NDIV is allowed)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor enabled)

// CONFIG2L
#pragma config MCLRE = EXTMCLR  // Master Clear Enable bit (If LVP = 0, MCLR pin is MCLR; If LVP = 1, RE3 pin function is MCLR )
#pragma config PWRTE = ON       // Power-up Timer Enable bit (Power up timer enabled)
#pragma config LPBOREN = ON     // Low-power BOR enable bit (ULPBOR enabled)
#pragma config BOREN = SBORDIS  // Brown-out Reset Enable bits (Brown-out Reset enabled , SBOREN bit is ignored)

// CONFIG2H
#pragma config BORV = VBOR_190  // Brown Out Reset Voltage selection bits (Brown-out Reset Voltage (VBOR) set to 2.85V)
#pragma config ZCD = OFF        // ZCD Disable bit (ZCD disabled. ZCD can be enabled by setting the ZCDSEN bit of ZCDCON)
#pragma config PPS1WAY = ON     // PPSLOCK bit One-Way Set Enable bit (PPSLOCK bit can be cleared and set only once; PPS registers remain locked after one clear/set cycle)
#pragma config STVREN = ON      // Stack Full/Underflow Reset Enable bit (Stack full/underflow will cause Reset)
#pragma config DEBUG = OFF      // Debugger Enable bit (Background debugger disabled)
#pragma config XINST = OFF      // Extended Instruction Set Enable bit (Extended Instruction Set and Indexed Addressing Mode disabled)

// CONFIG3L
#pragma config WDTCPS = WDTCPS_31// WDT Period Select bits (Divider ratio 1:65536; software control of WDTPS)
#pragma config WDTE = OFF    // WDT Disabled

// CONFIG3H
#pragma config WDTCWS = WDTCWS_7// WDT Window Select bits (window always open (100%); software control; keyed access not required)
#pragma config WDTCCS = SC      // WDT input clock selector (Software Control)

// CONFIG4L
#pragma config WRT0 = OFF       // Write Protection Block 0 (Block 0 (000800-001FFFh) not write-protected)
#pragma config WRT1 = OFF       // Write Protection Block 1 (Block 1 (002000-003FFFh) not write-protected)
#pragma config WRT2 = OFF       // Write Protection Block 2 (Block 2 (004000-005FFFh) not write-protected)
#pragma config WRT3 = OFF       // Write Protection Block 3 (Block 3 (006000-007FFFh) not write-protected)

// CONFIG4H
#pragma config WRTC = OFF       // Configuration Register Write Protection bit (Configuration registers (300000-30000Bh) not write-protected)
#pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot Block (000000-0007FFh) not write-protected)
#pragma config WRTD = OFF       // Data EEPROM Write Protection bit (Data EEPROM not write-protected)
#pragma config SCANE = ON       // Scanner Enable bit (Scanner module is available for use, SCANMD bit can control the module)
#pragma config LVP = ON         // Low Voltage Programming Enable bit (Low voltage programming enabled. MCLR/VPP pin function is MCLR. MCLRE configuration bit is ignored)

// CONFIG5L
#pragma config CP = OFF         // UserNVM Program Memory Code Protection bit (UserNVM code protection disabled)
#pragma config CPD = OFF        // DataNVM Memory Code Protection bit (DataNVM code protection disabled)

// CONFIG5H

// CONFIG6L
#pragma config EBTR0 = OFF      // Table Read Protection Block 0 (Block 0 (000800-001FFFh) not protected from table reads executed in other blocks)
#pragma config EBTR1 = OFF      // Table Read Protection Block 1 (Block 1 (002000-003FFFh) not protected from table reads executed in other blocks)
#pragma config EBTR2 = OFF      // Table Read Protection Block 2 (Block 2 (004000-005FFFh) not protected from table reads executed in other blocks)
#pragma config EBTR3 = OFF      // Table Read Protection Block 3 (Block 3 (006000-007FFFh) not protected from table reads executed in other blocks)

// CONFIG6H
#pragma config EBTRB = OFF      // Boot Block Table Read Protection bit (Boot Block (000000-0007FFh) not protected from table reads executed in other blocks)

typedef enum {
    OUTPUT = -1, ABOUT, HOME, EDIT_SET, EDIT_LIM, LIM_HOME, PARM_HOME, MODE
} Screen;

typedef enum {
    VOLTAGE_SOURCE = 0, CURRENT_SOURCE, BREAKDOWN_TEST
} SystemMode;

typedef enum{
    VOLTAGE = 0, CURRENT = 1
} Modification;

typedef enum{
    NO_PRESS = -1, BUTTON_A = 0, BUTTON_B, BUTTON_C, BUTTON_D, 
            BUTTON_LEFT, BUTTON_UP, BUTTON_DOWN, BUTTON_RIGHT,
            EXIT, HV_ENABLE
} Button;

void WriteAndClose();

void HomeScreenUI(); //Main Loop

void OutputModeUI(); //Draw the Output
void OutputMode(); //Handles the Output Systems

void DrawParmUI(); //Draws the UI

void ParmMode(); //Parm Edit Mode Loop
void RunAbout(); //Info / Welcome Screen
void ModeSelect(); //Select a Mode
void DrawLimitUI();

void DiagAndConfig();
void DiagMenu();

Button button = NO_PRESS;
Screen screen = HOME;
Modification sysDir = VOLTAGE;
SystemMode sysMode = VOLTAGE_SOURCE;


void GotoSleep()
{ //Note: Actually "Idles" the CPU. The [WDT] will still be active.
    button = NO_PRESS;
    do
    {
        asm("SLEEP");
    } while(button == NO_PRESS);
}

void LoadSettingsFromMemory();
void WriteSettingsToMemory();

void RestoreCursor(int digit);
void interrupt low_priority ButtonHit();


static int Version;
unsigned long Vcurr;
unsigned long Icurr;

unsigned long Vstop;
unsigned long Istop;

void main(void) {    
    //Fosc = 4MHz, 1MHz Output, 1uS Cycle   
    
    TRISA = 0b00011110; //SELECT B,A,C DRDY[4-1], WDT_OUT
    PORTA = 0xE0; //SPI 7, x4 X, WDT = 0
        
    PORTB = 0xFF;
    TRISB = 0x0;
    TRISC = 0xFC; //0b1111 0b1100
    ANSELC = 0x03; //0b0000 0b0011
    PORTC = 0x0;
    
    T0CON0 = 0b10000010; //Turn on the timer, 8-bit mode, 1:3 divider
    T0CON1 = 0b01010000; //Use Fosc/4, Async, Scale 1:1
    
    T2CON = 0b00001111; //Timer2 Off, 1:1 Prescaler, 1:16 Divider
    T2HLT = 0b10101000; //Sync to Oscillator, Rising Edge Trigger, Sync to ON, One-Shop Software Mode
    T2CLKCON = 0b00000101; //Runs on the 31kHz Oscillator
    /*long wait = 0;
    do
    {
        _delay(10);
        asm("CLRWDT");
        wait++;
    } while (wait != 30000);*/
    
    int konami = 0; //Konami Code Counter
    
    CPUDOZEbits.IDLEN = 1; //Allow the CPU to enter into IDLE for SLEEP instructions (not Sleep)
    
    INTCONbits.GIEH = 0b1; //Enable Interrupts (HIGH)
    INTCONbits.GIEL = 0b1; //Enable Interrupts (LOW)
    INTCONbits.IPEN = 0b1; //Priority for Interrupts
    INTCONbits.INT0EDG = 0b1; //Set INT0 to Rising Edge
    
    PIE0bits.IOCIE = 0b1; //Enable interrupt-on-change
    PIE0bits.INT0IE = 0b1; //Enable INT0 Interrupt
    PIE0bits.TMR0IE = 0b1; //Enable Timer0 Interrupts
    
    PIE4bits.TMR2IE = 0b1; //Enable Timer2 Interrupts
    
    IPR0bits.IOCIP = 0b0; //Set IOC to Low Priority
    IPR0bits.INT0IP = 0b0; //Set INT0 to Low Priority
    IPR0bits.TMR0IP = 0b1; //Set TMR0 to High Priority
    
    IPR4bits.TMR2IP = 0b0; //Low Priority Timer2 Interrupt
    
    IOCCN = 0x0;
    IOCCP = 0xFE; //Enable RC4-RC7 rising edge interrupts
    
    //Move RB0 PPS to RB5 (DRDY))
    INT0PPSbits.INT0PPS2 = 0b1;
    INT0PPSbits.INT0PPS1 = 0b0;
    INT0PPSbits.INT0PPS0 = 0b1;
    
    Vcurr = 0;
    Icurr = 0;
    
    Version = 3; //Version of the Code (Used for Display)
    
    Vstop = 2500; //2.5kV
    Istop = 2000; //20mA
        
    int inCode = 0;
    int digit = 0;
    
    int* digit_ptr = 0;
    
    for (uint i = 0; i < 20; i++)
    {
        OutputBuffer[i] = BLANK;
    }
    
    double baseVal = 0.0;
    
    InitDisplay();
    
    screen = HOME;
    sysMode = VOLTAGE_SOURCE;
    RunAbout();
    
    int temp = 0;
    //Decode Loaded Memory Values
    
    digit_ptr = 0x0;
    
    HomeScreenUI();
    while (1 == 1)
    {
        if (inCode == 0)
        {
            konami = 0;
        }
        inCode = 0;
        if (konami == 10)
        {
            //TODO: Easter Egg!
            konami = 0;
        }
        GotoSleep();
        if ((screen == HOME) || (screen == LIM_HOME))
        {
            if ((button == BUTTON_A) && (konami != 9) && (((screen == HOME) && (sysMode != CURRENT_SOURCE)) || ((screen == LIM_HOME) && (sysMode == CURRENT_SOURCE))))
            {
                digit = 3;
                if (screen == HOME)
                {
                    digit_ptr = &VOutDigits[3];
                    screen = EDIT_SET;
                }
                else
                {
                    digit_ptr = &VLimDigits[3];
                    screen = EDIT_LIM;
                }
                sysDir = VOLTAGE;
                SayHelloCommand();
                WriteLCD(CMD_CURSOR_ON);
                CloseLCD();
                RestoreCursor(digit);
            }
            else if ((button == BUTTON_B) && (konami != 8) && (((screen == HOME) && (sysMode == CURRENT_SOURCE)) || ((screen == LIM_HOME) && (sysMode != CURRENT_SOURCE))))
            {
                digit = 3;
                if (screen == HOME)
                {
                    digit_ptr = &IOutDigits[3];
                    screen = EDIT_SET;
                } 
                else
                {
                    digit_ptr = &ILimDigits[3];
                    screen = EDIT_LIM;
                }
                sysDir = CURRENT;
                SayHelloCommand();
                WriteLCD(CMD_CURSOR_ON);
                CloseLCD();
                RestoreCursor(digit);
            }
            else if (button == BUTTON_C)
            {
                screen = PARM_HOME;
                ParmMode();
                if (screen == OUTPUT)
                {
                    OutputMode();
                }
            }
            else if (button == BUTTON_D)
            {
                continue;
            }
            else if (button == EXIT)
            {
                //Diagnostic Menu?
                
                //TEMP: Save Settings to EEPROM
                //WriteSettingsToMemory();
                DiagAndConfig();
                if (screen == OUTPUT)
                {
                    OutputMode();
                }
            }
            else if (button == HV_ENABLE)
            {
                screen = OUTPUT;
                OutputMode();
            }
            else
            {
                inCode = 1;
                switch (konami)
                {
                    case 0:
                    case 1:
                        if (button == BUTTON_UP) konami++;
                        else konami = 0;
                        break;
                    case 2:
                    case 3:
                        if (button == BUTTON_DOWN) konami++;
                        else konami = 0;
                        break;
                    case 4:
                    case 6:
                        if (button == BUTTON_LEFT) konami++;
                        else konami = 0;
                        break;
                    case 5:
                    case 7:
                        if (button == BUTTON_RIGHT) konami++;
                        else konami = 0;
                        break;
                    case 8:
                        if (button == BUTTON_B) konami++;
                        else konami = 0;
                        break;
                    case 9:
                        if (button == BUTTON_A) konami++;
                        else konami = 0;
                        break;
                }
                continue;
            }
        }
        else if ((screen == EDIT_SET) || (screen == EDIT_LIM))
        {
            if (button == EXIT)
            {
                SayHelloCommand();
                WriteLCD(CMD_CURSOR_OFF);
                CloseLCD();
                if (screen == EDIT_SET)
                {
                    screen = HOME;
                }
                else
                {
                    screen = LIM_HOME;
                }
            }
            else if (button == HV_ENABLE)
            {
                SayHelloCommand();
                WriteLCD(CMD_CURSOR_OFF);
                CloseLCD();
                screen = OUTPUT;
                OutputMode();
            }
            switch (button)
            {
                case BUTTON_LEFT:
                    if (digit != 3)
                    {
                        digit_ptr++;
                        digit++;
                        RestoreCursor(digit);
                    }
                    continue;
                    break;
                case BUTTON_A:
                {
                    int target = 3;
                    int dif = target - digit;
                    digit += dif;
                    digit_ptr += dif;
                    RestoreCursor(digit);
                    continue;
                    break;
                }
                case BUTTON_B:
                {
                    int target = 2;
                    int dif = target - digit;
                    digit += dif;
                    digit_ptr += dif;
                    RestoreCursor(digit);
                    continue;
                    break;
                }
                case BUTTON_C:
                {
                    int target = 1;
                    int dif = target - digit;
                    digit += dif;
                    digit_ptr += dif;
                    RestoreCursor(digit);
                    continue;
                    break;
                }
                case BUTTON_D:
                {
                    int target = 0;
                    int dif = target - digit;
                    digit += dif;
                    digit_ptr += dif;
                    RestoreCursor(digit);
                    continue;
                    break;
                }
                case BUTTON_RIGHT:
                    if (digit != 0)
                    {
                        digit_ptr--;
                        digit--;
                        RestoreCursor(digit);
                    }
                    continue;
                    break;
                case BUTTON_UP:
                    if (*digit_ptr != 9)
                        (*digit_ptr)++;
                    else
                        *digit_ptr = 0;
                    break;
                case BUTTON_DOWN:
                    if (*digit_ptr != 0)
                        (*digit_ptr)--;
                    else
                        *digit_ptr = 9;
                    break;
                case EXIT:
                    break;
            }
        }
        if (sysDir == VOLTAGE)
        {
            if (screen == EDIT_SET)
            {
                
                Vout = VOutDigits[3] * 1000 + VOutDigits[2] * 100 + VOutDigits[1] * 10 + VOutDigits[0];
                if (Vout > Vstop)
                {
                    VOutDigits[3] = VStopDigits[3];
                    VOutDigits[2] = VStopDigits[2];
                    VOutDigits[1] = VStopDigits[1];
                    VOutDigits[0] = VStopDigits[0];
                    Vout = Vstop;
                }
            }
            else if (screen == EDIT_LIM)
            {
                Vlim = VLimDigits[3] * 1000 + VLimDigits[2] * 100 + VLimDigits[1] * 10 + VLimDigits[0];
                if (Vlim > Vstop)
                {
                    VLimDigits[3] = VStopDigits[3];
                    VLimDigits[2] = VStopDigits[2];
                    VLimDigits[1] = VStopDigits[1];
                    VLimDigits[0] = VStopDigits[0];
                    Vlim = Vstop;
                }
            }
        }
        else if (sysDir == CURRENT)
        {
            if (screen == EDIT_SET)
            {
                Iout = IOutDigits[3] * 1000 + IOutDigits[2] * 100 + IOutDigits[1] * 10 + IOutDigits[0];
                if (Iout > Istop)
                {
                    IOutDigits[3] = IStopDigits[3];
                    IOutDigits[2] = IStopDigits[2];
                    IOutDigits[1] = IStopDigits[1];
                    IOutDigits[0] = IStopDigits[0];
                    Iout = Istop;
                }
            }
            else if (screen == EDIT_LIM)
            {
                Ilim = ILimDigits[3] * 1000 + ILimDigits[2] * 100 + ILimDigits[1] * 10 + ILimDigits[0];
                if (Ilim > Istop)
                {
                    ILimDigits[3] = IStopDigits[3];
                    ILimDigits[2] = IStopDigits[2];
                    ILimDigits[1] = IStopDigits[1];
                    ILimDigits[0] = IStopDigits[0];
                    Ilim = Istop;
                }
            }
        }
        HomeScreenUI();
        if ((screen == EDIT_SET) || (screen == EDIT_LIM))
        {
            RestoreCursor(digit);
        }
        //ButtonHit();
    }
    return;
}

void RestoreCursor(int digit)
{
    SayHelloCommand();
    if ((sysDir == VOLTAGE))
    {
        WriteLCD(0x80 | (0x29 + (3 - digit)));
    }
    else if ((sysDir == CURRENT))
    {
        WriteLCD(0x80 | (0x49 + (3 - digit)));
    }
    CloseLCD();
}

void interrupt low_priority ButtonHit()
{
    button = NO_PRESS;
    if (PORTCbits.RC7 == 1) //Key Press Detect
    {
        button = 7;
        int value = 0;
        if (PORTCbits.RC6)
        {
            value = 1;
        }
        value = value << 1;
        if (PORTCbits.RC5)
        {
            value += 1;
        }
        value = value << 1;
        if (PORTCbits.RC4)
        {
            value += 1;
        }
        button = (Button) value;
    }
    else if (PORTCbits.RC3)
    {
        //EXIT.
        button = EXIT;
    }
    else if (IOCCFbits.IOCCF2)
    {
        //HV Enable
        button = HV_ENABLE;
    }

    IOCCF = 0x0;
    IOCCN = IOCCP; //Enable Falling Edge Detect
    IOCCP = 0x0; //Disable Rising Edge
    do
    {
        IOCCF = 0x0;
        _delay(50);
        asm("CLRWDT");
    } while ((IOCCF != 0x0) || (PORTC != 0x0));
    IOCCF = 0x0;
    IOCCP = IOCCN; //Return settings
    IOCCN = 0x0;
}

void RunAbout()
{
    SayHelloCommand();
    WriteLCD(CMD_CLEAR);
    WriteLCD(CMD_GOHOME);
    WriteLCD(CMD_CURSOR_OFF);
    CloseLCD();
    
    for (int lines = 0; lines < 4; ++lines)
    {
        ClearBuffer();
        if (lines == 0)
        {
            //HV Power Supply
            OutputBuffer[0] = 0x48;
            OutputBuffer[1] = 0x56;
            //OutputBuffer[2] = BLANK;
            OutputBuffer[3] = 0x50;
            OutputBuffer[4] = 0x6F;
            OutputBuffer[5] = 0x77;
            OutputBuffer[6] = 0x65;
            OutputBuffer[7] = 0x72;
            //OutputBuffer[8] = BLANK;
            OutputBuffer[9] = 0x53;
            OutputBuffer[10] = 0x75;
            OutputBuffer[11] = 0x70;
            OutputBuffer[12] = 0x70;
            OutputBuffer[13] = 0x6C;
            OutputBuffer[14] = 0x79;
            //OutputBuffer[9] = BLANK;
            OutputBuffer[16] = 0x76;
            OutputBuffer[17] = NumbersBase + 1;
            OutputBuffer[18] = PERIOD;
            OutputBuffer[19] = NumbersBase + Version;
        }
        else if (lines == 1)
        {
            //Robert Perkel
            OutputBuffer[0] = 0x52;
            OutputBuffer[1] = 0x6F;
            OutputBuffer[2] = 0x62;
            OutputBuffer[3] = 0x65;
            OutputBuffer[4] = 0x72;
            OutputBuffer[5] = 0x74;
            //(Blank)
            OutputBuffer[7] = 0x50;
            OutputBuffer[8] = 0x65;
            OutputBuffer[9] = 0x72;
            OutputBuffer[10] = 0x6B;
            OutputBuffer[11] = 0x65;
            OutputBuffer[12] = 0x6C;
        }
        else if (lines == 2)
        {
            //Bob Lineberry
            OutputBuffer[0] = 0x42;
            OutputBuffer[1] = 0x6F;
            OutputBuffer[2] = 0x62;
            //(Blank)
            OutputBuffer[4] = 0x4C;
            OutputBuffer[5] = 0x69;
            OutputBuffer[6] = 0x6E;
            OutputBuffer[7] = 0x65;
            OutputBuffer[8] = 0x62;
            OutputBuffer[9] = 0x65;
            OutputBuffer[10] = 0x72;
            OutputBuffer[11] = 0x72;
            OutputBuffer[12] = 0x79;
        }
        else if (lines == 3)
        {
            //Rich Dumene
            OutputBuffer[0] = 0x52;
            OutputBuffer[1] = 0x69;
            OutputBuffer[2] = 0x63;
            OutputBuffer[3] = 0x68;
            //(Blank)
            OutputBuffer[5] = 0x44;
            OutputBuffer[6] = 0x75;
            OutputBuffer[7] = 0x6D;
            OutputBuffer[8] = 0x65;
            OutputBuffer[9] = 0x6E;
            OutputBuffer[10] = 0x65;

        }
        WriteAndClose();
    }
    
    if (screen != HOME)
    {
        GotoSleep();
        button = NO_PRESS;
        return;
    }
    
    LoadSettingsFromMemory();
    
    //31kHz Clock Source. Divider: 1:16
    T2PR = 0xFF;
    
    //~7.598 ms  per Overflow
    //759.8ms delay
    
    //TODO: Why is the interrupt being cleared (or not existing?)
    for (int i = 0; i < 100; ++i)
    {
        T2TMR = 0x00; //Clear the Timer
        T2CONbits.ON = 0b0; //Turn it off, then...
        T2CONbits.ON = 0b1; //Turn Timer 2 On
        PIR4bits.TMR2IF = 0b0; //Clear the Interrupt
        do{
            asm("SLEEP");
        } while((PIR4bits.TMR2IF == 0b0) && (T2CONbits.ON == 0b0));
    }
   
    
    IOCCF = 0x0;
    SayHelloCommand();
    WriteLCD(CMD_CLEAR);
    WriteLCD(CMD_GOHOME);
    WriteLCD(CMD_CURSOR_OFF);
    CloseLCD();
    
    for (int lines = 0; lines < 4; ++lines)
    {
        ClearBuffer();
        if (lines == 0)
        {
            //HV Power Supply
            OutputBuffer[0] = 0x48;
            OutputBuffer[1] = 0x56;
            //OutputBuffer[2] = BLANK;
            OutputBuffer[3] = 0x50;
            OutputBuffer[4] = 0x6F;
            OutputBuffer[5] = 0x77;
            OutputBuffer[6] = 0x65;
            OutputBuffer[7] = 0x72;
            //OutputBuffer[8] = BLANK;
            OutputBuffer[9] = 0x53;
            OutputBuffer[10] = 0x75;
            OutputBuffer[11] = 0x70;
            OutputBuffer[12] = 0x70;
            OutputBuffer[13] = 0x6C;
            OutputBuffer[14] = 0x79;
            //OutputBuffer[9] = BLANK;
            OutputBuffer[16] = 0x76;
            OutputBuffer[17] = NumbersBase + 1;
            OutputBuffer[18] = PERIOD;
            OutputBuffer[19] = NumbersBase + Version;
        }
        else if (lines == 1)
        {
            //Todo: Add self-check
            
           
            if (1 == 1)
            {
                //No errors found.
                OutputBuffer[0] = 0x4E;
                OutputBuffer[1] = 0x6F;
                //(Blank)
                OutputBuffer[3] = 0x65;
                OutputBuffer[4] = 0x72;
                OutputBuffer[5] = 0x72;
                OutputBuffer[6] = 0x6F;
                OutputBuffer[7] = 0x72;
                OutputBuffer[8] = 0x73;
                //(Blank)
                OutputBuffer[10] = 0x66;
                OutputBuffer[11] = 0x6F;
                OutputBuffer[12] = 0x75;
                OutputBuffer[13] = 0x6E;
                OutputBuffer[14] = 0x64;
                OutputBuffer[15] = PERIOD;
            }
        }
        else if (lines == 3)
        {
           //Press any key 
            OutputBuffer[0] = 0x50;
            OutputBuffer[1] = 0x72;
            OutputBuffer[2] = 0x65;
            OutputBuffer[3] = 0x73;
            OutputBuffer[4] = 0x73;
            //(Blank)
            OutputBuffer[6] = 0x61;
            OutputBuffer[7] = 0x6E;
            OutputBuffer[8] = 0x79;
            //(Blank)
            OutputBuffer[10] = 0x6B;
            OutputBuffer[11] = 0x65;
            OutputBuffer[12] = 0x79;
        }
        WriteAndClose();
    }
    
    GotoSleep();
    button = NO_PRESS;
}

void ModeSelect()
{
    uchar letter = 0x41;
    for (int lines = 0; lines < 4; ++lines)
    {
        ClearBuffer();
        if (lines == 0)
        {
            //Select a mode
            OutputBuffer[0] = 0x53;
            OutputBuffer[1] = 0x65;
            OutputBuffer[2] = 0x6C;
            OutputBuffer[3] = 0x65;
            OutputBuffer[4] = 0x63;
            OutputBuffer[5] = 0x74;
            //(Blank)
            OutputBuffer[7] = 0x61;
            //(Blank)
            OutputBuffer[9] = 0x6D;
            OutputBuffer[10] = 0x6F;
            OutputBuffer[11] = 0x64;
            OutputBuffer[12] = 0x65;
        }
        else
        {
            //(X) 
            OutputBuffer[0] = LEFT_PAR;
            OutputBuffer[1] = letter;
            OutputBuffer[2] = RIGHT_PAR;
            //(Blank)
            if (lines == 1)
            {
                //Voltage
                OutputBuffer[4] = 0x56;
                OutputBuffer[5] = 0x6F;
                OutputBuffer[6] = 0x6C;
                OutputBuffer[7] = 0x74;
                OutputBuffer[8] = 0x61;
                OutputBuffer[9] = 0x67;
                OutputBuffer[10] = 0x65;
            }
            else if (lines == 2)
            {
                //Current Source
                OutputBuffer[4] = 0x43;
                OutputBuffer[5] = 0x75;
                OutputBuffer[6] = 0x72;
                OutputBuffer[7] = 0x72;
                OutputBuffer[8] = 0x65;
                OutputBuffer[9] = 0x6E;
                OutputBuffer[10] = 0x74;
            }
            else if (lines == 3)
            {
                //Breakdown test
                OutputBuffer[4] = 0x42;
                OutputBuffer[5] = 0x72;
                OutputBuffer[6] = 0x65;
                OutputBuffer[7] = 0x61;
                OutputBuffer[8] = 0x6B;
                OutputBuffer[9] = 0x64;
                OutputBuffer[10] = 0x6F;
                OutputBuffer[11] = 0x77;
                OutputBuffer[12] = 0x6E;
                //(Blank)
                OutputBuffer[14] = 0x54;
                OutputBuffer[15] = 0x65;
                OutputBuffer[16] = 0x73;
                OutputBuffer[17] = 0x74;
            }
            //(Blank)
            if (lines != 3)
            {
                OutputBuffer[12] = 0x53;
                OutputBuffer[13] = 0x6F;
                OutputBuffer[14] = 0x75;
                OutputBuffer[15] = 0x72;
                OutputBuffer[16] = 0x63;
                OutputBuffer[17] = 0x65;
                //Source
            }
            letter++;
        }
        WriteAndClose();
    }
    while ((screen == MODE))
    {
        GotoSleep();
        if (button == BUTTON_A)
        {
            sysMode = VOLTAGE_SOURCE;
            screen = PARM_HOME;
        }
        else if (button == BUTTON_B)
        {
            sysMode = CURRENT_SOURCE;
            screen = PARM_HOME;
        }
        else if (button == BUTTON_C)
        {
            sysMode = BREAKDOWN_TEST;
            screen = PARM_HOME;
        }
        else if (button == EXIT)
        {
            screen = PARM_HOME;
        }
        else if (button == HV_ENABLE)
        {
            screen = OUTPUT;
        }
    }
    button = NO_PRESS;
}

void ParmMode()
{
    DrawParmUI();
    button = NO_PRESS;
    while ((screen == PARM_HOME))
    {
        GotoSleep();
        if (button == BUTTON_A)
        {
            screen = LIM_HOME;
        }
        else if (button == BUTTON_B)
        {
            screen = MODE;
            ModeSelect();
            if (screen != OUTPUT)
            {
                screen = PARM_HOME;
                DrawParmUI();
            }
            
            //return;
        }
        else if (button == BUTTON_C)
        {
            screen = HOME;
        }
        else if (button == BUTTON_D)
        {
            button = NO_PRESS;
            RunAbout();
            DrawParmUI();
        }
        else if (button == EXIT)
        {
            DiagAndConfig();
            DrawParmUI();
        }
        else if (button == HV_ENABLE)
        {
            screen = OUTPUT;
        }
        button = NO_PRESS;
        //ButtonHit();
    }
}

void DrawParmUI()
{
    SayHelloCommand();
    WriteLCD(CMD_CLEAR);
    WriteLCD(CMD_GOHOME);
    WriteLCD(CMD_CURSOR_OFF);
    CloseLCD();
    uchar base = 0x41;
    for (int lines = 0; lines < 4; ++lines)
    {
        ClearBuffer();
        if (lines == 0)
        {
            //Parameters
            OutputBuffer[0] = 0x50;
            OutputBuffer[1] = 0x61;
            OutputBuffer[2] = 0x72;
            OutputBuffer[3] = 0x61;
            OutputBuffer[4] = 0x6D;
            OutputBuffer[5] = 0x65;
            OutputBuffer[6] = 0x74;
            OutputBuffer[7] = 0x65;
            OutputBuffer[8] = 0x72;
            OutputBuffer[9] = 0x73;
        }
        else
        {
            OutputBuffer[0] = LEFT_PAR;
            OutputBuffer[1] = base;
            OutputBuffer[2] = RIGHT_PAR;
            //OutputBuffer[3] = BLANK;
            if (lines != 3)
            {
                //Set
                OutputBuffer[4] = 0x53;
                OutputBuffer[5] = 0x65;
                OutputBuffer[6] = 0x74;
                //OutputBuffer[7] = Blank;
                if (lines == 1)
                {
                    //Limits
                    OutputBuffer[8] = 0x4C;
                    OutputBuffer[9] = 0x69;
                    OutputBuffer[10] =  0x6D;
                    OutputBuffer[11] = 0x69;
                    OutputBuffer[12] = 0x74;
                    OutputBuffer[13] = 0x73;
                }
                else
                {
                    //Mode
                    OutputBuffer[8] = 0x4D;
                    OutputBuffer[9] = 0x6F;
                    OutputBuffer[10]= 0x64;
                    OutputBuffer[11] = 0x65;
                } 
            }
            else if (lines == 3)
            {
                //Return
                OutputBuffer[4] = 0x52;
                OutputBuffer[5] = 0x65;
                OutputBuffer[6] = 0x74;
                OutputBuffer[7] = 0x75;
                OutputBuffer[8] = 0x72;
                OutputBuffer[9] = 0x6E;
            }
            base++;
        }
        WriteAndClose();
    }
}

void HomeScreenUI()
{
    SayHelloCommand();
    WriteLCD(CMD_CLEAR);
    WriteLCD(CMD_GOHOME);
    CloseLCD();
    uchar base = 0x41;//'A'
    for (int lines = 0; lines < 4; ++lines)
    {
        ClearBuffer();
        if (lines == 0)
        {
            if ((screen != EDIT_LIM) && (screen != LIM_HOME))
            {
                //Editing Mode
                OutputBuffer[0] = 0x45;
                OutputBuffer[1] = 0x64;
                OutputBuffer[2] = 0x69;
                OutputBuffer[3] = 0x74;
                OutputBuffer[4] = 0x69;
                OutputBuffer[5] = 0x6E;
                OutputBuffer[6] = 0x67;
                //OutputBuffer[7] = BLANK;
                OutputBuffer[8] = 0x4D;
                OutputBuffer[9] = 0x6F;
                OutputBuffer[10] = 0x64;
                OutputBuffer[11] = 0x65;
            }
            else
            {
                    OutputBuffer[0] = 0x4C;
                    OutputBuffer[1] = 0x69;
                    OutputBuffer[2] =  0x6D;
                    OutputBuffer[3] = 0x69;
                    OutputBuffer[4] = 0x74;
                    OutputBuffer[5] = 0x73;
            }
        }
        else
        {
            if ((screen == HOME) || (screen == LIM_HOME))
            {   //(A), (B), (C), (D)
                OutputBuffer[0] = LEFT_PAR;
                OutputBuffer[1] = base;
                OutputBuffer[2] = RIGHT_PAR;
            }
            else
            {
                //->
                if (((sysDir == VOLTAGE) && (lines == 1)) || ((sysDir == CURRENT) && (lines == 2)))
                {
                    OutputBuffer[0] = 0x2D;
                    OutputBuffer[1] = 0x3E;
                }
                
                //OutputBuffer[0] = 0x52;
                //OutputBuffer[1] = 0x54;
                //OutputBuffer[2] = 0x4E;
            }
            //OutputBuffer[3] = BLANK;
            if (lines == 3)
            {
                
                if ((screen == EDIT_LIM) || (screen == LIM_HOME))
                {
                    //Return
                    OutputBuffer[4] = 0x52;
                    OutputBuffer[5] = 0x65;
                    OutputBuffer[6] = 0x74;
                    OutputBuffer[7] = 0x75;
                    OutputBuffer[8] = 0x72;
                    OutputBuffer[9] = 0x6E;
                }
                else
                {
                    //Parameters
                    OutputBuffer[4] = 0x50;
                    OutputBuffer[5] = 0x61;
                    OutputBuffer[6] = 0x72;
                    OutputBuffer[7] = 0x61;
                    OutputBuffer[8] = 0x6D;
                    OutputBuffer[9] = 0x65;
                    OutputBuffer[10] = 0x74;
                    OutputBuffer[11] = 0x65;
                    OutputBuffer[12] = 0x72;
                    OutputBuffer[13] = 0x73;
                }
                
            }
            else
            {
                OutputBuffer[4] = 0x56; //V
                if (lines == 2)
                {
                    //I
                    OutputBuffer[4] = 0x49;
                }
                
                if ((screen == EDIT_SET) || (screen == HOME))
                {
                    //set
                    OutputBuffer[5] = 0x73;
                    OutputBuffer[6] = 0x65;
                    OutputBuffer[7] = 0x74;
                }
                else if ((screen == EDIT_LIM) || (screen == LIM_HOME))
                {
                    //lim
                    OutputBuffer[5] = 0x6C;
                    OutputBuffer[6] = 0x69;
                    OutputBuffer[7] = 0x6D;
                }
                //OutputBuffer[8] = BLANK;
                if ((((lines == 1) && (sysMode == CURRENT_SOURCE)) || (lines == 2) && (sysMode != CURRENT_SOURCE)) && ((screen == HOME) || (screen == EDIT_SET)))
                {
                    //-NA-
                    OutputBuffer[9] = 0x2D;
                    OutputBuffer[10] = 0x4E;
                    OutputBuffer[11] = 0x41;
                    OutputBuffer[12] = 0x2D;
                }
                else if (lines == 1)
                {
                    //V
                    if ((sysMode == CURRENT_SOURCE) || (screen == EDIT_SET) || (screen == HOME))
                    {
                        uint pos = 0;
                        if ((screen != LIM_HOME) && (screen != EDIT_LIM))
                        {
                           pos = writeLargeNumber(1, 9, Vout);
                        }   
                        else
                        {
                            pos = writeLargeNumber(1, 9, Vlim);
                        }
                        OutputBuffer[pos] = 0x56;
                    }
                    else
                    { //-NA-
                        OutputBuffer[9] = 0x2D;
                        OutputBuffer[10] = 0x4E;
                        OutputBuffer[11] = 0x41;
                        OutputBuffer[12] = 0x2D;
                    }
                    
                }
                else
                {
                    if ((sysMode != CURRENT_SOURCE) || (screen == EDIT_SET) || (screen == HOME))
                    {
                        //0uA
                        uint pos = 0;
                        if ((screen != LIM_HOME) && (screen != EDIT_LIM))
                        {
                             pos = writeLargeNumber(1, 9, Iout);
                        }
                        else
                        {
                             pos = writeLargeNumber(1, 9, Ilim);
                        }
                        OutputBuffer[pos] = NumbersBase;
                        OutputBuffer[pos + 1] = 0x75;
                        OutputBuffer[pos + 2] = 0x41;
                    }
                    else
                    { //-NA-
                        OutputBuffer[9] = 0x2D;
                        OutputBuffer[10] = 0x4E;
                        OutputBuffer[11] = 0x41;
                        OutputBuffer[12] = 0x2D;
                    }
                }
                
            }
            base++;
        }
        WriteAndClose();
        
        //writeLargeNumber(2, 9, Vout);
        //OutputBuffer[3] = BLANK;
    }
}

void OutputMode()
{
    SayHelloCommand();
    WriteLCD(CMD_CURSOR_OFF);
    WriteLCD(CMD_CLEAR);
    WriteLCD(CMD_GOHOME);
    CloseLCD();
    
    int offset = 0;
    
    for (int lines = 0; lines < 4; ++lines)
    {
        ClearBuffer();
        if (lines == 0)
        {
            //Display the System Mode
            if (sysMode == BREAKDOWN_TEST)
            {
               //Breakdown
                OutputBuffer[0] = 0x42;
                OutputBuffer[1] = 0x72;
                OutputBuffer[2] = 0x65;
                OutputBuffer[3] = 0x61;
                OutputBuffer[4] = 0x6B;
                OutputBuffer[5] = 0x64;
                OutputBuffer[6] = 0x6F;
                OutputBuffer[7] = 0x77;
                OutputBuffer[8] = 0x6E;
                offset = 10;
            }
            else if (sysMode == VOLTAGE_SOURCE)
            {
                //Voltage
                OutputBuffer[0] = 0x56;
                OutputBuffer[1] = 0x6F;
                OutputBuffer[2] = 0x6C;
                OutputBuffer[3] = 0x74;
                OutputBuffer[4] = 0x61;
                OutputBuffer[5] = 0x67;
                OutputBuffer[6] = 0x65;
                offset = 8;
            }
            else if (sysMode == CURRENT_SOURCE)
            {
                //Current
                OutputBuffer[0] = 0x43;
                OutputBuffer[1] = 0x75;
                OutputBuffer[2] = 0x72;
                OutputBuffer[3] = 0x72;
                OutputBuffer[4] = 0x65;
                OutputBuffer[5] = 0x6E;
                OutputBuffer[6] = 0x74;
                offset = 8;
            }
            
            //Mode
            OutputBuffer[offset]  = 0x4D;
            OutputBuffer[offset + 1] = 0x6F;
            OutputBuffer[offset + 2] = 0x64;
            OutputBuffer[offset + 3] = 0x65;
        }
        else if (lines == 1)
        {
            if (sysMode == CURRENT_SOURCE)
            {
                //I
                OutputBuffer[0] = 0x49;
            }
            else
            {
                //V
                OutputBuffer[0] = 0x56;
            }
            
            //set:
            OutputBuffer[1] = 0x73;
            OutputBuffer[2] = 0x65;
            OutputBuffer[3] = 0x74;
            OutputBuffer[4] = COLON;
            
            if (sysMode == CURRENT_SOURCE)
            {
                offset = writeLargeNumber(0, 6, Iout);
                OutputBuffer[offset] = NumbersBase;
                OutputBuffer[offset + 1] = 0x75; //u
                OutputBuffer[offset + 2] = 0x41; //A
            }
            else
            {
                offset = writeLargeNumber(0, 6, Vout);
                OutputBuffer[offset] = 0x56; //V
            }
            
        }
        else if (lines == 2)
        {
            if (sysMode == CURRENT_SOURCE)
            {
                //I
                OutputBuffer[0] = 0x56;
            }
            else
            {
                //V
                OutputBuffer[0] = 0x49;
            }
            
            //lim:
            OutputBuffer[1] = 0x6C;
            OutputBuffer[2] = 0x69;
            OutputBuffer[3] = 0x6D;
            OutputBuffer[4] = COLON;
            
            if ((sysMode == CURRENT_SOURCE) && (Vlim != 0000))
            {
                offset = writeLargeNumber(0, 6, Vlim);
                OutputBuffer[offset] = 0x56; //V
            }
            else if ((sysMode != CURRENT_SOURCE) && (Ilim != 0000))
            {
                offset = writeLargeNumber(0, 6, Ilim);
                OutputBuffer[offset] = NumbersBase;
                OutputBuffer[offset + 1] = 0x75; //u
                OutputBuffer[offset + 2] = 0x41; //A
            }
            else
            {
                //-NA-
                OutputBuffer[6] = 0x2D;
                OutputBuffer[7] = 0x4E;
                OutputBuffer[8] = 0x41;
                OutputBuffer[9] = 0x2D;
            }
        }
        else if (lines == 3)
        {
            //(A)
            OutputBuffer[0] = LEFT_PAR;
            OutputBuffer[1] = 0x41;
            OutputBuffer[2] = RIGHT_PAR;
            
            //Enable?
            OutputBuffer[4] = 0x45;
            OutputBuffer[5] = 0x6E;
            OutputBuffer[6] = 0x61;
            OutputBuffer[7] = 0x62;
            OutputBuffer[8] = 0x6C;
            OutputBuffer[9] = 0x65;
            OutputBuffer[10] = 0x3F;
        }
        WriteAndClose();
    }
    
    GotoSleep();
    if (button == BUTTON_A)
    {
        OutputModeUI();
        button = NO_PRESS;
        do
        {
            GotoSleep();
        } while ((button != HV_ENABLE) && (button != EXIT));
    }
    
    button = NO_PRESS;
    screen = HOME;
}

void OutputModeUI()
{
    SayHelloCommand();
    WriteLCD(CMD_CLEAR);
    WriteLCD(CMD_GOHOME);
    CloseLCD();
    
    //Line 1
    for (int lines = 0; lines < 4; ++lines)
    {
        ClearBuffer();
        if (sysMode != BREAKDOWN_TEST)
        {
            if (lines == 0)
            {
                //V
                OutputBuffer[0] = 0x56;
            }
            else if (lines == 2)
            {
                //I
                OutputBuffer[0] = 0x49;
            }

            if (lines % 2 == 0)
            {
                if (((sysMode == VOLTAGE_SOURCE) && (lines == 0)) || ((sysMode == CURRENT_SOURCE) && (lines == 2)))
                {
                    //set
                    OutputBuffer[1] = 0x73;
                    OutputBuffer[2] = 0x65;
                    OutputBuffer[3] = 0x74;
                }
                else
                {
                    //lim
                    OutputBuffer[1] = 0x6C;
                    OutputBuffer[2] = 0x69;
                    OutputBuffer[3] = 0x6D;
                }
            }
            else{
                //out
                OutputBuffer[1] = 0x6F;
                OutputBuffer[2] = 0x75;
                OutputBuffer[3] = 0x74;
            }
            OutputBuffer[4] = COLON;
            //OutputBuffer[9] = BLANK;
            int pos = 0;
            
            if (lines == 0)
            {
                pos = writeLargeNumber(0, 6, Vout);
            }
            else if (lines == 1)
            {
                pos = writeLargeNumber(0, 6, Vcurr);
            }
            else if (lines == 2)
            {
                pos = writeLargeNumber(0, 6, Iout);
            }
            else if (lines == 3)
            {
                pos = writeLargeNumber(0, 6, Icurr);
            }
            
            if (lines < 2)
            {
                //V
                OutputBuffer[pos] = 0x56;
            }
            else
            {
                //0uA
                OutputBuffer[pos] = NumbersBase;
                OutputBuffer[pos + 1] = 0x75;
                OutputBuffer[pos + 2] = 0x41;
            }
        }
        else
        {
            //Breakdown Test Loop
            if (lines == 0)
            {
                //Vlim: xxxxV
                OutputBuffer[0] = 0x56;
                OutputBuffer[1] = 0x6C;
                OutputBuffer[2] = 0x69;
                OutputBuffer[3] = 0x6D;
                OutputBuffer[4] = COLON;
                //(Blank)
                int lPos = writeLargeNumber(0, 6, Vout);
                OutputBuffer[lPos] = 0x56;
            }
            else if (lines == 1)
            {
                //Istop: xxxxxuA
                OutputBuffer[0] = 0x49;
                OutputBuffer[1] = 0x73;
                OutputBuffer[2] = 0x74;
                OutputBuffer[3] = 0x6F;
                OutputBuffer[4] = 0x70;
                OutputBuffer[5] = COLON;
                //(Blank)
                int lPos = writeLargeNumber(0, 7, Ilim);
                OutputBuffer[lPos] = NumbersBase;
                OutputBuffer[lPos + 1] = 0x75;
                OutputBuffer[lPos + 2] = 0x41;
            }
            else if (lines == 2)
            {
                
            }
            else
            {
                //Progress XX.XX%
                OutputBuffer[0] = 0x50;
                OutputBuffer[1] = 0x72;
                OutputBuffer[2] = 0x6F;
                OutputBuffer[3] = 0x67;
                OutputBuffer[4] = 0x72;
                OutputBuffer[5] = 0x65;
                OutputBuffer[6] = 0x73;
                OutputBuffer[7] = 0x73;
                //(Blank)
                double data = 0.0 / Vout;
                int digit = 0;
                if (data >= 10.0)
                {
                    digit = _GetDoubleDigit(data, 10);
                    data -= digit;
                    OutputBuffer[9] = data + NumbersBase;
                }
                else
                {
                    OutputBuffer[9] = NumbersBase;
                }
                digit = _GetDoubleDigit(data, 1);
                OutputBuffer[10] = NumbersBase + digit;
                OutputBuffer[11] = 0x25;
            }
        }
        

        WriteAndClose();
    }
}

void DiagAndConfig()
{
    button = NO_PRESS;
    SayHelloCommand();
    WriteLCD(CMD_CLEAR);
    WriteLCD(CMD_GOHOME);
    CloseLCD();
        
    for (int lines = 0; lines < 4; ++lines)
    {
        ClearBuffer();
        if (lines != 0)
        {   
            OutputBuffer[0] = LEFT_PAR;
            OutputBuffer[1] = lines + 0x41 - 1;
            OutputBuffer[2] = RIGHT_PAR;
        }
        //Blank
        if (lines == 0)
        {
            //System Menu
            OutputBuffer[0] = 0x53;
            OutputBuffer[1] = 0x79;
            OutputBuffer[2] = 0x73;
            OutputBuffer[3] = 0x74;
            OutputBuffer[4] = 0x65;
            OutputBuffer[5] = 0x6D;
            //Blank
            OutputBuffer[7] = 0x4D;
            OutputBuffer[8] = 0x65;
            OutputBuffer[9] = 0x6E;
            OutputBuffer[10] = 0x75;
        }
        else if (lines != 3)
        {
            if (lines == 1)
            {
                //Save
                OutputBuffer[4] = 0x53;
                OutputBuffer[5] = 0x61;
                OutputBuffer[6] = 0x76;
                OutputBuffer[7] = 0x65;
            }
            else if (lines == 2)
            {
                //Load
                OutputBuffer[4] = 0x4C;
                OutputBuffer[5] = 0x6F;
                OutputBuffer[6] = 0x61;
                OutputBuffer[7] = 0x64;
            }
            
            //Settings
            OutputBuffer[9] = 0x53;
            OutputBuffer[10] = 0x65;
            OutputBuffer[11] = 0x74;
            OutputBuffer[12] = 0x74;
            OutputBuffer[13] = 0x69;
            OutputBuffer[14] = 0x6E;
            OutputBuffer[15] = 0x67;
            OutputBuffer[16] = 0x73;
        }
        else
        {
            //Diagnostics
            OutputBuffer[4] = 0x44;
            OutputBuffer[5] = 0x69;
            OutputBuffer[6] = 0x61;
            OutputBuffer[7] = 0x67;
            OutputBuffer[8] = 0x6E;
            OutputBuffer[9] = 0x6F;
            OutputBuffer[10] = 0x73;
            OutputBuffer[11] = 0x74;
            OutputBuffer[12] = 0x69;
            OutputBuffer[13] = 0x63;
            OutputBuffer[14] = 0x73;
        }
        WriteAndClose();
    }
    int good = 1;
    do{
        GotoSleep();
        if (button == BUTTON_A)
        {
            WriteSettingsToMemory();
            good = 0;
        }
        else if (button == BUTTON_B)
        {
            LoadSettingsFromMemory();
            good = 0;
        }
        else if (button == BUTTON_C)
        {
            //Diagnostic Menu
            DiagMenu();
            good = 0;
        }
        else if (button == EXIT)
        {
            good = 0;
            button = NO_PRESS;
        }
        else if (button == HV_ENABLE)
        {
            good = 0;
            screen = OUTPUT;
        }
    } while(good == 1);
    button = NO_PRESS;
}

void DiagMenu()
{
    SayHelloCommand();
    WriteLCD(CMD_CLEAR);
    WriteLCD(CMD_GOHOME);
    CloseLCD();
    
    int counter = 0;
    
    for (int lines = 0; lines < 4; ++lines)
    {
        ClearBuffer();
        if (lines != 0)
        {
            OutputBuffer[0] = LEFT_PAR;
            OutputBuffer[1] = counter + 0x41;
            OutputBuffer[2] = RIGHT_PAR;
            counter++;
        }
        
        if (lines == 0)
        {
            //Diagnostics
            OutputBuffer[0] = 0x44;
            OutputBuffer[1] = 0x69;
            OutputBuffer[2] = 0x61;
            OutputBuffer[3] = 0x67;
            OutputBuffer[4] = 0x6E;
            OutputBuffer[5] = 0x6F;
            OutputBuffer[6] = 0x73;
            OutputBuffer[7] = 0x74;
            OutputBuffer[8] = 0x69;
            OutputBuffer[9] = 0x63;
            OutputBuffer[10] = 0x73;
        }
        else if (lines == 1)
        {
            //Reset
            OutputBuffer[4] = 0x52;
            OutputBuffer[5] = 0x65;
            OutputBuffer[6] = 0x73;
            OutputBuffer[7] = 0x65;
            OutputBuffer[8] = 0x74;
        }
        else if (lines == 2)
        {
            //Comm. Test
            OutputBuffer[4] = 0x43;
            OutputBuffer[5] = 0x6F;
            OutputBuffer[6] = 0x6D;
            OutputBuffer[7] = 0x6D;
            OutputBuffer[8] = 0x2E;
            
            OutputBuffer[10] = 0x54;
            OutputBuffer[11] = 0x65;
            OutputBuffer[12] = 0x73;
            OutputBuffer[13] = 0x74;
        }
        else if (lines == 3)
        {
            //Calibration
            OutputBuffer[4] = 0x43;
            OutputBuffer[5] = 0x61;
            OutputBuffer[6] = 0x6C;
            OutputBuffer[7] = 0x69;
            OutputBuffer[8] = 0x62;
            OutputBuffer[9] = 0x72;
            OutputBuffer[10] = 0x61;
            OutputBuffer[11] = 0x74;
            OutputBuffer[12] = 0x69;
            OutputBuffer[13] = 0x6F;
            OutputBuffer[14] = 0x6E;
        }
       
        WriteAndClose();
    }
    
    int optionOK = 0;
    
    do {
        GotoSleep();
        if (button == BUTTON_A)
        {
            Vout = 0;
            Iout = 0;
            Vlim = 0;
            Ilim = 0;
            sysMode = VOLTAGE_SOURCE;

            WriteSettingsToMemory();
            LoadSettingsFromMemory();
            optionOK = 1;
        }
        else if (button == BUTTON_B)
        {
            //Comm Check
            optionOK = 1;
        }
        else if (button == BUTTON_C)
        {
            //Cal.
            optionOK = 1;
        }
        else if (button == EXIT)
        {
            //Exit
            optionOK = 1;
        }
    } while(optionOK == 0);
    
    
}

void WriteAndClose()
{
    //ClearBuffer();
    SayHelloWrite();
    WriteLine();
    CloseLCD();
}

void LoadSettingsFromMemory()
{
     /*
            * Read Order:
            * Mode (0x0)
            * VSet
            * ISet
            * VLim
            * ILim
        */
        unsigned char address = 0x0;
        NVMDAT = 0x0;
        for (int i = 0; i < 9; ++i)
        {
            NVMADRL = address;
            NVMCON1bits.RD = 0b1;
            switch (i)
            {
                case 0:
                    sysMode = NVMDAT;
                    if (sysMode == 0xFF)
                    {
                        sysMode = 0x0;
                        WriteSettingsToMemory();
                    }
                    break;
                case 1:
                    Vout = NVMDAT;
                    Vout = Vout << 7;
                    break;
                case 2:
                    Vout += NVMDAT;
                    break;
                case 3:
                    Iout = NVMDAT;
                    Iout = Iout << 7;
                    break;
                case 4:
                    Iout += NVMDAT;
                    break;
                case 5:
                    Vlim = NVMDAT;
                    Vlim = Vlim << 7;
                    break;
                case 6:
                    Vlim += NVMDAT;
                    break;
                case 7:
                    Ilim = NVMDAT;
                    Ilim = Ilim << 7;
                    break;
                case 8:
                    Ilim += NVMDAT;
                    break; 
            }
            NVMDAT = 0x0;
            address++;
        }
        unsigned long temp = 0;
        int* ptr = 0x0;
        for (int i = 0; i < 6; ++i)
        {
            switch (i)
            {
                case 0:
                    ptr = &VOutDigits[3];
                    temp = Vout;
                    break;
                case 1:
                    ptr = &IOutDigits[3];
                    temp = Iout;
                    break;
                case 2:
                    ptr = &VLimDigits[3];
                    temp = Vlim;
                    break;
                case 3:
                    ptr = &ILimDigits[3];
                    temp = Ilim;
                    break;
                case 4:
                    ptr = &VStopDigits[3];
                    temp = Vstop;
                    break;
                case 5:
                    ptr = &IStopDigits[3];
                    temp = Istop;
                    break;
            }
            *ptr = 0;
            while (temp >= 1000)
            {
                (*ptr)++;
                temp -= 1000;
            }
            ptr--;
            *ptr = 0;
            while (temp >= 100)
            {
                (*ptr)++;
                temp -= 100;
            }
            ptr--;
            *ptr = 0;
            while (temp >= 10)
            {
                (*ptr)++;
                temp -= 10;
            }
            ptr--;
            *ptr = temp;
        }
}

void WriteSettingsToMemory()
{
    /*
     * Write Order:
     * Mode
     * VSet
     * ISet
     * VLim
     * ILim
     */
    unsigned char address = 0x0;
    unsigned char dataToWrite = sysMode;
    NVMCON1 = 0x0;
    NVMCON1bits.WREN = 0b1;
    INTCONbits.GIE = 0b0;
    for (int i = 0; i < 9; ++i)
    {
        switch (i)
        {
            case 0:
                dataToWrite = sysMode;
                break;
            case 1:
                dataToWrite = (Vout & 0xF00) >> 7;
                break;
            case 2:
                dataToWrite = Vout & 0xFF;
                break;
            case 3:
                dataToWrite = (Iout & 0xF00) >> 7;
                break;
            case 4:
                dataToWrite = Iout & 0xFF;
                break;
            case 5:
                dataToWrite = (Vlim & 0xF00) >> 7;
                break;
            case 6:
                dataToWrite = Vlim & 0xFF;
                break;
            case 7:
                dataToWrite = (Ilim & 0xF00) >> 7;
                break;
            case 8:
                dataToWrite = Ilim & 0xFF;
                break;
            default:
                dataToWrite = 0x0;
        }
        NVMADRL = address;
        NVMDAT = dataToWrite;
        //Unlock Sequence: Write 55h, AAh to NVMCON2; then WR = 1
        NVMCON2 = 0x55;
        NVMCON2 = 0xAA;
        NVMCON1bits.WR = 0b1;
        PIR7bits.NVMIF = 0b0;
        while (PIR7bits.NVMIF == 0b0)
        {
            asm("CLRWDT"); //Clear the WDT
        }
        NVMCON1bits.WR = 0b0;
        PIR7bits.NVMIF = 0b0;
        address++;
    }
    INTCONbits.GIE = 0b1;
    NVMCON1bits.WREN = 0b0;
}