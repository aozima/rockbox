/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Jonathan Gordon
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

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include "config.h"
#include "lang.h"
#include "action.h"
#include "settings.h"
#include "menu.h"
#include "tree.h"
#include "list.h"
#include "color_picker.h"
#include "lcd.h"
#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif
#include "backdrop.h"
#include "exported_menus.h"
#include "appevents.h"
#include "viewport.h"
#include "statusbar-skinned.h"
#include "skin_engine/skin_engine.h"
#include "icons.h"

#ifdef HAVE_BACKDROP_IMAGE
/**
* Menu to clear the backdrop image
 */
static int clear_main_backdrop(void)
{
    global_settings.backdrop_file[0] = '-';
    global_settings.backdrop_file[1] = '\0';
    skin_backdrop_load_setting();
    viewportmanager_theme_enable(SCREEN_MAIN, false, NULL);
    viewportmanager_theme_undo(SCREEN_MAIN, true);
    settings_save();
    return 0;
}
MENUITEM_FUNCTION(clear_main_bd, 0, ID2P(LANG_CLEAR_BACKDROP),
                    clear_main_backdrop, NULL, NULL, Icon_NOICON);
#endif
#ifdef HAVE_LCD_COLOR

enum Colors {
    COLOR_FG = 0,
    COLOR_BG,
    COLOR_LSS,
    COLOR_LSE,
    COLOR_LST,
    COLOR_COUNT
};
static struct colour_info
{
    int *setting;
    int lang_id;
} colors[COLOR_COUNT] = {
    [COLOR_FG] = {&global_settings.fg_color, LANG_FOREGROUND_COLOR},
    [COLOR_BG] = {&global_settings.bg_color, LANG_BACKGROUND_COLOR},
    [COLOR_LSS] = {&global_settings.lss_color, LANG_SELECTOR_START_COLOR},
    [COLOR_LSE] = {&global_settings.lse_color, LANG_SELECTOR_END_COLOR},
    [COLOR_LST] = {&global_settings.lst_color, LANG_SELECTOR_TEXT_COLOR},
};

/**
 * Menu for fore/back/selection colors
 */
static int set_color_func(void* color)
{
    int res, c = (intptr_t)color, banned_color=-1, old_color;
    /* Don't let foreground be set the same as background and vice-versa */
    if (c == COLOR_BG)
        banned_color = *colors[COLOR_FG].setting;
    else if (c == COLOR_FG)
        banned_color = *colors[COLOR_BG].setting;

    old_color = *colors[c].setting;
    res = (int)set_color(&screens[SCREEN_MAIN],str(colors[c].lang_id),
                         colors[c].setting, banned_color);
    if (old_color != *colors[c].setting)
    {
        settings_save();
        settings_apply(false);
        settings_apply_skins();
    }
    return res;
}

static int reset_color(void)
{
    global_settings.fg_color = LCD_DEFAULT_FG;
    global_settings.bg_color = LCD_DEFAULT_BG;
    global_settings.lss_color = LCD_DEFAULT_LS;
    global_settings.lse_color = LCD_DEFAULT_BG;
    global_settings.lst_color = LCD_DEFAULT_FG;
    
    settings_save();
    settings_apply(false);
    settings_apply_skins();
    return 0;
}
MENUITEM_FUNCTION(set_bg_col, MENU_FUNC_USEPARAM, ID2P(LANG_BACKGROUND_COLOR),
                  set_color_func, (void*)COLOR_BG, NULL, Icon_NOICON);
MENUITEM_FUNCTION(set_fg_col, MENU_FUNC_USEPARAM, ID2P(LANG_FOREGROUND_COLOR),
                  set_color_func, (void*)COLOR_FG, NULL, Icon_NOICON);
MENUITEM_FUNCTION(set_lss_col, MENU_FUNC_USEPARAM, ID2P(LANG_SELECTOR_START_COLOR),
                  set_color_func, (void*)COLOR_LSS, NULL, Icon_NOICON);
MENUITEM_FUNCTION(set_lse_col, MENU_FUNC_USEPARAM, ID2P(LANG_SELECTOR_END_COLOR),
                  set_color_func, (void*)COLOR_LSE, NULL, Icon_NOICON);
MENUITEM_FUNCTION(set_lst_col, MENU_FUNC_USEPARAM, ID2P(LANG_SELECTOR_TEXT_COLOR),
                  set_color_func, (void*)COLOR_LST, NULL, Icon_NOICON);
MENUITEM_FUNCTION(reset_colors, 0, ID2P(LANG_RESET_COLORS),
                    reset_color, NULL, NULL, Icon_NOICON);

MAKE_MENU(lss_settings, ID2P(LANG_SELECTOR_COLOR_MENU),
            NULL, Icon_NOICON,
            &set_lss_col, &set_lse_col, &set_lst_col
         );

/* now the actual menu */
MAKE_MENU(colors_settings, ID2P(LANG_COLORS_MENU),
            NULL, Icon_Display_menu,
            &lss_settings,
            &set_bg_col, &set_fg_col, &reset_colors
         );
         
#endif /* HAVE_LCD_COLOR */
/*    LCD MENU                    */
/***********************************/


/************************************/
/*    BARS MENU                     */
/*                                  */


#ifdef HAVE_LCD_BITMAP
static int statusbar_callback_ex(int action,const struct menu_item_ex *this_item,
                                enum screen_type screen)
{
    (void)this_item;
    /* we save the old statusbar value here, so the old statusbars can get
     * removed and cleared from the display properly on exiting
     * (in gui_statusbar_changed() ) */
    static enum statusbar_values old_bar[NB_SCREENS];
    switch (action)
    {
        case ACTION_ENTER_MENUITEM:
            old_bar[screen] = statusbar_position(screen);
            break;
        case ACTION_EXIT_MENUITEM:
            if (old_bar[screen] != statusbar_position(screen))
                settings_apply_skins();
            break;
    }
    return ACTION_REDRAW;
}

#ifdef HAVE_REMOTE_LCD
static int statusbar_callback_remote(int action,const struct menu_item_ex *this_item)
{
    return statusbar_callback_ex(action, this_item, SCREEN_REMOTE);
}
#endif
static int statusbar_callback(int action,const struct menu_item_ex *this_item)
{
    return statusbar_callback_ex(action, this_item, SCREEN_MAIN);
}

#ifdef HAVE_BUTTONBAR
static int buttonbar_callback(int action, const struct menu_item_ex *this_item)
{
    (void)this_item;
    switch (action)
    {
        case ACTION_EXIT_MENUITEM:
            viewportmanager_theme_changed(THEME_BUTTONBAR);
        break;
    }
    return ACTION_REDRAW;
}
#endif
MENUITEM_SETTING(scrollbar_item, &global_settings.scrollbar, NULL);
MENUITEM_SETTING(scrollbar_width, &global_settings.scrollbar_width, NULL);
MENUITEM_SETTING(statusbar, &global_settings.statusbar,
                                                    statusbar_callback);
#ifdef HAVE_REMOTE_LCD
MENUITEM_SETTING(remote_statusbar, &global_settings.remote_statusbar,
                                                    statusbar_callback_remote);
#endif
#ifdef HAVE_BUTTONBAR
MENUITEM_SETTING(buttonbar, &global_settings.buttonbar, buttonbar_callback);
#endif
MENUITEM_SETTING(volume_type, &global_settings.volume_type, NULL);
MENUITEM_SETTING(battery_display, &global_settings.battery_display, NULL);
MAKE_MENU(bars_menu, ID2P(LANG_BARS_MENU), 0, Icon_NOICON,
          &scrollbar_item, &scrollbar_width, &statusbar,
#ifdef HAVE_REMOTE_LCD
          &remote_statusbar,
#endif  
#if CONFIG_KEYPAD == RECORDER_PAD
          &buttonbar,
#endif
          &volume_type
#if (CONFIG_BATTERY_MEASURE != 0)
          , &battery_display
#endif
          );
#endif /* HAVE_LCD_BITMAP */

/*                                  */
/*    BARS MENU                     */
/************************************/

#ifdef HAVE_LCD_BITMAP
static struct browse_folder_info fonts = {FONT_DIR, SHOW_FONT};
static struct browse_folder_info sbs   = {SBS_DIR, SHOW_SBS};
#if CONFIG_TUNER
static struct browse_folder_info fms   = {WPS_DIR, SHOW_FMS};
#endif
#endif
static struct browse_folder_info wps = {WPS_DIR, SHOW_WPS};
#ifdef HAVE_REMOTE_LCD
static struct browse_folder_info rwps = {WPS_DIR, SHOW_RWPS};
static struct browse_folder_info rsbs = {SBS_DIR, SHOW_RSBS};
#if CONFIG_TUNER
static struct browse_folder_info rfms   = {WPS_DIR, SHOW_RFMS};
#endif
#endif
static struct browse_folder_info themes = {THEME_DIR, SHOW_CFG};

int browse_folder(void *param)
{
    const char *ext, *setting;
    int lang_id = -1;
    char selected[MAX_FILENAME+10];
    const struct browse_folder_info *info =
        (const struct browse_folder_info*)param;
    struct browse_context browse;
    browse_context_init(&browse, info->show_options, 0,
                        NULL, NOICON, info->dir, NULL);

    /* if we are in a special settings folder, center the current setting */
    switch(info->show_options)
    {
        case SHOW_LNG:
            ext = "lng";
            if (global_settings.lang_file[0])
                setting = global_settings.lang_file;
            else
                setting = "english";
            lang_id = LANG_LANGUAGE;
            break;
        case SHOW_WPS:
            ext = "wps";
            setting = global_settings.wps_file;
            lang_id = LANG_WHILE_PLAYING;
            break;
#ifdef HAVE_LCD_BITMAP
        case SHOW_FONT:
            ext = "fnt";
            setting = global_settings.font_file;
            lang_id = LANG_CUSTOM_FONT;
            break;
        case SHOW_SBS:
            ext = "sbs";
            setting = global_settings.sbs_file;
            lang_id = LANG_BASE_SKIN;
            break;
#if CONFIG_TUNER
        case SHOW_FMS:
            ext = "fms";
            setting = global_settings.fms_file;
            lang_id = LANG_RADIOSCREEN;
            break;
#endif /* CONFIG_TUNER */
#endif
#ifdef HAVE_REMOTE_LCD
        case SHOW_RWPS:
            ext = "rwps";
            setting = global_settings.rwps_file;
            lang_id = LANG_REMOTE_WHILE_PLAYING;
            break;
        case SHOW_RSBS:
            ext = "rsbs";
            setting = global_settings.rsbs_file;
            lang_id = LANG_REMOTE_BASE_SKIN;
            break;
#if CONFIG_TUNER
        case SHOW_RFMS:
            ext = "rfms";
            setting = global_settings.rfms_file;
            lang_id = LANG_REMOTE_RADIOSCREEN;
            break;
#endif /* CONFIG_TUNER */
#endif
        default:
            ext = setting = NULL;
            break;
    }

    /* If we've found a file to center on, do it */
    if (setting)
    {
        /* if setting != NULL, ext is initialized */
        snprintf(selected, sizeof(selected), "%s.%s", setting, ext);
        browse.selected = selected;
        browse.icon = Icon_Questionmark;
        browse.title = str(lang_id);
    }

    return rockbox_browse(&browse);
}

#ifdef HAVE_LCD_BITMAP
MENUITEM_FUNCTION(browse_fonts, MENU_FUNC_USEPARAM, 
        ID2P(LANG_CUSTOM_FONT), 
        browse_folder, (void*)&fonts, NULL, Icon_Font);

MENUITEM_FUNCTION(browse_sbs, MENU_FUNC_USEPARAM, 
        ID2P(LANG_BASE_SKIN), 
        browse_folder, (void*)&sbs, NULL, Icon_Wps);
#if CONFIG_TUNER
MENUITEM_FUNCTION(browse_fms, MENU_FUNC_USEPARAM, 
        ID2P(LANG_RADIOSCREEN), 
        browse_folder, (void*)&fms, NULL, Icon_Wps);
#endif
#endif
MENUITEM_FUNCTION(browse_wps, MENU_FUNC_USEPARAM, 
        ID2P(LANG_WHILE_PLAYING), 
        browse_folder, (void*)&wps, NULL, Icon_Wps);
#ifdef HAVE_REMOTE_LCD
MENUITEM_FUNCTION(browse_rwps, MENU_FUNC_USEPARAM, 
        ID2P(LANG_REMOTE_WHILE_PLAYING), 
        browse_folder, (void*)&rwps, NULL, Icon_Wps);
MENUITEM_FUNCTION(browse_rsbs, MENU_FUNC_USEPARAM, 
        ID2P(LANG_REMOTE_BASE_SKIN), 
        browse_folder, (void*)&rsbs, NULL, Icon_Wps);
#if CONFIG_TUNER
MENUITEM_FUNCTION(browse_rfms, MENU_FUNC_USEPARAM, 
        ID2P(LANG_REMOTE_RADIOSCREEN), 
        browse_folder, (void*)&rfms, NULL, Icon_Wps);
#endif
#endif


static int showicons_callback(int action, const struct menu_item_ex *this_item)
{
    (void)this_item;
    static bool old_icons;
    switch (action)
    {
        case ACTION_ENTER_MENUITEM:
            old_icons = global_settings.show_icons;
            break;
        case ACTION_EXIT_MENUITEM:
            if (old_icons != global_settings.show_icons)
                icons_init();
            break;
    }
    return ACTION_REDRAW;
}

MENUITEM_SETTING(show_icons, &global_settings.show_icons, showicons_callback);
MENUITEM_FUNCTION(browse_themes, MENU_FUNC_USEPARAM, 
        ID2P(LANG_CUSTOM_THEME), 
        browse_folder, (void*)&themes, NULL, Icon_Config);
#ifdef HAVE_LCD_BITMAP
MENUITEM_SETTING(cursor_style, &global_settings.cursor_style, NULL);
#endif

MAKE_MENU(theme_menu, ID2P(LANG_THEME_MENU),
            NULL, Icon_Wps,
            &browse_themes,
#ifdef HAVE_LCD_BITMAP
            &browse_fonts,
#endif
            &browse_wps,
#ifdef HAVE_REMOTE_LCD
            &browse_rwps,
#endif
#if CONFIG_TUNER
            &browse_fms,
#ifdef HAVE_REMOTE_LCD
            &browse_rfms,
#endif
#endif
#ifdef HAVE_LCD_BITMAP
            &browse_sbs,
#endif
#ifdef HAVE_REMOTE_LCD
            &browse_rsbs,
#endif
            &show_icons,
#ifdef HAVE_BACKDROP_IMAGE
            &clear_main_bd,
#endif
#ifdef HAVE_LCD_BITMAP
            &bars_menu,
            &cursor_style,
#endif
#ifdef HAVE_LCD_COLOR
            &colors_settings,
#endif
            );
