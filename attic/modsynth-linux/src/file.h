/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Fri Sep  4 10:37:24 2009 texane
** Last update Fri Sep  4 12:46:32 2009 texane
*/



#ifndef FILE_H_INCLUDED
# define FILE_H_INCLUDED



#include <sys/types.h>



struct file_context
{
  int fd;
  unsigned char* data;
  size_t size;
};

typedef struct file_context file_context_t;



int file_open(file_context_t*, const char*);
void file_close(file_context_t*);


static inline void* file_get_data(file_context_t* fc)
{
  return fc->data;
}

static inline size_t file_get_size(file_context_t* fc)
{
  return fc->size;
}



#endif /* ! FILE_H_INCLUDED */
