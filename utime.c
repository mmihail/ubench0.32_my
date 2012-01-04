/*
 * utime - time clock a command call
 * Copyright (C) January, 2000 Sergei Viznyuk <sv@phystech.com>
 * 
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

int tlock(unsigned *tml,int (*fct)(),...);
/*****************************************************************************/
int ftc(ar)
char **ar;
{
  int i;
  int s;
  i=fork();
  if ( i>0 )
    waitpid(i,&s,0);
  else
    if ( i )
      return -1;
    else
      if ( execvp(ar[0],ar) == -1 )
        {
	  fprintf(stderr,"%s: not found\n",ar[0]);
	  return -1;
	}
  if ( WIFEXITED(s) )
    return WEXITSTATUS(s);
  return -1;
}
/*****************************************************************************/
int main(argn,argc)
int argn;
char *argc[];
{
  int r;
  unsigned tml=0;
  double d;
  if ( argn < 2 || argc[1]==NULL )
    {
      fprintf(stderr,"utime: no arguments\n");
      exit(-1);
    }
  r=tlock(&tml,&ftc,&argc[1]);
  d=tml;
  d=d/CLK_TCK;
  if ( ! r )
    fprintf(stdout,"****  time=%.3fs\n",d);
  exit(r);
}
