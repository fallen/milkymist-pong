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



#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "pcm.h"
#include "file.h"
#include "mod.h"



/* main */

int main(int ac, char** av)
{
  file_context_t fc = FILE_STATIC_INITIALIZER;
  pcm_buf_t* pcm_buf = NULL;
  mod_context_t mc;

  printf("ac=%d,av=%s\n",ac,av[1]);
  if (file_open(&fc, ac > 1 ? av[1] : "../data/3.mod") == -1)
    {
      printf("failed opening file\n");
      goto on_error;
    }

  if (mod_init(&mc, file_get_data(&fc), file_get_size(&fc)) == -1)
    {
      printf("failed mod_init\n");
      goto on_error;
    }

  pcm_alloc_buf(&pcm_buf, 48000);

  {
    FILE *f=fopen("output.raw","wb");
    unsigned int i;

    for (i = 0; i < 230; ++i)
      {
	mod_fetch(&mc, pcm_get_buf_data(pcm_buf), pcm_get_buf_count(pcm_buf));
#if 0	
	write(1, pcm_get_buf_data(pcm_buf), pcm_get_buf_size(pcm_buf));
#else
	fwrite(pcm_get_buf_data(pcm_buf),1,pcm_get_buf_size(pcm_buf),f);
#endif
      }
    fclose(f);
  }

 on_error:
  if (pcm_buf != NULL)
    pcm_free_buf(pcm_buf);

  file_close(&fc);

  return 0;
}
