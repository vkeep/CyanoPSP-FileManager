#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <pspsdk.h>
#include <oslib/oslib.h>
#include <pspiofilemgr.h>
#include <pspiofilemgr_kernel.h>
#include <pspiofilemgr_dirent.h>
#include <stdio.h>
#include <string.h>
#include "filebrowser.c"

#define rootdir "ms0:/"


PSP_MODULE_INFO("CyanoPSP File Manager", 0x200, 2, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-128);

OSL_IMAGE *filemanagerbg, *diricon, *imageicon, *mp3icon, *txticon, *unknownicon;

OSL_FONT *pgfFont;

OSL_COLOR black = RGB(0,0,0),red = RGB(255,0,0), blue = RGB(0,0,255);

/* Globals */

char fileName;
SceIoDirent dirent;
SceUID dirId = 0;
int dirStatus = 1;
int curr;
int amount;
int lim;
int ini;

int initOSLib(){
    oslInit(0);
    oslInitGfx(OSL_PF_8888, 1);
	oslSetBilinearFilter(1);
    oslInitAudio();
    oslSetQuitOnLoadFailure(1);
    oslSetKeyAutorepeatInit(40);
    oslSetKeyAutorepeatInterval(10);
	initialized = 1;
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

int main(int argc, char *argv[])
{
	initOSLib();
	pspDebugScreenInit();
	oslIntraFontInit(INTRAFONT_CACHE_ALL | INTRAFONT_STRING_UTF8);

	//loads our images into memory
	filemanagerbg = oslLoadImageFilePNG("system/app/filemanager/filemanagerbg.png", OSL_IN_RAM, OSL_PF_8888);
	diricon = oslLoadImageFilePNG("system/app/filemanager/dir.png", OSL_IN_RAM, OSL_PF_8888);
	imageicon = oslLoadImageFilePNG("system/app/filemanager/image.png", OSL_IN_RAM, OSL_PF_8888);
	mp3icon = oslLoadImageFilePNG("system/app/filemanager/mp3.png", OSL_IN_RAM, OSL_PF_8888);
	txticon = oslLoadImageFilePNG("system/app/filemanager/txt.png", OSL_IN_RAM, OSL_PF_8888);
	unknownicon = oslLoadImageFilePNG("system/app/filemanager/unknownfile.png", OSL_IN_RAM, OSL_PF_8888);
	
	pgfFont = oslLoadFontFile("system/fonts/DroidSans.pgf");
	oslIntraFontSetStyle(pgfFont, 0.5, RGBA(0,0,0,255), RGBA(0,0,0,0), INTRAFONT_ALIGN_LEFT);
	oslSetFont(pgfFont);

	if (!filemanagerbg)
		oslDebug("It seems certain files necessary for the program to run are missing. Please make sure you have all the files required to run the program.");

		
		// Test out the function
	char * testDirectory = dirBrowse("ms0:");

	while (!osl_quit)
	{		
		oslStartDrawing();
		oslClearScreen(RGB(0,0,0));	
		
		centerText(480/2, 272/2, testDirectory, 50);	// Shows the path that 'testDirectory' was supposed to receive from dirBrowse();
	 
		oslEndDrawing();
		oslSyncFrame();	
        oslAudioVSync();
	}		
}




