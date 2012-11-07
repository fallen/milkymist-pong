/*
 * Milkymist Pong
 * Copyright (C) 2007, 2008, 2009 Sebastien Bourdeauducq
 * Copyright (C) 2009 Johan Euphrosine
 * Copyright (C) 2012 Yann Sionneau
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

int main(void)
{

	CSR_DBG_CTRL = DBG_CTRL_GDB_ROM_LOCK;
	CSR_DBG_CTRL = DBG_CTRL_BUS_ERR_EN;

	irq_setmask(0);
	irq_enable(1);
	uart_init();
	vga_init(0);
	brd_init();
	tmu_init();
	vga_unblank();

	int j;      /* counter to iterate over framebuffer's pixel */
	int x = 42; /* horizontal coordinate of the Pong ball */
	int y = 42; /* vertical coordinate of the Pong ball */
	int dx = 3; /* horizontal speed of the Pong ball */
	int dy = 3; /* vertical speed of the Pong ball */
	int w;      /* counter to iterate over the width of the Pong ball and the paddle */
	int h;      /* counter to iterate over the height of Pong ball */

	int input = 0; /* contains the state of the 3 push buttons of Milkymist One board */

	int p1x = 0; /* horizontal coordinate of Player1's paddle */
	int p1y = 0; /* vertical coordinate of Player1's paddle */
	int p1s = 40;/* height of Player1's paddle */
	/*
	*  When the player's paddle hits the Pong ball, the paddle's height will be reduced to make the game harder.
	*  When the player's paddle misses the Pong ball, the paddle's height will be increased to make the game easier.
	*/
	
	while(1) {

	  input = CSR_GPIO_IN; /* read from GPIO input register containing state of 3 push buttons */
	  if (input & GPIO_BTN1) { /* check the state of the 1st push button */
	    p1y-=5;
	  }
	  if (input & GPIO_BTN2) { /* check the state of the 2nd push button */
	    p1y+=5;
	  }

	  if (p1y < 0)
	    p1y = 0;

	  if ((p1y + p1s) > 480)
	    p1y = 480 - p1s;
	  
	  for(j = 0; j < vga_vres*vga_hres; ++j)
	    vga_backbuffer[j] = MAKERGB565(0, 0, 0);

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
	      dx = 3;
	    }
	  } else if ((x + w) > (vga_hres - 10) && (dx > 0))
	  {
	    x = vga_hres - 10;
	    dx = -dx;
	  }

	  if (((y + 10) > vga_vres) || (y < 0)) {
	    dy = - dy;
	  }

	}	
	return 0;
}
