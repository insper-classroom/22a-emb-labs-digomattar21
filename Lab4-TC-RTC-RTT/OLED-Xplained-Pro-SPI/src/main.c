#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"


#define LED_PIO PIOC
#define LED_ID ID_PIOC
#define LED_IDX 8
#define LED_IDX_MASK (1 << LED_IDX)

#define LED_1_PIO  PIOA
#define LED_1_ID  ID_PIOA
#define LED_1_IDX  0
#define LED_1_IDX_MASK  (1 << LED_1_IDX)

#define LED_2_PIO  PIOC
#define LED_2_ID  ID_PIOC
#define LED_2_IDX  30
#define LED_2_IDX_MASK  (1 << LED_2_IDX)

#define LED_3_PIO  PIOB
#define LED_3_ID  ID_PIOB
#define LED_3_IDX  2
#define LED_3_IDX_MASK  (1 << LED_3_IDX)


#define BUT_1_PIO  PIOD
#define BUT_1_ID  ID_PIOD
#define BUT_1_IDX  28
#define BUT_1_IDX_MASK  (1u << BUT_1_IDX)


#define BUT_2_PIO  PIOC
#define BUT_2_ID  ID_PIOC
#define BUT_2_IDX  31
#define BUT_2_IDX_MASK  (1u << BUT_2_IDX)

#define BUT_3_PIO  PIOA
#define BUT_3_ID  ID_PIOA
#define BUT_3_IDX  19
#define BUT_3_IDX_MASK  (1u << BUT_3_IDX)

typedef struct  {
	uint32_t year;
	uint32_t month;
	uint32_t day;
	uint32_t week;
	uint32_t hour;
	uint32_t minute;
	uint32_t second;
} calendar;

void LED_init(int estado);
void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq);
static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource);
void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type);
void but_1_callback(void);
void pin_toggle(Pio *pio, uint32_t mask);
void pisca_led(int n, int delay);


volatile char flag_alarme_rtc = 0;
volatile char flag_sec = 1;
volatile char flag_but_1 = 0;
volatile char flag_piscando = 0;
volatile char flag_para_pisca = 0;

uint32_t current_hour, current_min, current_sec;
uint32_t current_year, current_month, current_day, current_week;

void TC1_Handler(void) {
	/**
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	* Isso é realizado pela leitura do status do periférico
	**/
	volatile uint32_t status = tc_get_status(TC0, 1);

	/** Muda o estado do LED (pisca) **/
	pin_toggle(LED_1_PIO, LED_1_IDX_MASK);  
}

void TC4_Handler(void) {
	/**
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	* Isso é realizado pela leitura do status do periférico
	**/
	volatile uint32_t status = tc_get_status(TC1, 1);

	/** Muda o estado do LED (pisca) **/
	pin_toggle(LED_PIO, LED_IDX_MASK);  
}

void TC7_Handler(void) {
	/**
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	* Isso é realizado pela leitura do status do periférico
	**/
	volatile uint32_t status = tc_get_status(TC2, 1);

	/** Muda o estado do LED (pisca) **/
	pin_toggle(LED_3_PIO, LED_3_IDX_MASK);  
}

void RTT_Handler(void) {
	uint32_t ul_status;

	/* Get RTT status - ACK */
	ul_status = rtt_get_status(RTT);

	/* IRQ due to Alarm */
	if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {
		RTT_init(4, 16, RTT_MR_ALMIEN);
	}
	
	/* IRQ due to Time has changed */
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {
		pin_toggle(LED_2_PIO, LED_2_IDX_MASK);    // BLINK Led
	}

}

void RTC_Handler(void) {
	uint32_t ul_status = rtc_get_status(RTC);
	
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		flag_sec = 1;
	}
	
	if ((ul_status & RTC_SR_ALARM) == RTC_SR_ALARM) {
		flag_alarme_rtc = 1;
		
		if (flag_piscando) {
			flag_alarme_rtc = 0;
			flag_para_pisca = 1;
			flag_piscando = 0;
		}
	}

	rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
}

void but_1_callback(void) {
	pio_get(BUT_1_PIO, PIO_INPUT, BUT_1_IDX_MASK) ? (flag_but_1 = 0) : (flag_but_1 = 1);
}

void pin_toggle(Pio *pio, uint32_t mask) {
 (pio_get_output_data_status(pio, mask))?(pio_clear(pio, mask)):(pio_set(pio,mask));
}


void pisca_led (int n, int t) {
	int i =0;
	while(i<n){
		pio_clear(LED_3_PIO, LED_3_IDX_MASK);
		delay_ms(t);
		pio_set(LED_3_PIO, LED_3_IDX_MASK);
		delay_ms(t);
		i++;
	}
}
	
	
void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq){
	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk = sysclk_get_cpu_hz();

	pmc_enable_periph_clk(ID_TC);

	tc_find_mck_divisor(freq, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC, TC_CHANNEL, ul_tcclks | TC_CMR_CPCTRG);
	tc_write_rc(TC, TC_CHANNEL, (ul_sysclk / ul_div) / freq);

	NVIC_SetPriority(ID_TC, 4);
	NVIC_EnableIRQ((IRQn_Type) ID_TC);
	tc_enable_interrupt(TC, TC_CHANNEL, TC_IER_CPCS);
	
};

static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource) {

	uint16_t pllPreScale = (int) (((float) 32768) / freqPrescale);
	
	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);
	
	if (rttIRQSource & RTT_MR_ALMIEN) {
		uint32_t ul_previous_time;
		ul_previous_time = rtt_read_timer_value(RTT);
		while (ul_previous_time == rtt_read_timer_value(RTT));
		rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);
	}

	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 4);
	NVIC_EnableIRQ(RTT_IRQn);

	if (rttIRQSource & (RTT_MR_RTTINCIEN | RTT_MR_ALMIEN))
	rtt_enable_interrupt(RTT, rttIRQSource);
	else
	rtt_disable_interrupt(RTT, RTT_MR_RTTINCIEN | RTT_MR_ALMIEN);
	
};

static float get_time_rtt(){
	uint ul_previous_time = rtt_read_timer_value(RTT);
}

void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type) {
	pmc_enable_periph_clk(ID_RTC);

	rtc_set_hour_mode(rtc, 0);

	rtc_set_date(rtc, t.year, t.month, t.day, t.week);
	rtc_set_time(rtc, t.hour, t.minute, t.second);

	NVIC_DisableIRQ(id_rtc);
	NVIC_ClearPendingIRQ(id_rtc);
	NVIC_SetPriority(id_rtc, 4);
	NVIC_EnableIRQ(id_rtc);

	rtc_enable_interrupt(rtc,  irq_type);
}

void init (void) {
	sysclk_init();
	WDT->WDT_MR = WDT_MR_WDDIS;
	board_init();
	delay_init();
	gfx_mono_ssd1306_init();
	
	pmc_enable_periph_clk(LED_ID);
	pio_set_output(LED_PIO, LED_IDX_MASK, 1, 0, 0);
	
	pmc_enable_periph_clk(LED_1_ID);
	pio_set_output(LED_1_PIO, LED_1_IDX_MASK, 1, 0, 0);
	
	pmc_enable_periph_clk(LED_2_ID);
	pio_set_output(LED_2_PIO, LED_2_IDX_MASK, 1, 0, 0);
	
	pmc_enable_periph_clk(LED_3_ID);
	pio_set_output(LED_3_PIO, LED_3_IDX_MASK, 1, 0, 0);
	
	pmc_enable_periph_clk(BUT_1_ID);
	
	pio_configure(BUT_1_PIO, PIO_INPUT, BUT_1_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	
	pio_set_debounce_filter(BUT_1_PIO, BUT_1_IDX_MASK, 60);
	
	pio_handler_set(BUT_1_PIO, BUT_1_ID, BUT_1_IDX_MASK, PIO_IT_EDGE, but_1_callback);

	pio_enable_interrupt(BUT_1_PIO, BUT_1_IDX_MASK);
	pio_get_interrupt_status(BUT_1_PIO);
	
	NVIC_EnableIRQ(BUT_1_ID);
	NVIC_SetPriority(BUT_1_ID, 4);
	
	TC_init(TC0, ID_TC1, 1, 4);
	TC_init(TC1, ID_TC4, 1, 5);
	tc_start(TC0, 1);
	tc_start(TC1, 1);
	
	RTT_init(4, 16, RTT_MR_ALMIEN);
	
	calendar rtc_initial = {2018, 3, 19, 12, 15, 45 ,1};
	RTC_init(RTC, ID_RTC, rtc_initial, RTC_IER_ALREN | RTC_IER_SECEN);
	
};
	
void rtcGetTime(){
	rtc_get_time(RTC, &current_hour, &current_min, &current_sec);
	rtc_get_date(RTC, &current_year, &current_month, &current_day, &current_week);
};

void rtcSetTime(int para_pisca){
	if (para_pisca == 0){
		rtc_set_date_alarm(RTC, 1, current_month, 1, current_day);
		rtc_set_time_alarm(RTC, 1, current_hour, 1, current_min, 1, current_sec + 20);
	}else{
		TC_init(TC2, ID_TC7, 1, 5);
		tc_start(TC2, 1);
		
		flag_piscando = 1;
		
		rtc_set_date_alarm(RTC, 1, current_month, 1, current_day);
		rtc_set_time_alarm(RTC, 1, current_hour, 1, current_min, 1, current_sec + 2);
		
		flag_alarme_rtc = 0;
	}
}

void stopLed(){
	tc_stop(TC2, 1);
	pio_set(LED_3_PIO, LED_3_IDX_MASK);
	flag_para_pisca = 0;
}

int main (void)
{
	init();
	char tempo[15];
  
  /* Insert application code here, after the board has been initialized. */
	while(1) {
		rtcGetTime();


		if (flag_sec) {
			sprintf(tempo, "%02d:%02d:%02d", current_hour, current_min, current_sec);
			gfx_mono_draw_string(tempo, 5,16, &sysfont);
		}

		(flag_but_1)?(rtcSetTime(0)):"";

		(!flag_para_pisca && flag_alarme_rtc)?(rtcSetTime(1)):"";
		
		(flag_para_pisca) ?(stopLed()):"";
	
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	
			
	}
}
