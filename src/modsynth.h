/*
 * Milkymist Democompo
 * Copyright (C) 2007, 2008, 2009 Sebastien Bourdeauducq
 * Copyright (C) 2009 Alexandre Harly
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __MODSYNTH_H
#define __MODSYNTH_H

struct modsynth_sc {
	/* TODO */
};

void modsynth_init(struct modsynth_sc *sc, void *mod_data);
/* 
 * n is the number of samples we must put in the buffer.
 * a sample is a stereo sample, made of two 16-bit values
 * (one for each channel).
 * returns the number of samples actually put in the buffer
 * and -1 in case of error.
 * The function must fill the buffer with zeros after the end
 * of the MOD file.
 */
int modsynth_synth(struct modsynth_sc *sc, short *buffer, int n);

#endif /* __MODSYNTH_H */
