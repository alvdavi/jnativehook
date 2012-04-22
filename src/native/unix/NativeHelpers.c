/* JNativeHook: Global keyboard and mouse hooking for Java.
 * Copyright (C) 2006-2012 Alexander Barker.  All Rights Received.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include "XKeyboardHelper.h"

#ifdef XKB
#include <X11/XKBlib.h>
#endif

#ifdef XF86MISC
#include <X11/extensions/xf86misc.h>
#include <X11/extensions/xf86mscstr.h>
#endif

#ifdef XT
#include <X11/Intrinsic.h>

static XtAppContext app_context;
#endif

Display * disp;

long GetAutoRepeatRate() {
	bool successful = false;
	long value = -1;
	unsigned int kb_delay = 0, kb_rate = 0;

	#ifdef XKB
	//Attempt to acquire the keyboard auto repeat rate using the XKB extension if available.
	if (!successful) {
		successful = XkbGetAutoRepeatRate(disp, XkbUseCoreKbd, &kb_delay, &kb_rate);
	}
	#endif

	#ifdef XF86MISC
	//Fallback to the XF86 Misc extension if available and other efforts failed.
	if (!successful) {
		XF86MiscKbdSettings kb_info;
		successful = (bool) XF86MiscGetKbdSettings(disp, &kb_info);
		if (successful) {
			kb_delay = (unsigned int) kbdinfo.delay;
			kb_rate = (unsigned int) kbdinfo.rate;
		}
	}
	#endif

	if (successful) {
		value = (long) kb_delay;
	}

	(void) kb_rate;
	return value;
}

long GetAutoRepeatDelay() {
	bool successful = false;
	long value = -1;
	unsigned int kb_delay = 0, kb_rate = 0;

	#ifdef XKB
	//Attempt to acquire the keyboard auto repeat rate using the XKB extension if available.
	if (!successful) {
		successful = XkbGetAutoRepeatRate(disp, XkbUseCoreKbd, &kb_delay, &kb_rate);
	}
	#endif

	#ifdef XF86MISC
	//Fallback to the XF86 Misc extension if available and other efforts failed.
	if (!successful) {
		XF86MiscKbdSettings kb_info;
		successful = (bool) XF86MiscGetKbdSettings(disp, &kb_info);
		if (successful) {
			kb_delay = (unsigned int) kbdinfo.delay;
			kb_rate = (unsigned int) kbdinfo.rate;
		}
	}
	#endif

	if (successful) {
		value = (long) kb_rate;
	}

	(void) kb_delay;
	return value;
}

long GetPointerAccelerationMultiplier() {
	long value = -1;
	int accel_numerator, accel_denominator, threshold;

	XGetPointerControl(disp, &accel_numerator, &accel_denominator, &threshold);
	if (accel_denominator >= 0) {
		value = accel_denominator;
	}
	(void) accel_numerator;
	(void) threshold;

	return value;
}

long GetPointerAccelerationThreshold() {
	long value = -1;
	int accel_numerator, accel_denominator, threshold;

	XGetPointerControl(disp, &accel_numerator, &accel_denominator, &threshold);
	if (threshold >= 0) {
		value = threshold;
	}
	(void) accel_numerator;
	(void) accel_denominator;

	return value;
}

long GetPointerSensitivity() {
	long value = -1;
	int accel_numerator, accel_denominator, threshold;

	XGetPointerControl(disp, &accel_numerator, &accel_denominator, &threshold);
	if (accel_numerator >= 0) {
		value = accel_numerator;
	}
	(void) accel_denominator;
	(void) threshold;

	return value;
}

long GetMultiClickTime() {
	long value = -1;
	int clicktime;
	bool successful = false;

	//Try and acquire the multi-click time from the user defined Xdefaults
	char * xprop = XGetDefault(disp, "*", "multiClickTime");
	if (xprop != NULL && sscanf(xprop, "%i", &clicktime) != EOF) {
		successful = true;
	}

	xprop = XGetDefault(disp, "OpenWindows", "MultiClickTimeout");
	if (xprop != NULL && sscanf(xprop, "%i", &clicktime) != EOF) {
		successful = true;
	}

	#ifdef XT
	int argc = 0;
	char ** argv = {NULL};

	//Fall back to the X Toolkit extension if available and other efforts failed.
    XtDisplayInitialize(app_context, disp, "JNativeHook", "JNativeHook", NULL, 0, &argc, argv);
	if (!successful) {
		clicktime = XtGetMultiClickTime(disp);
		if (clicktime >= 0) {
			successful = true;
		}
	}
	#endif

	if (successful) {
		value = clicktime;
	}

	return value;
}


void OnLibraryLoad() {
	//Tell X Threads are OK
	XInitThreads();

	#ifdef XT
	XtToolkitInitialize();
	app_context = XtCreateApplicationContext();
	#endif

	//Open local display.
	disp = XOpenDisplay(XDisplayName(NULL));
	#ifdef DEBUG
	if (disp != NULL) {
		fprintf(stdout, "OnLibraryLoad(): XOpenDisplay successful.\n");
	}
	else {
		fprintf(stderr, "OnLibraryLoad(): XOpenDisplay failure!\n");
	}
	#endif
	
	Bool isAutoRepeat = false;
	#ifdef XKB
	//enable detectable autorepeat.
	XkbSetDetectableAutoRepeat(disp, True, &isAutoRepeat);
	#else
	XAutoRepeatOn(disp);

	XKeyboardState kb_state;
	XGetKeyboardControl(disp, &kb_state);

	isAutoRepeat = (kb_state.global_auto_repeat == AutoRepeatModeOn);
	#endif

	//Initialize Native Keyboard Functions
	KeyboardHelperInit();

	#ifdef DEBUG
	if (isAutoRepeat) {
		fprintf(stdout, "OnLibraryLoad(): Successfully enabled detectable autorepeat.\n");
	}
	else {
		fprintf(stderr, "OnLibraryLoad(): Could not enable detectable auto-repeat!\n");
	}
	#endif
}

void OnLibraryUnload() {
	#ifdef XT
	XtDestroyApplicationContext(app_context);
	#endif

	//Cleanup Native Keyboard Functions
	KeyboardHelperCleanup();

	//Destroy the native displays.
	if (disp != NULL) {
		XCloseDisplay(disp);
		disp = NULL;
	}
}
