/*====================================================*/
/* Jacob Parmer, Austin Harris							   */
/* ELEC 3040/3050 - Lab 6=7, Program 1 						*/
/* Generate PWM waveform using timer and keypad to control */
/*====================================================*/
#include "STM32L1xx.h" /* Microcontroller information */

typedef enum {false, true} bool;

/* Define global variables */

uint16_t key_selected;
uint16_t duty;
/*---------------------------------------------------*/
/* Initialize GPIO pins used in the program */
/* PA1 = keypad interrupt input */
/* PA6 = PWM waveform output */
/* PB[0:7] = Keypad input/output rows and columns */
/* PC[0:3] = BCD output of selected PWM */
/*---------------------------------------------------*/
void pinSetup () {

	RCC->AHBENR |= 0x07; /* Enable GPIOA clock (bits 2-0) */
	
	GPIOA->MODER &= ~(0x00000300C); /* General purpose input mode for PA1 */
	GPIOA->MODER |= 0x000002000; // AF mode for PA6
	
	GPIOA->AFR[0] &= ~(0x0F000000); // clear AFRL6
	GPIOA->AFR[0] |= 0x03000000; // PA6 = AF3 
	
	GPIOB->MODER &= ~(0x0000FFFF); /* General purpose input mode for PB[0:7] */
	GPIOB->MODER |= 0x00005500; /* Output mode for PB[4:7] */
	
	GPIOB->PUPDR &= 0x00000000;
	GPIOB->PUPDR |= 0x00000055; // pulls up pins PB[0:3]
	GPIOB->ODR &= ~(0x00F0);
	
	GPIOC->MODER &= ~(0x000000FF);
	GPIOC->MODER |= 0x00000055; /* Output mode for PC[0:3] */

}

/*--------------------------------------------------------*/
/* Initialize interrupt pins needed to trigger interrupts */
/* Sets up interrupts EXTI1_IRQ and TIM10_IRQ             */
/*--------------------------------------------------------*/
void interruptSetup() {
	
	SYSCFG->EXTICR[0] &= ~(0x000F);
	
	EXTI->FTSR |= 0x000002; // allows for interrupts at clock falling edge
	EXTI->IMR |= 0x000002; // enables interrupt request lines for EXTI1.
	
	NVIC_EnableIRQ(EXTI1_IRQn); // enable external interrupt EXTI1
	NVIC_SetPriority(EXTI1_IRQn, 1); // sets priority of EXTI1
	
	NVIC_EnableIRQ(TIM10_IRQn); // enable external interrupt TIM10
	NVIC_SetPriority(TIM10_IRQn, 2); // sets priority of TIM10
	
	__enable_irq(); // global interrupt enable

}

/*--------------------------------------------------------*/
/* Initializes timer enables and prescales					 */
/*--------------------------------------------------------*/
void timerSetup() {
	
	RCC->APB2ENR |= RCC_APB2ENR_TIM10EN;	// enables clock timer
	TIM10->DIER |= TIM_DIER_UIE;  // enables TIM10
	TIM10->DIER |= TIM_DIER_CC1IE; // enabled capture/compare interrupt

	TIM10->CCER |= 0x0001;
	
	TIM10->ARR &= 0x0000;
	TIM10->ARR |= 0x001F; // sets Auto Reload value
	
	TIM10->PSC &= 0x0000;
	TIM10->PSC |= 0x000F; // sets Prescale value
	
	TIM10->CCMR1 &= ~(0x73);
	TIM10->CCMR1 |= 0x60;
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
/* Scans keypad to determine what key was pressed    */
/* Called during interrupt triggered by keypress     */
/*---------------------------------------------------*/
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
	
	GPIOB->MODER &= ~(0x0000FFFF); /* General purpose input mode for PB[0:7] */
	GPIOB->MODER |= 0x00005500; /* Output mode for PB[4:7] */
	
	
	GPIOB->PUPDR &= 0x000000FF;
	GPIOB->ODR = 0xFF7F;
	
	for (i=0; i<4; i++) { // loops through 4 columns from right to left
		
		delay(5);
		
		switch (GPIOB->IDR & 0x000F) {
			case 0x000E:
				row_selected = 3;
				keypress_found = true;
				break;
			case 0x000D:
				row_selected = 2;
				keypress_found = true;
				break;
			case 0x000B:
				row_selected = 1;
				keypress_found = true;
				break;
			case 0x0007:
				row_selected = 0;
				keypress_found = true;
				break;
			default:
				break;
		}
		
		if (keypress_found == true) {
			key_selected = keypad[row_selected][i];
			keypress_found = false;
			
			duty = (uint16_t) (TIM10->ARR * (key_selected / 0x000A)); // duty = ARR * (0:10 / 10)
			TIM10->CCR1 = duty;
			GPIOC->ODR &= ~(0x000F);
			GPIOC->ODR |= key_selected;
		}
			
		GPIOB->ODR = GPIOB->ODR>>1;
	} 
}

/*---------------------------------------------------*/
/* Interrupt triggered by keypad					        */
/* Initializes scan, resets pins to original state   */
/*---------------------------------------------------*/
void EXTI1_IRQHandler(void) {

	scan();
	delay(500); // delay to prevent retriggering interrupt due to bouncing
	
	pinSetup();

	EXTI->PR |= 0x000002; // clears interrupt pending bits
	NVIC_ClearPendingIRQ(EXTI1_IRQn);
}

/*---------------------------------------------------*/
/* */
void TIM10_IRQHandler(void) {
	
	TIM10->SR &= ~TIM_SR_UIF;
	TIM10->SR &= ~TIM_SR_CC1IF;
	EXTI->PR |= 0x000002; // clears interrupt pending bits
	NVIC_ClearPendingIRQ(TIM10_IRQn);
	
}

/*------------------------------------------------*/
/* 					Main program					  	  */
/*------------------------------------------------*/
int main(void) {

	pinSetup(); // Configure GPIO pins
	interruptSetup(); // Configure interrupt pins
	key_selected = 0x0005;
	timerSetup();	// Configure timer pins
	
	TIM10->CCR1 = (uint16_t) (TIM10->ARR * (key_selected / 0x000A));
	TIM10->CR1 |= TIM_CR1_CEN; 
	
 /* Endless loop */
	while (1) {

	} /* repeat forever */
}
