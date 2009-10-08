#ifndef __MM_CONFIG
#define __MM_CONFIG


#include <libc.h>
#include <console.h>
#include <uart.h>
#include <cffat.h>
#include <system.h>
#include <irq.h>
#include <board.h>
#include <version.h>
#include <hw/sysctl.h>
#include <hw/gpio.h>
#include <hw/interrupts.h>

#include "brd.h"
#include "mem.h"
#include "time.h"
#include "vga.h"
#include "snd.h"
#include "tmu.h"
#include "pfpu.h"
#include "music.h"
#include "sprites.h"
static void banner()
{
    putsnonl("\n\n\e[1m     |\\  /|'||      |\\  /|'   |\n"
              "     | \\/ ||||_/\\  /| \\/ ||(~~|~\n"
              "     |    |||| \\ \\/ |    ||_) |\n"
              "                _/\n"
              "\e[0m           MAIN#4 DemoCompo\n\n\n");
}


int scan_keys() {
    return 0;
}

void demo_sleep(int ms) {
    return;
}

void demo_quit() {
    return;
}

#endif //__MM_CONFIG
