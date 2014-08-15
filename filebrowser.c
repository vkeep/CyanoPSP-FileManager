/*
Part of Slasher's code.
*/

#ifndef _FILEBROWSE_C
#define _FILEBROWSE_C

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <string.h>
#include <stdio.h>
#include <pspsdk.h>
#include <unistd.h>
#include <pspiofilemgr.h>
#include <stdlib.h>
#include <oslib/oslib.h>


#define printf				pspDebugScreenPrintf
#define WHITE				RGB(255, 255, 255)

#define MAX_FILES			255 // max amount of files needed to load.
#define MAX_DISPLAY			5 // max amount of files displayed on-screen.
#define DISPLAY_X			70 // X value of where the filebrowser is displayed.
#define DISPLAY_Y			70 // Y value of the filebrowser is displayed.

OSL_IMAGE *filemanagerbg, *diricon;
OSL_FONT *pgfFont;

typedef struct fileIcon {

	int		active;

	char	name[255];
	char	filePath[255];
	char	fileType[255];

	int		x;
	int		y;

} fileIcon;

fileIcon folderIcons[MAX_FILES];

typedef struct File {

	int exist;

	char path[255];
	char name[255];

	int size;
	int directory;

} File;

File dirScan[MAX_FILES];


SceIoDirent g_dir;

static int dispBufferNumber;
static int initialized = 0;
int i;
int current;
int curScroll;
char lastDir[512];
int timer;
char returnMe[512];
SceCtrlData pad, oldpad;


/////////////////////////////////////////////////////////////////
///////////////////////// PROTOTYPES ////////////////////////////
/////////////////////////////////////////////////////////////////

int folderScan(char* path);
int runFile( char* path, char* type );
void centerText(int centerX, int centerY, char * centerText, int centerLength);
void dirVars();
void dirBack();
void dirUp();
void dirDown();
void dirControls();
char * dirBrowse(char * path);

/////////////////////////////////////////////////////////////////
///////////////////////// FUNCTIONS /////////////////////////////
/////////////////////////////////////////////////////////////////

int folderScan( char* path )
{
	curScroll = 1;
	sprintf(lastDir, path);

	int i;
	for (i=0; i<=MAX_FILES; i++)	// erase old folders
		dirScan[i].exist = 0;

	int x;
	for (x=0; x<=MAX_FILES; x++) {
		folderIcons[x].active = 0;
	}

	int fd = sceIoDopen( path );

	i = 1;
	
	if (fd) {
		if (!(stricmp(path, "ms0:")==0 || (stricmp(path, "ms0:/")==0))) {

			sceIoDread(fd, &g_dir);		// get rid of '.' and '..'
			sceIoDread(fd, &g_dir);

			// Create our own '..'
			folderIcons[1].active = 1; 
			sprintf(folderIcons[1].filePath, "doesn't matter");
			sprintf(folderIcons[1].name, "..");
			sprintf(folderIcons[1].fileType, "dotdot");

			x = 2;
		} else {
			x = 1;
		}
		while ( sceIoDread(fd, &g_dir) && i<=MAX_FILES ) {
			sprintf( dirScan[i].name, g_dir.d_name );
			sprintf( dirScan[i].path, "%s/%s", path, dirScan[i].name );

			if (g_dir.d_stat.st_attr & FIO_SO_IFDIR) {
				dirScan[i].directory = 1;
				dirScan[i].exist = 1;
			} else {
				dirScan[i].directory = 0;
				dirScan[i].exist = 1;
			}

			dirScan[i].size = g_dir.d_stat.st_size;
			i++;
		}
	}

	sceIoDclose(fd);

	for (i=1; i<MAX_FILES; i++) {
		if (dirScan[i].exist == 0) break;
		folderIcons[x].active = 1;
		sprintf(folderIcons[x].filePath, dirScan[i].path);
		sprintf(folderIcons[x].name, dirScan[i].name);

		char *suffix = strrchr(dirScan[i].name, '.');
		
		if (dirScan[i].directory == 1) {      // if it's a directory
			sprintf(folderIcons[x].fileType, "fld");
		} 
		else if ((dirScan[i].directory == 0) && (suffix)) {		// if it's not a directory
			sprintf(folderIcons[x].fileType, "none");
		}
		else if (!(suffix)) {
			sprintf(folderIcons[x].fileType, "none");
		}
		x++;
	}

	return 1;
}

int runFile( char* path, char* type )
{
	// Folders
	if (strcmp(type, "fld")==0) {
		folderScan(path);
	}
	// '..' or 'dotdot'
	else if (strcmp(type, "dotdot")==0){
		dirBack();
	}
	// Other
	else if (strcmp(type, "none")==0){
		sprintf(returnMe, path);
	}

	return 1;
}

void centerText(int centerX, int centerY, char * centerText, int centerLength)
{
	if (strlen(centerText) <= centerLength) {
		int center = ((centerX)-((strlen(centerText)/2)*8));
		oslDrawStringf(center, centerY, centerText);
	}
	else {
		int center = ((centerX)-(centerLength/2)*8);
		char str[255];
		strncpy(str, centerText, centerLength);
		str[centerLength-3] = '.';
		str[centerLength-2] = '.';
		str[centerLength-1] = '.';
		str[centerLength] = '\0';
		oslDrawStringf(center, centerY, str);
	}
}

void dirVars()
{
	sprintf(lastDir, "ms0:");
	sprintf(returnMe, "blah");
	returnMe[5] = '\0';
	current = 1;
	curScroll = 1;
	timer = 0;
}

void dirUp()
{
	current--; // Subtract a value from current so the ">" goes up
	if ((current <= curScroll-1) && (curScroll > 1)) {
		curScroll--; // To do with how it scrolls
	}
}

void dirDown()
{
	if (folderIcons[current+1].active) current++; // Add a value onto current so the ">" goes down
	if (current >= (MAX_DISPLAY+curScroll)) {
		curScroll++; // To do with how it scrolls
	}
}

void dirDisplay()
{	
	oslDrawImageXY(filemanagerbg, 0, 19);
	oslDrawStringf(66, 29, lastDir); // Displays the current directory.
	
	// Displays the directories, while also incorporating the scrolling
	for(i=curScroll;i<MAX_DISPLAY+curScroll;i++) {
	
		// Handles the cursor and the display to not move past the MAX_DISPLAY.
		// For moving down
		//if ((folderIcons[i].active == 0) && (current >= i-1)) {
				
		if ((folderIcons[i].active == 0) && (current >= i-1)) {
			current = i-1;
			break;
		}
		// For moving up
		if (current <= curScroll-1) {
			current = curScroll-1;
			break;
		}
			
		// If the currently selected item is active, then display the name
		if (folderIcons[i].active == 1) {
			oslDrawStringf(DISPLAY_X, (i - curScroll)*44+DISPLAY_Y, folderIcons[i].name);	// change the X & Y value accordingly if you want to move it (for Y, just change the +10)		
		}
	}
	oslDrawStringf(DISPLAY_X-10, (current - curScroll)*44+DISPLAY_Y, ">");	
}

void dirControls()
{
	///// CONTROLS /////
	if (pad.Buttons != oldpad.Buttons) {
		if ((pad.Buttons & PSP_CTRL_DOWN) && (!(oldpad.Buttons & PSP_CTRL_DOWN))) {
			dirDown();
			timer = 0;
		}
		else if ((pad.Buttons & PSP_CTRL_UP) && (!(oldpad.Buttons & PSP_CTRL_UP))) {
			dirUp();
			timer = 0;
		}
		if ((pad.Buttons & PSP_CTRL_CROSS) && (!(oldpad.Buttons & PSP_CTRL_CROSS))) {
			runFile(folderIcons[current].filePath, folderIcons[current].fileType);
		}
		if ((pad.Buttons & PSP_CTRL_TRIANGLE) && (!(oldpad.Buttons & PSP_CTRL_TRIANGLE))) {
			if (!(stricmp(lastDir, "ms0:")==0) || (stricmp(lastDir, "ms0:/")==0)) {
				curScroll = 1;
				current = 1;
			}
		}
	}
		
	timer++;
	if ((timer > 30) && (pad.Buttons & PSP_CTRL_UP)) {
		dirUp();
		timer = 25;
	} else if ((timer > 30) && (pad.Buttons & PSP_CTRL_DOWN)) {
		dirDown();
		timer = 25;
	}

	if (current < 1) current = 1; // Stop the ">" from moving past the minimum files
	if (current > MAX_FILES) current = MAX_FILES; // Stop the ">" from moving past the max files

}

void dirBack()
{
	int a = 0;
	int b = 0;
	if (strlen(lastDir) > strlen("ms0:/")) {
		for (a=strlen(lastDir);a>=0;a--) {
			if (lastDir[a] == '/') {
				b++;
			}
			lastDir[a] = '\0';
			if (b == 1) {
				break;
			}
		}
		curScroll = 1;
		folderScan(lastDir);
	} 
}

void flipScreen()
{
	if (!initialized) return;
	sceGuSwapBuffers();
	dispBufferNumber ^= 1;
}



// Just call 'path' with whichever path you want it to start off in
char * dirBrowse(char * path)
{
	folderScan(path);
	dirVars();

	
	while (!osl_quit)
	{		
		oslStartDrawing();
		
		oslClearScreen(RGB(0,0,0));	
		oldpad = pad;
		sceCtrlReadBufferPositive(&pad, 1);
		dirDisplay();
		dirControls();
		
		sceDisplayWaitVblankStart();
		
		if (strlen(returnMe) > 4) {
			break;
		}
		oslEndDrawing();
		oslSyncFrame();	
        oslAudioVSync();
	}
		
	return returnMe;
}

#endif