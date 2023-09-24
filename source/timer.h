#ifndef TIMER_H
#define TIMER_H
#ifdef _PSX
#include <psxetc.h>
#include <psxapi.h>
#include <hwregs_c.h>

void setup_timers();
#else 
void setup_timers() {}
#endif

#endif