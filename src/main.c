/*
 * Milkymist Democompo
 * Copyright (C) 2007, 2008, 2009 Sebastien Bourdeauducq
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
#include "intro.h"
#include "music.h"

static void banner()
{
	putsnonl("\n\n\e[1m     |\\  /|'||      |\\  /|'   |\n"
			  "     | \\/ ||||_/\\  /| \\/ ||(~~|~\n"
			  "     |    |||| \\ \\/ |    ||_) |\n"
			  "                _/\n"
			  "\e[0m           MAIN#4 DemoCompo\n\n\n");
}

int main()
{
	irq_setmask(0);
	irq_enable(1);
	uart_async_init();
	uart_force_sync(1);
	banner();
	brd_init();
	time_init();
	mem_init();
	vga_init();
	snd_init();
	tmu_init();
	pfpu_init();

	music_start();
	while(1) {
		intro_csv();
		while(!(CSR_GPIO_IN & GPIO_DIP3));
	}
	
	return 0;
}
