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

#define MEMBENCH_TIME	180
#define MEMREFTIME	0.4
#define MEMREFSCORE	415
#define MAX_CHILDS	128
#define MUFSIZE		1024

#include <sys/types.h>
#include <sys/times.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#if defined HPUX || defined _AIX
extern char *sys_errlist[];
#endif
#ifdef SunOS
extern char *_sys_errlist[];
#define sys_errlist _sys_errlist
#endif

#ifdef SunOS
extern		sigjmp_buf	env;
#else
extern		jmp_buf	env;
#endif

extern		int	MEMflag,ONEflag;
extern	pid_t		child_pid[MAX_CHILDS];
extern	int		child_number;
extern	int		child;
extern	int		parent;
extern	int		cpu_score;
extern	unsigned	itim;
/*****************************************************************************/
double memload(pmin)
unsigned pmin;
{
  double dlt;
  struct tms tmsb;
  unsigned i,j;
  void **p;
  clock_t clticks;
  clticks=times(&tmsb);
  p=(void **)malloc(pmin*sizeof(void *));
  for(i=0;i<pmin;i++)
    {
      p[i]=malloc(MUFSIZE);
      memset(p[i],i,MUFSIZE);
      if ( i%2 )
	memcpy(p[i-1],p[i],MUFSIZE);
      else
	if ( i )
	  for (j=0;j<MUFSIZE;j++)
	    ((char *)p[i-1])[j]=((char *)p[i])[j];
    }
  for(i=0;i<pmin;i++)
    free(p[i]);
  free(p);
  dlt=((double )times(&tmsb)-(double )clticks)/(double )sysconf(_SC_CLK_TCK)
;
  return dlt;
}
/*****************************************************************************/
unsigned memcalibrate(cdt)
double cdt;
{
  unsigned i,j;
  double dlt;
  i=1;
  while ( memload(i) < cdt ) i*=2;
  while ( memload(i) < cdt ) i*=2;
  j=i/4;
  do
   {
     while ( (dlt=memload(i-j)) < cdt ) j/=2;
     i-=j;
   }
  while ( (dlt > cdt) && j );
  return i;
}
/*****************************************************************************/
int membench()
{
  int sv[2],i;
  int d=0;
  double dlt;
  if ( pipe(sv) == -1 )
    {
      fprintf(stderr,"****  membench: pipe: %s\n",sys_errlist[errno]);
      MEMflag=0;
      return 0;
    }
  cpu_score=0;
  alarm(MEMBENCH_TIME);
  switch ( (i=sigsetjmp(env,0xffff)) )
    {
      case 0:
        break;
      case SIGALRM:
        for (i=0;i<child_number;i++) kill(child_pid[i],SIGALRM);
        if ( child ) exit(0);
	close(sv[0]);
        dlt=(double )cpu_score*(double )itim;
        dlt=dlt/(double )MEMREFSCORE;
        cpu_score=dlt;
	fprintf(stdout,"Ubench MEM: %8d\n",cpu_score);
	fflush(stdout);
        return cpu_score;
      default:
	if ( ! child )
          fprintf(stderr,"****  membench: exiting on signal %d\n",i);
        for (i=0;i<child_number;i++) kill(child_pid[i],SIGKILL);
        MEMflag=0;
        return 0;
    }
  itim=memcalibrate(MEMREFTIME);
  if ( ONEflag )
    {
      dlt=itim*(double )MEMBENCH_TIME/(double )MEMREFTIME/(double )MEMREFSCORE;
      cpu_score=dlt;
      fprintf(stdout,"Ubench Single MEM: %8d (%.2fs)\n",
	      cpu_score,memload(itim));
      return cpu_score;
    }
  alarm(MEMBENCH_TIME);
  child_pid[child_number]=fork();
  if ( child_pid[child_number] == -1 )
    {
      fprintf(stderr,"****  membench: fork: %s\n",sys_errlist[errno]);
      MEMflag=0;
      return 0;
    }
  if ( child_pid[child_number] > 0 )
    {
      close(sv[1]);
      child_number++;
      while ( read(sv[0],&d,sizeof(d)) == sizeof(d) )
        cpu_score++;
    }
  else
    {
      close(sv[0]);
      child=1;
      while ( 1 )
	{
	  if ( (memload(itim) <= MEMREFTIME) )
	    if ( parent && (child_number < MAX_CHILDS ) )
	      {
	        child_pid[child_number]=fork();
	        if ( child_pid[child_number] > 0 )
		  child_number++;
	        else
		  {
		    parent=0;
		    d++;
		    continue;
		  }
	      }
	  if ( write(sv[1],&d,sizeof(d)) != sizeof(d) ) exit(0); 
	}
    }
  return 0;
}
