/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Fri Sep  4 09:58:21 2009 texane
** Last update Sun Sep  6 17:57:18 2009 texane
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
