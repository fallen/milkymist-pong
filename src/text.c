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

#include <hal/vga.h>

#include "fontmb.h"
#include "text.h"

void draw_char(int x, int y, unsigned short color, char c)
{
	unsigned int shift;
	int dx, dy;
	int fx, fy;

	shift = ((unsigned int)c)*FONT_W*FONT_H;
	for(dy=0;dy<FONT_H;dy++)
		for(dx=0;dx<FONT_W;dx++) {
			fx = x + dx;
			fy = y + dy;
			if((fx >= 0) && (fx < vga_hres) && (fy >= 0) && (fy < vga_vres))
				if(font_data[shift+FONT_W*dy+dx])
					vga_backbuffer[vga_hres*fy+fx] = color;
		}
}

void draw_text(int x, int y, unsigned short color, const char *str)
{
	while(*str) {
		draw_char(x, y, color, *str);
		str++;
		x += FONT_W;
	}
}
