/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Thu Sep  3 05:42:47 2009 texane
** Last update Mon Sep  7 20:29:44 2009 texane
*/



#include <stdio.h>
#include <stdlib.h>
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


struct chan_state
{
  unsigned int ichan;

  unsigned int freq;
  unsigned int tempo;
  unsigned int vol;

  /* current division */
  unsigned int ipat;
  unsigned int idiv;
  unsigned int ismp;

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

  /* struct sstream* ss; */
};


struct mod_context
{
  file_context_t fc;

  const struct sample_desc* sdescs;
  const void* sdata[32];

  const unsigned char* song_pos;

  unsigned int ipat; /* initial pattern */
  unsigned int npats;
  const void* pdata;

  struct chan_state cstates[4];
};



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
      DEBUG_ERROR("size(%u) > rc->size(%u)\n", rc->size, size);
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
      DEBUG_ERROR("size(%u) > rc->size(%u)\n", rc->size, size);
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


static inline const void* get_chan_data(const mod_context_t* mc, const struct chan_state* cs)
{
#define DIVS_PER_PAT 64
#define CHANS_PER_DIV 4
#define BYTES_PER_CHAN 4
#define BYTES_PER_DIV (CHANS_PER_DIV * BYTES_PER_CHAN)
#define BYTES_PER_PAT (DIVS_PER_PAT * BYTES_PER_DIV) /* 1024 */

  return
    (const unsigned char*)mc->pdata +
    cs->ipat * BYTES_PER_PAT +
    cs->idiv * BYTES_PER_DIV +
    cs->ichan * BYTES_PER_CHAN;
}


static int load_file(mod_context_t* mc)
{
  unsigned int npats;
  unsigned int ismp;
  unsigned char ipos;
  unsigned char npos;
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

  mc->sdescs = (const struct sample_desc*)rc.pos;

  /* todo: check descs (length, offset...) */

  reader_skip_safe(&rc, 31 * sizeof(struct sample_desc));

  /* song position count */

  if ((reader_read(&rc, &npos, 1) == -1) || (npos > 128))
    {
      DEBUG_ERROR("reader_read() == 0x%02x\n", npos);
      goto on_error;
    }

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

  mc->song_pos = rc.pos;

  mc->ipat = *rc.pos;

  for (pos = rc.pos, npats = 0, ipos = 0; ipos < npos; ++ipos, ++pos)
    if (npats < *pos)
      npats = *pos;

  mc->npats = npats;

  reader_skip_safe(&rc, 128);

  /* "M.K." signature */

  if (reader_skip(&rc, 4) == -1)
    {
      DEBUG_ERROR("reader_skip()\n");
      goto on_error;
    }

  /* pattern data: each pattern is divided into 64
     divisions, each division contains the channels
     data. channel data are represented by 4 bytes
  */

  if (reader_check_size(&rc, npats * BYTES_PER_PAT) == -1)
    {
      DEBUG_ERROR("reader_check_size() == -1\n");
      goto on_error;
    }

  mc->pdata = rc.pos;

  reader_skip_safe(&rc, npats * BYTES_PER_PAT);

  /* sample data. build sdata lookup table. */

  size = 0;

  for (ismp = 0; ismp < 31; ++ismp)
    {
      /* todo: check for addition sum overflows */
      mc->sdata[ismp] = rc.pos + size;
      size += get_sample_length(mc, ismp);
    }

  /* check there is enough room for samples */

  if (reader_check_size(&rc, size) == -1)
    {
      DEBUG_ERROR("reader_check_size(%u, %u)\n", size, rc.size);
      goto on_error;
    }

  return 0;

 on_error:

  return -1;
}


/* fx handling */


static inline unsigned int fx_get_first_param(unsigned int fx)
{
  return (fx & 0xf0) >> 4;
}


static inline unsigned int fx_get_second_param(unsigned int fx)
{
  return fx & 0x0f;
}


static void fx_do_arpeggio(mod_context_t* mc,
			   struct chan_state* cs,
			   unsigned int fx)
{
  DEBUG_PRINTF("(%x, %x)\n", fx_get_first_param(fx), fx_get_second_param(fx));
}


static void fx_do_set_sample_offset(mod_context_t* mc,
				    struct chan_state* cs,
				    unsigned int fx)
{
  unsigned int smpoff =
    fx_get_first_param(fx) * 4096 +
    fx_get_second_param(fx) * 256 *
    BYTES_PER_WORD;

  DEBUG_PRINTF("(%x)\n", smpoff);

  if (smpoff >= (get_sample_length(mc, cs->ismp) - 1))
    {
      DEBUG_ERROR("smpoff >= length\n");
      smpoff = 0;
    }

  cs->smpoff = smpoff;
}


static void fx_do_volume_slide(mod_context_t* mc,
			       struct chan_state* cs,
			       unsigned int fx)
{
  DEBUG_PRINTF("(%x, %x)\n", fx_get_first_param(fx), fx_get_second_param(fx));
}


static void fx_do_position_jump(mod_context_t* mc,
				struct chan_state* cs,
				unsigned int fx)
{
  /* continue at song position */
  
  DEBUG_PRINTF("(%x, %x)\n", fx_get_first_param(fx), fx_get_second_param(fx));  

  cs->ipat = mc->song_pos[(fx_get_first_param(fx) * 16 + fx_get_second_param(fx)) & 0x7f];
}


static void fx_do_set_volume(mod_context_t* mc,
			     struct chan_state* cs,
			     unsigned int fx)
{
  /* legal volumes from 0 to 64 */

  cs->vol = fx_get_first_param(fx) * 16 + fx_get_second_param(fx);
  if (cs->vol >= 64)
    cs->vol = 64;

  DEBUG_PRINTF("(%u)\n", cs->vol);
}


static void fx_do_pattern_break(mod_context_t* mc,
				struct chan_state* cs,
				unsigned int fx)
{
  /* continue at next pattern, new division */

  DEBUG_PRINTF("(%x, %x)\n", fx_get_first_param(fx), fx_get_second_param(fx));

  cs->ipat += 1;
  cs->idiv = (fx_get_first_param(fx) * 10 + fx_get_second_param(fx)) & 0x3f;
}


static void fx_do_set_speed(mod_context_t* mc,
			    struct chan_state* cs,
			    unsigned int fx)
{
  unsigned int speed = fx_get_first_param(fx) * 16 + fx_get_second_param(fx);

  DEBUG_PRINTF("(%x, %x)\n", fx_get_first_param(fx), fx_get_second_param(fx));

  if (speed == 0)
    speed = 1;
}


static void fx_do_unknown(mod_context_t* mc,
			  struct chan_state* cs,
			  unsigned int fx)
{
  DEBUG_PRINTF("unknown fx(0x%03x)\n", fx);
}


struct fx_pair
{
  void (*fn)(mod_context_t*, struct chan_state*, unsigned int);
  const char* name;
};


static const struct fx_pair base_fxs[] =
  {
    { fx_do_arpeggio, "arpeggio" },
    { fx_do_unknown, "slide up" },
    { fx_do_unknown, "slide down" },
    { fx_do_unknown, "slide to node" },
    { fx_do_unknown, "vibrato" },
    { fx_do_unknown, "" },
    { fx_do_unknown, "" },
    { fx_do_unknown, "" },
    { fx_do_unknown, "" },
    { fx_do_set_sample_offset, "set sample offset" },
    { fx_do_volume_slide, "volume slide" },
    { fx_do_position_jump, "position jump" },
    { fx_do_set_volume, "set volume" },
    { fx_do_pattern_break, "pattern break" },
    { fx_do_unknown, "" },
    { fx_do_set_speed, "set speed" }
  };


static const struct fx_pair extended_fxs[] =
  {
    { fx_do_unknown, "" },
    { fx_do_unknown, "" },
    { fx_do_unknown, "" },
    { fx_do_unknown, "" },
    { fx_do_unknown, "" },
    { fx_do_unknown, "" },
    { fx_do_unknown, "" },
    { fx_do_unknown, "" },
    { fx_do_unknown, "" },
    { fx_do_unknown, "" },
    { fx_do_unknown, "" },
    { fx_do_unknown, "" },
    { fx_do_unknown, "" },
    { fx_do_unknown, "" },
    { fx_do_unknown, "" },
    { fx_do_unknown, "" }
  };


static inline const char* fx_xid_to_string(unsigned int i)
{

#if 0

#define FX_XID_CASE(I) case FX_XID_ ## I: s = #I; break

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

  const char* s;

  switch (i)
    {
      FX_XID_CASE(SET_FILTER);
      FX_XID_CASE(FINESLIDE_UP);
      FX_XID_CASE(FINESLIDE_DOWN);
      FX_XID_CASE(SET_GLISSANDO);
      FX_XID_CASE(SET_VIBRATO_WAVEFORM);
      FX_XID_CASE(SET_FINETUNE_VALUE);
      FX_XID_CASE(LOOP_PATTERN);
      FX_XID_CASE(SET_TREMOLO_WAVEFORM);
      FX_XID_CASE(UNUSED);
      FX_XID_CASE(RERTRIGGER_SAMPLE);
      FX_XID_CASE(FINE_VOLUME_SLIDE_UP);
      FX_XID_CASE(FINE_VOLUME_SLIDE_DOWN);
      FX_XID_CASE(CUT_SAMPLE);
      FX_XID_CASE(DELAY_SAMPLE);
      FX_XID_CASE(DELAY_PATTERN);
      FX_XID_CASE(INVERT_LOOP);

    default:
      s = "unknown";
      break;
    }

  return s;

#else

  return extended_fxs[i].name;

#endif

}


static inline const char* fx_id_to_string(unsigned int i)
{

#if 0

#define FX_ID_CASE(I) case FX_ID_ ## I: s = #I; break

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

  const char* s;

  switch (i)
    {
      FX_ID_CASE(ARPEGGIO);
      FX_ID_CASE(SLIDE_UP);
      FX_ID_CASE(SLIDE_DOWN);
      FX_ID_CASE(SLIDE_TO_NOTE);
      FX_ID_CASE(VIBRATO);
      FX_ID_CASE(CONT_SLIDE_TO_NOTE_AND_DO_VOL_SLIDE);
      FX_ID_CASE(CONT_VIBRATO_AND_DO_VOL_SLIDE);
      FX_ID_CASE(TREMOLO);
      FX_ID_CASE(UNUSED);
      FX_ID_CASE(SET_SAMPLE_OFFSET);
      FX_ID_CASE(VOLUME_SLIDE);
      FX_ID_CASE(POS_JUMP);
      FX_ID_CASE(SET_VOLUME);
      FX_ID_CASE(PATTERN_BREAK);
      FX_ID_CASE(EXTENDED);
      FX_ID_CASE(SET_SPEED);

    default:
      s = "unknown";
      break;
    }

  return s;

#else

  return base_fxs[i].name;

#endif

}


static inline void fx_apply(mod_context_t* mc,
			    struct chan_state* cs,
			    unsigned int fx)
{
  const unsigned int i = (fx & 0xf00) >> 8;

  if (i != 14)
    base_fxs[i].fn(mc, cs, fx);
  else
    extended_fxs[(fx & 0xf0) >> 4].fn(mc, cs, fx);
}



/* on per channel state */

static inline void chan_init_state(struct chan_state* cs,
				   unsigned int ichan,
				   unsigned int ipat)
{
  memset(cs, 0, sizeof(struct chan_state));

  cs->ichan = ichan;
  cs->ipat = ipat;

  CHAN_SET_FLAG(cs, IS_SAMPLE_STARTING);
}


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

  /* reposition */

  if (CHAN_HAS_FLAG(cs, IS_SAMPLE_STARTING))
    {
      const unsigned char* chan_data;
      unsigned int ismp;
      unsigned int period;
      unsigned int fx;

      /* extract channel data */

      chan_data = get_chan_data(mc, cs);

      ismp = (chan_data[0] & 0xf0) | (chan_data[2] >> 4);
      period = ((unsigned int)(chan_data[0] & 0x0f) << 8) | chan_data[1];
      fx = ((unsigned int)(chan_data[2] & 0x0f) << 8) | chan_data[3];

      /* todo: is this necessary */

      if (!cs->ismp && !ismp)
	return 0;

      CHAN_CLEAR_FLAG(cs, IS_SAMPLE_STARTING);

      /* decrement since samples are 1 based */

      if (ismp)
	cs->ismp = ismp - 1;

      {
	DEBUG_PRINTF("newSample: [%u, %u, %u, %u] {%u, %u, %u}\n",
		     cs->ichan, cs->ipat, cs->idiv, cs->ismp,
		     get_sample_length(mc, cs->ismp),
		     get_sample_repeat_offset(mc, cs->ismp),
		     get_sample_repeat_length(mc, cs->ismp));
      }

      /* sample volume */

      cs->vol = get_sample_volume(mc, cs->ismp);

      /* sample rate (bytes per sec to send). todo, finetune. */

#define PAL_SYNC_RATE 7093789.2
#define NTSC_SYNC_RATE 7159090.5

      if (period)
	cs->smprate = PAL_SYNC_RATE / (period * 2);

      /* handle effect */

      fx_apply(mc, cs, fx);
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

	  if ((++cs->idiv) >= DIVS_PER_PAT)
	    {
	      cs->idiv = 0;

	      if ((++cs->ipat) >= mc->npats)
		cs->ipat = 0;
	    }
	}

      goto produce_more_samples;
    }

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

  chan_produce_samples(&mc->cstates[0], mc, obuf, nsmps);
  chan_produce_samples(&mc->cstates[1], mc, obuf, nsmps);
  chan_produce_samples(&mc->cstates[2], mc, obuf, nsmps);
  chan_produce_samples(&mc->cstates[3], mc, obuf, nsmps);

  mix_8bits_4chans_buffer(obuf, nsmps);

  return 0;
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
