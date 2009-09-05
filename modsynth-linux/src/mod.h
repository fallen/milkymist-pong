/*
** Made by fabien le mentec <texane@gmail.com>
** 
** Started on  Wed Sep  2 19:55:22 2009 texane
** Last update Sat Sep  5 14:59:13 2009 texane
*/



#ifndef MOD_H_INCLUDED
# define MOD_H_INCLUDED



typedef struct mod_context mod_context_t;


int mod_load_file(mod_context_t**, const char*);
void mod_destroy(mod_context_t*);
int mod_fetch(mod_context_t*, unsigned char*, unsigned int);



#endif /* ! MOD_H_INCLUDED */
