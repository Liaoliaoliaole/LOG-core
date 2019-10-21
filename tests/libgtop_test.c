#include <glibtop.h>
#include <glibtop/uptime.h>
#include <glibtop/cpu.h>
#include <glibtop/mem.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <unistd.h>
#include <time.h>

int main()
{
	glibtop_uptime buff_uptime;
	glibtop_cpu buff_cpuload_before,buff_cpuload_after;
	glibtop_mem buff_mem;
	double cpu_usage=0,mem_usage;
		
	glibtop_get_uptime (&buff_uptime);
	glibtop_get_mem (&buff_mem);
	glibtop_get_cpu (&buff_cpuload_before);
	sleep(1);
	glibtop_get_cpu (&buff_cpuload_after);
	cpu_usage=100.0*(buff_cpuload_after.user-buff_cpuload_before.user);
	cpu_usage/=(buff_cpuload_after.total-buff_cpuload_before.total);
	
	mem_usage=100.0*(buff_mem.used - buff_mem.buffer - buff_mem.cached);
	mem_usage/=buff_mem.total;
	printf("Uptime=%ld sec"
		   "\nCPU_load=%.2f%%"
		   "\nUsed_memory=%.2f%%"
		   "\n",(long)buff_uptime.uptime,
				cpu_usage,
				mem_usage);
	
}
