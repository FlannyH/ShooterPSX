#include "timer.h"
#include "music.h"
#include <stdio.h>

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
	EnterCriticalSection();
	InterruptCallback(IRQ_TIMER0, &timer0_handler);
	ExitCriticalSection();
}