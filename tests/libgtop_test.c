#include <glibtop.h>
#include <glibtop/uptime.h>
#include <glibtop/cpu.h>
#include <glibtop/mem.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <unistd.h>

int main()
{
	glibtop_uptime buff_uptime;
	glibtop_cpu buff_cpuload;
	glibtop_mem buff_mem;
	
	glibtop_get_uptime (&buff_uptime);
	glibtop_get_cpu (&buff_cpuload);
	glibtop_get_mem (&buff_mem);
	
	printf("Uptime=%f"
		   "\nCPU_load=%"G_GUINT64_FORMAT
		   "\nUsed_mem=%"G_GUINT64_FORMAT
		   "\n",buff_uptime.uptime/3600.0,
				buff_cpuload.idle/buff_cpuload.frequency,
				buff_mem.total);
	
}
