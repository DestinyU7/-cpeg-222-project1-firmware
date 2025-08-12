/*================================================================================
 * Program:      Project 3 template
 * Authors:     [Timothy M, Griffin T]
 * Date:        10/19/2023
 * Description: Implementation of a number-guessing game for a microcontroller,
 *              specifically targeted at a PIC32MX370512L. The game involves
 *              multiple modes, player input through a keypad, and output through
 *              an LCD display and seven-segment display (SSD). The game progresses
 *              through different stages, allowing players to input their guesses
 *              and receive feedback on whether the guess is too high, too low, or correct.
================================================================================*/


/*-------------- Board system settings. PLEASE DO NOT MODIFY THIS PART ----------*/
#ifndef _SUPPRESS_PLIB_WARNING
#define _SUPPRESS_PLIB_WARNING
#endif
#pragma config FPLLIDIV = DIV_2
#pragma config FPLLMUL = MUL_20
#pragma config FPLLODIV = DIV_1
#pragma config FNOSC = PRIPLL
#pragma config FSOSCEN = OFF
#pragma config POSCMOD = XT
#pragma config FPBDIV = DIV_8
/*----------------------------------------------------------------------------*/

#include <xc.h>
#include <stdio.h>
#include <sys/attribs.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include "config.h"
#include "lcd.h"
#include "acl.h"
#include "ssd.h"

#define TRUE 1
#define FALSE 0

// Keypad definitions
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

typedef enum _KEY { K0, K1, K2, K3, K4, K5, K6, K7, K8, K9, K_A, K_B, K_C, K_D, K_E, K_F, K_NONE } eKey;
typedef enum _MODE { MODE1, MODE2, MODE3, MODE4, READY } eModes;

eModes mode = MODE1;

char new_press = FALSE;
int flag = 0;
int number = 0;
int number_of_players = 0;

// Subroutines
void CNConfig();
void handle_new_keypad_press(eKey key);

int main(void) {
    eKey key = K_NONE;

    /* Initialization of LED, LCD, SSD, etc */
    DDPCONbits.JTAGEN = 0;
    LCD_Init();
    ACL_Init();
    SSD_Init();

    float rgACLGVals[3];
    ACL_ReadGValues(rgACLGVals);
    int seed = rgACLGVals[0] * 10000;
    srand((unsigned)seed);

    // Keypad row and column configurations
    TRISDbits.TRISD0 = 0;
    TRISDbits.TRISD1 = 0;
    ANSELDbits.ANSD1 = 0;
    TRISCbits.TRISC14 = 0;
    TRISCbits.TRISC13 = 0;

    TRISDbits.TRISD8 = 1;
    TRISDbits.TRISD9 = 1;
    TRISDbits.TRISD10 = 1;
    TRISDbits.TRISD11 = 1;

    R1 = R2 = R3 = R4 = 0;

    CNConfig();
    handle_new_keypad_press(key);

    /* Other initialization and configuration code */

    for (int i = 0; i < 4; i++) {
        array[i] = 31;
    }
}

void CNConfig() {
    // Configure Change Notification (CN) for keypad
    macro_disable_interrupts;

    CNCONDbits.ON = 1;
    CNEND = 0xf00;
    CNPUD = 0xf00;

    IPC8bits.CNIP = 5;
    IPC8bits.CNIS = 3;

    IFS1bits.CNDIF = 0;
    IEC1bits.CNDIE = 1;

    int j = PORTD;
    macro_enable_interrupts();
}

void __ISR(_CHANGE_NOTICE_VECTOR) CN_Handler(void) {
    eKey key = K_NONE;

    // 1. Disable CN interrupts
    IEC1bits.CNDIE = 0;

    // 2. Debounce keys for 10ms
    for (int i = 0; i < 1426; i++) {}

    // 3. Handle "button locking" logic
    unsigned char key_is_pressed = (!C1 || !C2 || !C3 || !C4);

    // If a key is already pressed, don't execute the rest second time to eliminate double pressing
    if (!key_is_pressed) {
        new_press = FALSE;
    }
    else if (!new_press) {
        new_press = TRUE;

        // 4. Decode which key was pressed
        // Check each row and column to identify the pressed key
        // Update the 'key' variable accordingly
        R1 = 0; R2 = R3 = R4 = 1;
        if (C1 == 0) { key = K1; }
        else if (C2 == 0) { key = K2; }
        else if (C3 == 0) { key = K3; }
        else if (C4 == 0) { key = K_A; }

        R2 = 0; R1 = R3 = R4 = 1;
        if (C1 == 0) { key = K4; }
        else if (C2 == 0) { key = K5; }
        else if (C3 == 0) { key = K6; }
        else if (C4 == 0) { key = K_B; }

        R3 = 0; R1 = R2 = R4 = 1;
        if (C1 == 0) { key = K7; }
        else if (C2 == 0) { key = K8; }
        else if (C3 == 0) { key = K9; }
        else if (C4 == 0) { key = K_C; }

        R4 = 0; R1 = R2 = R3 = 1;
        if (C1 == 0) { key = K0; }
        else if (C2 == 0) { key = K_F; }
        else if (C3 == 0) { key = K_E; }
        else if (C4 == 0) { key = K_D; }

        // Re-enable all the rows for the next round
        R1 = R2 = R3 = R4 = 0;
    }

    // If any key has been pressed, update the next state and outputs
    if (key != K_NONE) {
        handle_new_keypad_press(key);
    }

    int j = PORTD; // Read port to clear mismatch on CN pins

    // 5. Clear the interrupt flag
    IFS1bits.CNDIF = 0;

    // 6. Re-enable CN interrupts
    IEC1bits.CNDIE = 1;
}

int secret_number;
int number_players;
int q = 17, r = 17;
int range_low = 0;
int range_high = 99;
int toohigh = 0;
int toolow = 0;

// Mode 2: User input handling and initial game setup
void handle_new_keypad_press(eKey key) {
    switch (mode) {
    case MODE1:
        // Display the number of players selection on LCD
        // Check if the key is a numeric digit (0-9) and update the number of players
        if (key >= K1 && key <= K4) {
            int x = 0;
            if (key == K1) { x = 1; }
            else if (key == K2) { x = 2; }
            else if (key == K3) { x = 3; }
            else if (key == K4) { x = 4; }

            number_players = x; // Convert key to the number of players (1-4)

            // Format the number_of_players into a string
            char array[16];

            // Use snprintf to format the string with number_of_players.
            snprintf(array, sizeof(array), "    # guess %d", number_players);
            LCD_DisplayClear();

            // Write the formatted string to the LCD at position (0,0).
            LCD_WriteStringAtPos(array, 0, 0);
            LCD_WriteStringAtPos("Rand[E] Det[D]?", 1, 0);
            mode = MODE2; // Transition to Mode 2
        }
        break;

    case MODE2:
        // Display "Number Guessing" and options for generating the secret number
        LCD_WriteStringAtPos("Rand[E] Det[D]?", 1, 0);

        if (key == K_E) {
            // User chose a random secret number (non-deterministic)
            secret_number = rand() % 100;
            LCD_DisplayClear();
            LCD_WriteStringAtPos("    Random", 1, 0);
            mode = MODE3; // Transition to Mode 3

            // Display appropriate message based on previous game state
            if (toohigh == 0 && toolow == 0) {
                LCD_WriteStringAtPos("Enter your guess:", 1, 0);
            }
            else if (toohigh == 1) {
                LCD_WriteStringAtPos("Too high       ", 1, 0);
            }
            else if (toolow == 1) {
                LCD_WriteStringAtPos("Too low         ", 1, 0);
            }
        }
        else if (key == K_D) {
            // User chose a deterministic secret number
            secret_number = 47; // Replace with your hardcoded secret number
            LCD_DisplayClear();
            LCD_WriteStringAtPos("    Deterministic", 1, 0);
            mode = MODE3; // Transition to Mode 3

            // Display appropriate message based on previous game state
            if (toohigh == 0 && toolow == 0) {
                LCD_WriteStringAtPos("Enter your guess:", 1, 0);
            }
            else if (toohigh == 1) {
                LCD_WriteStringAtPos("Too high          ", 1, 0);
            }
            else if (toolow == 1) {
                LCD_WriteStringAtPos("Too low          ", 1, 0);
            }
        }
        break;

    case MODE3:
        // Display "Game Started" on the LCD in MODE3
        LCD_DisplayClear();
        LCD_WriteStringAtPos("      Game Started", 0, 0);
        SSD_WriteDigits(17, 17, 17, 17, 0, 0, 0, 0);

        TRISA = 0x00;
        LATA = 0;

        // Define the secret number and initial range
        int current_player = 1;   // The first player's turn

        // Display current player's turn using LEDs (e.g., player 1->0b0001; player 4->0b1000)
        LATA = 1 << (current_player - 1);

        // Display the remaining time and the guessed number on the SSD
        SSD_WriteDigits(17,17,17,17,0,0,0,0);
        //char time_str[2];
        //char guess_str[2];
        //snprintf(time_str, sizeof(time_str), "%02d", 0);
        //snprintf(guess_str, sizeof(guess_str), "%02d", 0); // Replace 0 with the player's guessed number

        // You should display the time_str and guess_str on the SSD here

        // Prompt the player to enter a number
        if (toohigh ==0 &&  toolow ==0){
                LCD_WriteStringAtPos("Enter your guess:", 1, 0);
        }else if (toohigh == 1){
            LCD_WriteStringAtPos("Too high         ", 1, 0);
        }else if (toolow ==1){
            LCD_WriteStringAtPos("Too low           ", 1, 0);
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
        
 SSD_WriteDigits(r,q,17,17,0,0,0,0); 
 
 
if (key == K_F) {
      
    mode = MODE4;
    int a, b, c, d;

    a = range_low / 10; // Extracts the tens digit
    b = range_low % 10; // Extracts the unit digit
    c = range_high / 10; // Extracts the tens digit
    d = range_high % 10;
    
    SSD_WriteDigits(d,c,b,a,0,0,0,0); 
    
}



 
 if (key == K_E) 
 
 if (guess == secret_number) {
                // The current player guessed correctly
                LCD_WriteStringAtPos("You got it!         ", 1, 0);
                mode = MODE1;
                r=17;
                        q=17;
                        LATA = 0;
                        range_low = 0;      // Initial lower bound
                        range_high = 99;
                        toohigh=0;
                        toolow=0;
                // Exit the game or return to Mode 1 when any key is pressed
                //WaitForAnyKeyPress(); // Implement this function to wait for any key press 
                return;
            } else if (guess < secret_number) {
                // The guess is too low
                LCD_WriteStringAtPos("Too low          ", 1, 0);
                toolow = 1;
                toohigh = 0;
                range_low = guess + 1;
            } else if (guess > secret_number){
                // The guess is too high
                LCD_WriteStringAtPos("Too high        ", 1, 0);
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
    SSD_WriteDigits(r,q,17,17,0,0,0,0); 
}

}
    
    break;
    
    }
}





