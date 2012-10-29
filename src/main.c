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

#include <stdio.h>
#include <console.h>
#include <uart.h>
//#include <cffat.h>
#include <system.h>
#include <irq.h>
#include <board.h>
#include <version.h>
#include <hw/sysctl.h>
#include <hw/gpio.h>
#include <hw/interrupts.h>

#include <hal/brd.h>
#include <hal/mem.h>
#include <hal/time.h>
#include <hal/vga.h>
#include <hal/snd.h>
#include <hal/tmu.h>
#include <hal/pfpu.h>

#include "color.h"
//#include "plasma.h"

int main(void)
{
	irq_setmask(0);
	irq_enable(1);
	uart_async_init();
	uart_force_sync(1);
	brd_init();
	time_init();
	mem_init();
	vga_init(0);
	snd_init();
	tmu_init();
	pfpu_init();

	int x = 42;
	int y = 42;
	int dx = 3;
	int dy = 3;
	int w;
	int h;

	int k = 0;
	int input = 0;

	int p1x = 0;
	int p1y = 0;
	int p1s = 40;

//	init_plasma();
	
	while(1) {

	  input = CSR_GPIO_IN;
	  if (input & GPIO_BTN1) {
	    p1y-=5;
	  }
	  if (input & GPIO_BTN2) {
	    p1y+=5;
	  }

	  if (p1y < 0)
	    p1y = 0;

	  if ((p1y + p1s) > 480)
	    p1y = 480 - p1s;
	  
	  //for(j = 0; j < vga_vres*vga_hres; ++j)
	  //  vga_backbuffer[j] = MAKERGB565(0, 0, 0);

	  update_plasma(x, y);

	  for (w = 0; w < 10; ++w) {
	    for (h = 0; h < 10; ++h) {
	      vga_backbuffer[(y+h)*vga_hres + (x+w)] = MAKERGB565(0xff, 0, 0xff);
	    }
	  }

	  for (w = 0; w < 10; ++w) {
	    for (h = 0; h < p1s; ++h) {
	      vga_backbuffer[(p1y+h)*vga_hres + (p1x+w)] = MAKERGB565(0xff, 0, 0);
	    }
	  }

	  flush_bridge_cache();

	  vga_swap_buffers();

	  x+= dx;
	  y+= dy;

	  if (x < 10) {
	    if ((y > (p1y - 10)) && (y < (p1y + p1s))) {
	      x = 10;
	      dx = -dx;
	      p1s -= 10;
	      if (p1s < 10)
		p1s = 10;
	    } else {
	      x = 320;
	      y = 240;
	      p1s += 10;
	    }
	  }

	  if (((y + 10) > vga_vres) || (y < 0)) {
	    dy = - dy;
	  }

	  k = k + 1;
	}	
	return 0;
}
