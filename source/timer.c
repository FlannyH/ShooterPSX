#include "timer.h"
#include "music.h"

static int timer0_count = 0;
static int timer1_count = 0;
static int timer2_count = 0;

static void timer0_handler(void) {
    music_tick(6);
	timer0_count++;
}

static void timer1_handler(void) {
	timer1_count++;
}

static void timer2_handler(void) {
	timer2_count++;
}

void setup_timers() {
	TIMER_CTRL(0) = 0x0160; // Dotclock input, repeated IRQ on overflow
	TIMER_CTRL(1) = 0x0160; // Hblank input, repeated IRQ on overflow
	TIMER_CTRL(2) = 0x0260; // CLK/8 input, repeated IRQ on overflow
	EnterCriticalSection();
	InterruptCallback(IRQ_TIMER0, &timer0_handler);
	InterruptCallback(IRQ_TIMER1, &timer1_handler);
	InterruptCallback(IRQ_TIMER2, &timer2_handler);
	ExitCriticalSection();
}