/*
 * Copyright (c) 2011 Ramesh Raj
 * Copyright (c) 2011 Colin Jones
 * Based on code by Matteo Marchesotti
 * Copyright (c) 2007 Matteo Marchesotti <matteo.marchesotti@fsfe.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
 
#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "libnotify/notify.h"
#include "libacpi.h"

int BATT_CAUT = 25;
int BATT_LOW =50;
int BATT_GOOD= 75;
int BATT_WARNING= 25;

int warning_displayed = FALSE;
int prev_state = -1;

static update_batt_info(GtkStatusIcon *tray_icon);
gchar* get_icon_name(int val,int state);
void update_tool_tip(GtkStatusIcon *tray_icon,int percent,int time,int state);
void update_status_icon(GtkStatusIcon *tray_icon,int percent,int time,int state);

void notify_user(char* message);
static GtkStatusIcon *create_tray_icon();

void display_help();
void command_opt_get(int argc,char **argv);


static update_batt_info(GtkStatusIcon *tray_icon){
	
	global_t *global = malloc (sizeof (global_t));
	battery_t *binfo;
	adapter_t *ac = &global->adapt;
	int state, battstate;
	int bperc,btime;
	battstate = init_acpi_batt(global);
	
	state = init_acpi_acadapt(global);
	if(state == SUCCESS && ac->ac_state == P_BATT)
		state = P_BATT;
	else if(state == SUCCESS && ac->ac_state == P_AC)
		state=P_AC;
		
	if(battstate == SUCCESS){
		binfo = &batteries[0];
		read_acpi_batt(0);

		if(binfo->present){
			bperc=binfo->percentage;
			btime = binfo->charge_time;
		}else{
			printf("could not get battery info.\n");
			exit(-2);
		}
			
	}
	free(global);
	update_status_icon(tray_icon,bperc,btime,state);
}

void update_status_icon(GtkStatusIcon *tray_icon,int percent,int time,int state){
	gtk_status_icon_set_from_icon_name(tray_icon,get_icon_name(percent,state));
	update_tool_tip(tray_icon,percent,time,state);
	
	if(prev_state == -1)
		prev_state = state;
	
	if(state==P_BATT && percent<BATT_WARNING && warning_displayed == FALSE){
			notify_user("WARNING! Low battery!");
			warning_displayed=TRUE;
	}
	
	if(state !=prev_state){
		if(state == P_AC)
			notify_user("Plugged into AC");
		else
			notify_user("Unplugged from AC");
		warning_displayed=FALSE;
	}
	prev_state=state;
	printf("updated icon\n");	
}


gchar* get_icon_name(int val,int state){
	GString *fn;
	
	fn = g_string_new("battery-");
	if(val==100&&state==P_AC){
		g_string_append(fn,"full-charged");
	}else{
		if(val <BATT_CAUT){
			g_string_append(fn,"caution");
		}else if(val <BATT_LOW)
			g_string_append(fn,"low");
		else if(val <BATT_GOOD)
			g_string_append(fn,"good");
		else
			g_string_append(fn,"full");
			
		if(state==P_AC)
			g_string_append(fn,"-charging");
	}	
	return fn->str;
}

gchar *get_time(int mins){
	GString *time;
	gint hours, minutes;
	
	hours = mins / 60;
	minutes = mins - (60 * hours);
	
	time = g_string_new("");
	if (hours > 0)
		g_string_sprintf(time, "%2d hours, %2d minutes remaining", hours, minutes);
	else if(minutes>0)
		g_string_sprintf(time, "%2d minutes remaining", minutes);
	return time->str;
	
}


void update_tool_tip(GtkStatusIcon *tray_icon,int percent,int time, int state){
	GString *tooltip;
	tooltip = g_string_new("Battery ");
	
	if(state == P_BATT)
		g_string_append(tooltip, " dis");
	
	g_string_append(tooltip, "charging");
	g_string_sprintfa(tooltip, " (%i\%)", percent);
	
	gchar* timeC = get_time(time);
	if (timeC != "")
	{
		g_string_sprintfa(tooltip, "\n%s", timeC);
	}
	
	gtk_status_icon_set_tooltip_text (tray_icon,tooltip->str);
}


static GtkStatusIcon *create_tray_icon(){
	 GtkStatusIcon *tray_icon;
	 tray_icon = gtk_status_icon_new_from_stock(GTK_STOCK_QUIT);
	 
	 gtk_status_icon_set_visible(tray_icon, TRUE);
     update_batt_info(tray_icon);
     g_timeout_add_seconds(2,(GSourceFunc)update_batt_info,(gpointer)tray_icon);
 
     return tray_icon;
		
}


void notify_user(char* message){
		NotifyNotification *alertUser;
		alertUser = notify_notification_new("cbatticon",message,get_icon_name(100,1));
		notify_notification_set_timeout(alertUser,5000);
		GError *error = NULL;
		notify_notification_show(alertUser,&error);
		
}

void display_help(){
		printf("Usage:\n\tcbatticon [OPTION...][VALUES...]\n\n");
		printf("\t-g Good battery level(default: %d)\n"
				"\t-l Low battery level(default: %d)\n"
				"\t-c Caution battery level(default: %d)\n"
				"\t-w Warning battery level(default: %d)\n"
				,BATT_GOOD,BATT_LOW,BATT_CAUT,BATT_WARNING);
		printf("\n\texample: cbatticon -g 90\n\tcbatticon -g 90 -b10");
		abort();
}

void command_opt_get(int argc,char **argv){
	int c;
		char *value= NULL;
		while ((c = getopt (argc, argv, "g:l:c:w:h")) != -1){
			switch (c){
				case 'g':
					BATT_GOOD=atoi(optarg);
					break;
				case 'l':
					BATT_LOW=atoi(optarg);
					break;
				case 'c':
					BATT_CAUT=atoi(optarg);
					break;
				case 'w':
					BATT_WARNING=atoi(optarg);
					break;
				case 'h':
					display_help();
					break;
				default:
					display_help();
					abort ();
			   }
		 }

}

int main(int argc, char **argv) {
	if(check_acpi_support() == NOT_SUPPORTED){
		printf("No acpi support for your system?\n");
		return -1;
	}
	
	command_opt_get(argc,argv);
	
    GtkStatusIcon *tray_icon;
    notify_init("cbatticon");
    gtk_init(&argc, &argv);
    tray_icon = create_tray_icon();
    
    
	gtk_main();
 
    return 0;
}
