/*
 * copy right juan.j.zhao@intel.com intel
**/
#include <vconf.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <signal.h>
#include <aul.h>


#define VCONF_KEY_WAYLAND_HIDE_PIDS "db/wayland/hpids"

void wayland_vconf_set_hide_pids(char **pstring)
{
	char *s_key = NULL;

	s_key=malloc(strlen(VCONF_KEY_WAYLAND_HIDE_PIDS)+1);
	snprintf(s_key, strlen(VCONF_KEY_WAYLAND_HIDE_PIDS)+1, "%s", VCONF_KEY_WAYLAND_HIDE_PIDS );
	weston_log("the sting to be set to %s: %s\n", s_key, *pstring);

	vconf_set_str(s_key, *pstring);
	free(s_key);
	return;
}

void wayland_vconf_init()
{
	char *s_key = NULL;
	char *pids;
	char buf[128];
	char *pstring_last;
	char delim[2]=" ";
	const char initchar[2]=" ";
	int pid;

	s_key=malloc(strlen(VCONF_KEY_WAYLAND_HIDE_PIDS)+1);
	snprintf(s_key, strlen(VCONF_KEY_WAYLAND_HIDE_PIDS)+1, "%s", VCONF_KEY_WAYLAND_HIDE_PIDS );
	pids = vconf_get_str(s_key);

	weston_log("From %s, the total pids paused are: %s\n", s_key, pids);
	pstring_last=strtok(pids,delim);
	while(pstring_last!=NULL) {
		pid=atoi(pstring_last);
		if (aul_app_get_appid_bypid(pid, buf, sizeof(buf)) >= 0) {
			weston_log("the pid to be killed is: %d\n", pid);
			aul_terminate_pid(pid);
		}
		pstring_last=strtok(NULL,delim);
	}

	vconf_set_str(s_key, initchar);
	free(s_key);
	return;
}
