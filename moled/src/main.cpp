/*
 * Mole - Mobile Organic Localisation Engine
 * Copyright 2010 Nokia Corporation.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "core.h"

#define QT_NO_DEBUG_OUTPUT 0

// from http://www.enderunix.org/docs/eng/daemon.php

//#define RUNNING_DIR	"/tmp"
#define PID_FILE	"/var/run/moled.pid"
//#define LOG_FILE	"/var/run/moledexampled.log"


void signal_handler(int sig)
{
	switch(sig) {
	case SIGHUP:
	  //log_message(LOG_FILE,"hangup signal catched");
	  fprintf (stderr, "sighup");
		exit(0);
		break;
	case SIGTERM:
	  fprintf (stderr, "sigterm");
	  //	log_message(LOG_FILE,"terminate signal catched");
		exit(0);
		break;
	}
}

void daemonize()
{
int i;
//int i,lfp;
 FILE *lfp;
 //char str[10];
 if(getppid()==1) {printf ("already a daemon\n"); return; }/* already a daemon */
	i=fork();
	if (i<0) { printf ("fork error\n"); exit(1); } /* fork error */
	if (i>0) { printf ("parent\n"); exit(0); } /* parent exits */
	/* child (daemon) continues */
	//setsid(); /* obtain a new process group */
	//for (i=getdtablesize();i>=0;--i) close(i); /* close all descriptors */

	//fopen("/dev/null","rw"); dup(i); dup(i); /* handle standard I/O */
	//umask(027); /* set newly created file permissions */
	//chdir(RUNNING_DIR); /* change running directory */
	printf ("child");

	lfp=fopen(PID_FILE,"rw");
	//lfp=fopen(PID_FILE,"rw",0640);
	if (lfp<0) { printf ("cannot open pid file"); exit(1); } /* can not open */
	//if (lockf(lfp,F_TLOCK,0)<0) exit(0); /* can not lock */
	/* first instance continues */
	//sprintf(str,"%d\n",getpid());
	//printf("%d\n",getpid());
	//write(lfp,str,strlen(str)); /* record pid to lockfile */
	fprintf (lfp, "%d\n", getpid());
	fclose (lfp);
	signal(SIGCHLD,SIG_IGN); /* ignore child */
	signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	signal(SIGHUP,signal_handler); /* catch hangup signal */
	signal(SIGTERM,signal_handler); /* catch kill signal */
}



int main(int argc, char *argv[])
{
  //daemonize();

  //QCoreApplication *app = new QCoreApplication (argc, argv);
  Core *app = new Core (argc, argv);

  //return app->exec();
  return app->run();
}


