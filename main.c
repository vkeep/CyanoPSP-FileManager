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

// Thanks to Omega2058, for helping me out with this.

int listFiles(void) {

	int dfd, result = 0, y = 23, iconY = 18, sCurr = 0;
	
	// Clear out "dir" by setting all it's members to 0 
	memset(&dirent, 0, sizeof(dirent));
	
	// Open up the directory
	dfd = sceIoDopen(rootdir);
	
	// Make sure that the directory was able to open
	if(dfd < 0) {
		return dfd;
	}

	// This portion will continue to read contents in 
	// the directory and print it one-by-one :)
	while (sceIoDread(dfd, &dirent) > 0) {	
		//Confirm that the file is an actual name, blah, blah
		if (strcmp(dirent.d_name, ".") == 0)
			continue;
		if (strcmp(dirent.d_name, ".") == 0)
			continue;
			
		if (curr == sCurr) {
			oslIntraFontSetStyle(pgfFont, 0.5, RGBA(41,118,195,255), RGBA(0,0,0,0), INTRAFONT_ALIGN_LEFT);
			oslDrawStringf(80, y+=46, "%s", dirent.d_name);
			goto end;
		}	

		if(sCurr > 5)
		sCurr++;
		
		if (sCurr < 0)
		sCurr--;
		
		// Print the file inside the directory
		oslIntraFontSetStyle(pgfFont, 0.5, RGBA(0,0,0,255), RGBA(0,0,0,0), INTRAFONT_ALIGN_LEFT);
		oslDrawStringf(80, y+=46, "%s", dirent.d_name);
		oslDrawImageXY(diricon, 36, 56);
		
		end:
		sCurr++;
		result++;
	}

	// Close and return, we're done :D
	sceIoDclose(dfd);
	return result;

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

		oslStartDrawing();

		
		
		// Test out the function
	char * testDirectory = dirBrowse("ms0:");

	for(;;)
	{	
		pspDebugScreenClear();
		oslDrawFillRect(0, 0, 480, 272, RGB(255,255,255));
		centerText(480/2, 272/2, testDirectory, 50);	// Show the path that 'testDirectory' was supposed to recieve from dirBrowse();
		sceGuSwapBuffers();
		sceDisplayWaitVblankStart(); 
	 
	}
	sceKernelSleepThread();
	return 0;
}




