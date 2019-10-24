#include <glib.h>
#include <glibtop.h>
#include <glibtop/uptime.h>
#include <glibtop/cpu.h>
#include <glibtop/mem.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>


void timer_handler (int signum)
{
	unsigned int uptime;
	double cpu_usage=0,mem_usage;
	static glibtop_cpu buff_cpuload_before={0};
	glibtop_cpu buff_cpuload_after={0};
	glibtop_uptime buff_uptime;
	glibtop_mem buff_mem;
	
		
	glibtop_get_uptime (&buff_uptime);//get uptime
	glibtop_get_mem (&buff_mem);//get mem stats
	glibtop_get_cpu (&buff_cpuload_after);//get cpu stats
	//Calc CPU Utilization. Using current and old sample
	cpu_usage=100.0*(buff_cpuload_after.user-buff_cpuload_before.user);
	cpu_usage/=(buff_cpuload_after.total-buff_cpuload_before.total);
	//store current CPU stat sample to old
	memcpy(&buff_cpuload_before,&buff_cpuload_after,sizeof(glibtop_cpu));
	
	//Calc mem utilization
	mem_usage=100.0*(buff_mem.used - buff_mem.buffer - buff_mem.cached);
	mem_usage/=buff_mem.total;
	uptime=buff_uptime.uptime;
	//Print results on screen
	printf("\n\n-------------------------"
		   "\nUptime=%u sec"
		   "\nCPU_load=%.2f%%"
		   "\nUsed_memory=%.2f%%"
		   "\n",uptime,
				cpu_usage,
				mem_usage);
}

int main()
{
	struct sigaction sa;
	struct itimerval timer;

	/* Install timer_handler as the signal handler for SIGALRM. */
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = &timer_handler;
	sigaction (SIGALRM, &sa, NULL);

	/* Configure the timer to expire after 250 msec... */
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 250000;
	/* ... and every 250 msec after that. */
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 500000;
	/* Start a virtual timer. It counts down whenever this process is
	executing. */
	setitimer (ITIMER_REAL, &timer, NULL);

	/* Do busy work. */
	while (1)
		sleep(500);

}
