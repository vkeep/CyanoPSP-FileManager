#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <string.h>
#include <stdio.h>
#include <pspsdk.h>
#include <unistd.h>
#include <pspiofilemgr.h>
#include <dirent.h>
#include <stdlib.h>
#include <oslib/oslib.h>
#include "psploadexec_kernel.h"
#include "mp3player.h"

#define MAX_FILES			255 // max amount of files needed to load.
#define MAX_DISPLAY			5 // max amount of files displayed on-screen.
#define DISPLAY_X			85 // X value of where the filebrowser is displayed.
#define DISPLAY_Y			70 // Y value of the filebrowser is displayed.
#define ICON_DISPLAY_Y      56 
#define CURR_DISPLAY_Y     	54 
#define rootdir "ms0:/" //Define root directory


PSP_MODULE_INFO("CyanoPSP File Manager", 0x200, 2, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-128);

OSL_IMAGE *filemanagerbg, *diricon, *imageicon, *mp3icon, *txticon, *unknownicon, *documenticon, *binaryicon, *videoicon, *archiveicon, *bar, *deletion, *action, *nowplaying, *textview;

OSL_FONT *pgfFont;

OSL_COLOR black = RGB(0,0,0),red = RGB(255,0,0), blue = RGB(0,0,255);

/* Globals */

char *file;
char fileName;
SceIoDirent dirent;
SceUID dirId = 0;
int dirStatus = 1;
int curr;
int amount;
int lim;
int ini;

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

int i;
int current;
int curScroll;
char lastDir[512];
int timer;
char returnMe[512];
SceCtrlData pad, oldpad;

int initOSLib(){
    oslInit(0);
    oslInitGfx(OSL_PF_8888, 1);
	oslSetBilinearFilter(1);
    oslInitAudio();
    oslSetQuitOnLoadFailure(1);
    oslSetKeyAutorepeatInit(40);
    oslSetKeyAutorepeatInterval(10);
    return 0;
}

void filemanager_unload()
{
	oslDeleteImage(filemanagerbg);
	oslDeleteImage(diricon);
	oslDeleteImage(imageicon);
	oslDeleteImage(mp3icon);
	oslDeleteImage(txticon);
	oslDeleteImage(unknownicon);
}

int folderScan(const char* path);
int runFile(const char* path, char* type );
void centerText(int centerX, int centerY, char * centerText, int centerLength);
void dirVars();
void dirBack();
void dirUp();
void dirDown();
void dirControls();
char * dirBrowse(const char * path);

int folderScan(const char* path )
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

	int fd = sceIoDopen(path);

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

int runFile(const char* path, char* type)
{
	// Folders
	if (strcmp(type, "fld")==0) {
		current = 1;
		folderScan(path);
	}
	// '..' or 'dotdot'
	else if (strcmp(type, "dotdot")==0){
		current = 1;
		dirBack();
	}		
	return 1;
}

void refresh()
{
	folderScan(lastDir);
	current = 1;
	dirBrowse(lastDir);
}

void DeleteFile(const char * path)
{
	deletion = oslLoadImageFilePNG("system/app/filemanager/deletion.png", OSL_IN_RAM, OSL_PF_8888);
	
	while (!osl_quit) {
	oslStartDrawing();	
	oslDrawImageXY(deletion, 96,59);
	oslDrawStringf(116,125,"This action cannot be undone. Do you");
	oslDrawStringf(116,135,"want to continue?");
	
	oslDrawStringf(130,180,"Press circle");
	oslDrawStringf(130,190,"to cancel");
	oslDrawStringf(270,180,"Press cross");
	oslDrawStringf(270,190,"to confirm");
	
	oslReadKeys();
	
	if (osl_keys->pressed.cross) {
			sceIoRemove(path);
			oslDeleteImage(deletion);
			refresh();
		}

	else if (osl_keys->pressed.circle) {
			oslDeleteImage(deletion);
			return;
		}
	oslEndDrawing();
	oslSyncFrame();	
	}
};

void OptionMenu()
{
	action = oslLoadImageFilePNG("system/app/filemanager/actions.png", OSL_IN_RAM, OSL_PF_8888);
	
	while (!osl_quit) {
	oslStartDrawing();	
	oslDrawImageXY(action, 98,47);
	oslDrawStringf(120,105,"Press X");
	oslDrawStringf(120,115,"to Copy");
	oslDrawStringf(260,105,"Press Triangle");
	oslDrawStringf(260,115,"to Cut");
	
	oslDrawStringf(120,150,"Press Square");
	oslDrawStringf(120,160,"to Delete");
	oslDrawStringf(260,150,"Press Circle");
	oslDrawStringf(260,160,"to Rename");
	oslDrawStringf(165,200,"Press Select to Cancel");
	
	oslReadKeys();
	
	if (osl_keys->pressed.cross) 
	{
			refresh();
	}

	else if (osl_keys->pressed.triangle) 
	{
			refresh();
	}

	else if (osl_keys->pressed.square) 
	{
			DeleteFile(folderIcons[current].filePath);
	}
	
	
	else if (osl_keys->pressed.circle) 
	{
			refresh();
	}
	
	else if (osl_keys->pressed.select) 
	{
			return;
	}
	
	oslEndDrawing();
	oslSyncFrame();	
	}
};

void recursiveDelete(const char *dir) 
{ 
 int fd; 
 SceIoDirent dirent; 
 fd = sceIoDopen(dir); 
 char fullname[512]; 

  memset(&dirent, 0, sizeof dirent);
 
 while (!osl_quit) // && sceIoDread(fd, &dirent) > 0
 { 
  sprintf(fullname, "%s/%s", dir, dirent.d_name); 
 
   if ((FIO_S_IFREG & (dirent.d_stat.st_mode & FIO_S_IFMT)) == 0) 
      { 
             recursiveDelete(fullname); 
             printf("Deleting directory: %s\n", fullname); 
             sceIoRmdir(fullname); 
         } 
  else 
      { 
      printf("Deleting file: %s %i\n", fullname, FIO_S_IFREG & (dirent.d_stat.st_mode & FIO_S_IFMT)); 
      sceIoRemove(fullname); 
      } 
  } 
 sceIoDclose(fd); 
 sceIoRmdir(dir); 
}

void showImage(const char * path)
{
	 OSL_IMAGE * image = oslLoadImageFile(path, OSL_IN_RAM, OSL_PF_8888);
	 
	 if(!image)
		return 0;
		
	while (!osl_quit) {

		oslReadKeys();
		oslClearScreen(RGB(0,0,0));
		oslStartDrawing();	
		
		oslDrawFillRect(0,0,480,272,RGB(255,255,255));
		oslDrawImageXY(image, 240 - oslGetImageWidth(image) / 2, 136 - oslGetImageHeight(image) / 2);//draw image
		
		oslEndDrawing();
		oslSyncFrame();	
		
		if (osl_keys->pressed.circle) {
			return;
		}
		
	}
	//delete image
	oslDeleteImage(image);	
	return 1;
}
    
void oslPrintText(int x, int y, float size, char * text, OSL_COLOR color) {
   oslIntraFontSetStyle(pgfFont, size, color, RGBA(0,0,0,0), INTRAFONT_ALIGN_LEFT);
   oslDrawStringf(x,y,text);
}	

int checkTextFile(char *textfile)
{
	SceUID fd = sceIoOpen(textfile, PSP_O_RDONLY, 0777);
	
	if(fd < 0)
	   return -1;

	sceIoClose(fd);
	
	file = textfile;

	return 0;
}

char *getTextFromFile()
{
	char text_File[2048];

	memset(text_File, 0, sizeof(text_File));
	SceUID fd = sceIoOpen(file, PSP_O_RDONLY, 0777);
	
	sceIoRead(fd, text_File, sizeof(text_File));

	sceIoClose(fd);

	return text_File;
}


void displayTextFromFile()
{
	while (!osl_quit)
  {

	oslStartDrawing();	
	oslReadKeys();
	
	oslClearScreen(RGB(0,0,0));
	oslDrawImageXY(textview,0,19);
	
	if(checkTextFile(folderIcons[current].filePath) == -1)
	   oslDrawStringf(40,33,"Unable to Open");

	oslDrawStringf(40,33,folderIcons[current].name);	
    oslDrawStringf(10,55," \n%s", getTextFromFile());	

	if(osl_keys->pressed.circle)
	{
		return;
	}
				
	oslEndDrawing();
	oslSyncFrame();	
    oslAudioVSync();
	}
}

void MP3Play(char * path)
{	

	nowplaying = oslLoadImageFilePNG("system/app/apollo/nowplaying.png", OSL_IN_RAM, OSL_PF_8888);

	if (!nowplaying)
		oslDebug("It seems certain files necessary for the program to run are missing. Please make sure you have all the files required to run the program.");
	
	scePowerSetClockFrequency(333, 333, 166);
	
	pspAudioInit();
	
	int i;
	MP3_Init(1);
	MP3_Load(path);
	MP3_Play();
	
	while (!osl_quit)
  {
		//Draws images onto the screen
		oslStartDrawing();		
		
		oslClearScreen(RGB(0,0,0));

		oslReadKeys();
		
		oslDrawImageXY(nowplaying, 0, 19);
		oslPrintText(250,71,0.5,folderIcons[current].name,RGB(255,255,255));
		
		if(osl_pad.held.triangle) {
		break;
		}
		
		else if(osl_pad.held.cross) 
		{
		MP3_Pause();
		for(i=0; i<10; i++) {
		sceDisplayWaitVblankStart();
		}
		}
		
		if (MP3_EndOfStream() == 1) 
		{
		MP3_Stop();
		}
		
		if(osl_keys->pressed.circle)
		{
		MP3_Pause();
		MP3_Stop();
		MP3_FreeTune();
		return;
		}
		
		oslEndDrawing();
		oslSyncFrame();	
        oslAudioVSync();
		}
	MP3_Pause();
	MP3_Stop();
	MP3_FreeTune();
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
	sprintf(lastDir, "ms0:/");
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
	oslDrawImageXY(bar,0,(current - curScroll)*44+CURR_DISPLAY_Y);

	// Displays the directories, while also incorporating the scrolling
	for(i=curScroll;i<MAX_DISPLAY+curScroll;i++) {
	
		char * ext = strrchr(dirScan[i].name, '.'); //For file extension.
	
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
		
		if (dirScan[i].directory == 0)
		{  
			oslDrawImageXY(unknownicon,45,(i - curScroll)*44+ICON_DISPLAY_Y);
		}
		
		if(((ext) != NULL) && ((strcmp(ext ,".mp3") == 0) || (strcmp(ext ,".mov") == 0) || (strcmp(ext ,".m4a") == 0) || (strcmp(ext ,".wav") == 0) || (strcmp(ext ,".ogg") == 0)))
		{
			//Checks if the file is a music file.
			oslDrawImageXY(mp3icon,45,(i - curScroll)*44+ICON_DISPLAY_Y);
		}
		
		if(((ext) != NULL) && ((strcmp(ext ,".mp4") == 0) || (strcmp(ext ,".mpg") == 0) || (strcmp(ext ,".flv") == 0) || (strcmp(ext ,".mpeg") == 0))) 
		{
			//Checks if the file is a video.
			oslDrawImageXY(videoicon,45,(i - curScroll)*44+ICON_DISPLAY_Y);
		}
		
		if(((ext) != NULL) && ((strcmp(ext ,".png") == 0) || (strcmp(ext ,".jpg") == 0) || (strcmp(ext ,".jpeg") == 0) || (strcmp(ext ,".gif") == 0))) 
		{
			//Checks if the file is an image.
			oslDrawImageXY(imageicon,45,(i - curScroll)*44+ICON_DISPLAY_Y);
		}
		
		if(((ext) != NULL) && ((strcmp(ext ,".PBP") == 0) || (strcmp(ext ,".prx") == 0) || (strcmp(ext ,".PRX") == 0) || (strcmp(ext ,".elf") == 0))) 
		{
			//Checks if the file is a binary file.
			oslDrawImageXY(binaryicon,45,(i - curScroll)*44+ICON_DISPLAY_Y);
		}
		
		if(((ext) != NULL) && ((strcmp(ext ,".txt") == 0) || (strcmp(ext ,".TXT") == 0) || (strcmp(ext ,".log") == 0) || (strcmp(ext ,".prop") == 0) || (strcmp(ext ,".lua") == 0)))
		{
			//Checks if the file is a text document.
			oslDrawImageXY(txticon,45,(i - curScroll)*44+ICON_DISPLAY_Y);
		}
		
		if(((ext) != NULL) && ((strcmp(ext ,".doc") == 0) || (strcmp(ext ,".docx") == 0) || (strcmp(ext ,".pdf") == 0) || (strcmp(ext ,".ppt") == 0))) 
		{
			//Checks if the file is a document.
			oslDrawImageXY(documenticon,45,(i - curScroll)*44+ICON_DISPLAY_Y);
		}
		
		if(((ext) != NULL) && ((strcmp(ext ,".rar") == 0) || (strcmp(ext ,".zip") == 0) || (strcmp(ext ,".7z") == 0))) 
		{
			//Checks if the file is an archive.
			oslDrawImageXY(archiveicon,45,(i - curScroll)*44+ICON_DISPLAY_Y);
		}
		
		if (dirScan[i].directory == 1 && (!dirScan[i].directory == 0))
		{      
			// if it's a directory
			oslDrawImageXY(diricon,45,(i - curScroll)*44+ICON_DISPLAY_Y);
		}
		
		// If the currently selected item is active, then display the name
		if (folderIcons[i].active == 1) {
			
			oslDrawStringf(DISPLAY_X, (i - curScroll)*44+DISPLAY_Y, folderIcons[i].name);	// change the X & Y value accordingly if you want to move it (for Y, just change the +10)		
		}
	}
}

void dirControls() //Controls
{
	oslReadKeys();	

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
		if ((pad.Buttons & PSP_CTRL_CIRCLE) && (!(oldpad.Buttons & PSP_CTRL_CIRCLE))) {
		if(!strcmp("ms0:/", lastDir)) //pressed circle in root folder
			{
			dirBack();
			}
		}
	}
	
	char * ext = strrchr(folderIcons[current].filePath, '.'); 
	
	if (osl_keys->pressed.select)
	{
			OptionMenu();
	}
	
	if (((ext) != NULL) && ((strcmp(ext ,".png") == 0) || (strcmp(ext ,".jpg") == 0) || (strcmp(ext ,".jpeg") == 0) || (strcmp(ext ,".gif") == 0)) && (osl_keys->pressed.cross))
	{
			showImage(folderIcons[current].filePath);
	}
	
	if (((ext) != NULL) && ((strcmp(ext ,".PBP") == 0) || ((strcmp(ext ,".pbp") == 0))) && (osl_keys->pressed.cross))
	{
			struct SceKernelLoadExecParam param;

			memset(&param, 0, sizeof(param));
			
			param.size = sizeof(param);
			param.args = strlen(folderIcons[current].filePath)+1;
			param.argp = folderIcons[current].filePath;
			param.key = "updater";

			printf("Starting program...\n");

			sceKernelLoadExec(folderIcons[current].filePath, &param);
	}
	
	if (((ext) != NULL) && ((strcmp(ext ,".mp3") == 0) || ((strcmp(ext ,".MP3") == 0))) && (osl_keys->pressed.cross))
	{
		MP3Play(folderIcons[current].filePath);
	}
	
	if (((ext) != NULL) && ((strcmp(ext ,".txt") == 0) || ((strcmp(ext ,".TXT") == 0)) || ((strcmp(ext ,".c") == 0)) || ((strcmp(ext ,".h") == 0)) || ((strcmp(ext ,".cpp") == 0))) && (osl_keys->pressed.cross))
	{
		displayTextFromFile(folderIcons[current].filePath);
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
		folderScan(lastDir);
	} 
}

// Just call 'path' with whichever path you want it to start off in
char * dirBrowse(const char * path)
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

int main(int argc, char *argv[])
{
	initOSLib();
	oslIntraFontInit(INTRAFONT_CACHE_ALL | INTRAFONT_STRING_UTF8);

	//loads our images into memory
	filemanagerbg = oslLoadImageFilePNG("system/app/filemanager/filemanagerbg.png", OSL_IN_RAM, OSL_PF_8888);
	diricon = oslLoadImageFilePNG("system/app/filemanager/dir.png", OSL_IN_RAM, OSL_PF_8888);
	imageicon = oslLoadImageFilePNG("system/app/filemanager/image.png", OSL_IN_RAM, OSL_PF_8888);
	mp3icon = oslLoadImageFilePNG("system/app/filemanager/mp3.png", OSL_IN_RAM, OSL_PF_8888);
	txticon = oslLoadImageFilePNG("system/app/filemanager/txt.png", OSL_IN_RAM, OSL_PF_8888);
	unknownicon = oslLoadImageFilePNG("system/app/filemanager/unknownfile.png", OSL_IN_RAM, OSL_PF_8888);
	bar = oslLoadImageFilePNG("system/app/filemanager/bar.png", OSL_IN_RAM, OSL_PF_8888);
	textview = oslLoadImageFilePNG("system/app/filemanager/textview.png", OSL_IN_RAM, OSL_PF_8888);
	documenticon = oslLoadImageFilePNG("system/app/filemanager/documenticon.png", OSL_IN_RAM, OSL_PF_8888);
	binaryicon = oslLoadImageFilePNG("system/app/filemanager/binaryicon.png", OSL_IN_RAM, OSL_PF_8888);
	videoicon = oslLoadImageFilePNG("system/app/filemanager/videoicon.png", OSL_IN_RAM, OSL_PF_8888);
	archiveicon = oslLoadImageFilePNG("system/app/filemanager/archiveicon.png", OSL_IN_RAM, OSL_PF_8888);
	
	pgfFont = oslLoadFontFile("system/fonts/DroidSans.pgf");
	oslIntraFontSetStyle(pgfFont, 0.5, RGBA(0,0,0,255), RGBA(0,0,0,0), INTRAFONT_ALIGN_LEFT);
	oslSetFont(pgfFont);

	if (!filemanagerbg)
		oslDebug("It seems certain files necessary for the program to run are missing. Please make sure you have all the files required to run the program.");

	char * Directory = dirBrowse("ms0:");

	while (!osl_quit)
	{		
		oslStartDrawing();
		oslClearScreen(RGB(0,0,0));	
		
		centerText(480/2, 272/2, Directory, 50);	// Shows the path that 'Directory' was supposed to receive from dirBrowse();
	 
		oslEndDrawing();
		oslSyncFrame();	
        oslAudioVSync();
	}	
	return 0;
}




