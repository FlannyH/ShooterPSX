#ifndef TIMER_H
#define TIMER_H
#ifdef _PSX
#include <hwregs_c.h>
#include <psxetc.h>
#include <psxapi.h>

void setup_timers();
#else 
void setup_timers() {}
#endif

#endif