#include <msp430.h>
#include <stdio.h>
#include "LiquidCrystal_I2C.h"
#include "speed_protocol.h"

#define COMMAND_SIZE 5
#define PWM_PERIOD 500
#define MOTOR_PIN BIT0

volatile char received[COMMAND_SIZE];
volatile unsigned int rxCount = 0;
volatile unsigned char commandReady = 0;
volatile unsigned char overflow = 0;

void setupUart(void) {
    UCA1CTLW0 = UCSWRST | UCSSEL__SMCLK;
    UCA1BR0 = 8;              // 1 MHz SMCLK, 115200 baud
    UCA1MCTLW = 0xD600;
    P4SEL1 &= ~(BIT2 | BIT3);
    P4SEL0 |= BIT2 | BIT3;
    UCA1CTLW0 &= ~UCSWRST;
    UCA1IE |= UCRXIE;
}

void setupMotor(void) {
    P4DIR |= MOTOR_PIN;
    P4OUT &= ~MOTOR_PIN;
    TB0CTL = TBCLR | TBSSEL__ACLK | MC__UP;
    TB0CCR0 = PWM_PERIOD;
    TB0CCR1 = 0;
    TB0CCTL0 = CCIE;
    TB0CCTL1 = 0;
}

void showSpeed(unsigned int value) {
    char label[17];
    sprintf(label, "Speed: %u%%", value);
    LCD_ClearDisplay();
    LCD_SetCursor(0, 0);
    LCD_Write(label);
}

void setMotorSpeed(unsigned int value) {
    // Keep the pin at a fixed level for the endpoint settings.
    TB0CCTL1 &= ~CCIE;
    if (value == 0) {
        P4OUT &= ~MOTOR_PIN;
    } else if (value == 100) {
        P4OUT |= MOTOR_PIN;
    } else {
        TB0CCR1 = value * PWM_PERIOD / 100;
        TB0CCTL1 &= ~CCIFG;
        TB0CCTL1 |= CCIE;
    }
    showSpeed(value);
}

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;
    setupUart();
    setupMotor();
    PM5CTL0 &= ~LOCKLPM5;
    I2C_Init(0x27);
    LCD_Setup();
    showSpeed(0);
    __enable_interrupt();

    while (1) {
        char copy[COMMAND_SIZE];
        unsigned char ready = 0;
        unsigned int i;
        __disable_interrupt();
        if (commandReady) {
            for (i = 0; i < COMMAND_SIZE; ++i) copy[i] = received[i];
            commandReady = 0;
            ready = 1;
        }
        __enable_interrupt();
        if (ready) {
            int speed = parse_speed(copy);
            if (speed >= 0) setMotorSpeed((unsigned int)speed);
        }
    }
}

#pragma vector=TIMER0_B0_VECTOR
__interrupt void timerPeriod(void) {
    // Active for the start of each PWM cycle.
    if (TB0CCTL1 & CCIE) P4OUT |= MOTOR_PIN;
}

#pragma vector=TIMER0_B1_VECTOR
__interrupt void timerDuty(void) {
    switch (__even_in_range(TB0IV, TB0IV_TBIFG)) {
        case TB0IV_TBCCR1:
            P4OUT &= ~MOTOR_PIN;
            break;
        default:
            break;
    }
}

#pragma vector=USCI_A1_VECTOR
__interrupt void uartReceive(void) {
    if (UCA1IV != USCI_UART_UCRXIFG) return;
    char ch = (char)UCA1RXBUF;
    if (ch == '\r') return;
    if (ch == '\n') {
        if (!overflow && rxCount && !commandReady) {
            received[rxCount] = '\0';
            commandReady = 1;
        }
        rxCount = 0;
        overflow = 0;
    } else if (!commandReady) {
        if (rxCount < COMMAND_SIZE - 1) received[rxCount++] = ch;
        else overflow = 1;
    }
}