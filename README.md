DESCRIPTION
-----------
A GTK+ battery icon which uses libacpi to be lightweight and fast.

Fork of the project by [ColinJones][cj]

[cj]: https://github.com/ColinJones/cbatticon

COMPILING/INSTALLATION
----------------------
Requires [libnotify][ln], [libacpi][la], and [GTK+][gt] to build.

Run make and it should compile if you have all the previous dependecies.
Will add an install option soon.

[ln]: http://www.galago-project.org/downloads.php 
[la]: http://www.ngolde.de/libacpi.html
[gt]: http://www.gtk.org/download.html


COMMAND LINE ARGUMENTS
----------------------
###Most of these are available by typing in ./cbatticon -h
	
>**-c** Caution battery level Default: 25%  
>**-l** Low battery level 	 Default: 50%  
>**-g** Good battery level 	 Default: 75%  

###If caution is greater than any other value, the other values will not be shown.

>**-w** Warning notification level Default: 40%  
>**-n** Disable ALL notifications  

###For the next two arguments,  0 means always on.
>**-t** Number of milliseconds to display regular notifications    Default: 5000  
>**-r** Number of milliseconds to display warning notifications    Default: 0, always on  

###Custom icons  
>**-i** path to icons(will use current icon theme if not specified)  
> Path must have the trailing / at the end of the path.  
> In the directory these files must be present:  
	
>>+ battery-full-charged.png
+ battery-caution.png
+ battery-low.png
+ battery-good.png
+ battery-full.png
+ battery-caution-charging.png
+ battery-low-charing.png
+ battery-good-charging.png
+ battery-full.png

>**-e** to change the extension of the icons.     Default: .png  
>Remeber the **'.'** before the extension

