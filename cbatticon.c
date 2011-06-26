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
#include <libnotify/notify.h>
#include <libacpi.h>

#define PAC 0
#define PBATT 1

#define OPT_BATT_VALUES "battery"
#define OPT_NOTIFICATION_VALUES  "notification"
#define OPT_ICON_VALUES  "icons"

int BATT_CAUT = 25;
int BATT_LOW =50;
int BATT_GOOD= 75;
int BATT_WARNING= 41;

int prev_state = PBATT;
int prev_perc = 101;


int display_notification = TRUE;
int notification_time = 5000;
int warning_notification_time = 0;

GString* icon_location = NULL;
gchar* icon_extension = ".png";
char* config_file_path=NULL;


//Functions start
void update_batt_info(GtkStatusIcon *tray_icon);
void update_status_icon(GtkStatusIcon *tray_icon,int percent,int time,int state);
gchar* get_icon_name(int val,int state);
gchar *get_time(int mins);
void update_tool_tip(GtkStatusIcon *tray_icon,int percent,int time,int state);


void notify_user(char* message,int timeout);
static GtkStatusIcon *create_tray_icon();



void file_get_battery_values(GKeyFile* file);
void file_get_notification_values(GKeyFile* file);
void file_get_icon_values(GKeyFile* file);
void file_opt_get();

void display_help();
void command_opt_get(int argc,char **argv);
//Functions end


//function called every 2 seconds
void update_batt_info(GtkStatusIcon *tray_icon){

	global_t *global = malloc (sizeof (global_t));
	battery_t *binfo;
	adapter_t *ac = &global->adapt;
	int state, battstate;
	int bperc,btime;
	battstate = init_acpi_batt(global);

	state = init_acpi_acadapt(global);
	if(state == SUCCESS && ac->ac_state == P_BATT)
		state = PBATT;
	else if(state == SUCCESS && ac->ac_state == P_AC)
		state=PAC;

	if(battstate == SUCCESS){
		
		if (global->batt_count < 0){
			fprintf(stderr, "No batteries in system!\n");
			exit(-1);
		}
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

	//display low battery (once)
	if(percent<BATT_WARNING && prev_perc>=BATT_WARNING){
			notify_user("WARNING! Low battery!",warning_notification_time);
	}
		
	//full charge
	if(percent ==100 && prev_perc<100){
		notify_user("Fully charged",notification_time);
	}
	
	//adapter notifications
	if(state !=prev_state){
		if(state == PAC){
			notify_user("Plugged into AC",notification_time);
		}else
			notify_user("Unplugged from AC",notification_time);
        prev_state=state;
	}
	

	//changing tray icon
    gchar* icon_name = get_icon_name(percent,state);
    if(icon_location==NULL){
		gtk_status_icon_set_from_icon_name(tray_icon,icon_name);
	}else{
		GString *icon_full_path = g_string_new(icon_location->str);
		g_string_append(icon_full_path,icon_name);
		g_string_append(icon_full_path,icon_extension);
		gtk_status_icon_set_from_file (tray_icon,icon_full_path->str);
		printf("%s\n",icon_full_path->str);
		g_string_free(icon_full_path,TRUE);
	}
	
	prev_perc = percent;
	update_tool_tip(tray_icon,percent,time,state);
	
}


gchar* get_icon_name(int val,int state){
	GString *fn;

	fn = g_string_new("battery-");
	if(val==100&&state==PAC){
		g_string_append(fn,"full-charged");
	}else{
		if(val <BATT_CAUT)
			g_string_append(fn,"caution");
		else if(val <BATT_LOW)
			g_string_append(fn,"low");
		else if(val <BATT_GOOD)
			g_string_append(fn,"good");
		else
			g_string_append(fn,"full");

		if(state==PAC)
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
	g_string_sprintfa(tooltip, " (%i%%)", percent);

	//get_time returns "" if minute remaining == 0
	gchar* timeC = get_time(time);
	if (g_strcasecmp(timeC,""))	{
		g_string_sprintfa(tooltip, "\n%s", timeC);
	}
	
	free(timeC);
	gtk_status_icon_set_tooltip_text (tray_icon,tooltip->str);
}


void notify_user(char* message,int timeout){
	if(display_notification==TRUE){
		NotifyNotification *alertUser;
		alertUser = notify_notification_new("cbatticon",message,NULL);
		notify_notification_set_timeout(alertUser,timeout);
		GError *error = NULL;
		notify_notification_show(alertUser,&error);
	}

}


static GtkStatusIcon *create_tray_icon(){
	 GtkStatusIcon *tray_icon;
	 tray_icon = gtk_status_icon_new_from_stock(GTK_STOCK_QUIT);

	 gtk_status_icon_set_visible(tray_icon, TRUE);
     update_batt_info(tray_icon);
     g_timeout_add_seconds(3,(GSourceFunc)update_batt_info,(gpointer)tray_icon);

     return tray_icon;

}



int main(int argc, char **argv) {
	if(check_acpi_support() == NOT_SUPPORTED){
		printf("No acpi support for your system?\n");
		return -1;
	}
	file_opt_get();
	command_opt_get(argc,argv);

    GtkStatusIcon *tray_icon;
    notify_init("cbatticon");
    gtk_init(&argc, &argv);
    tray_icon = create_tray_icon();


	gtk_main();

    return 0;
}


void file_opt_get(){
	GKeyFile* file =  g_key_file_new();
	gsize *length;
	char *p;
	
	if(config_file_path==NULL){
		p = getenv("HOME");
		strcat(p,"/.config/cbatticon/config");
	}
	if(!g_key_file_load_from_file(file,"~/.config/cbatticon/config",G_KEY_FILE_NONE,NULL)){
		printf("Error reading file\n");
		return ;
	}
	if(g_key_file_has_group(file,OPT_BATT_VALUES)){
		file_get_battery_values(file);
	}
	if(g_key_file_has_group(file,OPT_NOTIFICATION_VALUES)){
		file_get_notification_values(file);
	}
	if(g_key_file_has_group(file,OPT_ICON_VALUES)){
		file_get_icon_values(file);
	}
}

void file_get_battery_values(GKeyFile* file){
	gint val;
	if ((val=g_key_file_get_integer(file,OPT_BATT_VALUES,"good",NULL))!=0){
			BATT_GOOD=val;
	}
	if ((val=g_key_file_get_integer(file,OPT_BATT_VALUES,"low",NULL))!=0){
			BATT_LOW=val;
	}
	if ((val=g_key_file_get_integer(file,OPT_BATT_VALUES,"caution",NULL))!=0){
			BATT_CAUT=val;
	}
		
		
}

void file_get_notification_values(GKeyFile* file){
	gint val;
	gboolean enabled;
	if(g_key_file_has_key(file,OPT_NOTIFICATION_VALUES,"enabled",NULL)!=FALSE){
			enabled = g_key_file_get_boolean(file,OPT_NOTIFICATION_VALUES,"enabled",NULL);
			if(!enabled){
				display_notification=FALSE;
				return;
			}
	}
	
	
	if ((val=g_key_file_get_integer(file,OPT_NOTIFICATION_VALUES,"warning",NULL))!=0){
			BATT_WARNING=val;
	}
	if ((val=g_key_file_get_integer(file,OPT_NOTIFICATION_VALUES,"regular-delay",NULL))!=0){
			notification_time = val;
			if(val==-1)
				notification_time=0;
		
	}
	if ((val=g_key_file_get_integer(file,OPT_NOTIFICATION_VALUES,"warning-delay",NULL))!=0){
			notification_time = val;
			if(val==-1)
				notification_time=0;
	}
}

void file_get_icon_values(GKeyFile* file){
	gchar* val=NULL;
	if(g_key_file_has_key(file,OPT_ICON_VALUES,"path",NULL)==FALSE){
			return;
	}
	
	if ((val=g_key_file_get_string(file,OPT_ICON_VALUES,"path",NULL))!=NULL){
		icon_location=g_string_new(val);
	}
	
	if ((val=g_key_file_get_string(file,OPT_ICON_VALUES,"extension",NULL))!=NULL){
		icon_extension=val;
	}
}

//terminal help
void display_help(){
		printf("testUsage:\n\tcbatticon [OPTION...][VALUES...]\n\n");
		printf( "\t-g Good battery level(default: %d)\n"
				"\t-l Low battery level(default: %d)\n"
				"\t-c Caution battery level(default: %d)\n"
				"\t-w Warning notification level(default: %d)\n"
				"\t-n Disable notifications\n"
				"\tFor the following 2 arguements, 0 will mean always on\n"
				"\t-t Number of milliseconds to display notfication(default %d)\n"
				"\t-r Number of milliseconds to display warning notfication(default %d)\n"
				"\t-i Path to icons(will use current icon theme if not specified). Need trailing / Look at readme for proper usage\n"
				"\t-e The extensions for the icons,(default %s)\n"
				"\n\n\t-o The config file location (do not use ~, only full path followed by )\n"
				,BATT_GOOD,BATT_LOW,BATT_CAUT,BATT_WARNING,notification_time,warning_notification_time,icon_extension);
		printf("\n\texample: cbatticon -g 90\n\tcbatticon -g 90 -b10\n\tcbatticon -i /path/to/icons/\n");
		exit(0);
}

//command line arguments	
void command_opt_get(int argc,char **argv){
	int c;

	while ((c = getopt (argc, argv, "g:l:c:w:t:r:i:e:o:nh")) != -1){
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
			case 't':
				notification_time=atoi(optarg);
				break;
			case 'r':
				warning_notification_time=atoi(optarg);
				break;
			case 'n':
				display_notification=FALSE;
				break;
			case 'i':
				icon_location=g_string_new(optarg);
				break;
			case 'e':
				icon_extension=optarg;
				break;
			case 'o':
				config_file_path=optarg;
				break;
			case 'h':
				display_help();
				break;
			default:
				display_help();
				break;
		   }
	 }

}
