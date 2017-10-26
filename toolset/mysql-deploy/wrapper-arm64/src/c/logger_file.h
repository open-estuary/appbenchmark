/*
 * Copyright (c) 1999, 2017 Tanuki Software, Ltd.
 * http://www.tanukisoftware.com
 * All rights reserved.
 *
 * This software is the proprietary information of Tanuki Software.
 * You shall use it only in accordance with the terms of the
 * license agreement you entered into with Tanuki Software.
 * http://wrapper.tanukisoftware.com/doc/english/licenseOverview.html
 */

/**
 * Author:
 *   Tanuki Software Development Team <support@tanukisoftware.com>
 */

#ifndef _LOGGER_FILE_H
#define _LOGGER_FILE_H

#ifdef WIN32
 #include <tchar.h>
#endif
#include "wrapper_i18n.h"

/*#define LOGGER_FILE_DEBUG*/

#define LOGGER_FILE_SORT_MODE_TIMES 100
#define LOGGER_FILE_SORT_MODE_NAMES_ASC 101
#define LOGGER_FILE_SORT_MODE_NAMES_DEC 102

/**
 * Returns a valid sort mode given a name: "TIMES", "NAMES_ASC", "NAMES_DEC".
 *  In the event of an invalid value, TIMES will be returned.
 */
extern int loggerFileGetSortMode(const TCHAR *modeName);

/**
 * Returns a NULL terminated list of file names within the specified pattern.
 *  The files will be sorted new to old for TIMES.  Then incremental ordering
 *  for NAMES.  The numeric components of the names will be treated as
 *  numbers and sorted accordingly.
 */
extern TCHAR** loggerFileGetFiles(const TCHAR* pattern, int sortMode);

/**
 * Frees the array of file names returned by wrapperFileGetFiles()
 */
extern void loggerFileFreeFiles(TCHAR** files);

extern TCHAR *combinePath(const TCHAR *path1, const TCHAR *path2);

extern TCHAR *getRealPath(const TCHAR *path, const TCHAR *pathDesc, int errorLevel, int useQueue);

TCHAR* getAbsolutePathOfFile(const TCHAR* path, const TCHAR *pathDesc, int errorLevel, int useQueue);
#endif
