/**
 * 5 semestre - Eng. da Computação - Insper
 * Rafael Corsi - rafael.corsi@insper.edu.br
 *
 * Projeto 0 para a placa SAME70-XPLD
 *
 * Objetivo :
 *  - Introduzir ASF e HAL
 *  - Configuracao de clock
 *  - Configuracao pino In/Out
 *
 * Material :
 *  - Kit: ATMEL SAME70-XPLD - ARM CORTEX M7
 */

/************************************************************************/
/* includes                                                             */
/************************************************************************/

#include "asf.h"

/************************************************************************/
/* defines                                                              */
/************************************************************************/
#define DELAY 100;

// Configuracoes do LED1
#define LED1_PIO           PIOA                
#define LED1_PIO_ID        ID_PIOA             
#define LED1_PIO_IDX       0                   
#define LED1_PIO_IDX_MASK  (1 << LED1_PIO_IDX) 

// Configuracoes do LED2
#define LED2_PIO           PIOC                 
#define LED2_PIO_ID        ID_PIOC             
#define LED2_PIO_IDX       30                   
#define LED2_PIO_IDX_MASK  (1 << LED2_PIO_IDX) 

// Configuracoes do LED3
#define LED3_PIO           PIOB                
#define LED3_PIO_ID        ID_PIOB              
#define LED3_PIO_IDX       2                    
#define LED3_PIO_IDX_MASK  (1 << LED3_PIO_IDX)  

// Configuracoes do botao1
#define BUT1_PIO        PIOD
#define BUT1_PIO_ID     ID_PIOD
#define BUT1_PIO_IDX    28                     
#define BUT1_PIO_IDX_MASK (1u << BUT1_PIO_IDX)  

// Configuracoes do botao2
#define BUT2_PIO        PIOC
#define BUT2_PIO_ID     ID_PIOC
#define BUT2_PIO_IDX    31                     
#define BUT2_PIO_IDX_MASK (1u << BUT2_PIO_IDX) 

// Configuracoes do botao3
#define BUT3_PIO        PIOA
#define BUT3_PIO_ID     ID_PIOA
#define BUT3_PIO_IDX    19                     
#define BUT3_PIO_IDX_MASK (1u << BUT3_PIO_IDX) 

/*  Default pin configuration (no attribute). */
#define _PIO_DEFAULT             (0u << 0)
/*  The internal pin pull-up is active. */
#define _PIO_PULLUP              (1u << 0)
/*  The internal glitch filter is active. */
#define _PIO_DEGLITCH            (1u << 1)
/*  The internal debouncing filter is active. */
#define _PIO_DEBOUNCE            (1u << 3)



void init(void);
void blink(Pio* pio_led, int pio_led_idx);


void _pio_set(Pio *p_pio, const uint32_t ul_mask) {
	
	p_pio->PIO_SODR = ul_mask;
}

void _pio_clear(Pio *p_pio, const uint32_t ul_mask)
{
	p_pio->PIO_CODR = ul_mask;
}

void _pio_pull_up(Pio *p_pio, const uint32_t ul_mask,
const uint32_t ul_pull_up_enable){
	if (!ul_pull_up_enable){
		p_pio->PIO_PUDR = ul_mask;
		return;
	}
	p_pio->PIO_PUER = ul_mask;
}

void _pio_set_input(Pio *p_pio, const uint32_t ul_mask, const uint32_t ul_attribute) {

	p_pio->PIO_ODR = ul_mask;
	p_pio->PIO_PER = ul_mask;
	
	(ul_attribute & _PIO_PULLUP) ? pio_pull_up(p_pio, ul_mask, 1): "";
	
	p_pio->PIO_IFER = ul_mask;
		
	if (ul_attribute & _PIO_DEBOUNCE){
		p_pio->PIO_IFSCER = ul_mask;
		p_pio->PIO_SCDR = ul_mask;
	}else if (ul_attribute & _PIO_DEGLITCH){
		p_pio->PIO_IFSCDR = ul_mask;
	}else{
		p_pio->PIO_IFDR = ul_mask;
	}
	
}



void _pio_set_output(Pio *p_pio, const uint32_t ul_mask,
const uint32_t ul_default_level,
const uint32_t ul_multidrive_enable,
const uint32_t ul_pull_up_enable)
{
	p_pio->PIO_PER = ul_mask;
	p_pio->PIO_OER = ul_mask;

	ul_default_level ? _pio_set(p_pio, ul_mask):_pio_clear(p_pio, ul_mask);
	ul_multidrive_enable ?( p_pio->PIO_MDER = ul_mask) : (p_pio->PIO_MDDR = ul_mask);

	_pio_pull_up(p_pio, ul_mask, ul_pull_up_enable);
}

uint32_t _pio_get(Pio *p_pio, const pio_type_t ul_type,
const uint32_t ul_mask)
{
	uint32_t ul_reg;
	ul_reg = p_pio->PIO_ODSR;
		
	if ((ul_type == PIO_OUTPUT_0) || (ul_type == PIO_OUTPUT_1)){
		ul_reg = p_pio->PIO_ODSR;
	}else{
		ul_reg = p_pio->PIO_PDSR;
	}
	
	 if ((ul_reg & ul_mask) == 0)  return 0; else return 1;
	
}

void _delay_ms(int ms){
	int i = 0;
	int stop = ms*150000;
	while (i<stop){
		asm("NOP");
		i++;
	}
}

void blink(Pio* pio_led, int pio_led_idx_mask){
	for (int i = 0; i < 5; i++)
	{
		_pio_set(pio_led, pio_led_idx_mask);
		_delay_ms(200);
		_pio_clear(pio_led,pio_led_idx_mask);
		_delay_ms(200);
	}
};


void init(void){
	// Initialize the board clock
	sysclk_init();

	// Desativa WatchDog Timer
	WDT->WDT_MR = WDT_MR_WDDIS;
	
	
	//LED1 BTN1
	pmc_enable_periph_clk(LED1_PIO_ID);
	pmc_enable_periph_clk(BUT1_PIO_ID);
	_pio_set_output(LED1_PIO, LED1_PIO_IDX_MASK, 0, 0, 0);
	_pio_set_input(BUT1_PIO, BUT1_PIO_IDX_MASK, PIO_DEFAULT);
	
	//LED2 BTN2
	pmc_enable_periph_clk(LED2_PIO_ID);
	pmc_enable_periph_clk(BUT2_PIO_ID);
	_pio_set_output(LED2_PIO, LED2_PIO_IDX_MASK, 0, 0, 0);
	_pio_set_input(BUT2_PIO, BUT2_PIO_IDX_MASK, PIO_DEFAULT);
	
	//LED 3 BTN3
	pmc_enable_periph_clk(LED3_PIO_ID);
	pmc_enable_periph_clk(BUT3_PIO_ID);
	_pio_set_output(LED3_PIO, LED3_PIO_IDX_MASK, 0, 0, 0);
	_pio_set_input(BUT3_PIO, BUT3_PIO_IDX_MASK, PIO_DEFAULT);
	
	
	_pio_pull_up(BUT1_PIO, BUT1_PIO_IDX_MASK, PIO_PULLUP);
	_pio_pull_up(BUT2_PIO, BUT2_PIO_IDX_MASK, PIO_PULLUP);
	_pio_pull_up(BUT3_PIO, BUT3_PIO_IDX_MASK, PIO_PULLUP);
	
	
}

/************************************************************************/
/* Main                                                                 */
/************************************************************************/

// Funcao principal chamada na inicalizacao do uC.
int main(void)
{
	init();

	// super loop
	// aplicacoes embarcadas não devem sair do while(1).
	while (1)
	{
		if(!_pio_get(BUT1_PIO, PIO_INPUT, BUT1_PIO_IDX_MASK))
		{
			blink(LED1_PIO, LED1_PIO_IDX_MASK);
			} else {
			_pio_clear(LED1_PIO, LED1_PIO_IDX_MASK);
		}
		
		if(!_pio_get(BUT2_PIO, PIO_INPUT, BUT2_PIO_IDX_MASK))
		{
			blink(LED2_PIO, LED2_PIO_IDX_MASK);
			} else {
			_pio_clear(LED2_PIO, LED2_PIO_IDX_MASK);
		}
		
		if(!_pio_get(BUT3_PIO, PIO_INPUT, BUT3_PIO_IDX_MASK))
		{
			blink(LED3_PIO, LED3_PIO_IDX_MASK);
			} else {
			_pio_clear(LED3_PIO, LED3_PIO_IDX_MASK);
		}
		
		
	}
	return 0;
}