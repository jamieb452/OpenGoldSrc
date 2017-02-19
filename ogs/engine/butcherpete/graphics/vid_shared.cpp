/*
============
VID_Restart_f

Console command to re-start the video mode and refresh DLL. We do this
simply by setting the modified flag for the vid_ref variable, which will
cause the entire video mode and refresh DLL to be reset on the next frame.
============
*/
/*
============
VID_Restart_f

Console command to re-start the video mode and refresh DLL. We do this
simply by setting the modified flag for the vid_ref variable, which will
cause the entire video mode and refresh DLL to be reset on the next frame.
============
*/
void VID_Restart_f()
{
	vid_ref->modified = true;
};

typedef struct vidmode_s
{
	const char *description;
	int width, height;
	int mode;
} vidmode_t;

/*
vidmode_t vid_modes[] = {
	{ "Mode 0: 320x240", 320, 240, 0 },
	{ "Mode 1: 400x300", 400, 300, 1 },
	{ "Mode 2: 512x384", 512, 384, 2 },
	{ "Mode 3: 640x480", 640, 480, 3 },
	{ "Mode 4: 800x600", 800, 600, 4 },
	{ "Mode 5: 960x720", 960, 720, 5 },
	{ "Mode 6: 1024x768", 1024, 768, 6 },
	{ "Mode 7: 1152x864", 1152, 864, 7 },
	{ "Mode 8: 1280x1024", 1280, 1024, 8 },
	{ "Mode 9: 1600x1200", 1600, 1200, 9 }
};
*/

vidmode_t vid_modes[] = {
	{ "320x240", 320, 240, 0 },
	{ "400x300", 400, 300, 1 },
	{ "512x384", 512, 384, 2 },
	{ "640x480", 640, 480, 3 },
	{ "800x600", 800, 600, 4 },
	{ "960x720", 960, 720, 5 },
	{ "1024x600", 1024, 600, 6 },
	{ "1024x768", 1024, 768, 7 },
	{ "1152x864", 1152, 864, 8 },
	{ "1280x720", 1280, 720, 9 },
	{ "1280x960", 1280, 960, 10 },
	{ "1600x900", 1600, 900, 11 },
	{ "1600x1200", 1600, 1200, 12 },
	{ "1920x1080", 1600, 1200, 13 },
	{ "1920x1200", 1600, 1200, 14 }
};

/*
** VID_GetModeInfo
*/
qboolean VID_GetModeInfo(int *width, int *height, int mode)
{
	if(mode < 0 || mode >= VID_NUM_MODES)
		return false;

	*width = vid_modes[mode].width;
	*height = vid_modes[mode].height;

	return true;
};

/*
** VID_NewWindow
*/
void VID_NewWindow(int width, int height)
{
	viddef.width = width;
	viddef.height = height;
	
	//cl.force_refdef = true; // can't use a paused refdef
};