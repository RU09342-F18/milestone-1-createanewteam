#include <msp430.h> 
/**
 * Kyle McKeown and Anwar Hussein
 * 10/15/2018
 * Milestone 1 - Addressing RGB LED's
 * Hardware PWM
 * main.c
 */

#define red            BIT2             // definition for the red LED
#define green          BIT3             // definition for the green LED
#define blue           BIT4             // definition for the blue LED
volatile int bit = 0;                   // to keep track of the current bit
volatile int packet_size = 0;           // to keep track of the packet size

void UARTinit(void);                    // initialization of the UART
void LEDinit(void);                     // initialization of the LED
void TIMERinit(void);                   // initialization of the TIMERS

void UARTinit(void)                     // function to initialize the UART interface
{
    P3SEL    |=  BIT4;                  // UART Receive
    P3SEL    |=  BIT3;                  // UART Transfer
    P4SEL    |=  (BIT4 + BIT5);         // for the UART communication
    UCA1CTL1 |=  UCSWRST;               // Resets state machine
    UCA1CTL1 |=  UCSSEL_2;              // SMCLK
                                        // For fBRCLK=1MHz, BR=9600: N=1000000/9600 = 104,16666667
    UCA1BR0   =  6;                     // 9600 Baud Rate
    UCA1BR1   =  0;                     // 9600 Baud Rate
    UCA1MCTL |=  UCBRS_0;               // PWM
    UCA1MCTL |=  UCBRF_13;              // PWM
    UCA1MCTL |=  UCOS16;                // PWM
    UCA1CTL1 &= ~UCSWRST;               // Initializes the state machine
    UCA1IE   |=  UCRXIE;                // Enables USCI_A0 RX Interrupt
}

void LEDinit(void)                      // function to initialize the LED interface
{
    P1DIR |=  (red + green + blue);     // P1.2 to output
    P1SEL |=  (red + green + blue);     // P1.2 to TA0.1, P1.3 to TA0.2, P1.4 to TA0.3
    P4DIR |=  BIT7;                     // Indicator LED to output
    P4OUT &= ~BIT7;                     // Indicator LED set to off
}

void TIMERinit(void)                    // function to initialize the TIMERS
{
                                        // need to use four timers TA0 TA1 TA2 TA3
    TA0CCR0  = 0x00FF;                  // Sets CCR0 to 255
    TA0CCTL1 = OUTMOD_7;                // Reset/Set behavior
    TA0CCTL2 = OUTMOD_7;                // Reset/Set behavior
    TA0CCTL3 = OUTMOD_7;                // Reset/Set behavior
    TA0CTL   = TASSEL_2 + MC_1 + ID_3;  // Timer A clock source select: 2 - SMCLK, up- mode , divide by 8
}


void main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	        // stop watchdog timer
	
    UARTinit();                         // initialization of the UART
    LEDinit();                          // initialization of the LED
    TIMERinit();                        // initialization of the TIMERS

    //UCA1TXBUF = 0xAA;

	__bis_SR_register(GIE);             // interrupts enabled
	while(1);                           // replaced the low power mode previously ran here

}


#pragma vector=USCI_A1_VECTOR           // USCI vector interrupt definition
__interrupt void USCI_A1_ISR(void)
{
    P4OUT |= BIT7;                      // toggles the indicator light to verify interrupt has been reached

    switch(bit)                         // processor sending one bit at a time
    {

    case 0:
        while(!(UCA1IFG & UCTXIFG));    // As long as you're not already transmitting continue
        packet_size = UCA1RXBUF;        // calculate and send length byte
        UCA1TXBUF = (packet_size - 3);  // subtract three from the packet size to transmit
        break;
    case 1:
        TA0CCR1 = UCA1RXBUF;            // set red LED PWM value
        break;
    case 2:
        TA0CCR2 = UCA1RXBUF;            // set green LED PWM value
        break;
    case 3:
        TA0CCR3 = UCA1RXBUF;            // set blue LED PWM value

        break;
    default:
        while(!(UCA1IFG & UCTXIFG));
        UCA1TXBUF = UCA1RXBUF;          // send the rest of the data to the next node
        break;

    }

    if(bit != (packet_size))            // increment reset bit counter
        {                               // If the end not reached
            bit += 1;                   // Increment the bit value
        }
        else
        {                               // the end byte is reached
            P4OUT &= ~BIT7;             // Turn off LED indicator
            bit = 0;                    // Resets the bit value
        }

}
