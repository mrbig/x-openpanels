/* Copyright (C) 2009 Nagy Attila Gabor <nagy.attila.gabor@gmail.com>
 *
 *     This file is part of OpenPanels.
 *
 *  OpenPanels is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenPanels is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define XPLM200 = 1;
#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"
#include "openpanels_plugin.h"


/*
 * Global Variables.  We will store our single window globally.
 */


int gVersion = OPENPANELS_VERSION;

// File handler for the device
int panelfd = -1;

// Previous message. We won't send messages, if nothing changeds
OUTPUT_MESSAGE prev_msg;

// Different callbacks
int OpenPanelsDrawCallback (
                                   XPLMDrawingPhase     inPhase,
                                   int                  inIsBefore,
                                   void *               inRefcon);




// Forward references

// Datarefs in use
XPLMDataRef gAvionics = NULL;
XPLMDataRef gGearDeploy = NULL;
XPLMDataRef gMasterCaution = NULL;
XPLMDataRef gMasterWarning = NULL;
XPLMDataRef gEngineFire = NULL;
XPLMDataRef gGeneratorOff = NULL;
XPLMDataRef gGenerator = NULL;
XPLMDataRef gLowVoltage = NULL;
//XPLMDataRef gBattChargeHi = NULL;
XPLMDataRef gFuelQuantity = NULL;
XPLMDataRef gGPWS = NULL;
XPLMDataRef gParkBrake = NULL;
XPLMDataRef gSpeedBrake = NULL;
XPLMDataRef gOilPressure = NULL;
XPLMDataRef gOilTemperature = NULL;



/*
 * XPluginStart
 * 
 * Our start routine registers our window and does any other initialization we 
 * must do.
 * 
 */
PLUGIN_API int XPluginStart(
                        char *        outName,
                        char *        outSig,
                        char *        outDesc)
{
    /* First we must fill in the passed in buffers to describe our
     * plugin to the plugin-system. */

    strcpy(outName, "OpenPanels");
    strcpy(outSig, "openpanels.data");
    strcpy(outDesc, "Drive openpanels leds");

    fprintf(stderr, "Openpanels plugin starting\n");

    /* Register drawing callback */
    int ret = XPLMRegisterDrawCallback(OpenPanelsDrawCallback, xplm_Phase_Objects, 0, NULL);

    panelfd = open(DRIVER_FILE, O_WRONLY);

    /* Necessary datarefs */

    gAvionics = XPLMFindDataRef("sim/cockpit/electrical/avionics_on");
    gGearDeploy = XPLMFindDataRef("sim/aircraft/parts/acf_gear_deploy");
    gMasterCaution = XPLMFindDataRef("sim/cockpit2/annunciators/master_caution");
    gMasterWarning = XPLMFindDataRef("sim/cockpit2/annunciators/master_warning");
    gFuelQuantity = XPLMFindDataRef("sim/cockpit2/annunciators/fuel_quantity");
    gGeneratorOff = XPLMFindDataRef("sim/cockpit2/annunciators/generator_off");
    gGenerator = XPLMFindDataRef("sim/cockpit2/annunciators/generator");
    gLowVoltage = XPLMFindDataRef("sim/cockpit2/annunciators/low_voltage");
    //gBattChargeHi = XPLMFindDataRef("sim/cockpit2/annunciators/battery_charge_hi");
    gEngineFire = XPLMFindDataRef("sim/cockpit2/annunciators/engine_fire");
    gGPWS = XPLMFindDataRef("sim/cockpit2/annunciators/GPWS");
    gParkBrake = XPLMFindDataRef("sim/flightmodel/controls/parkbrake");
    gSpeedBrake = XPLMFindDataRef("sim/cockpit2/annunciators/speedbrake");
    gOilPressure = XPLMFindDataRef("sim/cockpit2/annunciators/oil_pressure");
    gOilTemperature = XPLMFindDataRef("sim/cockpit2/annunciators/oil_temperature");
    
    fprintf(stderr, "Openpanels plugin initialised %d\n", ret);

    
    /* We must return 1 to indicate successful initialization, otherwise we
     * will not be called back again. */
    return 1;
}

/*
 * XPluginStop
 * 
 * Our cleanup routine deallocates our window.
 * 
 */
PLUGIN_API void    XPluginStop(void)
{
    if (panelfd>-1) {
        OUTPUT_MESSAGE msg;
        msg.val[0] = msg.val[1] = msg.val[2] = 0;

        write(panelfd, &msg, sizeof(msg));
        close(panelfd);
    }
    XPLMUnregisterDrawCallback(OpenPanelsDrawCallback, xplm_Phase_LastCockpit, 0, NULL);
}

/*
 * XPluginDisable
 * 
 * We do not need to do anything when we are disabled, but we must provide the handler.
 * 
 */
PLUGIN_API void XPluginDisable(void)
{
}

/*
 * XPluginEnable.
 * 
 * We don't do any enable-specific initialization, but we must return 1 to indicate
 * that we may be enabled at this time.
 * 
 */
PLUGIN_API int XPluginEnable(void)
{
    return 1;
}

/*
 * XPluginReceiveMessage
 * 
 * We don't have to do anything in our receive message handler, but we must provide one.
 * 
 */
PLUGIN_API void XPluginReceiveMessage(
                    XPLMPluginID    inFromWho,
                    long            inMessage,
                    void *            inParam)
{
}


/**
 * Drawing callback, here we do the headtracking processing
 */
int OpenPanelsDrawCallback (
                                   XPLMDrawingPhase     inPhase,
                                   int                  inIsBefore,
                                   void *               inRefcon)
{
    float gears[10];
    int buff[8];
    unsigned char i, pos, tmp;
    int data;
    OUTPUT_MESSAGE msg;

    if (panelfd < 0) return 1;
    
    XPLMGetDatavf(gGearDeploy, gears, 0, 3);

    pos = 0;
    msg.val[0] = 0;
    msg.val[1] = 0;
    msg.val[2] = 0;

    for (i=0; i<3; i++) {
        if (gears[i]>= 1.0) {
            tmp = 2;
        }
        else if (gears[i] > 0) {
            tmp = 1;
        }
        else {
            tmp = 0;
        }
        msg.val[1] |= tmp << pos;
        pos += 2;
    }

    msg.members.master_caution = XPLMGetDatai(gMasterCaution) ? 1 : 0;
    msg.members.master_warning = XPLMGetDatai(gMasterWarning) ? 1 : 0;

    msg.members.fuel_quantity = XPLMGetDatai(gFuelQuantity) ? 1 : 0;
    msg.members.battery = XPLMGetDatai(gLowVoltage) ? 1 : 0;
    msg.members.engine_fire = XPLMGetDatai(gEngineFire) ? 1 : 0;
    msg.members.gpws = XPLMGetDatai(gGPWS) ? 1 : 0;
    msg.members.brakes = ((XPLMGetDatai(gAvionics) > 0) && (XPLMGetDataf(gParkBrake) > 0)) ||
                         XPLMGetDatai(gSpeedBrake) > 0.1 ? 1 : 0;
    msg.members.oil_press = XPLMGetDatai(gOilPressure) ? 1 : 0;
    msg.members.oil_temp = XPLMGetDatai(gOilTemperature) ? 1 : 0;

    // Generator
    data = XPLMGetDatai(gGenerator);

    if (!data) {
        XPLMGetDatavi(gGeneratorOff, buff, 0, 8);

        for (i=0; i<8; i++) {
            data |= buff[i];
            if (data) break;
        }
    }
    msg.members.generator = data ? 1 : 0;



    if (msg.val[1] != prev_msg.val[1] || msg.val[2] != prev_msg.val[2]) {
        write(panelfd, &msg, sizeof(msg));
        prev_msg = msg;
    }

    return 1;
}

