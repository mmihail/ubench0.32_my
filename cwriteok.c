/*
 * cwriteok.c
 * 
 * Copyright (C) January, 1998 Sergei Viznyuk <sergei@phystech.com>
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

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

/*****************************************************************************/
int cwriteok(s,tmout)
int s;
unsigned tmout;
{
  fd_set fs,efs;
  struct timeval tv;

  FD_ZERO(&fs);
  FD_ZERO(&efs);
  FD_SET(s,&fs);
  FD_SET(s,&efs);
  tv.tv_sec=tmout/1000000;
  tv.tv_usec=tmout%1000000;

  errno = 0;
  if ( select(s+1,NULL,&fs,&efs,&tv) == -1 ) return -1;
  if ( FD_ISSET(s,&fs) ) return 0;
  if ( FD_ISSET(s,&efs) )
    {
      errno = EPIPE;
      return -1;
    }
  else
    errno = ETIME;
  return 1;
}
