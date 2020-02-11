/*====================================================*/
/* Jacob Parmer, Austin Harris */
/* ELEC 3040/3050 - Lab 3, Program 1 */
/* Count up or down from 0-9 on PC[0:7] depending on values of input switches */
/*====================================================*/
#include "STM32L1xx.h" /* Microcontroller information */

typedef enum {true, false} bool;

/* Define global variables */

uint16_t count;
uint16_t key_selected;
bool display_key;

/*---------------------------------------------------*/
/* Initialize GPIO pins used in the program */
/* PA1, PA2 = input switches */
/* PC[0:7] = output LED's to display value of count. */
/*---------------------------------------------------*/
void pinSetup () {

	RCC->AHBENR |= 0x01; /* Enable GPIOA clock (bit 0) */
	GPIOA->MODER &= ~(0x00000000C); /* General purpose input mode for PA1 */
	
	RCC->AHBENR |= 0x02; /*Enable GPIOB clock (bit 1) */
	GPIOB->MODER &= ~(0x0000FFFF); /* General purpose input mode for PB[0:7] */
	
	RCC->AHBENR |= 0x04; /* Enable GPIOC clock (bit 2) */
	GPIOC->MODER &= ~(0x000000FF);
	GPIOC->MODER |= 0x00000055; /* Output mode for PC[0:3] */
	
	GPIOB->PUPDR &= 0x00000000;
	GPIOB->PUPDR |= 0x0000AAAA; // pulls down pins PB[0:7]

}

void interruptSetup() {
	
	SYSCFG->EXTICR[0] &= ~(0x000F);
	
	EXTI->RTSR |= 0x000002; // allows for interrupts at clock rising edge
	EXTI->IMR |= 0x000002; // enables interrupt request lines for EXTI1.
	
	NVIC_EnableIRQ(EXTI1_IRQn); // enable external interrupt EXTI1
	NVIC_SetPriority(EXTI1_IRQn, 1); // sets priority of EXTI1
	
	__enable_irq(); // global interrupt enable

}
/*----------------------------------------------------------*/
/* Delay function - do nothing for delayTime */
/* delayTime = 20,000 = roughly a second. */
/*----------------------------------------------------------*/
void delay (int delayTime) {
	int i,j,n;
	for (i=0; i<20; i++) { //outer loop
		for (j=0; j<delayTime; j++) { //inner loop
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

void scan() {
	bool keypress_found = false;
	uint16_t keypad[4][4] = {
		{0x0001, 0x0002, 0x0003, 0x000A},
		{0x0004, 0x0005, 0x0006, 0x000B},
		{0x0007, 0x0008, 0x0009, 0x000C},
		{0x000E, 0x0000, 0x000F, 0x000D}
	};
	uint16_t row_selected;
	int i;
	
	GPIOB->PUPDR &= 0x00000000;
	GPIOB->PUPDR |= 0x55565555;	// pulls up all columns and rows except for PB[8], which is pulled down
	
	GPIOB->PUPDR = GPIOB->PUPDR>>2; // shifts pulldown register to the right by 2. Making PB[7] pulled down.
	
	for (i=4; i>0; i--) { // loops through 4 columns from right to left
		
		delay(10);
		
		switch (GPIOB->IDR & 0x000F) {
			case 0x000E:
				row_selected = 0;
				keypress_found = true;
				break;
			case 0x000D:
				row_selected = 1;
				keypress_found = true;
				break;
			case 0x000B:
				row_selected = 2;
				keypress_found = true;
				break;
			case 0x0007:
				row_selected = 3;
				keypress_found = true;
				break;
			default:
				break;
		}
		
		if (keypress_found == true) {
			key_selected = keypad[row_selected][i-1];
			display_key = true;
			keypress_found = false;
		}
			
			GPIOB->PUPDR = GPIOB->PUPDR>>2;
	} 
}
void EXTI1_IRQHandler(void) {

	scan();
	delay(200); // delay to prevent retriggering interrupt due to bouncing
	
	GPIOB->PUPDR &= 0x00000000;
	GPIOB->PUPDR |= 0x0000AAAA; // pulls down pins PB[0:7]
	
	EXTI->PR |= 0x000002; // clears interrupt pending bits
	NVIC_ClearPendingIRQ(EXTI1_IRQn);
}
/*------------------------------------------------*/
/* Main program */
/*------------------------------------------------*/
int main(void) {

	pinSetup(); //Configure GPIO pins
	interruptSetup(); // Configure interrupt pins
	count = 0x0001;
	display_key = false;
	int key_display_timer = 0;

 /* Endless loop */
	while (1) {
		
		counter();
		
		if (display_key == true)
		{	
			if (key_display_timer != 5)
			{	
				GPIOC->ODR &= 0x00;
				GPIOC->ODR &= key_selected;
				key_display_timer++;
				delay(20000);
			} else
			{			
				GPIOC->ODR &= 0x00;
				GPIOC->ODR &= count;
				key_display_timer = 0;
				display_key = false;
				delay(20000);			
			}
		} else
		{
			GPIOC->ODR &= 0x00;
			GPIOC->ODR |= count; // Outputs count to pins PC[0:3].
			delay(20000);			
		}
	} /* repeat forever */
}
