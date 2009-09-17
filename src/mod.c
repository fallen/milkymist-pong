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


#include <stdio.h>
#include <string.h>
#include <math.h>
#include "mod.h"
#include "debug.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define LIMIT(a,b,c) (((b)<(a))?(a):(((b)>(c))?(c):(b)))



#define PAL_SYNC_RATE (unsigned int)7093789.2
#define NTSC_SYNC_RATE (unsigned int)7159090.5



static void mod_reset(mod_context_t* mc);
static void mod_tick(mod_context_t* mc);
static void mod_produce(mod_context_t* mc,int16_t* obuf,unsigned int nsmps);

/* just to be clearer in the code */

#define BYTES_PER_WORD 2
#define BYTES_PER_SLE16_INTERLEAVED 2
#define STEREO_CHAN_COUNT 2
#define MOD_CHAN_COUNT 4



/* utility functions */

static inline uint16_t be16_to_host(const void* p)
{
#ifdef LITTLE_ENDIAN
  return (((uint16_t)((const uint8_t*)p)[0]) << 8) | (uint16_t)(((const uint8_t*)p)[1]);
#else
  return *(const uint16_t*)p;
#endif
}


/* memory reading wrapper */

struct reader_context
{
  const unsigned char* pos;
  size_t size; /* remaining size */
};


static inline void reader_init(struct reader_context* rc, const void* data, size_t size)
{
  rc->pos = data;
  rc->size = size;
}


static inline int reader_read(struct reader_context* rc, void* buf, size_t size)
{
  if (size > rc->size)
    {
      DEBUG_ERROR("size(%u) > rc->size(%u)\n", (uint32_t)rc->size, (uint32_t)size);
      return -1;
    }

  memcpy(buf, rc->pos, size);

  rc->pos += size;
  rc->size -= size;

  return 0;
}


static inline void reader_skip_safe(struct reader_context* rc, size_t size)
{
  rc->pos += size;
  rc->size -= size;
}


static inline int reader_skip(struct reader_context* rc, size_t size)
{
  if (size > rc->size)
    {
      DEBUG_ERROR("size(%u) > rc->size(%u)\n", (uint32_t)rc->size, (uint32_t)size);
      return -1;
    }

  reader_skip_safe(rc, size);

  return 0;
}


static inline int reader_check_size(const struct reader_context* rc, size_t size)
{
  return size > rc->size ? -1 : 0;
}



/* mod file loader (doc/mod-form.txt) */

struct sample_desc
{
  uint8_t name[22];
  uint16_t length;
  int8_t finetune;
  uint8_t volume;
  uint16_t roff; /* repeat from this offset in the sample */
  uint16_t rlength; /* repeat rcount times  */
} __attribute__((packed));


static inline unsigned int get_sdesc_length(const struct sample_desc* sd)
{
  return be16_to_host((const void*)&sd->length) * BYTES_PER_WORD;
}


static inline const struct sample_desc* get_sample_desc(const mod_context_t* mc,
							unsigned int ismp)
{
  return &mc->sdescs[ismp];
}


static inline const void* get_sample_data(const mod_context_t* mc,
					  unsigned int ismp)
{
  return mc->sdata[ismp];
}


static inline unsigned int get_sample_length(const mod_context_t* mc,
					     unsigned int ismp)
{
  return get_sdesc_length(get_sample_desc(mc, ismp));
}


static inline unsigned int get_sample_volume(const mod_context_t* mc,
					     unsigned int ismp)
{
  return get_sample_desc(mc, ismp)->volume;
}


static inline int8_t get_sample_finetune(const mod_context_t* mc,
					       unsigned int ismp)
{
  return (((int8_t)get_sample_desc(mc, ismp)->finetune)&0xf); //<<4)>>4;
}


static unsigned int __attribute__((unused))
get_sample_repeat_offset(const mod_context_t* mc, unsigned int ismp)
{
  return be16_to_host(&get_sample_desc(mc, ismp)->roff) * BYTES_PER_WORD;
}


static unsigned int __attribute__((unused))
get_sample_repeat_length(const mod_context_t* mc, unsigned int ismp)
{
  /* only valid if > 1 */

  unsigned int len;

  len = be16_to_host(&get_sample_desc(mc, ismp)->rlength);
  if (len <= 1)
    return 0;

  return len * BYTES_PER_WORD;
}


static int load_file(mod_context_t* mc, const void* data, size_t length)
{
  unsigned int npats;
  unsigned int ismp;
  unsigned char ipos;
  unsigned char npos;
  int i;
  const unsigned char* pos;
  size_t size;
  struct reader_context rc;

  memset(mc, 0, sizeof(mod_context_t));

  reader_init(&rc, data, length);

  /* skip title */

  if (reader_skip(&rc, 20) == -1)
    {
      DEBUG_ERROR("reader_skip()\n");
      goto on_error;
    }

  /* sample descriptions */

  if (reader_check_size(&rc, 31 * sizeof(struct sample_desc)) == -1)
    {
      DEBUG_ERROR("reader_check_size()\n");
      goto on_error;
    }

  mc->sdescs = (struct sample_desc*)rc.pos;
  for(i=0;i<31;i++)
    {
      mc->s_length[i]=be16_to_host(&(mc->sdescs[i].length))*2;
      mc->s_roff[i]=be16_to_host(&(mc->sdescs[i].roff))*2;
      mc->s_rlength[i]=be16_to_host(&(mc->sdescs[i].rlength))*2;
    }
  

  /* todo: check descs (length, offset...) */

  reader_skip_safe(&rc, 31 * sizeof(struct sample_desc));

  /* song position count */

  if ((reader_read(&rc, &npos, 1) == -1) || (npos > 128))
    {
      DEBUG_ERROR("reader_read() == 0x%02x\n", npos);
      goto on_error;
    }
  mc->songlength = npos;

  /* skip this byte */

  if (reader_skip(&rc, 1) == -1)
    {
      DEBUG_ERROR("reader_skip()\n");
      goto on_error;
    }

  /* pattern table for song pos. scan for highest pattern. */

  if (reader_check_size(&rc, 128) == -1)
    {
      DEBUG_ERROR("reader_check_size()\n");
      goto on_error;
    }

  mc->song = rc.pos;

  for (pos = rc.pos, npats = 0, ipos = 0; ipos < npos; ++ipos, ++pos)
    if (npats < *pos)
      npats = *pos;

  npats += 1;
  mc->npats = npats;

  reader_skip_safe(&rc, 128);

  /* "M.K." signature */

  if (reader_skip(&rc, 4) == -1)
    {
      DEBUG_ERROR("reader_skip()\n");
      goto on_error;
    }
  
  // depending on signature it could be 8 channels 
  mc->channels=4;
  mc->divisionsperpattern=64;
  mc->divisionsize=mc->channels<<2;
  mc->patternsize=mc->divisionsize*mc->divisionsperpattern;

  /* pattern data: each pattern is divided into 64
     divisions, each division contains the channels
     data. channel data are represented by 4 bytes
  */

  if (reader_check_size(&rc, npats * mc->patternsize) == -1)
    {
      DEBUG_ERROR("reader_check_size() == -1\n");
      goto on_error;
    }

  mc->pdata = rc.pos;

  reader_skip_safe(&rc, npats * mc->patternsize);

  /* sample data. build sdata lookup table. */

  size = 0;

  for (ismp = 0; ismp < 31; ++ismp)
    {
      int len=0;
      /* todo: check for addition sum overflows */
      mc->sdata[ismp] = (int8_t*)rc.pos + size;
      len=get_sample_length(mc, ismp);
#if 0
      printf("sample %2x at %08x %08x %08x %04x %04x\n",ismp+1,(uint32_t)size,rc.pos+size-(uint32_t)mc->pdata+0x43c,len,
	     (uint32_t)get_sample_repeat_offset(mc, ismp),
	     (uint32_t)get_sample_repeat_length(mc, ismp));
#endif
      DEBUG_PRINTF("sample %2x finetune %x\n",ismp+1,get_sample_finetune(mc,ismp));
      size += get_sample_length(mc, ismp);
    }
#if 0
  printf("ends at %08x\n",rc.pos+size-(uint32_t)mc->pdata+0x43c);
#endif

  /* check there is enough room for samples */

  // strangely 9.mod is 2 bytes shorter than size?!?
  if (reader_check_size(&rc, size-2) == -1)
    {
      DEBUG_ERROR("reader_check_size(%u, %u)\n", (uint32_t)size, (uint32_t)rc.size);
      goto on_error;
    }

  return 0;

 on_error:

  return -1;
}


/* change the channel period */

static unsigned int find_note_by_pitch(unsigned int);

static void inline update_chan_period(chan_state_t* cs)
{
  cs->smprate = PAL_SYNC_RATE / (((cs->currentperiod*cs->modperiod)>>16) * 2);
  cs->samplestep = (cs->smprate << 16) / 48000;
  cs->note = find_note_by_pitch(cs->currentperiod);
}



/* fx handling. the idea is to have 2 functions per
   effect: init and apply. at the very beginning of a
   pattern division, the effect gets a chance of
   initializing whatever it needs by the corresponding
   init routine being called. then the apply routine
   gets called every time until the division is done.
   there are both fx state and data per channel.
 */


/* helper routines */

static inline unsigned int fx_get_first_param(uint32_t fx)
{
  return (fx & 0xf0) >> 4;
}


static inline unsigned int fx_get_second_param(uint32_t fx)
{
  return fx & 0x0f;
}

static inline int8_t fx_get_second_param_signed(uint32_t fx)
{
  return ((int8_t)(fx<<4))>>4;
}


static inline unsigned int fx_get_byte_param(uint32_t fx)
{
  return fx & 0xff;
}


/* note table */


#include "pitch_table.h"


static unsigned int find_note_by_pitch(unsigned int pitch)
{
  /* binary search the node given the pitch */

#define FUZZ 2

  unsigned int a, b, i;

  if (pitch == 0)
    return MAX_NOTE;

  a = 0;

  b = MAX_NOTE - 1;

  while((int)b - (int)a > 1)
    {
      i = (a + b) / 2;

      if (pitch_table[i][0] == pitch)
	return i;

      if (pitch_table[i][0] > pitch)
	a = i;
      else
	b = i;
    }

  if (pitch_table[a][0] - FUZZ <= pitch)
    return a;

  if (pitch_table[b][0] + FUZZ >= pitch)
    return b;

  return MAX_NOTE;
}


/* arpeggio */

static void
fx_ondiv_arpeggio(mod_context_t* mc, chan_state_t* cs)
{
  /* note, note+x, note+y then return to orginal note.
     finetune is implemented by switching to a different
     table of period values for each finetunevalue.
   */

  int note;

  cs->arpindex = 0;

  if (cs->note >= MAX_NOTE)
    {
      /* no previous note */

      cs->arpnotes[0] = cs->currentperiod;
      cs->arpnotes[1] = cs->arpnotes[0];
      cs->arpnotes[2] = cs->arpnotes[0];

      return ;
    }

  cs->arpnotes[0] = pitch_table[cs->note][cs->finetune];

  note = cs->note + fx_get_first_param(cs->command);
  if (note >= MAX_NOTE)
    note = MAX_NOTE - 1;

  cs->arpnotes[1] = pitch_table[note][cs->finetune];

  note = cs->note + fx_get_second_param(cs->command);
  if (note >= MAX_NOTE)
    note = MAX_NOTE - 1;

  cs->arpnotes[2] = pitch_table[note][cs->finetune];

  DEBUG_FX("notes: %u, %u, %u\n", cs->arpnotes[0], cs->arpnotes[1], cs->arpnotes[2]);
}


static void
fx_ontick_arpeggio(mod_context_t* mc, chan_state_t* cs)
{
  if (cs->arpindex == 3)
    cs->arpindex = 0;


  cs->currentperiod = cs->arpnotes[cs->arpindex++];

  update_chan_period(cs);
  DEBUG_FX("[%u] arpindex: %u %08x %04x\n", cs->ichan, cs->arpindex,cs->modperiod,cs->currentperiod);
}


/* slide up */

static void
fx_ondiv_slide_up(mod_context_t* mc, chan_state_t* cs)
{
  int periodstep = (int)fx_get_byte_param(cs->command);
  cs->periodstep=periodstep?periodstep:cs->periodstep;

  DEBUG_FX("periodstep: %d\n", cs->periodstep);
}


static void
fx_ontick_slide_up(mod_context_t* mc, chan_state_t* cs)
{
#define MIN_NOTE_PERIOD 113 /* B3 note freq */
#define MAX_NOTE_PERIOD 856 /* C1 note freq */
  
  cs->currentperiod-=cs->periodstep;
  cs->currentperiod=LIMIT(MIN_NOTE_PERIOD,cs->currentperiod,MAX_NOTE_PERIOD);
  update_chan_period(cs);
}


/* slide down */

static void
fx_ondiv_slide_down(mod_context_t* mc, chan_state_t* cs)
{
  int periodstep = (int)fx_get_byte_param(cs->command);
  cs->periodstep=periodstep?periodstep:cs->periodstep;

  DEBUG_FX("periodstep: %d\n", cs->periodstep);
}


static void
fx_ontick_slide_down(mod_context_t* mc, chan_state_t* cs)
{
  cs->currentperiod+=cs->periodstep;
  cs->currentperiod=LIMIT(MIN_NOTE_PERIOD,cs->currentperiod,MAX_NOTE_PERIOD);
  update_chan_period(cs);
}


/* portamento (slide to note) */

static void
fx_ondiv_portamento(mod_context_t* mc, chan_state_t* cs)
{
  int periodstep = (int)fx_get_byte_param(cs->command);
  cs->periodstep=periodstep?periodstep:cs->periodstep;

  DEBUG_FX("periodstep: %d\n", cs->periodstep);
}


static void
fx_ontick_portamento(mod_context_t* mc, chan_state_t* cs)
{
  int pmax=MAX_NOTE_PERIOD,pmin=MIN_NOTE_PERIOD;
  int period=cs->currentperiod;

  if(cs->currentperiod>cs->periodtarget)
    {
      period-=cs->periodstep;
      pmin=cs->periodtarget;
    }
  else
    {
      period+=cs->periodstep;
      pmax=cs->periodtarget;
    }

  cs->currentperiod=LIMIT(pmin,period,pmax);
  update_chan_period(cs);
}


/* vibrato. from tracker/commands.c. */

static int vibrato_table[3][64] = 
  {
    {
        0,  50, 100, 149,  196, 241, 284, 325,  362, 396, 426, 452,  473, 490,  502, 510,
      512, 510, 502, 490,  473, 452, 426, 396,  362, 325, 284, 241,  196, 149,  100,  50,     
	0, -49, -99,-148, -195,-240,-283,-324, -361,-395,-425,-451, -472,-489, -501,-509,
     -511,-509,-501,-489, -472,-451,-425,-395, -361,-324,-283,-240, -195,-148,  -99, -49,
    },
#if 1
    {
      496, 480, 464, 448,  432, 416, 400, 384,  368, 352, 336, 320,  304, 288, 222, 256,
      240, 224, 208, 192,  176, 160, 144, 128,  112,  96,  48,  16,    0, -16, -32, -48,
      -64, -80, -96,-112, -128,-144,-160,-176, -192,-208,-224,-240, -256,-272,-288,-304,
     -320,-336,-352,-368, -384,-400,-416,-432, -448,-464,-480,-496, -512,-512,-512,-512,

    },
#else
    {
         0,  16,  32,  48,   64,  80,  96, 112,  128, 144, 160, 176,  192, 208, 224, 240,
       256, 272, 288, 304,  320, 336, 352, 368,  384, 400, 416, 432,  448, 464, 480,496,
      -512,-496,-480,-464, -448,-432,-416,-400, -384,-368,-352,-336, -320,-304,-288,-272,
      -256,-240,-224,-208, -192,-176,-160,-144, -128,-112, -96, -80,  -64, -48, -32, -16,
    },
#endif
    {
       512, 512, 512, 512,  512, 512, 512, 512,  512, 512, 512, 512,  512, 512, 512, 512,
       512, 512, 512, 512,  512, 512, 512, 512,  512, 512, 512, 512,  512, 512, 512, 512,
      -512,-512,-512,-512, -512,-512,-512,-512, -512,-512,-512,-512, -512,-512,-512,-512, 
      -512,-512,-512,-512, -512,-512,-512,-512, -512,-512,-512,-512, -512,-512,-512,-512,
    },
  };


static void
fx_ondiv_vibrato(mod_context_t* mc, chan_state_t* cs)
{
  unsigned int param;

  param = fx_get_first_param(cs->command);
  if (param)
    cs->vibrate = param;

  param = fx_get_second_param(cs->command);
  if (param)
    cs->vibdepth = param;

  if (cs->vibretrig)
    cs->viboffset = 0;

  DEBUG_FX("vibrato: %d\n", param);
}


static void
fx_ontick_vibrato(mod_context_t* mc, chan_state_t* cs)
{
  int p;
  float mod1,mod2,mod;

  // should be (x*ticks)/64 cycles in one division
  // 64*

  cs->viboffset = (cs->viboffset + cs->vibrate) & (0x40 - 1);
  p=(cs->vibtable[cs->viboffset] * cs->vibdepth); // +-512 * +-16
  mod1 = powf(2.0, (1.0f) / 12.0);
  mod2 = 1.0/mod1;
  mod=mod1+(mod2-mod1)*(0.5+p/(2*15*512.0f));

  cs->modperiod=(unsigned int)(mod*0x10000);
  //DEBUG_FX("vibrato: mod %d %d %d %f %08x\n", p, cs->vibrate,cs->vibdepth,p/(2*512.0f),cs->modperiod);
  update_chan_period(cs);
}


/* continue portamento slide to note do volume slide */

static void fx_ondiv_volume_slide(mod_context_t*, chan_state_t*);
static void fx_ontick_volume_slide(mod_context_t*, chan_state_t*);


static void
fx_ondiv_portamento_volume_slide(mod_context_t* mc, chan_state_t* cs)
{
  fx_ondiv_volume_slide(mc,cs);
}


static void
fx_ontick_portamento_volume_slide(mod_context_t* mc, chan_state_t* cs)
{
  fx_ontick_volume_slide(mc,cs);
  fx_ontick_portamento(mc,cs);
}



/* continue vibrato and do volume slide */

static void
fx_ondiv_vibrato_volume_slide(mod_context_t* mc, chan_state_t* cs)
{
  fx_ondiv_volume_slide(mc,cs);
}


static void
fx_ontick_vibrato_volume_slide(mod_context_t* mc, chan_state_t* cs)
{
  fx_ontick_volume_slide(mc,cs);
  fx_ontick_vibrato(mc,cs);
}

/* tremolo */

static void
fx_ondiv_tremolo(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_FX("tremolo: %x\n", cs->command);
}

static void
fx_ontick_tremolo(mod_context_t* mc, chan_state_t* cs)
{
}

/* set sample offset */

static void
fx_ondiv_set_sample_offset(mod_context_t* mc, chan_state_t* cs)
{
  cs->position = fx_get_byte_param(cs->command) * 256 * 2;

  DEBUG_FX("position: %u\n", cs->position);
}


/* volume slide */

static void
fx_ondiv_volume_slide(mod_context_t* mc, chan_state_t* cs)
{
  cs->volstep = fx_get_first_param(cs->command);

  if (!cs->volstep)
    cs->volstep = fx_get_second_param(cs->command) * -1;

  DEBUG_FX("volstep: %d\n", cs->volstep);
}


static void
fx_ontick_volume_slide(mod_context_t* mc, chan_state_t* cs)
{
  cs->volume+=cs->volstep;
  cs->volume=LIMIT(0,cs->volume,64);

  DEBUG_FX("volume: %u\n", cs->volume);
}


/* position jump */

static void
fx_ondiv_position_jump(mod_context_t* mc, chan_state_t* cs)
{
  /* continue at song position */

  mc->break_on_next_idiv = 1;
  mc->break_next_idiv = 0;
  mc->break_next_songpos = fx_get_byte_param(cs->command) & 0x7f;

  DEBUG_FX("songpos: %u\n", mc->break_next_songpos);
}


/* set volume */

static void
fx_ondiv_set_volume(mod_context_t* mc, chan_state_t* cs)
{
  /* legal volumes from 0 to 64 */

  cs->volume = fx_get_byte_param(cs->command);
  cs->volume = LIMIT(0,cs->volume,64);

  DEBUG_FX("volume: %u\n", cs->volume);
}


/* pattern break */

static void
fx_ondiv_pattern_break(mod_context_t* mc, chan_state_t* cs)
{
  /* continue at next pattern, new division */

  mc->break_on_next_idiv = 1;

  mc->break_next_idiv =
    (fx_get_first_param(cs->command) * 10 +
     fx_get_second_param(cs->command)) &
    0x3f;

  mc->break_next_songpos = mc->songpos + 1;

  DEBUG_FX("next_songpos: %u\n", mc->break_next_songpos);
}


/* set speed (tempo) */

static void
fx_ondiv_set_speed(mod_context_t* mc, chan_state_t* cs)
{
  const unsigned int speed = fx_get_byte_param(cs->command);

  if (speed <= 32)
    {
      mc->ticksperdivision = speed;

      DEBUG_FX("ticksperdivision: %u\n", mc->ticksperdivision);
    }
  else
    {
      // [fx] fx_ondiv_set_speed_810: tickspersecond: 25, samplespertick: 1920

      // set beats per minute, at 6 ticks per division and 4 divisions is one beat
      // samples per tick is really what we want to set here
      // number of ticks per beat is 4*mc->ticksperdivision
      //mc->tickspersecond = speed * 4 * mc->ticksperdivision / 60;
      mc->tickspersecond = speed * 4 * 6 / 60;
      mc->samplespertick = 48000 / mc->tickspersecond;

      DEBUG_FX("tickspersecond: %u, samplespertick: %u\n", mc->tickspersecond, mc->samplespertick);
    }
}


/* fineslide up */

static void
fx_ondiv_fineslide_up(mod_context_t* mc, chan_state_t* cs)
{
  /* no actual sliding */
  cs->currentperiod+=fx_get_second_param(cs->command);
  cs->currentperiod=MIN(cs->currentperiod,MAX_NOTE_PERIOD);

  DEBUG_FX("period: %u\n", cs->currentperiod);
  update_chan_period(cs);
}


/* fineslide down */

static void
fx_ondiv_fineslide_down(mod_context_t* mc, chan_state_t* cs)
{
  /* no actual sliding */
  cs->currentperiod-=fx_get_second_param(cs->command);
  cs->currentperiod=MAX(cs->currentperiod,MIN_NOTE_PERIOD);

  DEBUG_FX("period: %u\n", cs->currentperiod);
  update_chan_period(cs);
}

/* glissando */

static void
fx_ondiv_glissando(mod_context_t* mc, chan_state_t* cs)
{
}


/* vibrato waveform  */

static void
fx_ondiv_set_vibrato_waveform(mod_context_t* mc, chan_state_t* cs)
{
  const unsigned int param = fx_get_second_param(cs->command);

  DEBUG_FX("vibrato waveform: %x\n", param);

  cs->vibtable = vibrato_table[param & 0x3];
  cs->vibretrig=!(param&4);
}

/* set finetune value */

static void
fx_ondiv_set_finetune_value(mod_context_t* mc, chan_state_t* cs)
{
/*
  Finetuning are done by multiplying the frequency of the playback by 
  X^(finetune), where X ~= 1.0072382087
  This means that Amiga PERIODS, which represent delay times before 
  fetching the next sample, should be multiplied by X^(-finetune)
*/


  DEBUG_FX("set finetune value: %x\n", cs->command);
}

/* loop pattern */

static void
fx_ondiv_loop_pattern(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_FX("loop pattern: %x\n", cs->command);
}

/* set tremolo waveform */

static void
fx_ondiv_set_tremolo_waveform(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_FX("set tremolo waveform: %x\n", cs->command);
}

/* retriggersample */

static void
fx_ondiv_retrigger_sample(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_FX("retrigger sample: %x\n", cs->command);
}

static void
fx_ontick_retrigger_sample(mod_context_t* mc, chan_state_t* cs)
{
  int div=fx_get_second_param(cs->command);
  if(div && (mc->tick%div) == 0)
    cs->position=2;
}

/* fine volume slide up */

static void
fx_ondiv_fine_volume_slide_up(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_FX("fine volume slide up: %x\n", cs->command);
  cs->volume+=fx_get_second_param(cs->command);
  cs->volume=LIMIT(0,cs->volume,64);
}

/* fine volume slide down */

static void
fx_ondiv_fine_volume_slide_down(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_FX("fine volume slide down: %x\n", cs->command);
  cs->volume-=fx_get_second_param(cs->command);
  cs->volume=LIMIT(0,cs->volume,64);
}

/* cut sample */

static void
fx_ondiv_cut_sample(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_FX("cut sample: %x\n", cs->command);
}

/* delay sample */

static void
fx_ondiv_delay_sample(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_FX("delay sample: %x\n", cs->command);
}

/* delay pattern */

static void
fx_ondiv_delay_pattern(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_FX("delay pattern: %x\n", cs->command);
}

/* invert loop */

static void
fx_ondiv_invert_loop(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_FX("invert loop: %x\n", cs->command);
}




/* unknown effect */

static void fx_ondiv_unknown(mod_context_t* mc, struct chan_state* cs)
{
  DEBUG_FX("unknown: 0x%02x\n", (cs->command & 0xf00) >> 8);
}


static void fx_ontick_unknown(mod_context_t* mc, struct chan_state* cs)
{
}


/* effect dispatching tables */

struct fx_info
{
  const char* name;
  void (*ondiv)(mod_context_t*, chan_state_t*);
  void (*ontick)(mod_context_t*, chan_state_t*);
};


static const struct fx_info fx_table[] =
  {
#define EXPAND_FX_INFO_ENTRY(S) { #S, fx_ondiv_ ## S, fx_ontick_ ## S }
#define EXPAND_FX_INFO_ENTRY_NOTICK(S) { #S, fx_ondiv_ ## S, fx_ontick_unknown }

    /* base effects */

    EXPAND_FX_INFO_ENTRY(arpeggio),
    EXPAND_FX_INFO_ENTRY(slide_up),
    EXPAND_FX_INFO_ENTRY(slide_down),
    EXPAND_FX_INFO_ENTRY(portamento),
    EXPAND_FX_INFO_ENTRY(vibrato),
    EXPAND_FX_INFO_ENTRY(portamento_volume_slide),
    EXPAND_FX_INFO_ENTRY(vibrato_volume_slide),
    EXPAND_FX_INFO_ENTRY(tremolo),
    EXPAND_FX_INFO_ENTRY(unknown),
    EXPAND_FX_INFO_ENTRY_NOTICK(set_sample_offset),
    EXPAND_FX_INFO_ENTRY(volume_slide),
    EXPAND_FX_INFO_ENTRY_NOTICK(position_jump),
    EXPAND_FX_INFO_ENTRY_NOTICK(set_volume),
    EXPAND_FX_INFO_ENTRY_NOTICK(pattern_break),
    EXPAND_FX_INFO_ENTRY(unknown),
    EXPAND_FX_INFO_ENTRY_NOTICK(set_speed),

    /* extended effects */

    EXPAND_FX_INFO_ENTRY(unknown),
    EXPAND_FX_INFO_ENTRY_NOTICK(fineslide_up),
    EXPAND_FX_INFO_ENTRY_NOTICK(fineslide_down),
    EXPAND_FX_INFO_ENTRY_NOTICK(glissando),
    EXPAND_FX_INFO_ENTRY_NOTICK(set_vibrato_waveform),
    EXPAND_FX_INFO_ENTRY_NOTICK(set_finetune_value),
    EXPAND_FX_INFO_ENTRY_NOTICK(loop_pattern),
    EXPAND_FX_INFO_ENTRY_NOTICK(set_tremolo_waveform),
    EXPAND_FX_INFO_ENTRY(unknown),
    EXPAND_FX_INFO_ENTRY(retrigger_sample),
    EXPAND_FX_INFO_ENTRY_NOTICK(fine_volume_slide_up),
    EXPAND_FX_INFO_ENTRY_NOTICK(fine_volume_slide_down),
    EXPAND_FX_INFO_ENTRY_NOTICK(cut_sample),
    EXPAND_FX_INFO_ENTRY_NOTICK(delay_sample),
    EXPAND_FX_INFO_ENTRY_NOTICK(delay_pattern),
    EXPAND_FX_INFO_ENTRY_NOTICK(invert_loop)
  };


static inline unsigned int fx_get_index(uint32_t fx)
{
  /* get the fx table index */

  const unsigned int i = (fx & 0xf00) >> 8;
  
  return i != 14 ? i : ((fx & 0xf0) >> 4) + 0x10;
}


static inline const struct fx_info* fx_get_info(uint32_t fx)
{
  return &fx_table[fx_get_index(fx)];
}


static inline const char* fx_get_name(unsigned int fx)
{
  return fx_get_info(fx)->name;
}


static inline void
fx_ondiv(mod_context_t* mc, chan_state_t* cs)
{
  fx_get_info(cs->command)->ondiv(mc, cs);
}


static inline void
fx_ontick(mod_context_t* mc, chan_state_t* cs)
{
  fx_get_info(cs->command)->ontick(mc, cs);
}



/* on per channel state */

static inline void
chan_init_state(chan_state_t* cs, unsigned int ichan)
{
  memset(cs, 0, sizeof(chan_state_t));

  cs->ichan = ichan;
  cs->freq = 48000;

  cs->volume = 0x40;
  cs->sample = 1;

  cs->note = MAX_NOTE;

  cs->vibtable = vibrato_table[0];

  CHAN_SET_FLAG(cs, IS_SAMPLE_STARTING);
}



/* exported */

int mod_init(mod_context_t* mc, const void* data, size_t length)
{
  if (load_file(mc, data, length) == -1)
    return -1;

  chan_init_state(&mc->cstates[0], 0);
  chan_init_state(&mc->cstates[1], 1);
  chan_init_state(&mc->cstates[2], 2);
  chan_init_state(&mc->cstates[3], 3);

  mod_reset(mc);

  return 0;
}


void mod_fetch(mod_context_t* mc, void* obuf, unsigned int nsmps)
{
  /* fill a 48khz 16-bit signed interleaved stereo audio buffer
     from a mod file. name the mod filename, buffer the sample
     buffer, nsmps the sample count.
  */

  memset(obuf, 0, STEREO_CHAN_COUNT * BYTES_PER_SLE16_INTERLEAVED * nsmps);

  mod_produce(mc,obuf,nsmps);
}


static int inline chan_produce_sample(mod_context_t* mc,chan_state_t* cs,const struct sample_desc* sd)
{
  uint16_t roff=mc->s_roff[cs->sample-1];
  uint16_t rlength=mc->s_rlength[cs->sample-1];
  int32_t volume;

  if(!cs->samplestep) return 0;

  cs->backbuffer[3]=cs->backbuffer[2];
  cs->backbuffer[2]=cs->backbuffer[1];
  cs->backbuffer[1]=cs->backbuffer[0];

  // advance sample pointer
  cs->fraction+=cs->samplestep;
  cs->position+=(cs->fraction>>16);
  cs->fraction&=0xffff;

  // check for end
  if(roff>2 || rlength>2)
    {
      if(cs->position >= roff+rlength)
	{
	  //DEBUG_PRINTF("%d %02x loop at %08x.%04x to %04x\n",cs->ichan,cs->sample,cs->position,cs->fraction,roff);
	  cs->position=roff+(cs->position-roff)%rlength;
	  cs->fraction=0;
	  
	}
    }
  else
    {
      uint16_t length=mc->s_length[cs->sample-1];
      if(cs->position >= length)
	{
	  // should really disable instead - current steps outside of sample before stopping!!!
	  cs->smprate=0;
	  cs->samplestep=0;
	}
    }

  int sample=0;
  if(cs->samplestep)
    sample=mc->sdata[cs->sample-1][cs->position+0];
  cs->backbuffer[0]=sample;

  volume=(cs->lastvolume*cs->rampvolume+cs->volume*(64-cs->rampvolume))>>0;
  if(cs->rampvolume) cs->rampvolume--;


#if 0
  // Floor
  return (cs->backbuffer[0]*volume)>>6;
#endif
#if 1
  {
    // Linear interpolation
    int s0,s1;
    s0=cs->backbuffer[0];
    s1=cs->backbuffer[1];
    return (((s1*(int)cs->fraction+s0*(0x10000-(int)cs->fraction))>>6)*volume)>>(16);
  }
#endif
#if 0
  {
    // Cubic interpolation
    int64_t s0,s1,s2,s3;
    s0=cs->backbuffer[3]<<16;
    s1=cs->backbuffer[2]<<16;
    s2=cs->backbuffer[1]<<16;
    s3=cs->backbuffer[0]<<16;
    int64_t f2=(cs->fraction*cs->fraction)>>16;       // .16
    int64_t f1=cs->fraction;                          // .16
    int64_t frcu=(f2*s0)>>16;                         // .16
    int64_t t1=s3+3*s1;                               // .16                
    
    return ((
      s1+                                        // 16.16
      (frcu/2)+                                  // 16.16
      ( (f1*(s2-frcu/6-t1/6-s0/3))     >>16)+       
      ((f2*f1*(t1/6-s2/2))             >>32)+
      ((f2*(s2/2-s1))                  >>16) )
	    * volume) >>(16+6); 
  }
#endif
}


static void mod_produce_samples(mod_context_t* mc,int16_t* obuf,uint32_t nsmps)
{
  int32_t l,r,x;
  int32_t j;
  chan_state_t* cs;
  for(j=0;j<nsmps;j++)
    {
      l=r=0;
      cs=&mc->cstates[0];
      l+=chan_produce_sample(mc,cs,&mc->sdescs[cs->sample-1]);
      cs=&mc->cstates[1];
      r+=chan_produce_sample(mc,cs,&mc->sdescs[cs->sample-1]);
      cs=&mc->cstates[2];
      r+=chan_produce_sample(mc,cs,&mc->sdescs[cs->sample-1]);
      cs=&mc->cstates[3];
      l+=chan_produce_sample(mc,cs,&mc->sdescs[cs->sample-1]);
      //l*=2;
      //r*=2;
      (*obuf++)=(l+r/2);
      (*obuf++)=(r+l/2);
    }
}

static void mod_produce(mod_context_t* mc,int16_t* obuf,uint32_t nsmps)
{
  
  int32_t d;
  int32_t chunk;

  //printf("mod_produce %p %p %d\n",mc,obuf,nsmps);

  while(nsmps>0)
    {
      if(mc->samplesproduced>=mc->samplespertick)
	{	  
	  mod_tick(mc);
	  mc->samplesproduced=0;
	}

      d=mc->samplespertick-mc->samplesproduced;
      if(d<0) d=0;
      if(d>nsmps)
	chunk=nsmps;
      else
	chunk=d;
      if(chunk)
	mod_produce_samples(mc,obuf,chunk);
      mc->samplesproduced+=chunk;
      obuf+=chunk*2;
      nsmps-=chunk;
    }
}


static const void* mod_get_playhead(mod_context_t* mc)
{
  const uint32_t ipat = mc->song[mc->songpos];
  const void *patternpos=mc->pdata+ipat*mc->patternsize;
  const void *playhead=patternpos+mc->idiv*mc->divisionsize;

  //DEBUG_PRINTF("%08x ",playhead-(void*)mc->sdescs+20);
  return playhead;
}

static int32_t chan_process_div(chan_state_t* cs,uint8_t* playhead,mod_context_t* mc)
{
  uint32_t sample= (playhead[0] & 0xf0) | (playhead[2]>>4);
  uint32_t period= ((playhead[0] & 0x0f) << 8) | playhead[1];
  uint32_t fx    = ((playhead[2] & 0x0f) << 8) | playhead[3]; 
  //printf("[S%02x P%3x F%3x]",sample,period,fx);

  cs->modperiod=0x10000;

  if(period)
    {
      int cmd=fx>>8;
      if(cmd==3 || cmd==5)
	{
	  // portamento so we set target instead
	  cs->periodtarget = period;
	  DEBUG_PRINTF("portamento from %04x to %04x\n",cs->currentperiod,period);
	}
      else
	{	
	  cs->periodtarget=0;
	  cs->period = period;
	  cs->currentperiod = period;
	}	  
      update_chan_period(cs);
    }

  if (sample)
    {
      cs->sample=sample;
      cs->volume=mc->sdescs[sample-1].volume;
      cs->volstep = 0;
      cs->finetune = get_sample_finetune(mc, sample - 1);
    }

  cs->command=fx;

  if(sample || period)
    {
      // note on ie restart sample if sample or period is set
      cs->position=2;
      cs->fraction=0;
    }

  // per division command processing
  fx_ondiv(mc, cs);
  
  return 0;
}

static void chan_process_tick(chan_state_t* cs, mod_context_t* mc)
{
  // per tick command processing
  cs->lastvolume=cs->volume;
  cs->rampvolume=64;
  //fx_ontick(mc, cs);
}

static void mod_process_channels(mod_context_t* mc)
{
  uint8_t* playhead=(uint8_t*)mod_get_playhead(mc);
  uint32_t i=0;
  DEBUG_PRINTF("ticksperdiv %d samplespertick %d\n",mc->ticksperdivision,mc->samplespertick);
  //printf("s%3d p%3d d%3d t%2d ",mc->songpos,mc->song[mc->songpos],mc->idiv,mc->tick);
  for(i=0;i<mc->channels;i++,playhead+=4)
    {
      chan_state_t *cs=&mc->cstates[i];
      //printf("S%02x V%02x %04x.%04x ",cs->sample,cs->volume,cs->samplestep>>16,cs->samplestep&0xffff);
      chan_process_div(cs,playhead,mc);      
    }
  //printf("\n");
}

static void mod_tick_channels(mod_context_t* mc)
{
  uint32_t i;
  for(i=0;i<mc->channels;i++)
    chan_process_tick(&mc->cstates[i],mc);      
}

static void mod_reset(mod_context_t* mc)
{
  mc->tick=0;
  mc->idiv=0;
  mc->songpos=0;
  mc->play=0;
  mc->ticksperdivision=6;
  mc->tickspersecond=50;
  mc->samplespertick=48000/mc->tickspersecond;
  mod_process_channels(mc);
}

static void __attribute__((unused)) mod_play(mod_context_t* mc)
{
  mc->play=1;
}

static void __attribute__((unused)) chan_tick(chan_state_t* cs)
{
}

static void mod_tick(mod_context_t* mc)
{
  mc->tick++;

  if(mc->tick>=mc->ticksperdivision)
    {
      mc->tick=0;
      mc->idiv++;
      if(mc->idiv>=mc->divisionsperpattern)
	{
	  // end of pattern so advance song position
	  mc->idiv=0;
	  mc->songpos++;
	  if(mc->songpos>=mc->songlength)
	    {
	      // end of song: stop or restart from beginning?
	      mc->songpos=0;
	    }
	}

      if(mc->break_on_next_idiv)
	{
	  mc->idiv=mc->break_next_idiv&0x3f;
	  mc->songpos=mc->break_next_songpos;
	  if(mc->songpos>=mc->songlength)
	    mc->songpos=0;
	  mc->break_on_next_idiv=0;
	}

      // Process channel states
      mod_process_channels(mc);      
    }
  mod_tick_channels(mc);
}
