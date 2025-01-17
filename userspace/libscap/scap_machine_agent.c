/*
Copyright (C) 2023 The Falco Authors.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/

#include <stdio.h>
#ifdef __linux__
#include <unistd.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/utsname.h>
#endif // __linux__

#include "scap.h"
#include "scap-int.h"

#define SECOND_TO_NS 1000000000

#ifdef __linux__
static void scap_get_bpf_stats_enabled(scap_machine_info* machine_info)
{
	machine_info->flags &= ~PPM_BPF_STATS_ENABLED;
	FILE* f;
	if((f = fopen("/proc/sys/kernel/bpf_stats_enabled", "r")))
	{
		uint32_t bpf_stats_enabled = 0;
		fscanf(f, "%u", &bpf_stats_enabled);
		fclose(f);
		if (bpf_stats_enabled != 0)
		{
			machine_info->flags |= PPM_BPF_STATS_ENABLED;
		}
	}
}

static void scap_gethostname(char* buf, size_t size)
{
	char *env_hostname = getenv(SCAP_HOSTNAME_ENV_VAR);
	if(env_hostname != NULL)
	{
		snprintf(buf, size, "%s", env_hostname);
	}
	else
	{
		gethostname(buf, size);
	}
}
#endif

void scap_retrieve_machine_info(scap_machine_info* machine_info, uint64_t boot_time)
{
	machine_info->num_cpus = 0;
	machine_info->memory_size_bytes = 0;
	machine_info->reserved3 = 0;
	machine_info->reserved4 = 0;
#ifdef __linux__
	machine_info->num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
	machine_info->memory_size_bytes = (uint64_t)sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE);
	scap_gethostname(machine_info->hostname, sizeof(machine_info->hostname));
	machine_info->boot_ts_epoch = boot_time;
	scap_get_bpf_stats_enabled(machine_info);
#endif
}

void scap_retrieve_agent_info(scap_agent_info* agent_info)
{
	agent_info->start_ts_epoch = 0;
	agent_info->start_time = 0;
#ifdef __linux__

	/* Info 1:
	 *
	 * Get epoch timestamp based on procfs stat, only used for (constant) agent start time reporting.
	 */
	struct stat st = {0};
	char path[256];
	snprintf(path, sizeof(path), "/proc/%d/cmdline", getpid());
	if(stat(path, &st) == 0)
	{
		agent_info->start_ts_epoch = st.st_ctim.tv_sec * (uint64_t) SECOND_TO_NS + st.st_ctim.tv_nsec;
	}

	/* Info 2:
	 *
	 * Get /proc/self/stat start_time (22nd item) to calculate subsequent snapshots of the elapsed time
	 * of the agent for CPU usage calculations, e.g. sysinfo uptime - /proc/self/stat start_time.
	 */
	char proc_stat[256];
	FILE* f;
	snprintf(proc_stat, sizeof(proc_stat), "/proc/%d/stat", getpid());
	if((f = fopen(proc_stat, "r")))
	{
		unsigned long long stat_start_time = 0; // unit: USER_HZ / jiffies / clock ticks
		long hz = 100;
#ifdef _SC_CLK_TCK
		if ((hz = sysconf(_SC_CLK_TCK)) < 0)
		{
			ASSERT(false);
			hz = 100;
		}
#endif
		if(fscanf(f, "%*d %*s %*c %*d %*d %*d %*d %*d %*lu %*lu %*lu %*lu %*lu %*llu %*llu %*llu %*llu %*d %*d %*d %*lu %llu", &stat_start_time))
		{
			agent_info->start_time = (double)stat_start_time / hz; // unit: seconds as type (double)
		}
		fclose(f);
	}

	/* Info 3:
	 *
	 * Kernel release `uname -r` of the machine the agent is running on.
	 */

	struct utsname uts;
	uname(&uts);
	snprintf(agent_info->uname_r, sizeof(agent_info->uname_r), "%s", uts.release);
#endif
}
