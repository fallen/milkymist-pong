/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Thu Sep  3 05:42:47 2009 texane
** Last update Tue Sep  8 08:28:37 2009 texane
*/



#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stddef.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "mod.h"
#include "file.h"
#include "debug.h"



/* types */

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#if 0
typedef /* signed */ char int8_t;
typedef /* signed */ short int16_t;
typedef /* signed */ int int32_t;
#endif



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


#define PAL_SYNC_RATE 7093789.2
#define NTSC_SYNC_RATE 7159090.5


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
  struct fx_state fx_state;
};

typedef struct chan_state chan_state_t;


struct mod_context
{
  file_context_t fc;

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

  unsigned int play;

  struct chan_state cstates[4];
};



void mod_reset(mod_context_t* mc);
void mod_tick(mod_context_t* mc);
void mod_produce(mod_context_t* mc,int16_t* obuf,unsigned int nsmps);

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


static inline int get_sample_finetune(const mod_context_t* mc,
				      unsigned int ismp)
{
  const int finetune = get_sample_desc(mc, ismp)->finetune & 0x0f;
  return finetune & 0x8 ? -((finetune & 0x7) + 1) : finetune & 0x7;
}


static unsigned int get_sample_repeat_offset(const mod_context_t* mc,
					     unsigned int ismp)
{
  return be16_to_host(&get_sample_desc(mc, ismp)->roff) * BYTES_PER_WORD;
}


static unsigned int get_sample_repeat_length(const mod_context_t* mc,
					     unsigned int ismp)
{
  /* only valid if > 1 */

  unsigned int len;

  len = be16_to_host(&get_sample_desc(mc, ismp)->rlength);
  if (len <= 1)
    return 0;

  return len * BYTES_PER_WORD;
}

#if 0
static inline const void* get_chan_data(const mod_context_t* mc, const struct chan_state* cs)
{
#define DIVS_PER_PAT 64
#define CHANS_PER_DIV 4
#define BYTES_PER_CHAN 4
#define BYTES_PER_DIV (CHANS_PER_DIV * BYTES_PER_CHAN)
#define BYTES_PER_PAT (DIVS_PER_PAT * BYTES_PER_DIV) /* 1024 */

  return
    (const unsigned char*)mc->pdata
 +
    cs->ipat * BYTES_PER_PAT +
    cs->idiv * BYTES_PER_DIV +
    cs->ichan * BYTES_PER_CHAN
;
}
#endif


static int load_file(mod_context_t* mc)
{
  unsigned int npats;
  unsigned int ismp;
  unsigned char ipos;
  unsigned char npos;
  int i;
  const unsigned char* pos;
  size_t size;
  struct reader_context rc;

  reader_init(&rc, file_get_data(&mc->fc), file_get_size(&mc->fc));

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

      //printf("%02x %04x %04x %04x\n",i+1,mc->s_length[i],mc->s_roff[i],mc->s_rlength[i]);
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

  npats+=1;
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
      size += get_sample_length(mc, ismp);
    }
#if 0
  printf("ends at %08x\n",rc.pos+size-(uint32_t)mc->pdata+0x43c);
#endif

  /* check there is enough room for samples */

  if (reader_check_size(&rc, size) == -1)
    {
      DEBUG_ERROR("reader_check_size(%u, %u)\n", (uint32_t)size, (uint32_t)rc.size);
      goto on_error;
    }

  return 0;

 on_error:

  return -1;
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

static inline unsigned int fx_get_first_param(unsigned int fx)
{
  return (fx & 0xf0) >> 4;
}


static inline unsigned int fx_get_second_param(unsigned int fx)
{
  return fx & 0x0f;
}


static inline unsigned int fx_get_byte_param(unsigned int fx)
{
  /* (fx_get_first_param() * 16 + fx_get_second_param() */

  return fx & 0xff;
}


/* arpeggio */

static void
fx_init_arpeggio(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_PRINTF("(%x, %x)\n", fx_get_first_param(cs->fx_data),
	       fx_get_second_param(cs->fx_data));
}


static void
fx_apply_arpeggio(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_ENTER();
}


/* slide up */

static void
fx_init_slide_up(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_ENTER();
}


static void
fx_apply_slide_up(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_ENTER();
}


/* slide down */

static void
fx_init_slide_down(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_ENTER();
}


static void
fx_apply_slide_down(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_ENTER();
}


/* slide to note */

static void
fx_init_slide_to_note(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_ENTER();
}


static void
fx_apply_slide_to_note(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_ENTER();
}


/* vibrato */

static void
fx_init_vibrato(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_ENTER();
}


static void
fx_apply_vibrato(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_ENTER();
}


/* set sample offset */

static void
fx_init_set_sample_offset(mod_context_t* mc, chan_state_t* cs)
{
  unsigned int smpoff =
    fx_get_first_param(cs->fx_data) * 4096 +
    fx_get_second_param(cs->fx_data) * 256 *
    BYTES_PER_WORD;

  DEBUG_PRINTF("(%x)\n", smpoff);

  if (smpoff >= (get_sample_length(mc, cs->ismp) - 1))
    {
      DEBUG_ERROR("smpoff >= length\n");
      smpoff = 0;
    }

  cs->smpoff = smpoff;
}


static void
fx_apply_set_sample_offset(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_ENTER();
}


/* volume slide */

static void
fx_init_volume_slide(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_PRINTF("(%x, %x)\n", fx_get_first_param(cs->fx_data),
	       fx_get_second_param(cs->fx_data));
}


static void
fx_apply_volume_slide(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_ENTER();
}


/* position jump */

static void
fx_init_position_jump(mod_context_t* mc, chan_state_t* cs)
{
  /* continue at song position */

  const unsigned int pos = fx_get_byte_param(cs->fx_data);
  
  DEBUG_PRINTF("(%x)\n", pos);

  //cs->ipat = mc->song[pos & 0x7f];
}


static void
fx_apply_position_jump(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_ENTER();
}


/* set volume */

static void
fx_init_set_volume(mod_context_t* mc, chan_state_t* cs)
{
  /* legal volumes from 0 to 64 */

  cs->volume = fx_get_byte_param(cs->fx_data);

  DEBUG_PRINTF("(%u)\n", cs->volume);

  if (cs->volume >= 64)
    cs->volume = 64;
}


static void
fx_apply_set_volume(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_ENTER();
}


/* pattern break */

static void
fx_init_pattern_break(mod_context_t* mc, chan_state_t* cs)
{
  /* continue at next pattern, new division */

  //cs->ipat += 1;

#if 0
  cs->idiv =
    (fx_get_first_param(cs->fx_data) * 10 +
     fx_get_second_param(cs->fx_data)) &
    0x3f;

  DEBUG_PRINTF("(%x)\n", cs->idiv);
#endif
}


static void
fx_apply_pattern_break(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_ENTER();
}


/* set speed */

static void
fx_init_set_speed(mod_context_t* mc, chan_state_t* cs)
{
  unsigned int speed = fx_get_byte_param(cs->fx_data);

  DEBUG_PRINTF("(%x)\n", speed);

  if (speed == 0)
    speed = 1;
}


static void
fx_apply_set_speed(mod_context_t* mc, chan_state_t* cs)
{
  DEBUG_ENTER();
}


/* unknown effect */

static void fx_init_unknown(mod_context_t* mc, struct chan_state* cs)
{
  DEBUG_PRINTF("unknown fx(0x%03x)\n", cs->fx_data);
}


static void fx_apply_unknown(mod_context_t* mc, struct chan_state* cs)
{
  DEBUG_ENTER();
}


/* effect dispatching tables */

struct fx_info
{
  const char* name;
  void (*init)(mod_context_t*, chan_state_t*);
  void (*apply)(mod_context_t*, chan_state_t*);
};


static const struct fx_info fx_table[] =
  {
#define EXPAND_FX_INFO_ENTRY(S) { #S, fx_init_ ## S, fx_apply_ ## S }

    /* base effects */

    EXPAND_FX_INFO_ENTRY(arpeggio),
    EXPAND_FX_INFO_ENTRY(slide_up),
    EXPAND_FX_INFO_ENTRY(slide_down),
    EXPAND_FX_INFO_ENTRY(slide_to_note),
    EXPAND_FX_INFO_ENTRY(vibrato),
    EXPAND_FX_INFO_ENTRY(unknown),
    EXPAND_FX_INFO_ENTRY(unknown),
    EXPAND_FX_INFO_ENTRY(unknown),
    EXPAND_FX_INFO_ENTRY(unknown),
    EXPAND_FX_INFO_ENTRY(set_sample_offset),
    EXPAND_FX_INFO_ENTRY(volume_slide),
    EXPAND_FX_INFO_ENTRY(position_jump),
    EXPAND_FX_INFO_ENTRY(set_volume),
    EXPAND_FX_INFO_ENTRY(pattern_break),
    EXPAND_FX_INFO_ENTRY(unknown),
    EXPAND_FX_INFO_ENTRY(set_speed),

    /* extended effects */

    EXPAND_FX_INFO_ENTRY(unknown),
    EXPAND_FX_INFO_ENTRY(unknown),
    EXPAND_FX_INFO_ENTRY(unknown),
    EXPAND_FX_INFO_ENTRY(unknown),
    EXPAND_FX_INFO_ENTRY(unknown),
    EXPAND_FX_INFO_ENTRY(unknown),
    EXPAND_FX_INFO_ENTRY(unknown),
    EXPAND_FX_INFO_ENTRY(unknown),
    EXPAND_FX_INFO_ENTRY(unknown),
    EXPAND_FX_INFO_ENTRY(unknown),
    EXPAND_FX_INFO_ENTRY(unknown),
    EXPAND_FX_INFO_ENTRY(unknown),
    EXPAND_FX_INFO_ENTRY(unknown),
    EXPAND_FX_INFO_ENTRY(unknown),
    EXPAND_FX_INFO_ENTRY(unknown),
    EXPAND_FX_INFO_ENTRY(unknown)
  };


static inline unsigned int fx_get_index(unsigned int fx)
{
  /* get the fx table index */

  const unsigned int i = (fx & 0xf00) >> 8;

  return i != 14 ? i : fx & 0xf0;
}


static inline const struct fx_info* fx_get_info(unsigned int fx)
{
  return &fx_table[fx_get_index(fx)];
}


static inline const char* fx_get_name(unsigned int fx)
{
  return fx_get_info(fx)->name;
}


static inline void
fx_init(mod_context_t* mc, chan_state_t* cs)
{
  fx_get_info(cs->fx_data)->init(mc, cs);
}


static inline void
fx_apply(mod_context_t* mc, chan_state_t* cs)
{
  fx_get_info(cs->fx_data)->apply(mc, cs);
}



/* on per channel state */

static inline void
chan_init_state(chan_state_t* cs, unsigned int ichan, unsigned int ipat)
{
  memset(cs, 0, sizeof(struct chan_state));

  cs->ichan = ichan;
  cs->freq=48000;

  cs->volume=0x40;
  cs->sample=1;

  CHAN_SET_FLAG(cs, IS_SAMPLE_STARTING);
}




#if 0




static inline int is_end_of_repeating_sample(const mod_context_t* mc,
					     const struct chan_state* cs,
					     unsigned int ismp)
{
  return
    cs->smpoff >
    get_sample_repeat_offset(mc, ismp) +
    get_sample_repeat_length(mc, ismp) -
    MOD_CHAN_COUNT;
}


static inline int is_end_of_sample(const mod_context_t* mc,
				   const struct chan_state* cs,
				   unsigned int ismp)
{
  if ( ! CHAN_HAS_FLAG(cs, IS_SAMPLE_REPEATING) )
    return cs->smpoff > (get_sample_length(mc, ismp) - MOD_CHAN_COUNT);

  return is_end_of_repeating_sample(mc, cs, ismp);
}


static inline unsigned int
get_remaining_sample_length(const mod_context_t* mc,
			    const struct chan_state* cs)
{
  if ( ! CHAN_HAS_FLAG(cs, IS_SAMPLE_REPEATING) )
    return get_sample_length(mc, cs->ismp) - cs->smpoff;

  return
    get_sample_repeat_length(mc, cs->ismp) -
    (cs->smpoff - get_sample_repeat_offset(mc, cs->ismp));
}


static inline unsigned int
get_remaining_sample_count(const mod_context_t* mc,
			   const struct chan_state* cs)
{
  return get_remaining_sample_length(mc, cs) / MOD_CHAN_COUNT;
}


static inline const void* get_chan_sample_data(const mod_context_t* mc,
					       const struct chan_state* cs)
{
  return (const int8_t*)get_sample_data(mc, cs->ismp) + cs->smpoff;
}


static unsigned int compute_resampling_ratio(unsigned int nsmps_at_48khz,
					     unsigned int nsmps_at_smprate)
{
  /* todo: round to the nearest ratio */

  const unsigned int ratio = nsmps_at_48khz / nsmps_at_smprate;
  return ratio ? ratio : 1;
}


static int16_t mix(int8_t a, int8_t b)
{
  /* todo: optimize */

  int16_t sum = (int16_t)a + (int16_t)b;

#if 0
  {
#define MIN_INT8 -128
#define MAX_INT8 127

  if (sum < MIN_INT8)
    sum = MIN_INT8;
  else if (sum > MAX_INT8)
    sum = MAX_INT8;
  }
#endif

  return sum;
}


static void mix_8bits_4chans_buffer(int8_t* ibuf, unsigned int nsmps)
{
  /* todo: must be moved in the resample routine. */

  for (; nsmps; --nsmps, ibuf += MOD_CHAN_COUNT)
    {
      int16_t* const p = (int16_t*)ibuf;

      p[0] = p[1] =
	(int16_t)((int32_t)ibuf[0] +
		  (int32_t)ibuf[1] +
		  (int32_t)ibuf[2] +
		  (int32_t)ibuf[3]);
    }
}


static void*
resample(int16_t* obuf, const int8_t* ibuf,
	 unsigned int nsmps,
	 unsigned int ratio,
	 unsigned int ichan)
{
  /* obuf is sle16 interleaved
     ibuf is 8bits pcm
     nsmps the input sample count
     ratio the resampling ratio
   */

  /* todo: smoothing / averaging function */

#if 0

  size_t roff = 0;
  unsigned int i;

  ibuf += ichan;

  switch (ichan)
    {
    case 1:
      /* unalign buffers and fallthrough (dont break) */
      roff = 1;
      ++obuf;

    case 0:
      for (; nsmps; --nsmps, ibuf += MOD_CHAN_COUNT)
	for (i = 0; i < ratio; ++i, obuf += STEREO_CHAN_COUNT)
	  *obuf = *ibuf;
      break;

    case 2:
      /* unalign buffers and fallthrough (dont break) */
      roff = 1;
      ++obuf;

    case 3:
      /* assume chan0 sample already stored */
      for (; nsmps; --nsmps, ibuf += MOD_CHAN_COUNT)
	for (i = 0; i < ratio; ++i, obuf += STEREO_CHAN_COUNT)
	  *obuf = mix(*obuf, *ibuf);
      break;
    }

  return obuf - roff;

#else /* 4tracks -> mono -> stereo */

  /* use the output buffer as a temporary one
     to store 8bits pcm samples. when chan 3,
     mix all the previously stored samples.
   */

  unsigned int i;

  obuf += ichan;

  for (obuf += ichan; nsmps; --nsmps, ibuf += MOD_CHAN_COUNT)
    for (i = 0; i < ratio; ++i, obuf += STEREO_CHAN_COUNT)
      *obuf = *ibuf;

  obuf -= ichan;

  return obuf;

#endif
}


static inline unsigned int
nsmps_from_smprate_to_48khz(unsigned int nsmps, unsigned int smprate)
{
  const unsigned int count = (48000 * nsmps) / smprate;
  return count ? count : 1;
}


static inline unsigned int
nsmps_from_48khz_to_smprate(unsigned int nsmps, unsigned int smprate)
{
  const unsigned int count = (smprate * nsmps) / 48000;
  return count ? count : 1;
}


int chan_produce_samples(struct chan_state* cs,
			 mod_context_t* mc,
			 int16_t* obuf,
			 unsigned int nsmps)
{
  /* produce nsmps 48khz samples */

 produce_more_samples:

  if (CHAN_HAS_FLAG(cs, IS_SAMPLE_STARTING))
    {
      /* this is a new sample/division. we initialize
	 here whatever is needed to make progress in
	 the division, esp. the per chan state.
      */

      const unsigned char* chan_data;
      unsigned int ismp;
      unsigned int period;

      /* extract channel data */

      chan_data = get_chan_data(mc, cs);

      ismp = (chan_data[0] & 0xf0) | (chan_data[2] >> 4);
      period = ((unsigned int)(chan_data[0] & 0x0f) << 8) | chan_data[1];

      /* todo: is this necessary */

      if (!cs->ismp && !ismp)
	return 0;

      CHAN_CLEAR_FLAG(cs, IS_SAMPLE_STARTING);

      /* decrement since samples are 1 based */

      if (ismp)
	cs->ismp = ismp - 1;

      {
#if 0 
	DEBUG_PRINTF("newSample: [%u, %u, %u, %u] {%u, %u, %u}\n",
		     cs->ichan, cs->ipat, cs->idiv, cs->ismp,
		     get_sample_length(mc, cs->ismp),
		     get_sample_repeat_offset(mc, cs->ismp),
		     get_sample_repeat_length(mc, cs->ismp));
#endif
      }

      /* sample volume */

      cs->volume = get_sample_volume(mc, cs->ismp);

      /* sample rate (bytes per sec to send). todo, finetune. */

      if (period)
	cs->smprate = PAL_SYNC_RATE / (period * 2);

      /* fx initialization is done here */

      cs->fx_data = ((unsigned int)(chan_data[2] & 0x0f) << 8) | chan_data[3];
      fx_init(mc, cs);
    }
  else if (CHAN_HAS_FLAG(cs, IS_SAMPLE_REPEATING))
    {
      if (is_end_of_repeating_sample(mc, cs, cs->ismp))
	cs->smpoff = get_sample_repeat_offset(mc, cs->ismp);
    }
  else if (cs->smpoff >= get_sample_length(mc, cs->ismp))
    {
      /* sample done, is a repeating one */

      if (get_sample_repeat_length(mc, cs->ismp))
	{
	  CHAN_SET_FLAG(cs, IS_SAMPLE_REPEATING);
	  cs->smpoff = get_sample_repeat_offset(mc, cs->ismp);
	}
      else
	{
	  /* next sample */

	  CHAN_SET_FLAG(cs, IS_SAMPLE_STARTING);

	  cs->smpoff = 0;

	  /* next pattern on end of division */

#if 0
	  if ((++cs->idiv) >= DIVS_PER_PAT)
	    {
	      cs->idiv = 0;

	      if ((++cs->ipat) >= mc->npats)
		cs->ipat = 0;
	    }
#endif
	}

      goto produce_more_samples;
    }

  /* apply the effect. see above for initialization. */

  fx_apply(mc, cs);

  /* generate nsmps 48khz samples */

  while (nsmps)
    {
      unsigned int nsmps_at_smprate;
      unsigned int nsmps_at_48khz;
      unsigned int ratio;

      /* how much can we produce with the current sample */

      nsmps_at_smprate = get_remaining_sample_count(mc, cs);

      if (!nsmps_at_smprate)
	{
	  /* no more, sample done */

	  cs->smpoff = get_sample_length(mc, cs->ismp);
	}

      if (is_end_of_sample(mc, cs, cs->ismp))
	goto produce_more_samples;

      /* to remove. when only 2 chans, this is set
	 to 0 but it should be detected earlier
      */
      if (!cs->smprate)
	return 0;

      /* how much can we produce at 48khz */

      nsmps_at_48khz =
	nsmps_from_smprate_to_48khz(nsmps_at_smprate, cs->smprate);

      /* count would be greater than needed */

      if (nsmps_at_48khz > nsmps)
	{
	  nsmps_at_48khz = nsmps;

	  nsmps_at_smprate =
	    nsmps_from_48khz_to_smprate(nsmps_at_48khz, cs->smprate);
	}

      ratio =
	compute_resampling_ratio(nsmps_at_48khz, nsmps_at_smprate);

      DEBUG_PRINTF("{%u, %u}: %u - %u - %x\n", cs->ismp, cs->smpoff, nsmps_at_48khz, cs->smprate, cs->flags);

      obuf =
	resample(obuf, get_chan_sample_data(mc, cs),
		 nsmps_at_smprate, ratio, cs->ichan);

      nsmps -= nsmps_at_48khz;

      cs->smpoff += nsmps_at_smprate * MOD_CHAN_COUNT;
    }

  return 0;
}



#endif








/* exported */

int mod_load_file(mod_context_t** mc, const char* path)
{
  *mc = malloc(sizeof(mod_context_t));
  if (*mc == NULL)
    return -1;

  if (file_open(&(*mc)->fc, path) == -1)
    goto on_open_error;

  if (load_file(*mc) == -1)
    goto on_error;



  chan_init_state(&(*mc)->cstates[0], 0, (*mc)->ipat);
  chan_init_state(&(*mc)->cstates[1], 1, (*mc)->ipat);
  chan_init_state(&(*mc)->cstates[2], 2, (*mc)->ipat);
  chan_init_state(&(*mc)->cstates[3], 3, (*mc)->ipat);

  mod_reset(*mc);

  return 0;

 on_open_error:
  free(*mc);
  *mc = NULL;
  return -1;

 on_error:
  mod_destroy(*mc);
  *mc = NULL;
  return -1;
}


void mod_destroy(mod_context_t* mc)
{
  file_close(&mc->fc);
  free(mc);
}














int mod_fetch(mod_context_t* mc, void* obuf, unsigned int nsmps)
{
  /* fill a 48khz 16-bit signed interleaved stereo audio buffer
     from a mod file. name the mod filename, buffer the sample
     buffer, nsmps the sample count.
  */

  memset(obuf, 0, STEREO_CHAN_COUNT * BYTES_PER_SLE16_INTERLEAVED * nsmps);

#if 0
  chan_produce_samples(&mc->cstates[0], mc, obuf, nsmps);
  chan_produce_samples(&mc->cstates[1], mc, obuf, nsmps);
  chan_produce_samples(&mc->cstates[2], mc, obuf, nsmps);
  chan_produce_samples(&mc->cstates[3], mc, obuf, nsmps);
#else
  mod_produce(mc,obuf,nsmps);
#endif

  return 0;
}

















int inline chan_produce_sample(mod_context_t* mc,chan_state_t* cs,struct sample_desc* sd)
{
  uint16_t roff=mc->s_roff[cs->sample-1];
  uint16_t rlength=mc->s_rlength[cs->sample-1];
  // advance sample pointer
  cs->fraction+=cs->samplestep;
  cs->position+=(cs->fraction>>16);
  cs->fraction&=0xffff;

  
  
  // check for end
  if(roff>2 || rlength>2)
    {
      
      if(cs->position >= roff+rlength)
	{
	  //printf("%d %02x loop at %08x.%04x to %04x\n",cs->ichan,cs->sample,cs->position,cs->fraction,roff);
	  cs->position=roff;
	}
    }
  else
    {
      uint16_t length=mc->s_length[cs->sample-1];
      if(cs->position >= length)
	{
	  // should really disable instead - current steps outside of sample before stopping!!!
	  //printf("end at %08x.%04x\n",cs->position,cs->fraction);
	  cs->smprate=0;
	  cs->samplestep=0;
	}
    }

  if(0 && cs->samplestep)
    {
      printf("%d %2x %p %4d %08x.%04x %08x %08x\n",cs->ichan,cs->sample,mc->sdata[cs->sample-1],mc->sdata[cs->sample-1][cs->position],cs->position,cs->fraction,cs->smprate,cs->samplestep);
    }
  
  
  return mc->sdata[cs->sample-1][cs->position]*cs->volume;
}


void mod_produce_samples(mod_context_t* mc,int16_t* obuf,uint32_t nsmps)
{
  int32_t l,r;
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
      l*=2;
      r*=2;
      
      (*obuf++)=l;
      (*obuf++)=r;
    }
}

void mod_produce(mod_context_t* mc,int16_t* obuf,uint32_t nsmps)
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


const void* mod_get_playhead(mod_context_t* mc)
{
  uint32_t ipat=mc->song[mc->songpos];
  const void *patternpos=mc->pdata+ipat*mc->patternsize;
  const void *playhead=patternpos+mc->idiv*mc->divisionsize;
  //DEBUG_PRINTF("%08x ",playhead-(void*)mc->sdescs+20);
  return playhead;
}

int32_t chan_process_div(chan_state_t* cs,uint8_t* playhead,mod_context_t* mc)
{
  uint32_t sample= (playhead[0] & 0xf0) | (playhead[2]>>4);
  uint32_t period= ((playhead[0] & 0x0f) << 8) | playhead[1];
  uint32_t fx    = ((playhead[2] & 0x0f) << 8) | playhead[3]; 
  //DEBUG_PRINTF("[S%02x P%3x F%3x]\n",sample,period,fx);

  if(period)
    {
      cs->period=period;
      // number of samples to step per second in source sample
      cs->smprate = PAL_SYNC_RATE / (period * 2);
      cs->samplestep = (cs->smprate<<16)/48000;
      //DEBUG_PRINTF("%08x %08x\n",cs->smprate,cs->samplestep);
      //printf("%08d %08d %08d %08d\n",cs->smprate,cs->samplestep,(cs->smprate*48000)>>16,(cs->samplestep*48000)>>16);
    }
  if(sample)
    {
      cs->sample=sample;
      cs->volume=mc->sdescs[sample-1].volume;
    }
  cs->command=fx;

  if(sample || period)
    {
      // note on ie restart sample if sample or period is set
      cs->position=cs->fraction=0;
    }

  uint8_t fcmd=(fx>>8);
  uint8_t fdata=fx&0xff;
  switch(fcmd)
    {
    case 0xc: // Set volume
      if(fdata>64) fdata=64;
      cs->volume=fdata;
      //printf("volume %d %02d\n",cs->ichan,cs->volume);
      break;
    case 0x9: printf("set sample offset\n"); break;
    case 0xe:
      switch(fdata>>4)
	{
	case 0x9: printf("retrigger sample\n"); break;
	case 0xd: printf("delay sample\n"); break;
	}
      break;
    case 0xf: // Set tempo
      if(fdata<=32) mc->ticksperdivision=fdata;
      else 
	{
	  // set beats per minute, at 6 ticks per division and 4 divisions is one beat
	  // samples per tick is really what we want to set here?
	  // number of ticks per beat is 4*mc->ticksperdivision
	  // default is 50 ticks per second
	  // 50*60/bmp is new number of ticks per second

	  // ticks per minute
	  mc->tickspersecond=fdata*4*mc->ticksperdivision/60;
	  mc->samplespertick=48000/mc->tickspersecond;
	}
    }
  
  return 0;
}

void chan_process_tick(chan_state_t* cs,mod_context_t*mc)
{
  // process effects
}

void mod_process_channels(mod_context_t* mc)
{
  uint8_t* playhead=(uint8_t*)mod_get_playhead(mc);
  uint32_t i=0;
  //printf("s%3d p%3d d%3d t%2d ",mc->songpos,mc->song[mc->songpos],mc->idiv,mc->tick);
  for(i=0;i<mc->channels;i++,playhead+=4)
    {
      chan_state_t *cs=&mc->cstates[i];
      //printf("S%02x V%02x %04x.%04x ",cs->sample,cs->volume,cs->samplestep>>16,cs->samplestep&0xffff);
      chan_process_div(cs,playhead,mc);      
    }
  //printf("\n");
}

void mod_tick_channels(mod_context_t* mc)
{
  uint32_t i;
  for(i=0;i<mc->channels;i++)
    chan_process_tick(&mc->cstates[i],mc);      
}

void mod_reset(mod_context_t* mc)
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

void mod_play(mod_context_t* mc)
{
  mc->play=1;
}

void chan_tick(chan_state_t* cs)
{
}

void mod_tick(mod_context_t* mc)
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
      // Process channel states
      mod_process_channels(mc);      
    }
  mod_tick_channels(mc);
}









#if 0


/* sample stream */

struct sstream
{
  unsigned int nsamples;
  unsigned int pos;
  unsigned int32_t data[1];
};


static void sstream_init(struct sstream* ss)
{
}


static void sstream_release(struct sstream* ss)
{
  
}


static inline unsigned int sstream_count(const struct sstream* ss)
{
  return ss->count;
}


static inline int sstream_is_empty(const struct sstream* ss)
{
  return ss->pss == 0;
}


static void sstream_write(struct sstream* ss, const int8_t* smps, unsigned int nsmps, unsigned int freq)
{
  /* assumes 8bit pcm samples sampled at freq hz.
     nsmps is an inout param containing the sample
     count and being udpated with the number of
     samples consumed.
   */

  const unsigned int n = 48000 / freq;

  unsigned int i;
  unsigned int j;

  for (j = 0; j < *nsmps; ++j, ++smps)
    {
      if (ss->nsmps < n)
	break;

      for (i = 0; i < n; ++i, ++obuf)
	*obuf = *smps;
    }

  *nsmps = j;
}


static void mix_sstreams(int16_t* obuf, struct sstream* s0, struct sstream* s1, unsigned int nsmps)
{
  /* mix 4 sstreams into the interleaved le16 obuf */

  const unsigned int nsamples = ss[0]->nsamples;

  const uint32_t* p; = ss0->data;
  const uint32_t* q; = ss1->data;

  for (i = 0; i < ss->nsamples; ++i, ++p, ++q, ++r, ++s, obuf += 2)
    {
      obuf[0] = (uint16_t)((uint32_t)*p + (uint32_t)*q);
      obuf[1] = (uint16_t)((uint32_t)*r + (uint32_t)*s);
    }
}


#endif
