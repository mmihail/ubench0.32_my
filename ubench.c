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

#define	PROGRAM_VERSION		"0.3"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/utsname.h>
#include <limits.h>

#if defined HPUX || defined _AIX
extern char *sys_errlist[];
#endif
#ifdef SunOS
extern char *_sys_errlist[];
#define sys_errlist _sys_errlist
#endif

int	cpubench();
int	membench();
#ifdef  DISKNETYES
int	diskbench();
int	netbench();
#endif
void	signalSetup();
int ncpu_on=0;
int loop=0;

int	CPUflag		=	0;
int	MEMflag		=	0;
int	ncpu		=	1;
#ifdef DISKNETYES
int	NETflag		=	0;
int	DISKflag	=	0;
#endif
int	ONEflag		=	0;
/*****************************************************************************/
void print_version()
{
  printf("!!! Modified by Misha 2011(c) selected Loop=%i Ncpu=%i(use -p <Ncpu> key) !!!\n",loop,ncpu);

  fprintf(stdout,"\
Unix Benchmark Utility v."PROGRAM_VERSION"\n\
Copyright (C) July, 1999 PhysTech, Inc.\n\
Author: Sergei Viznyuk <sv@phystech.com>\n\
http://www.phystech.com/download/ubench.html\n");
}
/*****************************************************************************/
void print_usage()
{
#ifdef DISKNETYES
  fprintf(stderr,
  "Usage: ubench [-cmhspl] [-d [raw_device]] [-n [interface]]\n");
  fprintf(stderr,
  "Usage: ubench [-p <NCPU>]\n");
#else
  fprintf(stderr,
  "Usage: ubench [-cmhspl]\n");
  fprintf(stderr,
  "Usage: ubench [-p <NCPU>]\n");
#endif
}
/*****************************************************************************/
int main(argn,argc)
int argn;
char *argc[];
{
  struct utsname utsbuf;
  int s			=	1;
  int k			=	1;
  int i			=	1;
  
  while ( argc[i] )
    if ( argc[i][0]=='-' ) {
prgs: switch ( argc[i][s] )
	{
	  case 0:
	    i++;
	    s=1;
	    break;
	  case 'c':
	    s++;
	    CPUflag=1;
	    goto prgs;
	  case 'm':
	    s++;
	    MEMflag=1;
	    goto prgs;
	  case 'l':
	    s++;
	    loop=1;
	    goto prgs;
	  case 'p':
		ncpu_on=1;
		i++;
		break;	
#ifdef DISKNETYES
	  case 'd':
	    s++;
	    DISKflag=1;
	    goto prgs;
	  case 'n':
	    s++;
	    NETflag=1;
	    goto prgs;
#endif
	  case 's':
	    s++;
	    ONEflag=1;
	    goto prgs;
          default:
	    print_version();
	    print_usage();
	    exit(1);
	}
	if (ncpu_on && argc[i]) {
		ncpu_on=0;
		ncpu=atoi(argc[i]);
		if (ncpu<1) {
			printf("Wrong value for -p ... Must be >0\n");
		}
		i++;
	}
    }	
    else
      argc[k++]=argc[i++];

#ifdef DISKNETYES
  if ( ! (CPUflag|MEMflag|DISKflag|NETflag) )
    {
      CPUflag = 1;
      MEMflag = 1;
      DISKflag = 1;
      NETflag = 1;
    }
#else
  if ( ! (CPUflag|MEMflag) )
    {
      CPUflag = 1;
      MEMflag = 1;
    }
#endif
  signalSetup();
  print_version();
  if ( uname(&utsbuf) == -1 )
    fprintf(stderr,"****  ubench: uname: %s\n",sys_errlist[errno]);
  else
    {
#if defined HPUX
      char kmod[32],kbits[8];
      confstr(_CS_MACHINE_MODEL,kmod,32);
      confstr(_CS_KERNEL_BITS,kbits,8);
      fprintf(stdout,"%s %s %s %s-bit\n",
      utsbuf.sysname,utsbuf.release,kmod,kbits);
#elif defined _AIX
      fprintf(stdout,"%s "OSLEVEL" %d-bit\n",
      utsbuf.sysname,LONG_BIT);
#else
      fprintf(stdout,"%s %s %s %s\n",
      utsbuf.sysname,utsbuf.release,utsbuf.version,utsbuf.machine);
#endif
    }
  i=0;
  if ( CPUflag ) i+=cpubench();
  if ( MEMflag ) i+=membench();
#ifdef DISKNETYES
  if ( DISKflag ) i+=diskbench();
  if ( NETflag ) i+=netbench();
  if ( CPUflag&MEMflag&DISKflag&NETflag )
    {
      if ( ONEflag )
        fprintf(stdout,"-----------------------------------\nUbench Single AVG: %8d\n",
        i/(CPUflag+MEMflag+DISKflag+NETflag));
      else
        fprintf(stdout,"--------------------\nUbench AVG: %8d\n",
        i/(CPUflag+MEMflag+DISKflag+NETflag));
    }
#else
  if ( CPUflag&MEMflag )
    {
      if ( ONEflag )
        fprintf(stdout,"-----------------------------------\nUbench Single AVG: %8d\n",
        i/(CPUflag+MEMflag));
      else
        fprintf(stdout,"--------------------\nUbench AVG: %8d\n",
        i/(CPUflag+MEMflag));
    }
#endif
  exit(0);
}
