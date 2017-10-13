#include "./LCDSPI.h"
#include "./lookupTable.h"
#include "./displayNumbers.h"

// CONFIG1L
#pragma config FEXTOSC = OFF    // External Oscillator mode Selection bits (Oscillator not enabled)
#pragma config RSTOSC = HFINTOSC_1MHZ// Power-up default value for COSC bits (HFINTOSC with HFFRQ = 4 MHz and CDIV = 4:1)

// CONFIG1H
#pragma config CLKOUTEN = OFF   // Clock Out Enable bit (CLKOUT function is disabled)
#pragma config CSWEN = ON       // Clock Switch Enable bit (Writing to NOSC and NDIV is allowed)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor enabled)

// CONFIG2L
#pragma config MCLRE = EXTMCLR  // Master Clear Enable bit (If LVP = 0, MCLR pin is MCLR; If LVP = 1, RE3 pin function is MCLR )
#pragma config PWRTE = ON       // Power-up Timer Enable bit (Power up timer enabled)
#pragma config LPBOREN = ON     // Low-power BOR enable bit (ULPBOR enabled)
#pragma config BOREN = SBORDIS  // Brown-out Reset Enable bits (Brown-out Reset enabled , SBOREN bit is ignored)

// CONFIG2H
#pragma config BORV = VBOR_2P45 // Brown Out Reset Voltage selection bits (Brown-out Reset Voltage (VBOR) set to 2.45V)
#pragma config ZCD = OFF        // ZCD Disable bit (ZCD disabled. ZCD can be enabled by setting the ZCDSEN bit of ZCDCON)
#pragma config PPS1WAY = ON     // PPSLOCK bit One-Way Set Enable bit (PPSLOCK bit can be cleared and set only once; PPS registers remain locked after one clear/set cycle)
#pragma config STVREN = ON      // Stack Full/Underflow Reset Enable bit (Stack full/underflow will cause Reset)
#pragma config DEBUG = OFF      // Debugger Enable bit (Background debugger disabled)
#pragma config XINST = OFF      // Extended Instruction Set Enable bit (Extended Instruction Set and Indexed Addressing Mode disabled)

// CONFIG3L
#pragma config WDTCPS = WDTCPS_31// WDT Period Select bits (Divider ratio 1:65536; software control of WDTPS)
#pragma config WDTE = ON        // WDT operating mode (WDT enabled regardless of sleep)

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

void WriteAndClose();

void HomeScreenUI(); //Main Loop
void OutputModeUI(); //Draw the Output
void DrawParmUI(); //Draws the UI
void ParmMode(); //Parm Edit Mode Loop
void RunAbout(); //Info / Welcome Screen
void ModeSelect(); //Select a Mode
void DrawLimitUI();

void GotoSleep()
{
    //CPUDOZEbits.IDLEN = 0;
    //asm("SLEEP");
    //PEIEbits.
}

void DrawLine(const char* text, double* val, uchar prefix, char unit);

void RestoreCursor(int digit);
void ButtonHit();
uint button = 0;

Screen screen = HOME;
Modification sysDir = VOLTAGE;
SystemMode sysMode = VOLTAGE_SOURCE;

static int Version;

void main(void) {    
    //Fosc = 4MHz, 1MHz Output, 1uS Cycle   
    
    TRISA = 0x00;
    PORTA = 0x0;
    PORTB = 0xFF;
    TRISB = 0x0;
    TRISC = 0xF0;
    ANSELC = 0x0F;
    PORTC = 0x0;
    
    Version = 4;
    PIE0bits.IOCIE = 0b1; //Enable interrupt-on-change
    IOCCP = 0xF0; //Enable RC4-RC7 rising edge interrupts
        
    int digit = 0;
    
    int VOutDigits[4] = {0,0,0,0};
    int IOutDigits[4] = {0,0,0,0};
    
    int VLimDigits[4] = {0,0,0,0};
    int ILimDigits[4] = {0,0,0,0};
    
    for (uint i = 0; i < 20; i++)
    {
        OutputBuffer[i] = BLANK;
    }
    
    double baseVal = 0.0;
    
    InitDisplay();
    
    /*//Hello
    OutputBuffer[0] = 0x48;
    OutputBuffer[1] = 0x65;
    OutputBuffer[2] = 0x6C;
    OutputBuffer[3] = 0x6C;
    OutputBuffer[4] = 0x6F;
    
    //Space
    OutputBuffer[5] = BLANK;
    
    //World
    OutputBuffer[6] = 0x57;
    OutputBuffer[7] = 0x6F;
    OutputBuffer[8] = 0x72;
    OutputBuffer[9] = 0x6C;
    OutputBuffer[10] = 0x64;*/
    screen = HOME;
    sysMode = VOLTAGE_SOURCE;
    RunAbout();
    HomeScreenUI();
    while (1 == 1)
    {
        if (button == 7)
        {
            if (screen == HOME)
            {
                digit = 3;
                //baseVal = 1000;
                screen = EDIT_SET;
                sysDir = VOLTAGE;
                SayHelloCommand();
                Write(CMD_CURSOR_ON);
                CloseLCD();
                RestoreCursor(digit);
            }
            else if (screen == LIM_HOME)
            {
                screen = EDIT_LIM;
                sysDir = VOLTAGE;
                digit = 3;
                SayHelloCommand();
                Write(CMD_CURSOR_ON);
                CloseLCD();
                RestoreCursor(digit);
            }
            else if ((screen == EDIT_LIM) && (sysDir == VOLTAGE))
            {
                screen = LIM_HOME;
                SayHelloCommand();
                Write(CMD_CURSOR_OFF);
                CloseLCD();
            }
            else if ((screen == EDIT_SET) && (sysDir == VOLTAGE))
            {
                screen = HOME;
                SayHelloCommand();
                Write(CMD_CURSOR_OFF);
                CloseLCD();
            }
            else if ((screen == EDIT_SET) && (sysDir == CURRENT))
            {
                IOutDigits[digit]++;
                if (IOutDigits[digit] > 9)
                    IOutDigits[digit] = 0;
                Iout = IOutDigits[3] * 1000 + IOutDigits[2] * 100 + IOutDigits[1] * 10 + IOutDigits[0];
                HomeScreenUI();
                RestoreCursor(digit);
            }
            else if ((screen == EDIT_LIM) && (sysDir == CURRENT))
            {
                ILimDigits[digit]++;
                if (ILimDigits[digit] > 9)
                    ILimDigits[digit] = 0;
                Ilim = ILimDigits[3] * 1000 + ILimDigits[2] * 100 + ILimDigits[1] * 10 + ILimDigits[0];
                HomeScreenUI();
                RestoreCursor(digit);
            }
        }
        else if (button == 6)
        {
            if (screen == HOME)
            {
                digit = 3;
                //baseVal = 10;
                screen = EDIT_SET;
                sysDir = CURRENT;
                SayHelloCommand();
                Write(CMD_CURSOR_ON);
                CloseLCD();
                RestoreCursor(digit);
            }
            else if (screen == LIM_HOME)
            {
                screen = EDIT_LIM;
                sysDir = CURRENT;
                digit = 3;
                SayHelloCommand();
                Write(CMD_CURSOR_ON);
                CloseLCD();
                RestoreCursor(digit);
            }
            else if ((screen == EDIT_SET) && (sysDir == CURRENT))
            {
                screen = HOME;
                SayHelloCommand();
                Write(CMD_CURSOR_OFF);
                CloseLCD();
            }
            else if ((screen == EDIT_LIM) && (sysDir == CURRENT))
            {
                screen = LIM_HOME;
                SayHelloCommand();
                Write(CMD_CURSOR_OFF);
                CloseLCD();
            }
            else if ((screen == EDIT_SET) && (sysDir == VOLTAGE))
            {
                VOutDigits[digit]++;
                if (VOutDigits[digit] > 9)
                    VOutDigits[digit] = 0;
                Vout = VOutDigits[3] * 1000 + VOutDigits[2] * 100 + VOutDigits[1] * 10 + VOutDigits[0];
                HomeScreenUI();
                RestoreCursor(digit);
            }
            else if ((screen == EDIT_LIM) && (sysDir == VOLTAGE))
            {
                VLimDigits[digit]++;
                if (VLimDigits[digit] > 9)
                    VLimDigits[digit] = 0;
                Vlim = VLimDigits[3] * 1000 + VLimDigits[2] * 100 + VLimDigits[1] * 10 + VLimDigits[0];
                HomeScreenUI();
                RestoreCursor(digit);
            }
            //HomeScreenUI(screen, 0);
        }
        else if (button == 5)
        {
            if ((screen == HOME))
            {
                screen = PARM_HOME;
                ParmMode();
                HomeScreenUI();
            }
            else if (screen == LIM_HOME)
            {
                screen = PARM_HOME;
                ParmMode();
                HomeScreenUI();
            }
            else if ((screen == EDIT_SET) || (screen == EDIT_LIM))
            {
                digit--;
                if (digit < 0)
                    digit = 0;
                RestoreCursor(digit);
            }
            
        }
        else if (button == 4)
        {
            digit++;
            if (digit >= 4)
                digit = 4;
            RestoreCursor(digit);
        }
        GotoSleep();
        ButtonHit();
        asm ("CLRWDT"); //Clear the WDT 
    }
    return;
}

void RestoreCursor(int digit)
{
    SayHelloCommand();
    if ((sysDir == VOLTAGE))
    {
        Write(0x80 | (0x29 + (3 - digit)));
    }
    else if ((sysDir == CURRENT))
    {
        Write(0x80 | (0x49 + (3 - digit)));
    }
    CloseLCD();
}

void ButtonHit()
{
    button = 0;
    if (IOCCFbits.IOCCF7 == 1)
    {
        button = 7;
    }
    else if (IOCCFbits.IOCCF6 == 1)
    {
        button = 6;
    }
    else if (IOCCFbits.IOCCF5 == 1)
    {
        button = 5;
    }
    else if (IOCCFbits.IOCCF4 == 1)
    {
        button = 4;
    }
    else
    {
        return;
    }

    IOCCF = 0x0;
    IOCCN = IOCCP; //Enable Falling Edge Detect
    IOCCP = 0x0; //Disable Rising Edge
    do
    {
        IOCCF = 0x0;
        _delay(25);
        asm("CLRWDT");
    } while ((IOCCF != 0x0) || (PORTC != 0x0));
    IOCCF = 0x0;
    IOCCP = IOCCN; //Return settings
    IOCCN = 0x0;
}

void RunAbout()
{
    SayHelloCommand();
    Write(CMD_CLEAR);
    Write(CMD_GOHOME);
    Write(CMD_CURSOR_OFF);
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
            OutputBuffer[17] = NumbersBase;
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
        asm("CLRWDT");
        WriteAndClose();
    }
    
    if (screen != HOME)
    {
        button = 0;
        while (button == 0)
        {
            asm("CLRWDT");
            _delay(25);
            ButtonHit();
        }
        button = 0;
        return;
    }
    
    for (long i = 0; i < 10000; ++i)
    {
        _delay(25);
        asm("CLRWDT");
    }
    
    IOCCF = 0x0;
    SayHelloCommand();
    Write(CMD_CLEAR);
    Write(CMD_GOHOME);
    Write(CMD_CURSOR_OFF);
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
            OutputBuffer[17] = NumbersBase;
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
    
    do
    {
        asm("CLRWDT");
        GotoSleep();
        ButtonHit();
    } while (button == 0);
    button = 0;
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
        asm("CLRWDT");
        GotoSleep();
        ButtonHit();
        if (button > 3)
            screen = PARM_HOME;
        if (button == 7)
        {
            sysMode = VOLTAGE_SOURCE;
        }
        else if (button == 6)
        {
            sysMode = CURRENT_SOURCE;
        }
        else if (button == 5)
        {
            sysMode = BREAKDOWN_TEST;
        }
    }
    button = 0;
    IOCCF = 0x0;
}

void ParmMode()
{
    DrawParmUI();
    button = 0;
    while ((screen == PARM_HOME))
    {
        if (button == 7)
        {
            screen = LIM_HOME;
            button = 0;
        }
        else if (button == 6)
        {
            screen = MODE;
            ModeSelect();
            screen = PARM_HOME;
            DrawParmUI();
            button = 0;
            //return;
        }
        else if (button == 5)
        {
            screen = HOME;
            button = 0;
        }
        else if (button == 4)
        {
            button = 0;
            RunAbout();
            DrawParmUI();
        }
        asm("CLRWDT");
        GotoSleep();
        ButtonHit();
    }
}

void DrawParmUI()
{
    SayHelloCommand();
    Write(CMD_CLEAR);
    Write(CMD_GOHOME);
    Write(CMD_CURSOR_OFF);
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
    Write(CMD_CLEAR);
    Write(CMD_GOHOME);
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
            OutputBuffer[0] = LEFT_PAR;
            OutputBuffer[1] = base;
            OutputBuffer[2] = RIGHT_PAR;
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
                if (lines == 1)
                {
                    //V
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
                
            }
            base++;
        }
        WriteAndClose();
        
        //writeLargeNumber(2, 9, Vout);
        //OutputBuffer[3] = BLANK;
    }
}

void OutputModeUI()
{
    SayHelloCommand();
    Write(CMD_CURSOR_OFF);
    Write(CMD_CLEAR);
    Write(CMD_GOHOME);
    CloseLCD();
    
    //Line 1
    for (int lines = 0; lines < 4; ++lines)
    {
        ClearBuffer();
        
        if (lines < 2)
        {
            //V
            OutputBuffer[4] = 0x56;
        }
        else
        {
            //I
            OutputBuffer[4] = 0x49;
        }
        
        if (lines % 2 == 0)
        {
            //set
            OutputBuffer[5] = 0x73;
            OutputBuffer[6] = 0x65;
            OutputBuffer[7] = 0x74;
        }
        else{
            //out
            OutputBuffer[5] = 0x6F;
            OutputBuffer[6] = 0x75;
            OutputBuffer[7] = 0x74;
        }
        OutputBuffer[8] = COLON;
        //OutputBuffer[9] = BLANK;
        OutputBuffer[10] = NumbersBase;
        OutputBuffer[11] = NumbersBase;
        
        if (lines < 2)
        {
            //0000V
            OutputBuffer[12] = NumbersBase;
            OutputBuffer[13] = NumbersBase;
            OutputBuffer[14] = 0x56;
        }
        else
        {
            //00.00mA
            OutputBuffer[12] = PERIOD;
            OutputBuffer[13] = NumbersBase;
            OutputBuffer[14] = NumbersBase;
            OutputBuffer[15] = 0x6D;
            OutputBuffer[16] = 0x41;
        }

        WriteAndClose();
    }
}

void WriteAndClose()
{
    //ClearBuffer();
    SayHelloWrite();
    WriteLine();
    CloseLCD();
}