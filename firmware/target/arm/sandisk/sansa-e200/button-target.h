/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _BUTTON_TARGET_H_
#define _BUTTON_TARGET_H_

#define HAS_BUTTON_HOLD

void clickwheel_int(void);
void button_int(void);

/* Sandisk Sansa E200 button codes */

/* Main unit's buttons */
#define BUTTON_REC          0x00000001
#define BUTTON_DOWN         0x00000002
#define BUTTON_RIGHT        0x00000004
#define BUTTON_LEFT         0x00000008
#define BUTTON_SELECT       0x00000010
#define BUTTON_UP           0x00000020
#define BUTTON_POWER        0x00000040

#define BUTTON_SCROLL_BACK  0x00000080
#define BUTTON_SCROLL_FWD   0x00000100

#define BUTTON_MAIN         0x00000fff

#define POWEROFF_BUTTON BUTTON_POWER
#define POWEROFF_COUNT 10

#endif /* _BUTTON_TARGET_H_ */
