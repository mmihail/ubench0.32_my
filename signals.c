/*
 * ubench - Unix benchmark utility
 * Copyright (C) July, 1999 Sergei Viznyuk <sv@phystech.com>
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
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>

#ifdef SunOS
sigjmp_buf	env;
#else
jmp_buf		env;
#endif
extern int	child_number;
/*****************************************************************************/
void sigHandler(sig)
int sig;
{
  siglongjmp(env,sig);
}
/*****************************************************************************/
void signalSetup()
{
  int i;
  struct sigaction action;
  action.sa_handler= &sigHandler;
  action.sa_flags = 0;
  for (i=1;i<32;i++) sigaction(i,&action,NULL);
  action.sa_handler= SIG_IGN;
  sigaction(SIGCHLD,&action,NULL);
}
