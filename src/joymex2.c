/*
   Copyright 2011 Martijn Aben

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "mex.h"
#include <SDL/SDL.h>
#include <SDL/SDL_joystick.h>

SDL_Joystick **joy=NULL;
int *naxes=NULL;
int *nbuttons=NULL;
int *nhats=NULL;
int njoysticks=0;

/* Cleans up on exit */
static void onUnload() {
    int i;
    for (i=0;i<njoysticks;i++) {
        if (joy[i]) {
            SDL_JoystickClose(joy[i]);
            mexPrintf("Unloaded Joystick %d\n",i);
        }
    }
    if (joy)
        free(joy);
    if (nbuttons)
        free(nbuttons);
    if (naxes)
        free(naxes);
    if (nhats)
        free(nhats);
               
}

/* Initializes a joystick and stores information about number of axes and buttons */
void Init(int id) {
    /* Check if ID is valid at all */
    if (id >= njoysticks) {
      mexErrMsgTxt("Joystick with specified id does not exist.");
    }
    /* Open the joystick */
    joy[id]=SDL_JoystickOpen(id);

    if(joy[id]) /* If succesfull */
    {
        mexPrintf("Opened Joystick %d\n",id);
        /* Query information about number of axes and buttons */
        naxes[id] = SDL_JoystickNumAxes(joy[id]);
        nbuttons[id] = SDL_JoystickNumButtons(joy[id]);
        nhats[id] = SDL_JoystickNumHats(joy[id]);
        
        mexPrintf("Name: %s\n", SDL_JoystickName(id));
        mexPrintf("Number of Axes: %d\n", naxes[id]);
        mexPrintf("Number of Buttons: %d\n", nbuttons[id]);
        mexPrintf("Number of Hats: %d\n", nhats[id]);
        
    }
    else {
        mexErrMsgTxt("Couldn't open Joystick");
    }
}

/* Read status of Joystick */
mxArray* Query(int id) {
    mxArray *out;
    mxArray *d;
    short *dd;
    unsigned char *db;
    mwSize dims[1];
    int nfields;
    const char *fields[] = {"axes","buttons","hats"};
    const char *hatfields[] = {"up","right","down","left"};
    int i;
    short hat;
    
    /* Make sure ID is valid at all */
    if (id >= njoysticks) {
      mexErrMsgTxt("Joystick with specified id does not exist.");
    }
    /* Make sure joystick is initialized */
    if (!joy[id]) {
        mexErrMsgTxt("Joystick not initialized");
    }
    /* Read registers from Joystick */
    SDL_JoystickUpdate();
    
    /* Create an output struct */
    dims[0] = 1;
    out = mxCreateStructArray(1,dims,3,fields);
    
    /* Initialize output array for axes information */
    d = mxCreateNumericMatrix(1,naxes[id],mxINT16_CLASS,mxREAL);
    dd = (short*) mxGetData(d);
    /* For each axes read the state */
    for (i=0;i<naxes[id];i++) {
        *dd++=SDL_JoystickGetAxis(joy[id],i);
    }
    mxSetField(out,0,"axes",d);    
    
    /* Do the same for all buttons */
    d = mxCreateNumericMatrix(1,nbuttons[id],mxUINT8_CLASS,mxREAL);
    db = (unsigned char*) mxGetData(d);
    for (i=0;i<nbuttons[id];i++) {
        *db++ = SDL_JoystickGetButton(joy[id],i);
    }
    mxSetField(out,0,"buttons",d);

    /* Do the same for all hats */
    dims[0] = nhats[id];
    d = mxCreateStructArray(1,dims,4,hatfields);
    for (i=0;i<nhats[id];i++) {
        hat = SDL_JoystickGetHat(joy[id],i);
        mxSetField(d,i,"up",mxCreateLogicalScalar( (hat & SDL_HAT_UP) != 0 ));
        mxSetField(d,i,"right",mxCreateLogicalScalar( (hat & SDL_HAT_RIGHT) != 0 ));
        mxSetField(d,i,"down",mxCreateLogicalScalar( (hat & SDL_HAT_DOWN) != 0 ));
        mxSetField(d,i,"left",mxCreateLogicalScalar( (hat & SDL_HAT_LEFT) !=0 ));
    }
    mxSetField(out,0,"hats",d);
    
    return out;
}

/* Entry point of the MEX-function */
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {  
    char *action;
    /* Check inputs */
    if (nrhs==0)
        mexErrMsgTxt("Too few input parameters.");
    /* Initialize arrays if called for first time */
    if (joy==NULL) {
        SDL_InitSubSystem(SDL_INIT_JOYSTICK);
        njoysticks = SDL_NumJoysticks();
        joy = (SDL_Joystick**) calloc(njoysticks, sizeof(SDL_Joystick*));
        naxes = (int*) malloc(njoysticks * sizeof(int));
        nbuttons = (int*) malloc(njoysticks * sizeof(int));
        nhats = (int*) malloc(njoysticks * sizeof(int));
    }
    /* Determine what user is requesting */
    action = mxArrayToString(prhs[0]);
    switch (action[0]) {
        case 'o': /* open */
            if (nrhs!=2) mexErrMsgTxt("You need to specify device ID.");
            Init(*mxGetPr(prhs[1]));
            break;
        case 'c': /* close */
            onUnload();
            break;
        case 'q': /* query */
            if (nrhs!=2) mexErrMsgTxt("You need to specify device ID.");
            plhs[0] = Query(*mxGetPr(prhs[1]));
            break;
        default:
            mexErrMsgTxt("No valid action specified");
    }
    /* Make sure devices are released when MEX-file is cleared */
    mexAtExit(onUnload);
}