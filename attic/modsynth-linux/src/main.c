/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Wed Sep  2 19:20:16 2009 texane
** Last update Sun Sep  6 19:14:58 2009 texane
*/



#include <unistd.h>
#include <stdlib.h>
#include "pcm.h"
#include "mod.h"



/* main */

int main(int ac, char** av)
{
  pcm_dev_t* pcm_dev = NULL;
  pcm_buf_t* pcm_buf = NULL;
  mod_context_t* mc = NULL;

  if (mod_load_file(&mc, "../data/0.mod") == -1)
    goto on_error;

  if (pcm_open_dev(&pcm_dev) == -1)
    goto on_error;

  pcm_alloc_buf(&pcm_buf, 48000);

#if 1 /* 10 seconds to stdout */
  {
    unsigned int i;

    for (i = 0; i < 10; ++i)
      {
	mod_fetch(mc, pcm_get_buf_data(pcm_buf), pcm_get_buf_count(pcm_buf));
	write(1, pcm_get_buf_data(pcm_buf), pcm_get_buf_size(pcm_buf));
      }
  }
#else /* play to alsa pcm device */
  {
    while (mod_fetch(mc, pcm_get_buf_data(pcm_buf), pcm_get_buf_count(pcm_buf)) != -1)
      pcm_write_dev(pcm_dev, pcm_buf);
  }
#endif

 on_error:

  if (mc != NULL)
    mod_destroy(mc);

  if (pcm_dev != NULL)
    pcm_close_dev(pcm_dev);

  if (pcm_buf != NULL)
    pcm_free_buf(pcm_buf);

  return 0;
}
