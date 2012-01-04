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

#define DISKBENCH_TIME	180
#define DISKREFTIME	0.4
#define DISKREFSCORE	415
#define MAX_CHILDS	128

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
#define sys_errlist _sys_errlist;
#endif

#ifdef SunOS
extern		sigjmp_buf	env;
#else
extern		jmp_buf	env;
#endif

extern		int	DISKflag,ONEflag;
extern	pid_t		child_pid[MAX_CHILDS];
extern	int		child_number;
extern	int		child;
extern	int		parent;
extern	int		cpu_score;
extern	unsigned	itim;

/*****************************************************************************/
double diskload(pmin)
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
      p[i]=malloc(MUFSIZ);
      memset(p[i],i,MUFSIZ);
      if ( i%2 )
	memcpy(p[i-1],p[i],MUFSIZ);
      else
	if ( i )
	  for (j=0;j<MUFSIZ;j++)
	    ((char *)p[i-1])[j]=((char *)p[i])[j];
    }
  for(i=0;i<pmin;i++)
    free(p[i]);
  free(p);
  dlt=((double )times(&tmsb)-(double )clticks)/(double )CLK_TCK;
  return dlt;
}
/*****************************************************************************/
unsigned diskcalibrate(cdt)
double cdt;
{
  unsigned i,j;
  double dlt;
  i=1;
  while ( diskload(i) < cdt ) i*=2;
  while ( diskload(i) < cdt ) i*=2;
  j=i/4;
  do
   {
     while ( (dlt=diskload(i-j)) < cdt ) j/=2;
     i-=j;
   }
  while ( (dlt > cdt) && j );
  return i;
}
/*****************************************************************************/
int diskbench()
{
  int sv[2],i;
  int d=0;
  double dlt;
  if ( pipe(sv) == -1 )
    {
      fprintf(stderr,"****  diskbench: pipe: %s\n",sys_errlist[errno]);
      DISKflag=0;
      return 0;
    }
  cpu_score=0;
  alarm(DISKBENCH_TIME);
  switch ( (i=sigsetjmp(env,0xffff)) )
    {
      case 0:
        break;
      case SIGALRM:
        for (i=0;i<child_number;i++) kill(child_pid[i],SIGALRM);
        if ( child ) exit(0);
	close(sv[0]);
        dlt=(double )cpu_score*(double )itim;
        dlt=dlt/(double )DISKREFSCORE;
        cpu_score=dlt;
	fprintf(stdout,"Ubench DISK: %d\n",cpu_score);
	fflush(stdout);
        return cpu_score;
      default:
	if ( ! child )
          fprintf(stderr,"****  diskbench: exiting on signal %d\n",i);
        for (i=0;i<child_number;i++) kill(child_pid[i],SIGKILL);
        DISKflag=0;
        return 0;
    }
  itim=diskcalibrate(DISKREFTIME);
  if ( ONEflag )
    {
      dlt=itim*(double )DISKBENCH_TIME/(double )DISKREFTIME/(double )DISKREFSCORE;
      cpu_score=dlt;
      fprintf(stdout,"Ubench Single DISK: %d (%.2fs)\n",
	      cpu_score,diskload(itim));
      return cpu_score;
    }
  alarm(DISKBENCH_TIME);
  child_pid[child_number]=fork();
  if ( child_pid[child_number] == -1 )
    {
      fprintf(stderr,"****  diskbench: fork: %s\n",sys_errlist[errno]);
      DISKflag=0;
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
	  if ( (diskload(itim) <= DISKREFTIME) )
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
