/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Thu Sep  3 05:42:47 2009 texane
** Last update Sun Sep  6 01:25:44 2009 texane
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
  /* big endian, expressed in words */

  return be16_to_host((const void*)&sd->length) * 2;
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
  /* stored in words */
  return be16_to_host(&get_sample_desc(mc, ismp)->roff) * 2;
}


static unsigned int get_sample_repeat_length(const mod_context_t* mc,
					     unsigned int ismp)
{
  /* stored in words */
  return be16_to_host(&get_sample_desc(mc, ismp)->rlength) * 2;
}


static inline const void* get_pat_div_chan(const mod_context_t* mc,
					   unsigned int ipat,
					   unsigned int idiv,
					   unsigned int ichan)
{
#define DIVS_PER_PAT 64
#define CHANS_PER_DIV 4
#define BYTES_PER_CHAN 4
#define BYTES_PER_DIV (CHANS_PER_DIV * BYTES_PER_CHAN)
#define BYTES_PER_PAT (DIVS_PER_PAT * CHANS_PER_DIV * BYTES_PER_CHAN) /* 1024 */

  return
    (const unsigned char*)mc->pdata +
    ipat * BYTES_PER_PAT +
    idiv * BYTES_PER_DIV +
    ichan * BYTES_PER_CHAN;
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

#if 0
    unsigned int ipat;
    unsigned int ichan;
    unsigned int idiv;

    for (ipat = 0; ipat < npats; ++ipat)
      for (idiv = 0; idiv < DIVS_PER_PAT; ++idiv)
	for (ichan = 0; ichan < CHANS_PER_DIV; ++ichan)
	  {
	    const uint8_t sample = (rc.pos[0] & 0xf0) | (rc.pos[2] >> 4);
	    const uint16_t period = ((uint16_t)(rc.pos[0] & 0x0f) << 8) | rc.pos[1];
	    const uint16_t effect = ((uint16_t)(rc.pos[2] & 0x0f) << 8) | rc.pos[3];

	    if (idiv == 0)
	      printf("fx: %u, period: %u\n", (effect & 0xf0) >> 4, period);

	    /* 4 bytes per div/chan */
	    if (reader_skip_safe(&rc, 4) == -1)
	      goto on_error;

	    /* printf("p: %u, div: %u, chan: %u, sample: 0x%02x, fx: 0x%02x\n", ipat, idiv, ichan, sample, effect); */
	  }
#endif

  /* sample data. build sdata lookup table. */

  pos = rc.pos;
  mc->sdata[0] = (const void*)pos;
  size = get_sample_length(mc, 0);

  for (ismp = 1; ismp < 31; ++ismp)
    {
      mc->sdata[ismp] = (const unsigned char*)mc->sdata[ismp - 1] + get_sample_length(mc, ismp);
      size += get_sample_length(mc, ismp);
    }

  /* check there is enough room for samples */

  if (reader_check_size(&rc, size) == -1)
    {
      DEBUG_ERROR("reader_check_size()\n");
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

  DEBUG_PRINTF("(%x, %x)\n", fx_get_first_param(fx), fx_get_second_param(fx));

  cs->vol = fx_get_first_param(fx) * 16 + fx_get_second_param(fx);
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
    { fx_do_unknown, "" },
    { fx_do_unknown, "" },
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


int chan_produce_samples(struct chan_state* cs,
			 mod_context_t* mc,
			 unsigned int nsmps)
{
  /* produce nsmps 48khz samples */

  const unsigned char* chan_data;
  unsigned int rate;
  unsigned int ismp;
  unsigned int period;
  unsigned int fx;

  /* extract channel data */

  chan_data = get_pat_div_chan(mc, cs->ipat, cs->idiv, cs->ichan);

  ismp = (chan_data[0] & 0xf0) | (chan_data[2] >> 4);
  period = ((unsigned int)(chan_data[0] & 0x0f) << 8) | chan_data[1];
  fx = ((unsigned int)(chan_data[2] & 0x0f) << 8) | chan_data[3];

  if (CHAN_HAS_FLAG(cs, IS_SAMPLE_STARTING))
    {
      CHAN_CLEAR_FLAG(cs, IS_SAMPLE_STARTING);

      /* samples are 1 based, so decrement */

      if (ismp)
	cs->ismp = ismp - 1;

      /* compute sample rate */

      rate = 7159090.5 / (period * 2);
      printf("rate: %u\n", rate);

      /* handle effect */

      fx_apply(mc, cs, fx);
    }
  else if (CHAN_HAS_FLAG(cs, IS_SAMPLE_REPEATING))
    {
      if (cs->smpoff >= get_sample_repeat_length(mc, ismp))
	cs->smpoff = get_sample_repeat_offset(mc, ismp);
    }
  else if (cs->smpoff >= get_sample_length(mc, ismp))
    {
      /* repeating sample */

      if (get_sample_repeat_length(mc, ismp))
	{
	  CHAN_SET_FLAG(cs, IS_SAMPLE_REPEATING);
	  cs->smpoff = get_sample_repeat_offset(mc, ismp);
	}
      else
	{
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
    }


  /* play the sample */
  {
    printf("sample: %u, period: %u\n", ismp, period); getchar();
    ++cs->idiv;
  }

  /* todo: when sample is done, next division. when division is done, next pattern. */

#if 0
  while (sstream_count(ss) < nsmps)
#else
/*   while (1) */
#endif
    {
/*       freq = cs->freq; */

/*       while (freq == cs->freq) */
	{
#if 0
	  /* read chan samples and feed the sample stream */

	  if (sstream_write(ss, smps, nsmps, freq) == -1)
	    return -1;
#endif
	}
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
    {
      DEBUG_ERROR("file_open() == -1\n");
      goto on_open_error;
    }

  if (load_file(*mc) == -1)
    {
      DEBUG_ERROR("load_file() == -1\n");
      goto on_error;
    }

  chan_init_state(&(*mc)->cstates[0], 0, (*mc)->ipat);
  chan_init_state(&(*mc)->cstates[1], 1, (*mc)->ipat);
  chan_init_state(&(*mc)->cstates[2], 2, (*mc)->ipat);
  chan_init_state(&(*mc)->cstates[3], 3, (*mc)->ipat);

  return 0;

 on_open_error:
  free(*mc);
  return -1;

 on_error:
  mod_destroy(*mc);
  return -1;
}


void mod_destroy(mod_context_t* mc)
{
  file_close(&mc->fc);
  free(mc);
}


int mod_fetch(mod_context_t* mc, unsigned char* obuf, unsigned int nsmps)
{
  /* fill a 48khz 16-bit signed interleaved stereo audio buffer
     from a mod file. name the mod filename, buffer the sample
     buffer, nsmps the sample count.
  */

#define BYTES_PER_SLE16_INTERLEAVED 2
#define STEREO_CHAN_COUNT 2

  memset(obuf, 0, STEREO_CHAN_COUNT * BYTES_PER_SLE16_INTERLEAVED * nsmps);

  chan_produce_samples(&mc->cstates[0], mc, nsmps);
  chan_produce_samples(&mc->cstates[1], mc, nsmps);
  chan_produce_samples(&mc->cstates[2], mc, nsmps);
  chan_produce_samples(&mc->cstates[3], mc, nsmps);

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



/* fetching machine */

static void fetch(mod_context_t* mc, void* obuf, unsigned int nsmps)
{
  /* fetch nsmps 48khz based */

  /* produce samples */
  chan_produce_samples(&mc->chans[0], nsmps);
  chan_produce_samples(&mc->chans[1], nsmps);
  chan_produce_samples(&mc->chans[2], nsmps);
  chan_produce_samples(&mc->chans[3], nsmps);

  /* consume samples */
  mix_sstreams(obuf, mc->chans[0].ss, mc->chans[1], nsmps);
  mix_sstreams(obuf, mc->chans[2].ss, mc->chans[3], nsmps);
}


#endif
