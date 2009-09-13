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



#include <stdint.h>



/* int types */

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#if 0
typedef /* signed */ char int8_t;
typedef /* signed */ short int16_t;
typedef /* signed */ int int32_t;
#endif


/* effect state */

struct fx_state
{
#define FX_ID_ARPEGGIO 0
#define FX_ID_SLIDE_UP 1
#define FX_ID_SLIDE_DOWN 2
#define FX_ID_SLIDE_TO_NOTE 3
#define FX_ID_VIBRATO 4
#define FX_ID_CONT_SLIDE_TO_NOTE_AND_DO_VOL_SLIDE 5
#define FX_ID_CONT_VIBRATO_AND_DO_VOL_SLIDE 6
#define FX_ID_TREMOLO 7
#define FX_ID_UNUSED 8
#define FX_ID_SET_SAMPLE_OFFSET 9
#define FX_ID_VOLUME_SLIDE 10
#define FX_ID_POS_JUMP 11
#define FX_ID_SET_VOLUME 12
#define FX_ID_PATTERN_BREAK 13
#define FX_ID_EXTENDED 14
#define FX_ID_SET_SPEED 15

#define FX_XID_SET_FILTER 0
#define FX_XID_FINESLIDE_UP 1
#define FX_XID_FINESLIDE_DOWN 2
#define FX_XID_SET_GLISSANDO 3
#define FX_XID_SET_VIBRATO_WAVEFORM 4
#define FX_XID_SET_FINETUNE_VALUE 5
#define FX_XID_LOOP_PATTERN 6
#define FX_XID_SET_TREMOLO_WAVEFORM 7
#define FX_XID_UNUSED 8
#define FX_XID_RERTRIGGER_SAMPLE 9
#define FX_XID_FINE_VOLUME_SLIDE_UP 10
#define FX_XID_FINE_VOLUME_SLIDE_DOWN 11
#define FX_XID_CUT_SAMPLE 12
#define FX_XID_DELAY_SAMPLE 13
#define FX_XID_DELAY_PATTERN 14
#define FX_XID_INVERT_LOOP 15

  unsigned int fx;

  union
  {
    struct
    {
      unsigned int noff; /* note offset */
    } arpeggio;

  } u;

};

typedef struct fx_state fx_state_t;


/* channel state */

struct chan_state
{
  unsigned int ichan;

  unsigned int freq;
  unsigned int volume;

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
  uint32_t command;      // current fx/command 12-bits
  uint32_t sample;       // selected sample number (starting at 1 as in channel in pattern)

  uint32_t position;     // sample offset (the 32.0 of 32.16)
  uint32_t fraction;     // fraction (the 0.16 of 32.16, 32-bits to make it simple with overflow)
  uint32_t samplestep;   // 16.16 to add to position.fraction for each step of sample
  
  /* effect data and state */
  unsigned int fx_data;
  fx_state_t fx_state;
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
  uint32_t ipat;                   
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
