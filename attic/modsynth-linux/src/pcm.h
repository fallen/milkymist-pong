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



#ifndef PCM_H_INCLUDED
# define PCM_H_INCLUDED



#include <sys/types.h>



typedef struct pcm_dev pcm_dev_t;
typedef struct pcm_buf pcm_buf_t;



int pcm_open_dev(pcm_dev_t**);
void pcm_close_dev(pcm_dev_t*);
int pcm_write_dev(pcm_dev_t*, pcm_buf_t*);
void pcm_alloc_buf(pcm_buf_t**, unsigned int);
void pcm_free_buf(pcm_buf_t*);
void* pcm_get_buf_data(pcm_buf_t*);
size_t pcm_get_buf_size(pcm_buf_t*);
unsigned int pcm_get_buf_count(pcm_buf_t*);



#endif /* ! PCM_H_INCLUDED */
