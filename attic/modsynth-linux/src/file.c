/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Fri Sep  4 10:39:51 2009 texane
** Last update Sun Sep  6 15:08:58 2009 texane
*/



#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef USE_MMAP
# include <sys/mman.h>
#endif

#include "file.h"
#include "debug.h"



static inline void clear_file(file_context_t* fc)
{
#ifdef USE_MMAP
  {
    fc->fd = -1;
    fc->data = MAP_FAILED;
  }
#else
  {
    fc->data = NULL;
  }
#endif

  fc->size = 0;
}


/* exported */

void file_close(file_context_t* fc) 
{
#ifdef USE_MMAP
  {
    if (fc->data != MAP_FAILED)
      {
	munmap(fc->data, fc->size);
	if (fc->fd != -1)
	  close(fc->fd);
      }
  }
#else /* ! USE_MMAP */
  {
    if (fc->data != NULL)
      free(fc->data);
  }
#endif /* USE_MMAP */

  clear_file(fc);
}


int file_open(file_context_t* fc, const char* path)
{
  const int fd = open(path, O_RDONLY);
  struct stat st;

  clear_file(fc);

  if (fd == -1)
    {
      DEBUG_ERROR("open() == %d\n", errno);
      goto on_error;
    }

  if (fstat(fd, &st) == -1)
    goto on_error;

  fc->size = st.st_size;

#ifdef USE_MMAP
  {
    fc->fd = fd;

    fc->data = mmap(NULL, fc->size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (fc->data == MAP_FAILED)
      {
	DEBUG_ERROR("mmap() == %d\n", errno);
	goto on_error;
      }
  }
#else /* ! USE_MMAP */
  {
    ssize_t size = (ssize_t)fc->size;
    unsigned char* p;
    ssize_t nread;

    p = fc->data = malloc(fc->size);
    if (fc->data == NULL)
      goto on_error;

    while (size > 0)
      {
	nread = read(fd, p, (size_t)size);
	if (nread <= 0)
	  {
	    DEBUG_ERROR("read() == %d\n", errno);
	    close(fd);
	    goto on_error;
	  }

	p += nread;
	size -= nread;
      }

    close(fd);

    goto on_error;
  }
#endif /* USE_MMAP */

  return 0;

 on_error:

  file_close(fc);

  return -1;
}
