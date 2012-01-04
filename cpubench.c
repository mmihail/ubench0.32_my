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

#define CPUBENCH_TIME	180
#define CPUREFTIME	0.4
#define CPUREFSCORE	50190
#define MAX_CHILDS	128

#include <sys/types.h>
#include <sys/times.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
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

extern		int	CPUflag,ONEflag,ncpu,loop;
pid_t		child_pid[MAX_CHILDS];
int		child_number	=	0;
int		child		=	0;
int		parent		=	1;
int		cpu_score	=	0;
unsigned	itim		=	0;

/*****************************************************************************/
unsigned cpucalc(pmin)	/* performs rather senseless calcs */
unsigned pmin;
{
  double x,y;
  unsigned i,j,k=0;
  i=pmin;
  for (j=0;j<i;j++)
    if ( j%67 )
      k+=j%(i-j+1);
    else
      {
        x=i-j;
        y=log(1.0+x);
	x=abs(sqrt(y/(2.0+x))*(x*cos(atan(y/(3.0+x)))+y*sin(atan(y/(4.0+x)))));
	if ( x > 10.0 )
	  y=pow(1.0+j/(5.0+x),y/(6.0+x));
	else
	  y=pow(1.0+y/(1+j),x);
	x=x*exp(1.0/(1.0+y));
	k+=x;
      }
  i=k%99;
  if ( i==0 ) i++;
  return i;
}
/*****************************************************************************/
double cpuload(pmin)	/*  returns secs takes by cpucalc  */
unsigned pmin;
{
  double dlt;
  struct tms tmsb;
  clock_t clticks;
  clticks=times(&tmsb);
  if ( cpucalc(pmin) )
    dlt=((double )times(&tmsb)-(double )clticks)/(double )sysconf(_SC_CLK_TCK);
  else
    dlt=0.0;
  return dlt;
}
/*****************************************************************************/
unsigned cpucalibrate(cdt)
double cdt;
{
  unsigned i,j;
  double dlt;
  i=1;
  while ( cpuload(i) < cdt ) i*=2;
  while ( cpuload(i) < cdt ) i*=2;
  j=i/4;
  do
   {
     while ( (dlt=cpuload(i-j)) < cdt ) j/=2;
     i-=j;
   }
  while ( (dlt > cdt) && j );
  return i;
}
/*****************************************************************************/
int cpubench()
{
  int sv[2],i;
  int d=0;
  double dlt;
  if ( pipe(sv) == -1 )
    {
      fprintf(stderr,"****  cpubench: pipe: %s\n",sys_errlist[errno]);
      CPUflag=0;
      return 0;
    }
  cpu_score=0;
  alarm(CPUBENCH_TIME);
  switch ( (i=sigsetjmp(env,0xffff)) )
    {
      case 0:
        break;
      case SIGALRM:
	if (!loop) {
        for (i=0;i<child_number;i++) kill(child_pid[i],SIGALRM);
        if ( child ) exit(0);
        child_number=0;
	close(sv[0]);
        dlt=(double )cpu_score*(double )itim;
        dlt=dlt/(double )CPUREFSCORE;
        cpu_score=dlt;
	fprintf(stdout,"Ubench CPU: %8d\n",cpu_score);
        return cpu_score;
      default:
	if ( ! child )
          fprintf(stderr,"****  cpubench: exiting on signal %d\n",i);
        for (i=0;i<child_number;i++) kill(child_pid[i],SIGKILL);
        CPUflag=0;
        child_number=0;
        return 0;
	}
    }
  itim=cpucalibrate(CPUREFTIME);
  if ( ONEflag )
    {
      dlt=itim*(double )CPUBENCH_TIME/(double )CPUREFTIME/(double )CPUREFSCORE;
      cpu_score=dlt;
      fprintf(stdout,"Ubench Single CPU: %8d (%.2fs)\n",
	      cpu_score,cpuload(itim));
      return cpu_score;
    }
  alarm(CPUBENCH_TIME);
  child_pid[child_number]=fork();
  if ( child_pid[child_number] == -1 )
    {
      fprintf(stderr,"****  cpubench: fork: %s\n",sys_errlist[errno]);
      CPUflag=0;
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
//	  if ( (cpuload(itim) <= CPUREFTIME) )
	    cpuload(itim);
//	    if ( parent && (child_number < MAX_CHILDS ) )
	    if ( parent && (child_number < ncpu) )
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
  child_number=0;
  return 0;
}
