/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Fri Sep  4 10:02:42 2009 texane
** Last update Sun Sep  6 17:29:13 2009 texane
*/



#include <stddef.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include "pcm.h"



/* types */

struct pcm_dev
{
  snd_pcm_t* handle;
};


struct pcm_buf
{
  unsigned int count; /* sample count */
  unsigned char data[1]; /* samples */
};



/* pcm conf */

#define PCM_CHAN_COUNT 2
#define PCM_SAMPLE_FREQ 48000
#define PCM_BYTES_PER_SAMPLE 2
#define PCM_PERIOD_SIZE 32



/* exported */

int pcm_open_dev(pcm_dev_t** dev)
{
  snd_pcm_t* handle = NULL;
  snd_pcm_hw_params_t* params;
  snd_pcm_uframes_t frames;
  unsigned int val;
  int dir;

  if (snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0)
    goto on_error;

  snd_pcm_hw_params_alloca(&params);
  snd_pcm_hw_params_any(handle, params);
  snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
  snd_pcm_hw_params_set_channels(handle, params, PCM_CHAN_COUNT);

  val = PCM_SAMPLE_FREQ;
  snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);

  frames = PCM_PERIOD_SIZE;
  snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

  snd_pcm_hw_params(handle, params);

  *dev = malloc(sizeof(pcm_dev_t));
  if (*dev == NULL)
    return -1;

  (*dev)->handle = handle;

  return 0;

 on_error:

  return -1;
}


void pcm_close_dev(pcm_dev_t* dev)
{
  if (dev->handle != NULL)
    {
      snd_pcm_drain(dev->handle);
      snd_pcm_close(dev->handle);
    }

  free(dev);
}


int pcm_write_dev(pcm_dev_t* dev, pcm_buf_t* buf)
{
  return snd_pcm_writei(dev->handle, buf->data, buf->count);
}


void pcm_alloc_buf(pcm_buf_t** buf, unsigned int count)
{
  static const size_t size = PCM_CHAN_COUNT * PCM_BYTES_PER_SAMPLE;

  *buf = malloc(offsetof(pcm_buf_t, data) + count * size);
  if (*buf == NULL)
    return ;

  (*buf)->count = count;
}


void pcm_free_buf(pcm_buf_t* buf)
{
  free(buf);
}


void* pcm_get_buf_data(pcm_buf_t* buf)
{
  return buf->data;
}


size_t pcm_get_buf_size(pcm_buf_t* buf)
{
  /* total buf size */
  return buf->count * PCM_BYTES_PER_SAMPLE * PCM_CHAN_COUNT;
}
