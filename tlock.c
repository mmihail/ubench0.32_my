/*
 * tlock - time clock a function call
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
#include <setjmp.h>
//#include <stdarg.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#ifdef DEBUG
#include <stdio.h>
#endif
#define MAX_SIGNALS  32

#ifdef SunOS
sigjmp_buf tlock_env;
#else
jmp_buf	tlock_env;
#endif
pid_t	child_pid;

int creadok(int s,unsigned tmout);
int cwriteok(int s,unsigned tmout);

int rtv;
unsigned itv;
/*****************************************************************************/
void tsighandler(sig)
int sig;
{
  siglongjmp(tlock_env,sig);
}
/*****************************************************************************/
int tlock(unsigned *tml,int (*fct)(),char **ar)
{
  int sv[2],rv[2];
//  va_list ap;
  struct sigaction action,oction[MAX_SIGNALS];
  if ( tml == NULL )
    {
      errno = EINVAL;
      return -1;
    }
  if ( pipe(sv) == -1 ) return -1;
  if ( pipe(rv) == -1 ) return -1;
  action.sa_handler = &tsighandler;
  action.sa_flags = 0;
  for (itv=1;itv<SIGCHLD;itv++)
    sigaction(itv,&action,&oction[itv-1]);
  for (itv=SIGCHLD+1;itv<MAX_SIGNALS;itv++)
    sigaction(itv,&action,&oction[itv-1]);
  action.sa_handler = SIG_IGN;
  sigaction(SIGCHLD,&action,&oction[SIGCHLD-1]);
  rtv= -1;
  child_pid=fork();
  switch ( child_pid )
    {
      case 0:
        close(sv[1]);
        close(rv[0]);
        if ( (itv=sigsetjmp(tlock_env,0xffff)) )
	  {
#ifdef DEBUG
	    fprintf(stderr,"child: caught signal %d\n",itv);
#endif
	    sigaction(itv,&action,NULL);
#ifdef DEBUG
	    fprintf(stderr,"child: kill(%d,%d) returned %d\n",
	    getppid(),itv,kill(getppid(),itv));
#endif
	    kill(getppid(),itv);
	    break;
	  }
        if ( *tml )
          while ( itv < *tml )
	    if ( (rtv=creadok(sv[0],1000000/CLK_TCK)) > 0 )
	      itv++;
	    else
	      break;
        else
          while ( 1 )
	    if ( (rtv=creadok(sv[0],1000000/CLK_TCK)) > 0 )
	      itv++;
	    else
	      break;
#ifdef DEBUG
        fprintf(stderr,"child: creadok returned %d, count=%d\n",rtv,itv);
#endif
	if ( rtv )
#ifdef DEBUG
	  fprintf(stderr,"child: kill(%d,%d) returned %d\n",
	  getppid(),SIGUSR2,kill(getppid(),SIGUSR2));
#else
	  kill(getppid(),SIGUSR2);
#endif
	else
#ifdef DEBUG
	  fprintf(stderr,"child: %d bytes read\n",
	  read(sv[0],tml,sizeof(unsigned)));
#else
	  read(sv[0],tml,sizeof(unsigned));
#endif
        *tml=itv;
        if ( (itv=cwriteok(rv[1],1000000/CLK_TCK)) == 0 )
#ifdef DEBUG
	  fprintf(stderr,"child: cwriteok returned %d: %d bytes written\n",
	  itv,write(rv[1],tml,sizeof(unsigned)));
#else
	  write(rv[1],tml,sizeof(unsigned));
#endif
        break;
      case -1:
        close(sv[0]);
        close(rv[1]);
        break;
      default:
        close(sv[0]);
        close(rv[1]);
        switch ( (itv=sigsetjmp(tlock_env,0xffff)) )
	  {
	    case 0:
              *tml = 0;
//	      va_start(ap,ftc);
	      rtv=fct(ar);
#if DEBUG
	      fprintf(stderr,"parent: ftc returned %d\n",rtv);
#endif
              if ( cwriteok(sv[1],1000000/CLK_TCK) == 0 )
#ifdef DEBUG
	        fprintf(stderr,"parent: cwrite returned 0: %d bytes written\n",
			write(sv[1],tml,sizeof(unsigned)));
#else
	        write(sv[1],tml,sizeof(unsigned));
#endif
	    case SIGUSR2:
#ifdef DEBUG
	      if ( itv==SIGUSR2 )
	        fprintf(stderr,"parent: caught SIGUSR2\n");
#endif
	      if ( (itv=creadok(rv[0],1000000/CLK_TCK)) == 0 )
#ifdef DEBUG
	        fprintf(stderr,"parent: creadok returned %d: %d bytes read\n",
		      itv,read(rv[0],tml,sizeof(unsigned)));
#else
	        read(rv[0],tml,sizeof(unsigned));
#endif
	      break;
	    default:
#ifdef DEBUG
	      fprintf(stderr,"parent: caught signal %d\n",itv);
#endif
	      sigaction(itv,&action,NULL);
#ifdef DEBUG
	      fprintf(stderr,"parent: kill(%d,%d) returned %d\n",
	      child_pid,itv,kill(child_pid,itv));
#else
	      kill(child_pid,itv);
#endif
	  }
    }
  if ( child_pid )
    {
      close(sv[1]);
      close(rv[0]);
      waitpid(child_pid,NULL,0);
      for (itv=1;itv<MAX_SIGNALS;itv++) sigaction(itv,&oction[itv-1],NULL);
      return rtv;
    }
  close(sv[0]);
  close(rv[1]);
  exit(0);
}
