/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000  Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@devolution.com
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id: SDL_sysjoystick.c,v 1.1 2001/02/05 20:26:28 cahighlander Exp $";
#endif

/*  SDL stuff  --  "SDL_sysjoystick.c"
    MacOS joystick functions by Frederick Reitberger
    
    The code that follows is meant for SDL.  Use at your own risk.
    
*/

#include    "SDL_error.h"
#include    "SDL_joystick.h"
#include    "SDL_sysjoystick.h"
#include    "SDL_joystick_c.h"


#include    <InputSprocket.h>

/*  The max number of joysticks we will detect  */
#define     MAX_JOYSTICKS       16 
/*  Limit ourselves to 32 elements per device  */
#define     kMaxReferences      32 

#define		ISpSymmetricAxisToFloat(axis)	((((float) axis) - kISpAxisMiddle) / (kISpAxisMaximum-kISpAxisMiddle))
#define		ISpAsymmetricAxisToFloat(axis)	(((float) axis) / (kISpAxisMaximum))


static  long    numJoysticks;
static  long    joyActive;
static  ISpDeviceReference  SYS_Joysticks[MAX_JOYSTICKS];
static  ISpElementListReference SYS_Elements[MAX_JOYSTICKS];
static  ISpDeviceDefinition     SYS_DevDef[MAX_JOYSTICKS];

struct joystick_hwdata 
{
/*    Uint8   id;*/
    ISpElementReference refs[kMaxReferences];
    /*  gonna need some sort of mapping info  */
}; 


/* Function to scan the system for joysticks.
 * Joystick 0 should be the system default joystick.
 * This function should return the number of available joysticks, or -1
 * on an unrecoverable fatal error.
 */
int SDL_SYS_JoystickInit(void)
{
    OSErr   err;
    UInt32  numGot, numGot2;
    int     i, count;
    
    if((Ptr)0 == (Ptr)ISpStartup)
        return -1;  //  InputSprocket not installed
    
    if((Ptr)0 == (Ptr)ISpGetVersion)
        return -1;  //  old version of ISp (not at least 1.1)
    
    ISpStartup();
    
    count = MAX_JOYSTICKS;
    numGot = numGot2 = 0;
    /*  
        class of devices that we want:
        Joysticks
        Gamepads
        Wheels?
    */
    
    /*  Get as many joysticks as possible right now  */
/*    err = ISpDevices_ExtractByClass(
        kISpDeviceClass_Joystick,
        MAX_JOYSTICKS,
        &numGot,
        SYS_Joysticks);
    
    err = ISpDevices_ExtractByClass(
        kISpDeviceClass_Gamepad,
        MAX_JOYSTICKS-numGot,
        &numGot2,
        &SYS_Joysticks[numGot]);*/
    
    ISpDevices_DeactivateClass(kISpDeviceClass_Mouse);
    ISpDevices_DeactivateClass(kISpDeviceClass_Keyboard);
    
    err = ISpDevices_Extract(
        count,
        &numGot,
        SYS_Joysticks);
    
    /*  should have device references now  */
    
    numJoysticks = numGot+numGot2;
    
    for(i = 0; i < numJoysticks; i++)
    {
        ISpDevice_GetDefinition(
            SYS_Joysticks[i], sizeof(ISpDeviceDefinition),
            &SYS_DevDef[i]);
        
        err = ISpElementList_New(
            0, NULL,
            &SYS_Elements[i], 0);
        
        if(err) /*  out of memory  */
            return -1;
        
        ISpDevice_GetElementList(
            SYS_Joysticks[i],
            &SYS_Elements[i]);
    }
    
    ISpDevices_Deactivate(
      numJoysticks,
      SYS_Joysticks);
    
    joyActive = 0;
    
    SDL_numjoysticks = numJoysticks;
    
    return numJoysticks;
}



/* Function to get the device-dependent name of a joystick */
const char *SDL_SYS_JoystickName(int index)
{
    static char    name[63];
    int     i;
    
    /*  convert pascal string to c-string  */
    for(i = 0; i < 62; i++)
    {
        name[i] = SYS_DevDef[index].deviceName[i+1];
    }
    name[SYS_DevDef[index].deviceName[0]] = '\0';
    
    return name;
}



/* Function to open a joystick for use.
   The joystick to open is specified by the index field of the joystick.
   This should fill the nbuttons and naxes fields of the joystick structure.
   It returns 0, or -1 if there is an error.
 */
int SDL_SYS_JoystickOpen(SDL_Joystick *joystick)
{
    int     i;
    UInt32  count, gotCount, count2;
    long    numAxis, numButtons, numHats, numBalls;
    
    numAxis = numButtons = numHats = numBalls = 0;
    
    i = joystick->index;
    
    joystick->name = SDL_SYS_JoystickName(i);
    count = kMaxReferences;
    count2 = 0;
    
    
	/* allocate memory for system specific hardware data */
	joystick->hwdata = (struct joystick_hwdata *) malloc(sizeof(*joystick->hwdata));
	if (joystick->hwdata == NULL)
	{
		SDL_OutOfMemory();
		return(-1);
	}
	memset(joystick->hwdata, 0, sizeof(*joystick->hwdata));

    
    ISpElementList_ExtractByKind(
        SYS_Elements[i],
        kISpElementKind_Axis,
        count,
        &gotCount,
        joystick->hwdata->refs);
    
    numAxis = gotCount;
    count -= gotCount;
    count2 += gotCount;
    
    ISpElementList_ExtractByKind(
        SYS_Elements[i],
        kISpElementKind_DPad,
        count,
        &gotCount,
        &(joystick->hwdata->refs[count2]));
    
    numHats = gotCount;
    count -= gotCount;
    count2 += gotCount;
    
    ISpElementList_ExtractByKind(
        SYS_Elements[i],
        kISpElementKind_Button,
        count,
        &gotCount,
        &(joystick->hwdata->refs[count2]));
    
    numButtons = gotCount;
    count -= gotCount;
    count2 += gotCount;
    
    
    joystick->naxes = numAxis;
    joystick->nhats = numHats;
    joystick->nballs = numBalls;
    joystick->nbuttons = numButtons;
    
    ISpDevices_Activate(
        1,
        &SYS_Joysticks[i]);
    
    joyActive |= (0x1 << i);
    
    return  0;
}



/* Function to update the state of a joystick - called as a device poll.
 * This function shouldn't update the joystick structure directly,
 * but instead should call SDL_PrivateJoystick*() to deliver events
 * and update joystick device state.
 */
void SDL_SYS_JoystickUpdate(SDL_Joystick *joystick)
{
    int     i, j;
    ISpAxisData     a;
    ISpDPadData     b;
    //ISpDeltaData    c;
    ISpButtonData   d;
    signed short    a2;
    
    for(i = 0, j = 0; i < joystick->naxes; i++, j++)
    {
        ISpElement_GetSimpleState(
            joystick->hwdata->refs[j],
            &a);
//        a = (signed long)a - kISpAxisMiddle;
//        a -= kISpAxisMiddle;
        a2 = (ISpSymmetricAxisToFloat(a)* 32767.0);
        if((a2) != joystick->axes[i])
            SDL_PrivateJoystickAxis(joystick, i, a2);
        /*  only update when things change  */
    }
    
    for(i = 0; i < joystick->nhats; i++, j++)
    {
        ISpElement_GetSimpleState(
            joystick->hwdata->refs[j],
            &b);
        if(b != 0)
            b += 2; /*  rotate it to how SDL expects  */
        if(b >= 9)
            b -= 8; /*  and take care of rollover  */
        if((b) != joystick->hats[i])
            SDL_PrivateJoystickHat(joystick, i, b);
        /*  only update when things change  */
    }
    
    for(i = 0; i < joystick->nballs; i++, j++)
    {
        /*  ignore balls right now  */
    }
    
    for(i = 0; i < joystick->nbuttons; i++, j++)
    {
        ISpElement_GetSimpleState(
            joystick->hwdata->refs[j],
            &d);
        if((d) != joystick->buttons[i])
            SDL_PrivateJoystickButton(joystick, i, d);
    }
}



/* Function to close a joystick after use */
void SDL_SYS_JoystickClose(SDL_Joystick *joystick)
{
    int i;
    
    i = joystick->index;
    
    ISpDevices_Deactivate(
        1,
        &SYS_Joysticks[i]);
    
    joyActive &= ~(0x1 << i);
}


/* Function to perform any system-specific joystick related cleanup */
void SDL_SYS_JoystickQuit(void)
{
    ISpShutdown();
}

