#include "timer.h"
#include "music.h"

#include <stdio.h>

static void timer2_handler(void) {
    audio_tick(1);
}


void setup_timers(void) {
	TIMER_CTRL(2) = 
		(2 << 8) | // Use "System Clock / 8" source
		(1 << 4) | // Interrupt when hitting target value
		(1 << 6) | // Repeat this interrupt every time the timer reaches target
		(1 << 3);  // Reset counter to 0 after hitting target value
	TIMER_RELOAD(2) = 22050;

	EnterCriticalSection();
	InterruptCallback(IRQ_TIMER2, &timer2_handler);
	ExitCriticalSection();
}
