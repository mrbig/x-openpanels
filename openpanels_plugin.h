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

#ifndef _OPENPANELS_PLUGIN_H
#define	_OPENPANELS_PLUGIN_H

#ifdef	__cplusplus
extern "C" {
#endif

    #define DRIVER_FILE "/dev/openpanel"

    #define OPENPANELS_VERSION 3

    typedef union _OUTPUT_MESSAGE_TYPEDEF {
        struct {
            unsigned char filler;
            unsigned char gear1_red:1;
            unsigned char gear1_green:1;
            unsigned char gear2_red:1;
            unsigned char gear2_green:1;
            unsigned char gear3_red:1;
            unsigned char gear3_green:1;
            unsigned char master_warning:1;
            unsigned char master_caution:1;

            unsigned char fuel_quantity:1;
            unsigned char battery:1;
            unsigned char generator:1;
            unsigned char engine_fire:1;
            unsigned char brakes:1;
            unsigned char gpws:1;
            unsigned char oil_temp:1;
            unsigned char oil_press:1;
        } members;
        unsigned char val[3];
    } OUTPUT_MESSAGE;

#ifdef	__cplusplus
}
#endif

#endif	/* _OPENPANELS_PLUGIN_H */

