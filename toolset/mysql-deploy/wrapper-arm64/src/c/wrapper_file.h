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

#ifndef _WRAPPER_FILE_H
#define _WRAPPER_FILE_H

#ifdef WIN32
#include <tchar.h>
#endif
#include "property.h"
#include "wrapper_i18n.h"
#include "wrapper_hashmap.h"

/*#define WRAPPER_FILE_DEBUG*/

/**
 * Callback declaration which can be passed to calls to configFileReader.
 */
typedef int (*ConfigFileReader_Callback)(void *param, const TCHAR *fileName, int lineNumber, TCHAR *config, int exitOnOverwrite, int logLevelOnOverwrite);

/* Structure used by configFileReader to read files. */
typedef struct ConfigFileReader ConfigFileReader;
struct ConfigFileReader {
    ConfigFileReader_Callback callback;
    void *callbackParam;
    int enableIncludes;
    int preload;
    int debugIncludes;                  /* debugIncludes controls whether or not debug output is logged. It is set using directives in the file being read. */
    int exitOnOverwrite;                /* If TRUE, causes the wrapper to exit when any property is overwritten in the config files. */
    int logLevelOnOverwrite;            /* Defines the log level of the messages reported when properties are overwritten. */
};

/**
 * Tests whether a file exists.
 *
 * @return TRUE if exists, FALSE otherwise.
 */
extern int wrapperFileExists(const TCHAR * filename);

#ifdef WIN32
extern int wrapperGetUNCFilePath(const TCHAR *path, int advice);
#endif

#ifdef WRAPPER_FILE_DEBUG
extern void wrapperFileTests();
#endif

#define CONFIG_FILE_READER_SUCCESS   101
#define CONFIG_FILE_READER_FAIL      102
#define CONFIG_FILE_READER_HARD_FAIL 103

/**
 * Reads configuration lines from the file `filename' and calls `callback' with the line and
 *  `callbackParam' specified to its arguments.
 *
 * @param filename Name of configuration file to read.
 * @param fileRequired TRUE if the file specified by filename is required, FALSE if a missing
 *                     file will silently fail.
 * @param callback Pointer to a callback funtion which will be called for each line read.
 * @param callbackParam Pointer to additional user data which will be passed to the callback.
 * @param enableIncludes If TRUE then includes will be supported.
 * @param preload TRUE if this is being called in the preload step meaning that all errors
 *                should be suppressed.
 * @param argCommand Argument passed to the binary.
 * @param originalWorkingDir Working directory of the binary at the moment it was launched.
 * @param warnedVarMap Map of undefined environment variables for which the user was warned.
 * @param logWarnings Flag that controls whether or not warnings will be logged.
 * @param logWarningLogLevel Log level at which any log warnings will be logged.
 * @param isDebugging Flag that controls whether or not debug output will be logged.
 *
 * @return CONFIG_FILE_READER_SUCCESS if the file was read successfully,
 *         CONFIG_FILE_READER_FAIL if there were any problems at all, or
 *         CONFIG_FILE_READER_HARD_FAIL if the problem should cascaded all the way up.
 */
extern int configFileReader(const TCHAR *filename,
                            int fileRequired,
                            ConfigFileReader_Callback callback,
                            void *callbackParam,
                            int enableIncludes,
                            int preload,
                            const TCHAR *argCommand,
                            const TCHAR *originalWorkingDir,
                            PHashMap warnedVarMap,
                            int logWarnings,
                            int logWarningLogLevel,
                            int isDebugging);

#endif

