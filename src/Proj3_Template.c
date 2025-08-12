/*===================================CPEG222====================================
 * Program:      Project 3 template
 * Authors:     Timothy Mumford
 * Date:        10/19/2023
 * This is a template that you can use to write your project 3 code, for mid-stage and final demo.
==============================================================================*/
/*-------------- Board system settings. PLEASE DO NOT MODIFY THIS PART ----------*/
#ifndef _SUPPRESS_PLIB_WARNING          //suppress the plib warning during compiling
#define _SUPPRESS_PLIB_WARNING
#endif
#pragma config FPLLIDIV = DIV_2         // PLL Input Divider (2x Divider)
#pragma config FPLLMUL = MUL_20         // PLL Multiplier (20x Multiplier)
#pragma config FPLLODIV = DIV_1         // System PLL Output Clock Divider (PLL Divide by 1)
#pragma config FNOSC = PRIPLL           // Oscillator Selection Bits (Primary Osc w/PLL (XT+,HS+,EC+PLL))
#pragma config FSOSCEN = OFF            // Secondary Oscillator Enable (Disabled)
#pragma config POSCMOD = XT             // Primary Oscillator Configuration (XT osc mode)
#pragma config FPBDIV = DIV_8           // Peripheral Clock Divisor (Pb_Clk is Sys_Clk/8)
/*----------------------------------------------------------------------------*/

#include <xc.h>   //Microchip XC processor header which links to the PIC32MX370512L header
#include <stdio.h>  // need this for sprintf
#include <sys/attribs.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include "config.h" // Basys MX3 configuration header
#include "lcd.h"    // Digilent Library for using the on-board LCD
#include "acl.h"    // Digilent Library for using the on-board accelerometer
#include "ssd.h"


#define TRUE 1
#define FALSE 0

// below are keypad row and column definitions based on the assumption that JB will be used and columns are CN pins
// If you want to use JA or use rows as CN pins, modify this part
#define R4 LATCbits.LATC14
#define R3 LATDbits.LATD0
#define R2 LATDbits.LATD1
#define R1 LATCbits.LATC13
#define C4 PORTDbits.RD9
#define C3 PORTDbits.RD11
#define C2 PORTDbits.RD10
#define C1 PORTDbits.RD8
#define SSD_EMPTY_DIGIT 31

char array[4];
char n = 0;

typedef enum _KEY {K0, K1, K2, K3, K4, K5, K6, K7, K8, K9, K_A, K_B, K_C, K_D, K_E, K_F, K_NONE} eKey ;
typedef enum _MODE {MODE1,MODE2,MODE3,MODE4,READY} eModes ;

eModes mode = MODE1;

char new_press = FALSE;
int flag = 0;
int number = 0;
int number_of_players = 0;
int ttime = 2000;
int digit1 = -1;
int digit2 = -1;
int player;
int enterdigit;
char T3 = FALSE;



// subrountines
void CNConfig();
void handle_new_keypad_press(eKey key) ;


int main(void) {
      eKey key = K_NONE;
      
      
      
      

    /* Initialization of LED, LCD, SSD, etc */
    DDPCONbits.JTAGEN = 0; // Required to use Pin RA0 (connected to LED 0) as IO
    LCD_Init() ;
    ACL_Init();
    SSD_Init();

    float rgACLGVals[3];
    ACL_ReadGValues(rgACLGVals);
    int seed = rgACLGVals[0] * 10000;
    srand((unsigned) seed);
    // below are keypad row and column configurations based on the assumption that JB will be used and columns are CN pins
    // If you want to use JA or use rows as CN pins, modify this part

    // keypad rows as outputs
    TRISDbits.TRISD0 = 0;
    TRISDbits.TRISD1 = 0;
    ANSELDbits.ANSD1 = 0;
    TRISCbits.TRISC14 = 0;
    TRISCbits.TRISC13 = 0;

    // keypad columns as inputs
    TRISDbits.TRISD8 = 1;
    TRISDbits.TRISD9 = 1;
    TRISDbits.TRISD10 = 1;
    TRISDbits.TRISD11 = 1;
    
    // You need to enable all the rows
    R1 = R2 = R3 = R4 = 0;
    
    
    
    CNConfig();
   handle_new_keypad_press(key) ;
   
    /* Other initialization and configuration code */
    
    for(int i = 0; i < 4; i++){
        array[i] = 31;
    }

    }

    



void CNConfig() {
    /* Make sure vector interrupts is disabled prior to configuration */
    macro_disable_interrupts;
    
    
    CNCONDbits.ON = 1;   //all port D pins to trigger CN interrupts
    CNEND = 0xf00;      	//configure PORTD pins 8-11 as CN pins
    CNPUD = 0xf00;      	//enable pullups on PORTD pins 8-11

    IPC8bits.CNIP = 5;  	// set CN priority to  5
    IPC8bits.CNIS = 3;  	// set CN sub-priority to 3

    IFS1bits.CNDIF = 0;   	//Clear interrupt flag status bit
    IEC1bits.CNDIE = 1  ;   	//Enable CN interrupt on port D
    
    
    int j = PORTD;             //read port to clear mismatch on CN pins
    macro_enable_interrupts();	// re-enable interrupts
}


void __ISR(_CHANGE_NOTICE_VECTOR) CN_Handler(void) {
    eKey key = K_NONE;
    
    // 1. Disable CN interrupts
    IEC1bits.CNDIE = 0;     

    // 2. Debounce keys for 10ms
    for (int i=0; i<1426; i++) {}

    // 3. Handle "button locking" logic

    unsigned char key_is_pressed = (!C1 || !C2 || !C3 || !C4);
    // If a key is already pressed, don't execute the rest second time to eliminate double pressing
    if (!key_is_pressed)
    {
        new_press = FALSE;
    }
    else if (!new_press)
    {
        new_press = TRUE;

        // 4. Decode which key was pressed
        
        // check first row 
        R1 = 0; R2 = R3 = R4 = 1;
        if (C1 == 0) { key = K1; }      // 1
        else if (C2 == 0) { key = K2; } // 2
        else if (C3 == 0) { key = K3 ;} // 3
        else if (C4 == 0) { key = K_A ; } // A
//row two
        R2 = 0; R1 = R3 = R4 = 1;
        if (C1 == 0) { key = K4 ; }      // 4
        else if (C2 == 0) { key = K5;} // 5
        else if (C3 == 0) { key = K6 ; } // 6
        else if (C4 == 0) { key = K_B ; } // B
//row three
        R3 = 0; R1 = R2 = R4 = 1;
        if (C1 == 0) { key = K7 ; }      // 7
        else if (C2 == 0) { key = K8;  } // 8
        else if (C3 == 0) { key = K9 ; } // 9
        else if (C4 == 0) { key = K_C; } // C
//row four
        R4 = 0; R1 = R2 = R3 = 1;
        if (C1 == 0) { key = K0;  }      // 0
        else if (C2 == 0) { key = K_F; } // F
        else if (C3 == 0) { key = K_E;  } // E
        else if (C4 == 0) { key = K_D; } // D


        // re-enable all the rows for the next round
        R1 = R2 = R3 = R4 = 0;
    
    }
    
    // if any key has been pressed, update next state and outputs
    //if (key != K_NONE) {

    handle_new_keypad_press(key) ;
    //}
    
    
    
    int j = PORTD;              //read port to clear mismatch on CN pints
    
    // 5. Clear the interrupt flag
    IFS1bits.CNDIF = 0;     

    // 6. Reenable CN interrupts
    IEC1bits.CNDIE = 1; 
}






int secret_number;
int number_players;
int q=31, r=31;
int range_low = 0;      // Initial lower bound
int range_high = 99;
int toohigh=0;
int toolow=0;


void TimerSetup(){     
   //set period register, generates one interrupt every 3 ms
  PR3 = 39063;
  TMR3 = 0;                           //    initialize count to 0
  T3CONbits.TCKPS = 2;                //    1:64 prescale value
  T3CONbits.TGATE = 0;                //    not gated input (the default)
  T3CONbits.TCS = 0;                  //    PCBLK input (the default)
  T3CONbits.ON = 1;                   //    turn on Timer1
  IPC1bits.T1IP = 6;                  //    priority
  IPC1bits.T1IS = 3;                  //    subpriority
  IFS0bits.T3IF = 0;                  //    clear interrupt flag
  IEC0bits.T3IE = 1;                  //    enable interrupt
  macro_enable_interrupts();          //    enable interrupts at CPU
}

void __ISR(_TIMER_3_VECTOR) Timer3ISR(void){
    IEC0bits.T3IE = 0;
    
    if (T3 == TRUE){
        if (ttime>0){
            ttime--;
        if(ttime%100==0){
                if (digit2 ==-1){
                SSD_WriteDigits(digit1,31,(ttime/100)%10,(ttime/100)/10,0,0,0,0);
                }
                if (digit2!=-1){
                    SSD_WriteDigits(digit2,digit1,(ttime/100)%10,(ttime/100)/10,0,0,0,0);
                }
            }
        }
        else{
                digit1 =-1;
                digit2 =-1;
                player++;
                if(player<4){
                    player=1;
                }
                ttime = 2000;
        }
    }
    else
    {
        ttime = 2000;
        
    
    }
    IFS0bits.T3IF =0;
    IEC0bits.T3IE =1;
}

void handle_new_keypad_press(eKey key) {
    switch (mode) {
    case MODE1:
        SSD_WriteDigits(31,31,31,31,0,0,0,0);
        LCD_DisplayClear();
        LCD_WriteStringAtPos("    number guessing",0, 0);
        LCD_WriteStringAtPos("# players? 1- 4",1, 0);
    // Check if the key is a numeric digit (0-9) and update the number of players
       if(key >= 0 && key <= 4){
number_players = key; // Convert key to number of players (1-4)

        mode = MODE2; // Transition to Mode 2
                LCD_WriteStringAtPos("Rand[E] Det[D]?",1,0 );
       }

// Format the number_of_players into a string
char array[16]; // Assuming a single-digit number of players and some extra space for safety.

// Use snprintf to format the string with number_of_players.
snprintf(array, sizeof(array), "number guessing-%d", number_players);
T3 = TRUE;
// Write the formatted string to the LCD at position (0,0).
LCD_WriteStringAtPos(array, 0, 0);     
     break;   
   case MODE2:
       
   
   
       
       
        // Display "Number Guessing" and options for generating the secret number
        //LCD_WriteStringAtPos(" --", 0, 0);
        LCD_DisplayClear();
        LCD_WriteStringAtPos("    Rand[E] Det[D]?",1,0 );
        
        if (key == K_E) {
            // User chose random secret number (non-deterministic)
            // Generate the random secret number here
            // Update your display to indicate "Random secret" (optional)
            secret_number = rand() % 100;
            LCD_DisplayClear();
            LCD_WriteStringAtPos("    Random secret", 1, 0);
            mode = MODE3; // Transition to Mode 3
        } else if (key == K_D) {
            // User chose deterministic secret number
            // Use your hardcoded secret number here (replace with your actual number)
            secret_number = 47; // Replace with your hardcoded secret number
            // Update your display to indicate "Deterministic" (optional)
            LCD_DisplayClear();
            LCD_WriteStringAtPos("  Deterministic", 1, 0);
            mode = MODE3; // Transition to Mode 3
        }
        break;
    case MODE3:
    // Display "Game Started" on the LCD in MODE3
    
    LCD_DisplayClear();
    LCD_WriteStringAtPos("    Game Started", 0, 0);
    LCD_DisplayClear();


TRISA = 0x00;
LATA = 0;
    // Define the secret number and initial range
       // Initial upper bound

    // Initialize player-related variables
    int current_player = 1;   // The first player's turn
   
    
 

        // Display current player's turn using LEDs (e.g., player 1->0b0001; player 4->0b1000)
        LATA = 1 << (current_player - 1);

        // Display the remaining time and the guessed number on the SSD
        
        //char time_str[2];
        //char guess_str[2];
        //snprintf(time_str, sizeof(time_sthh   hhr), "%02d", 0);
        //snprintf(guess_str, sizeof(guess_str), "%02d", 0); // Replace 0 with the player's guessed number

        // You should display the time_str and guess_str on the SSD here

        // Prompt the player to enter a number
        if (toohigh ==0 &&  toolow ==0){
                LCD_WriteStringAtPos("    Enter your guess:", 1, 0);
        }else if (toohigh == 1){
            LCD_WriteStringAtPos("    Too high.", 1, 0);
        }else if (toolow ==1){
            LCD_WriteStringAtPos("    Too low.", 1, 0);
        }
        unsigned int guess;
        
        
        if (key == K0) {q=r, r=0;}
        else if (key == K1) {q=r, r=1;}
        else if (key == K2) {q=r,r=2;}
        else if (key == K3) {q=r,r=3;}
        else if (key == K4) {q=r,r=4;}
        else if (key == K5) {q=r,r=5;}
        else if (key == K6) {q=r,r=6;}
        else if (key == K7) {q=r,r=7;}
        else if (key == K8) {q=r,r=8;}
        else if (key == K9) {q=r,r=9;}
       
        
    int numDigits = 0;
    int multiplier = 1;

    // Calculate the number of digits in y and the multiplier
    int temp = r;
    while (temp > 0) {
        numDigits++;
        multiplier *= 10;
        temp /= 10;
    }

    // Special case when y is 0
    if (r == 0) {
        multiplier = 10;
    }

    // Create the new variable by shifting x to the left and adding y
    guess = (q * multiplier) + r;
        
SSD_WriteDigits(r,q,31,31,0,0,0,0);
if (key >=0 && key <=9){
    if(digit1== -1){
        digit1 = key;
        if(digit1 = 0){
            digit1=31;
        }
    }
    SSD_WriteDigits(digit1,31,(ttime/100)%10,(ttime/1000),0,0,0,0);
    }else if(digit2 == -1){
        digit2 = key;
        SSD_WriteDigits(digit2,digit1,(ttime/100)%10,(ttime/100)/10,0,0,0,0);
} 
 
 
if (key == K_F) {
      
    mode = MODE4;
}



 
 if (key == K_E) 
 
 if (guess == secret_number) {
                // The current player guessed correctly
                LCD_WriteStringAtPos("You got it!", 1, 0);
                mode = MODE1;
                r=17;
                        q=17;
                        LATA = 0;
                // Exit the game or return to Mode 1 when any key is pressed
                //WaitForAnyKeyPress(); // Implement this function to wait for any key press 
                return;
            } else if (guess < secret_number) {
                // The guess is too low
                
                LCD_WriteStringAtPos("Too low. ", 0, 0);
                toolow = 1;
                toohigh = 0;
                range_low = guess + 1;
            } else if (guess > secret_number){
                // The guess is too high
                
                LCD_WriteStringAtPos("Too high.", 0, 0);
                toolow = 0;
                toohigh = 1;
                range_high = guess - 1;
            }
         


    break;
case MODE4:
{
    int a, b, c, d;

    a = range_low / 10; // Extracts the tens digit
    b = range_low % 10; // Extracts the unit digit
    c = range_high / 10; // Extracts the tens digit
    d = range_high % 10;
    
    SSD_WriteDigits(d,c,b,a,0,0,0,0); 

    if (key == K_F) {

    mode = MODE3;
}

}
    
    break;
    
    }
}


void display_clear(){
    LCD_WriteStringAtPos("                 ",0,0);
    LCD_WriteStringAtPos("               ",1,0);
}
