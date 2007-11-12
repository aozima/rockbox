/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Nicolas Pennequin
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <string.h>
#include "sprintf.h"
#include "system.h"
#include "albumart.h"
#include "id3.h"
#include "gwps.h"
#include "buffering.h"
#include "dircache.h"
#include "debug.h"


/* Strip filename from a full path
 *
 * buf      - buffer to extract directory to.
 * buf_size - size of buffer.
 * fullpath - fullpath to extract from.
 *
 * Split the directory part of the given fullpath and store it in buf
 *   (including last '/').
 * The function return parameter is a pointer to the filename
 *   inside the given fullpath.
 */
static char* strip_filename(char* buf, int buf_size, const char* fullpath)
{
    char* sep;
    int   len;

    if (!buf || buf_size <= 0 || !fullpath)
        return NULL;

    /* if 'fullpath' is only a filename return immediately */
    sep = strrchr(fullpath, '/');
    if (sep == NULL)
    {
        buf[0] = 0;
        return (char*)fullpath;
    }

    len = MIN(sep - fullpath + 1, buf_size - 1);
    strncpy(buf, fullpath, len);
    buf[len] = 0;
    return (sep + 1);
}

/* Strip extension from a filename.
 *
 * buf      - buffer to output the result to.
 * buf_size - size of the output buffer buffer.
 * file     - filename to strip extension from.
 *
 * Return value is a pointer to buf, which contains the result.
 */
static char* strip_extension(char* buf, int buf_size, const char* file)
{
    char* sep;
    int len;

    if (!buf || buf_size <= 0 || !file)
        return NULL;

    buf[0] = 0;

    sep = strrchr(file,'.');
    if (sep == NULL)
        return NULL;

    len = MIN(sep - file, buf_size - 1);
    strncpy(buf, file, len);
    buf[len] = 0;
    return buf;
}

/* Test file existence, using dircache of possible */
static bool file_exists(const char *file)
{
    int fd;

    if (!file || strlen(file) <= 0)
        return false;

#ifdef HAVE_DIRCACHE
    if (dircache_is_enabled())
        return (dircache_get_entry_ptr(file) != NULL);
#endif

    fd = open(file, O_RDONLY);
    if (fd < 0)
        return false;
    close(fd);
    return true;
}

/* Look for the first matching album art bitmap in the following list:
 *  ./<trackname><size>.bmp
 *  ./<albumname><size>.bmp
 *  ./cover<size>.bmp
 *  ../<albumname><size>.bmp
 *  ../cover<size>.bmp
 * <size> is the value of the size_string parameter, <trackname> and
 * <albumname> are read from the ID3 metadata.
 * If a matching bitmap is found, its filename is stored in buf.
 * Return value is true if a bitmap was found, false otherwise.
 */
static bool search_files(const struct mp3entry *id3, const char *size_string,
                         char *buf, int buflen)
{
    char path[MAX_PATH + 1];
    char dir[MAX_PATH + 1];
    bool found = false;
    const char *trackname;

    if (!id3 || !buf)
        return false;

    trackname = id3->path;
    strip_filename(dir, sizeof(dir), trackname);

    /* the first file we look for is one specific to the track playing */
    strip_extension(path, sizeof(path) - strlen(size_string) - 4, trackname);
    strcat(path, size_string);
    strcat(path, ".bmp");
    found = file_exists(path);
    if (!found && id3->album && strlen(id3->album) > 0)
    {
        /* if it doesn't exist,
         * we look for a file specific to the track's album name */
        snprintf(path, sizeof(path) - 1,
                 "%s%s%s.bmp",
                 (strlen(dir) >= 1) ? dir : "",
                 id3->album, size_string);
        path[sizeof(path) - 1] = 0;
        found = file_exists(path);
    }

    if (!found)
    {
        /* if it still doesn't exist, we look for a generic file */
        snprintf(path, sizeof(path)-1,
                 "%scover%s.bmp",
                 (strlen(dir) >= 1) ? dir : "", size_string);
        path[sizeof(path)-1] = 0;
        found = file_exists(path);
    }

    if (!found)
    {
        /* if it still doesn't exist,
         * we continue to search in the parent directory */
        char temp[MAX_PATH + 1];
        strncpy(temp, dir, strlen(dir) - 1);
        temp[strlen(dir) - 1] = 0;

        strip_filename(dir, sizeof(dir), temp);
    }

    if (!found && id3->album && strlen(id3->album) > 0)
    {
        /* we look in the parent directory
         * for a file specific to the track's album name */
        snprintf(path, sizeof(path)-1,
                 "%s%s%s.bmp",
                 (strlen(dir) >= 1) ? dir : "",
                 id3->album, size_string);
        found = file_exists(path);
    }

    if (!found)
    {
        /* if it still doesn't exist, we look in the parent directory
         * for a generic file */
        snprintf(path, sizeof(path)-1,
                 "%scover%s.bmp",
                 (strlen(dir) >= 1) ? dir : "", size_string);
        path[sizeof(path)-1] = 0;
        found = file_exists(path);
    }

    if (!found)
        return false;

    strncpy(buf, path, buflen);
    DEBUGF("Album art found: %s\n", path);
    return true;
}

/* Look for albumart bitmap in the same dir as the track and in its parent dir.
 * Stores the found filename in the buf parameter.
 * Returns true if a bitmap was found, false otherwise */
bool find_albumart(const struct mp3entry *id3, char *buf, int buflen)
{
    if (!id3 || !buf)
        return false;

    char size_string[9];
    struct wps_data *data = gui_wps[0].data;

    if (!data)
        return false;

    DEBUGF("Looking for album art for %s\n", id3->path);

    /* Write the size string, e.g. ".100x100". */
    snprintf(size_string, sizeof(size_string), ".%dx%d",
             data->albumart_max_width, data->albumart_max_height);

    /* First we look for a bitmap of the right size */
    if (search_files(id3, size_string, buf, buflen))
        return true;

    /* Then we look for generic bitmaps */
    *size_string = 0;
    return search_files(id3, size_string, buf, buflen);
}

/* Draw the album art bitmap from the given handle ID onto the given WPS.
   Call with clear = true to clear the bitmap instead of drawing it. */
void draw_album_art(struct gui_wps *gwps, int handle_id, bool clear)
{
    if (!gwps || !gwps->data || !gwps->display || handle_id < 0)
        return;

    struct wps_data *data = gwps->data;

#ifdef HAVE_REMOTE_LCD
    /* No album art on RWPS */
    if (data->remote_wps)
        return;
#endif

    struct bitmap *bmp;
    if (bufgetdata(handle_id, 0, (void *)&bmp) <= 0)
        return;

    short x = data->albumart_x;
    short y = data->albumart_y;
    short width = bmp->width;
    short height = bmp->height;

    if (data->albumart_max_width > 0)
    {
        /* Crop if the bitmap is too wide */
        width = MIN(bmp->width, data->albumart_max_width);

        /* Align */
        if (data->albumart_xalign & WPS_ALBUMART_ALIGN_RIGHT)
            x += data->albumart_max_width - width;
        else if (data->albumart_xalign & WPS_ALBUMART_ALIGN_CENTER)
            x += (data->albumart_max_width - width) / 2;
    }

    if (data->albumart_max_height > 0)
    {
        /* Crop if the bitmap is too high */
        height = MIN(bmp->height, data->albumart_max_height);

        /* Align */
        if (data->albumart_yalign & WPS_ALBUMART_ALIGN_BOTTOM)
            y += data->albumart_max_height - height;
        else if (data->albumart_yalign & WPS_ALBUMART_ALIGN_CENTER)
            y += (data->albumart_max_height - height) / 2;
    }

    if (!clear)
    {
        /* Draw the bitmap */
        gwps->display->set_drawmode(DRMODE_FG);
        gwps->display->bitmap_part((fb_data*)bmp->data, 0, 0, bmp->width,
                                   x, y, width, height);
        gwps->display->set_drawmode(DRMODE_SOLID);
    }
    else
    {
        /* Clear the bitmap */
        gwps->display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        gwps->display->fillrect(x, y, width, height);
        gwps->display->set_drawmode(DRMODE_SOLID);
    }
}
