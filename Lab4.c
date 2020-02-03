/*====================================================*/
/* Jacob Parmer, Austin Harris */
/* ELEC 3040/3050 - Lab 3, Program 1 */
/* Count up or down from 0-9 on PC[0:7] depending on values of input switches */
/*====================================================*/
#include "STM32L1xx.h" /* Microcontroller information */

typedef enum {true, false} bool;

/* Define global variables */

uint16_t count;
uint16_t count2;
uint16_t sw1;
uint16_t sw2;
bool count_increasing = true;

/*---------------------------------------------------*/
/* Initialize GPIO pins used in the program */
/* PA1, PA2 = input switches */
/* PC[0:7] = output LED's to display value of count. */
/*---------------------------------------------------*/
void pinSetup () {

	RCC->AHBENR |= 0x01; /* Enable GPIOA clock (bit 0) */
	GPIOA->MODER &= ~(0x00000000F); /* General purpose input mode for PA0 and PA1 */
	
	RCC->AHBENR |= 0x04; /* Enable GPIOC clock (bit 2) */
	GPIOC->MODER &= ~(0x0000FFFF);
	GPIOC->MODER |= 0x00005555; /* Output mode for PC[0:7] */

}

void interruptSetup() {
	
	SYSCFG->EXTICR[0] &= ~(0x00FF);
	
	EXTI->RTSR |= 0x000003; // allows for interrupts at clock rising edge
	EXTI->IMR |= 0x000003; // enables interrupt request lines for EXTI0 and EXTI1.
	
	NVIC_EnableIRQ(EXTI0_IRQn); // enable external interrupt EXTI0 and EXTI1
	NVIC_EnableIRQ(EXTI1_IRQn);
	NVIC_SetPriority(EXTI0_IRQn, 1); // sets priority of EXTI0 to higher than EXTI1
	NVIC_SetPriority(EXTI1_IRQn, 2);
	
	__enable_irq(); // global interrupt enable

}
/*----------------------------------------------------------*/
/* Delay function - do nothing for about 0.5 seconds */
/*----------------------------------------------------------*/
void delay () {
	int i,j,n;
	for (i=0; i<20; i++) { //outer loop
		for (j=0; j<10000; j++) { //inner loop
			n = j; //dummy operation for single-step test
		} //do nothing
	}
}

/*---------------------------------------------------*/
/* Increments count from 0 to 9 */
/* count rolls over after hitting 0 or 9. */
/*---------------------------------------------------*/
void counter() {
		if (count < 0x0009) { // checks to see if current value of count is less than 9.
			count++;
		} else { // if it isn't, reset count to 0
			count = 0x0000;
		}
}

/*---------------------------------------------------*/
/* Increments or decrements count depending on value of direction. */
/* Order is opposite of counter(). */
/* count rolls over after hitting 0 or 9. */
/*---------------------------------------------------*/
void counter2() {
	if (count_increasing == true) {
		if (count2 < 0x0090) { // checks to see if current value of count2 is less than 9.
			count2 = count2 + 0x0010;
		} else { // if it isn't, reset count2 to 0.
			count2 = 0x0000;
		}
	} else {
		if (count2 > 0x0000) { // checks to see if current value of count2 is not zero
			count2 = count2 - 0x0010;
		} else { // if it is 0, set count2 to 9.
			count2 = 0x0090;
		}
	}
}

void EXTI0_IRQHandler(void) {
	count_increasing = false;
	EXTI->PR |= 0x000001;
	NVIC_ClearPendingIRQ(EXTI0_IRQn);
}

void EXTI1_IRQHandler(void) {
	count_increasing = true;
	EXTI->PR |= 0x000002;
	NVIC_ClearPendingIRQ(EXTI1_IRQn);
}
/*------------------------------------------------*/
/* Main program */
/*------------------------------------------------*/
int main(void) {

	pinSetup(); //Configure GPIO pins
	interruptSetup(); // Configure interrupt pins
	count = 0x0001;
	count2 = 0x0010;
	bool count2_enable = false;
	
 /* Endless loop */
	while (1) {
		sw1 = GPIOA->IDR & 0x0001;
		sw2 = GPIOA->IDR & 0x0002;
		
		counter();
		if (count2_enable == true) { // this makes it so count2 is only incremented once every 2 iterations of the while loop
			counter2();								 // hence, a 1 second delay for count2, and a 1/2 second delay for count
			count2_enable = false;
		} else {
			count2_enable = true;
		}
		GPIOC->ODR &= 0x00;
		GPIOC->ODR |= (count | count2); // Outputs count to pins PC[0:3], and count2 to pins PC[4:7].
		delay();
	} /* repeat forever */
}
