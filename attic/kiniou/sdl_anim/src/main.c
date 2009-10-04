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

#ifdef __SDLSDK__ //We are debugging on a computer :)
#include "sdl_config.h"
#else
#include "mm_config.h"
#endif


#include "intro.h"
#include "demo_1.h"
#include "music.h"
#include "sprites.h"
#include "transition.h"
#include "angle.h"



int main()
{
#ifndef __SDLSDK__ //We are debugging on a computer :)
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
#else //__SDLSDK__
    banner();
    vga_init();
    tmu_init();
    music_start();
#endif //__SDLSDK__
//    init_angles();

	while(1) {
		intro_csv();
        //demo_1();
        demo_2(); 
#ifndef __SDLSDK__ //We are debugging on a computer :)
		while(!(CSR_GPIO_IN & GPIO_DIP3));
#endif //__SDLSDK__
	}
	
	return 0;
}
