#include <msp430.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "LiquidCrystal_I2C.h"

#define MAX_MESSAGE_LENGTH 12  // Define maximum length for received message

volatile char message[MAX_MESSAGE_LENGTH];  // Define buffer for received message
volatile unsigned int msg_counter = 0;      // Initialize message counter
volatile unsigned int new_msg = 0;          // Initialize flag for new message
volatile unsigned int DutyCycle = 0;        // Initialize variable for duty cycle

// Function to initialize Bluetooth communication
void bluetooth_init() {
    UCA1CTLW0 |= UCSWRST;              // Put eUSCI in reset
    UCA1CTLW0 |= UCSSEL__SMCLK;        // Clock source SMCLK
    UCA1BR0 = 8;                       // 1000000/115200 = 8.68
    UCA1MCTLW = 0x0D600;               // 1000000/115200 - INT(1000000/115200) = 0.68
    P4SEL1 &= ~BIT2;                   // Configure UART pins
    P4SEL0 |= BIT2;
    P4SEL1 &= ~BIT3;                   // Configure UART pins
    P4SEL0 |= BIT3;
    UCA1CTLW0 &= ~UCSWRST;             // Release USCI from reset state
    UCA1IE |= UCRXIE;                  // Enable USCI A0 RX interrupt
}

// Function to initialize LCD display
void lcd_init() {
    I2C_Init(0x27);                   // Initialize I2C with appropriate LCD address
    LCD_Setup();                      // Setup LCD
    LCD_ClearDisplay();               // Clear LCD display
    LCD_SetCursor(0, 0);              // Set cursor to first line, first column
    LCD_Write("Speed:");              // Display initial message on LCD
}

// Function to display PWM value on LCD
void display_message(unsigned int pwmValue) {
    char displayBuffer[20];           // Buffer to format the display message
    if (pwmValue > 100) {
        pwmValue = 100;               // Clamp PWM value at 100%
    }
    sprintf(displayBuffer, "Speed:%d%%", pwmValue); // Use %d instead of %u
    LCD_ClearDisplay();               // Clear LCD display
    LCD_SetCursor(0, 0);              // Set cursor to first line, first column
    LCD_Write(displayBuffer);         // Display PWM value on LCD
    __delay_cycles(50000);            // Small delay to ensure display updates correctly
}

// PWM and motor control setup
void pwm_motor_setup(void) {
    P4DIR |= BIT0;                    // Set motor control pin as output
    P4OUT &= ~BIT0;                   // Clear motor control pin

    // Configure Timer B
    TB0CTL |= TBCLR;                  // Clear timer settings
    TB0CTL |= MC__UP;                 // Set up mode
    TB0CTL |= TBSSEL__ACLK;           // Use ACLK as timer clock source
    TB0CCR0 = 500;                    // Set timer period for PWM
    TB0CCR1 = 0;                      // Start with duty cycle of 0%

    // Enable timer interrupts
    TB0CCTL0 |= CCIE;
    TB0CCTL1 |= CCIE;
    TB0CCTL0 &= ~(CCIFG);
    TB0CCTL1 &= ~(CCIFG);
}

// Main function
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;         // Stop watchdog timer
    bluetooth_init();                 // Initialize Bluetooth (or UART) communication
    lcd_init();                       // Initialize LCD display
    pwm_motor_setup();                // Setup PWM and motor control
    PM5CTL0 &= ~LOCKLPM5;             // Unlock GPIO
    __enable_interrupt();             // Enable interrupts

    while (1) {
        if (new_msg) {                // Check if new message is received
            DutyCycle = atoi(message);  // Extract PWM value from message
            display_message(DutyCycle); // Display received PWM value on LCD
            TB0CCR1 = (DutyCycle == 100) ? TB0CCR0 - 1 : DutyCycle * 5;  // Set PWM duty cycle
            new_msg = 0;               // Clear new message flag
        }
    }
    return 0;
}

// Timer interrupt service routines
#pragma vector = TIMER0_B0_VECTOR
__interrupt void TMR0() {
    P4OUT &= ~1;  // Turn off motor control pin
    TB0CCTL0 &= ~(CCIFG);  // Clear interrupt flag
}

#pragma vector = TIMER0_B1_VECTOR
__interrupt void TMR1() {
    P4OUT |= 1;  // Turn on motor control pin
    TB0CCTL1 &= ~(CCIFG);  // Clear interrupt flag
}

// Interrupt service routine for UART communication
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void) {
    switch (UCA1IV) {                 // Check interrupt vector
        case USCI_NONE: break;        // No interrupt
        case USCI_UART_UCRXIFG:       // Receive interrupt
            message[msg_counter++] = UCA1RXBUF;  // Read received character
            if (message[msg_counter - 1] == '\n') {  // If newline character is received
                message[msg_counter - 1] = '\0';     // Terminate string
                msg_counter = 0;                     // Reset message counter
                new_msg = 1;                         // Set new message flag
            } else if (msg_counter >= MAX_MESSAGE_LENGTH - 1) {  // Prevent buffer overflow
                msg_counter = 0;                     // Reset message counter
            }
            break;
        case USCI_UART_UCTXIFG: break;  // Transmit interrupt
        case USCI_UART_UCSTTIFG: break;  // Start bit interrupt
        case USCI_UART_UCTXCPTIFG: break;  // Transmit complete interrupt
    }
}