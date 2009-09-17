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
#include <cffat.h>

#include "snd.h"
#include "mod.h"
#include "music.h"

#define NSAMPLES 4800

static mod_context_t mod_context;

static void callback(short *buffer, void *user)
{
	mod_fetch(&mod_context, buffer, NSAMPLES);
	snd_play_refill(buffer);
}

static char modfile[1024*1024];
static short sndbuf1[NSAMPLES*2];
static short sndbuf2[NSAMPLES*2];
static short sndbuf3[NSAMPLES*2];

void music_start()
{
	int size;
	int r;

	cffat_init();
	cffat_load("3.MOD", modfile, sizeof(modfile), &size);
	cffat_done();

	r = mod_init(&mod_context, modfile, size);

	mod_fetch(&mod_context, sndbuf1, NSAMPLES);
	mod_fetch(&mod_context, sndbuf2, NSAMPLES);
	mod_fetch(&mod_context, sndbuf3, NSAMPLES);
	snd_play_refill(sndbuf1);
	snd_play_refill(sndbuf2);
	snd_play_refill(sndbuf3);
	snd_play_start(callback, NSAMPLES, NULL);
}
