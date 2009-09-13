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


#define FILE_STATIC_INITIALIZER { -1, NULL, 0 }



int file_open(file_context_t*, const char*);
void file_close(file_context_t*);


static inline const void* file_get_data(file_context_t* fc)
{
  return fc->data;
}

static inline size_t file_get_size(file_context_t* fc)
{
  return fc->size;
}



#endif /* ! FILE_H_INCLUDED */
