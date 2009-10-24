/*
 * Milkymist Democompo
 * Copyright (C) 2007, 2008, 2009 Sebastien Bourdeauducq
 * Copyright (C) 2009 Alexandre Harly
 * Copyright (C) 2009 Bengt Sjolen
 * Copyright (C) 2009 Fabien Le Mentec
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



#ifndef MOD_H_INCLUDED
# define MOD_H_INCLUDED



/* int types */

#include <sys/types.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

#if 0
#if !defined(_INT8_T)
typedef /* signed */ char int8_t;
#endif
#if !defined(_INT16_T)
typedef /* signed */ short int16_t;
#endif
#if !defined(_INT32_T)
typedef /* signed */ int int32_t;
#endif
#if !defined(_INT64_T)
typedef /* signed */ long long int64_t;
#endif
#endif


/* channel state */

struct chan_state
{
  unsigned int ichan;

  unsigned int freq;

  /* volume step */
  int volume;
  int volstep;

  /* offset in the sample in bytes */
  unsigned int smpoff;

  /* sample rate */
  unsigned int smprate;

  /* chan flags */
#define CHAN_FLAG_IS_SAMPLE_STARTING (1 << 0)
#define CHAN_FLAG_IS_SAMPLE_REPEATING (1 << 1)
#define CHAN_SET_FLAG(C, F) do { (C)->flags |= CHAN_FLAG_ ## F; } while (0)
#define CHAN_CLEAR_FLAG(C, F) do { (C)->flags &= ~CHAN_FLAG_ ## F; } while (0)
#define CHAN_HAS_FLAG(C, F) ((C)->flags & (CHAN_FLAG_ ## F))
#define CHAN_CLEAR_FLAGS(C) do { (C)->flags = 0; } while (0)
  unsigned int flags;

  unsigned int ismp;

  uint32_t period;       // current period as in channel in pattern
  uint32_t currentperiod;
  uint32_t modperiod;
  int32_t periodstep;	 // periode step for slide up or down
  uint32_t periodtarget;
  uint32_t command;      // current fx/command 12-bits
  uint32_t sample;       // selected sample number (starting at 1 as in channel in pattern)
  int finetune;
  unsigned int note;

  /* arpeggio effect */
  uint32_t arpindex;
  uint32_t arpnotes[3];
  uint32_t arpmod;

  /* vibrato effect */
  unsigned int viboffset;
  unsigned int vibrate;
  unsigned int vibdepth;
  unsigned int vibretrig;
  const int* vibtable;

  uint32_t lastposition; // last sample offset (the 32.0 of 32.16)
  uint32_t position;     // sample offset (the 32.0 of 32.16)
  uint32_t fraction;     // fraction (the 0.16 of 32.16, 32-bits to make it simple with overflow)
  uint32_t samplestep;   // 16.16 to add to position.fraction for each step of sample

  int32_t rampvolume;
  int32_t lastvolume;

  int32_t rampsample;
  int32_t lastsample;

  int32_t backbuffer[4];
  
};

typedef struct chan_state chan_state_t;


/* mod context */

struct sample_desc;

struct mod_context
{
  struct sample_desc* sdescs;
  const int8_t* sdata[32];
  uint32_t s_length[32];           // length for each instrument in bytes 
  uint32_t s_roff[32];             // repeat offset for each instrument in bytes
  uint32_t s_rlength[32];          // repeat lenght for each instrument in bytes

  const unsigned char* song;

  unsigned int npats;
  const void* pdata;
  unsigned int songlength;

  /* current division */
  uint32_t tempo;
  uint32_t channels;               // nr of channels, normally 4
  uint32_t tickspersecond;
  uint32_t samplespertick;         // nr of samples to generate for each tick
  uint32_t samplesproduced;        // nr of samples we have produced in the current tick

  uint32_t ticksperdivision;       
  uint32_t divisionsperpattern;
  uint32_t patternsize;
  uint32_t divisionsize;

  /* current state */
  uint32_t songpos;                // which index in the song we are playing at
  uint32_t idiv;
  uint32_t tick;

  uint32_t break_on_next_idiv;
  uint32_t break_next_idiv;
  uint32_t break_next_songpos;

  unsigned int play;

  struct chan_state cstates[4];
};

typedef struct mod_context mod_context_t;



/* exported */

int mod_init(mod_context_t*, const void*, size_t);
void mod_fetch(mod_context_t*, void*, unsigned int);



#endif /* ! MOD_H_INCLUDED */
