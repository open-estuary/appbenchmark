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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef WIN32
 #include <errno.h>
 #include <tchar.h>
 #include <io.h>
 #include <winsock.h>
#else
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <langinfo.h>
 #include <limits.h>
 #if defined(IRIX)
  #define PATH_MAX FILENAME_MAX
 #endif
#endif

#include "wrapper_file.h"
#include "logger.h"

#ifndef TRUE
#define TRUE -1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define MAX_INCLUDE_DEPTH 10

/**
 * Tests whether a file exists.
 *
 * @return TRUE if exists, FALSE otherwise.
 */
int wrapperFileExists(const TCHAR * filename) {
    FILE * file;
    if ((file = _tfopen(filename, TEXT("r")))) {
        fclose(file);
        return TRUE;
    }
    return FALSE;
}

#ifdef WIN32
/**
 * @param path to check.
 * @param advice 0 if advice should be displayed.
 *
 * @return advice or advice + 1 if advice was logged.
 */
int wrapperGetUNCFilePath(const TCHAR *path, int advice) {
    TCHAR drive[4];
    DWORD result;

    /* See if the path starts with a drive.  Some users use forward slashes in the paths. */
    if ((path != NULL) && (_tcslen(path) >= 3) && (path[1] == TEXT(':')) && ((path[2] == TEXT('\\')) || (path[2] == TEXT('/')))) {
        _tcsncpy(drive, path, 2);
        drive[2] = TEXT('\\');
        drive[3] = TEXT('\0');
        result = GetDriveType(drive);
        if (result == DRIVE_REMOTE) {
            if (advice == 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE, TEXT("The following path in your Wrapper configuration file is to a mapped Network\n  Drive.  Using mapped network drives is not recommended as they will fail to\n  be resolved correctly under certain circumstances.  Please consider using\n  UNC paths (\\\\<host>\\<share>\\path). Additional references will be ignored.\n  Path: %s"), path);
                advice++;
            }
        } else if (result == DRIVE_NO_ROOT_DIR) {
            if (advice == 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE, TEXT("The following path in your Wrapper configuration file could not be resolved.\n  Please make sure the path exists.  If the path is a network share, it may be\n  that the current user is unable to resolve it.  Please consider using UNC\n  paths (\\\\<host>\\<share>\\path) or run the service as another user\n  (see wrapper.ntservice.account). Additional references will be ignored.\n  Path: %s"), path);
                advice++;
            }
        }
    }
    return advice;
}
#endif

/**
 * Read configuration file.
 */
int configFileReader_Read(ConfigFileReader *reader,
                          const TCHAR *filename,
                          int fileRequired,
                          int depth,
                          const TCHAR *parentFilename,
                          int parentLineNumber,
                          const TCHAR *argCommand,
                          const TCHAR *originalWorkingDir,
                          PHashMap warnedVarMap,
                          int logWarnings,
                          int logWarningLogLevel,
                          int isDebugging)
{
    FILE *stream;
    char bufferMB[MAX_PROPERTY_NAME_VALUE_LENGTH];
    TCHAR expBuffer[MAX_PROPERTY_NAME_VALUE_LENGTH];
    TCHAR *trimmedBuffer;
    size_t trimmedBufferLen;
    TCHAR *c;
    TCHAR *d;
    size_t i, j;
    size_t len;
    int quoted;
    TCHAR *absoluteBuffer;
    int hadBOM;
    int lineNumber;

    char *encodingMB;
#ifdef WIN32
    int encoding;
#else
    char* encoding;
    char* interumEncoding;
#endif
    int includeRequired;
    int readResult = CONFIG_FILE_READER_SUCCESS;
    int ret;
    TCHAR *bufferW;
#ifdef WIN32
    int size;
#endif
    int logLevelOnOverwrite;

#ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("configFileReader_Read('%s', required %d, depth %d, parent '%s', number %d, debugIncludes %d, preload %d)"),
        filename, fileRequired, depth, (parentFilename ? parentFilename : TEXT("<NULL>")), parentLineNumber, reader->debugIncludes, reader->preload );
#endif

    /* Look for the specified file. */
    if ((stream = _tfopen(filename, TEXT("rb"))) == NULL) {
        /* Unable to open the file. */
        if (reader->debugIncludes || fileRequired) {
            if (depth > 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                    TEXT("%sIncluded configuration file not found: %s\n  Referenced from: %s (line %d)\n  Current working directory: %s"),
                    (reader->debugIncludes ? TEXT("  ") : TEXT("")), filename, parentFilename, parentLineNumber, originalWorkingDir);
            } else {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                    TEXT("Configuration file not found: %s\n  Current working directory: %s"), filename, originalWorkingDir);
            }
        } else {
#ifdef _DEBUG
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("Configuration file not found: %s"), filename);
#endif
        }
        return CONFIG_FILE_READER_FAIL;
    }

    if (reader->debugIncludes) {
        if (!reader->preload) {
            if (depth > 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                    TEXT("  Reading included configuration file, %s"), filename);
            } else {
                /* Will not actually get here because the debug includes can't be set until it is loaded.
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                    TEXT("Reading configuration file, %s"), filename); */
            }
        }
    }

    /* Load in the first row of configurations to check the encoding. */
    if (fgets(bufferMB, MAX_PROPERTY_NAME_VALUE_LENGTH, stream)) {
        /* If the file starts with a BOM (Byte Order Marker) then we want to skip over it. */
        if ((bufferMB[0] == (char)0xef) && (bufferMB[1] == (char)0xbb) && (bufferMB[2] == (char)0xbf)) {
            i = 3;
            hadBOM = TRUE;
        } else {
            i = 0;
            hadBOM = FALSE;
        }

        /* Does the file start with "#encoding="? */
        if ((bufferMB[i++] == '#') && (bufferMB[i++] == 'e') && (bufferMB[i++] == 'n') && (bufferMB[i++] == 'c') &&
            (bufferMB[i++] == 'o') && (bufferMB[i++] == 'd') && (bufferMB[i++] == 'i') &&
            (bufferMB[i++] == 'n') && (bufferMB[i++] == 'g') && (bufferMB[i++] == '=')) {
            encodingMB = bufferMB + i;
            i = 0;
            while ((encodingMB[i] != ' ') && (encodingMB[i] != '\n') && (encodingMB[i]  != '\r')) {
               i++;
            }
            encodingMB[i] = '\0';

            if ((hadBOM) && (strIgnoreCaseCmp(encodingMB, "UTF-8") != 0)) {
            }
            if (getEncodingByName(encodingMB, &encoding) == TRUE) {
                fclose(stream);
                return CONFIG_FILE_READER_FAIL;
            }

        } else {
#ifdef WIN32
            encoding = GetACP();
#else 
            encoding = nl_langinfo(CODESET);
 #ifdef MACOSX
            if (strlen(encoding) == 0) {
                encoding = "UTF-8";
            }
 #endif
#endif
        }
    } else {
        /* Failed to read the first line of the file. */
#ifdef WIN32
        encoding = GetACP();
#else 
        encoding = nl_langinfo(CODESET);
 #ifdef MACOSX
        if (strlen(encoding) == 0) {
            encoding = "UTF-8";
        }
 #endif
#endif
    }
    fclose(stream);

    if ((stream = _tfopen(filename, TEXT("rb"))) == NULL) {
        /* Unable to open the file. */
        if (reader->debugIncludes || fileRequired) {
            if (depth > 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                    TEXT("%sIncluded configuration file, %s, was not found."), (reader->debugIncludes ? TEXT("  ") : TEXT("")), filename);
            } else {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                    TEXT("Configuration file, %s, was not found."), filename);
            }

        } else {
#ifdef _DEBUG
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("Configuration file not found: %s"), filename);
#endif
        }
        return CONFIG_FILE_READER_FAIL;
    }

    if (depth == 0) {
        /* At least log with LEVEL_DEBUG to help support. */
        reader->logLevelOnOverwrite = LEVEL_DEBUG;
    }
    
    /* Read all of the configurations */
    lineNumber = 1;
    do {
        c = (TCHAR*)fgets(bufferMB, MAX_PROPERTY_NAME_VALUE_LENGTH, stream);
        if (c != NULL) {
#ifdef WIN32
            ret = multiByteToWideChar(bufferMB, encoding, &bufferW, TRUE);
#else
            interumEncoding = nl_langinfo(CODESET);
 #ifdef MACOSX
            if (strlen(interumEncoding) == 0) {
                interumEncoding = "UTF-8";
            }
 #endif
            ret = multiByteToWideChar(bufferMB, encoding, interumEncoding, &bufferW, TRUE);
#endif
            if (ret) {
                if (bufferW) {
                    /* bufferW contains an error message. */
                    if (!reader->preload) {
                        if (depth > 0) {
                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                                TEXT("%sIncluded configuration file, %s, contains a problem on line #%d and could not be read. (%s)"),
                                (reader->debugIncludes ? TEXT("  ") : TEXT("")), filename, lineNumber, bufferW);
                        } else {
                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                                TEXT("Configuration file, %s, contains a problem on line #%d and could not be read. (%s)"), filename, lineNumber, bufferW);
                        }
                    }
                    free(bufferW);
                } else {
                    outOfMemory(TEXT("RCF"), 1);
                }
                fclose(stream);
                return CONFIG_FILE_READER_FAIL;
            }
            
#ifdef _DEBUG
            /* The line feeds are not yet stripped here. */
            /*
 #ifdef WIN32
            wprintf(TEXT("%s:%d (%d): [%s]\n"), filename, lineNumber, encoding, bufferW);
 #else
            wprintf(TEXT("%S:%d (%s to %s): [%S]\n"), filename, lineNumber, encoding, interumEncoding, bufferW);
 #endif
            */
#endif
            
            c = bufferW;
            /* Always strip both ^M and ^J off the end of the line, this is done rather
             *  than simply checking for \n so that files will work on all platforms
             *  even if their line feeds are incorrect. */
            if ((d = _tcschr(bufferW, 0x0d /* ^M */)) != NULL) {
                d[0] = TEXT('\0');
            }
            if ((d = _tcschr(bufferW, 0x0a /* ^J */)) != NULL) {
                d[0] = TEXT('\0');
            }
            /* Strip any whitespace from the front of the line. */
            trimmedBuffer = bufferW;
            while ((trimmedBuffer[0] == TEXT(' ')) || (trimmedBuffer[0] == 0x08)) {
                trimmedBuffer++;
            }

            /* If the line does not start with a comment, make sure that
             *  any comment at the end of line are stripped.  If any any point, a
             *  double hash, '##', is encountered it should be interpreted as a
             *  hash in the actual property rather than the beginning of a comment. */
            if (trimmedBuffer[0] != TEXT('#')) {
                len = _tcslen(trimmedBuffer);
                i = 0;
                quoted = 0;
                while (i < len) {
                    if (trimmedBuffer[i] == TEXT('"')) {
                        quoted = !quoted;
                    } else if ((trimmedBuffer[i] == TEXT('#')) && (!quoted)) {
                        /* Checking the next character will always be ok because it will be
                         *  '\0 at the end of the string. */
                        if (trimmedBuffer[i + 1] == TEXT('#')) {
                            /* We found an escaped #. Shift the rest of the string
                             *  down by one character to remove the second '#'.
                             *  Include the shifting of the '\0'. */
                            for (j = i + 1; j <= len; j++) {
                                trimmedBuffer[j - 1] = trimmedBuffer[j];
                            }
                            len--;
                        } else {
                            /* We found a comment. So this is the end. */
                            trimmedBuffer[i] = TEXT('\0');
                            len = i;
                        }
                    }
                    i++;
                }
            }

            /* Strip any whitespace from the end of the line. */
            trimmedBufferLen = _tcslen(trimmedBuffer);
            while ((trimmedBufferLen > 0) && ((trimmedBuffer[trimmedBufferLen - 1] == TEXT(' '))
            || (trimmedBuffer[trimmedBufferLen - 1] == 0x08))) {

                trimmedBuffer[--trimmedBufferLen] = TEXT('\0');
            }

            /* Only look at lines which contain data and do not start with a '#'
             *  If the line starts with '#include' then recurse to the include file */
            if (_tcslen(trimmedBuffer) > 0) {
                if (trimmedBuffer[0] != TEXT('#')) {
                    /*_tprintf(TEXT("%s\n"), trimmedBuffer);*/

                    if (!(*reader->callback)(reader->callbackParam, filename, lineNumber, trimmedBuffer, reader->exitOnOverwrite, reader->logLevelOnOverwrite)) {
                        readResult = CONFIG_FILE_READER_HARD_FAIL;
                        break;
                    }
                } else { /* so the line starts with a # */
                    if (reader->enableIncludes && strcmpIgnoreCase(trimmedBuffer, TEXT("#include.debug")) == 0) {
                        /* Enable include file debugging. */
                        if (reader->preload == FALSE) {
                            reader->debugIncludes = TRUE;
                            if (depth == 0) {
                                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                                    TEXT("Base configuration file is %s"), filename);
                            }
                        } else {
                            reader->debugIncludes = FALSE;
                        }
                    } else if (reader->enableIncludes && 
                    ((_tcsstr(trimmedBuffer, TEXT("#include ")) == trimmedBuffer) || (_tcsstr(trimmedBuffer, TEXT("#include.required ")) == trimmedBuffer))) {
                        if (_tcsstr(trimmedBuffer, TEXT("#include.required ")) == trimmedBuffer) {
                            /* The include file is required. */
                            includeRequired = TRUE;
                            c = trimmedBuffer + 18;
                        } else {
                            /* Include file, if the file does not exist, then ignore it */
                            includeRequired = FALSE;
                            c = trimmedBuffer + 9;
                        }
                        
                        /* Strip any leading whitespace */
                        while ((c[0] != TEXT('\0')) && (c[0] == TEXT(' '))) {
                            c++;
                        }
                        
                        /* The filename may contain environment variables, so expand them. */
                        if (reader->debugIncludes) {
                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                                TEXT("Found #include file in %s: %s"), filename, c);
                        }
                        evaluateEnvironmentVariables(c, expBuffer, MAX_PROPERTY_NAME_VALUE_LENGTH, logWarnings, warnedVarMap, logWarningLogLevel);
                        
                        if (reader->debugIncludes && (_tcscmp(c, expBuffer) != 0)) {
                            /* Only show this log if there were any environment variables. */
                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                                TEXT("  After environment variable replacements: %s"), expBuffer);
                        }
                        
                        /* Now obtain the real absolute path to the include file. */
#ifdef WIN32
                        /* Find out how big the absolute path will be */
                        size = GetFullPathName(expBuffer, 0, NULL, NULL); /* Size includes '\0' */
                        if (!size) {
                            if (reader->debugIncludes || includeRequired) {
                                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                                    TEXT("Unable to resolve the full path of included configuration file: %s (%s)\n  Referenced from: %s (line %d)\n  Current working directory: %s"),
                                    expBuffer, getLastErrorText(), filename, lineNumber, originalWorkingDir);
                            }
                            absoluteBuffer = NULL;
                        } else {
                            absoluteBuffer = malloc(sizeof(TCHAR) * size);
                            if (!absoluteBuffer) {
                                outOfMemory(TEXT("RCF"), 2);
                            } else {
                                if (!GetFullPathName(expBuffer, size, absoluteBuffer, NULL)) {
                                    if (reader->debugIncludes || includeRequired) {
                                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                                            TEXT("Unable to resolve the full path of included configuration file: %s (%s)\n  Referenced from: %s (line %d)\n  Current working directory: %s"),
                                            expBuffer, getLastErrorText(), filename, lineNumber, originalWorkingDir);
                                    }
                                    free(absoluteBuffer);
                                    absoluteBuffer = NULL;
                                }
                            }
                        }
#else
                        absoluteBuffer = malloc(sizeof(TCHAR) * (PATH_MAX + 1));
                        if (!absoluteBuffer) {
                            outOfMemory(TEXT("RCF"), 3);
                        } else {
                            if (_trealpathN(expBuffer, absoluteBuffer, PATH_MAX + 1) == NULL) {
                                if (reader->debugIncludes || includeRequired) {
                                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                                        TEXT("Unable to resolve the full path of included configuration file: %s (%s)\n  Referenced from: %s (line %d)\n  Current working directory: %s"),
                                        expBuffer, getLastErrorText(), filename, lineNumber, originalWorkingDir);
                                }
                                free(absoluteBuffer);
                                absoluteBuffer = NULL;
                            }
                        }
#endif
                        if (absoluteBuffer) {
                            if (depth < MAX_INCLUDE_DEPTH) {
                                readResult = configFileReader_Read(reader, absoluteBuffer, includeRequired, depth + 1, filename, lineNumber, argCommand, originalWorkingDir, warnedVarMap, logWarnings, logWarningLogLevel, isDebugging);
                                if (readResult == CONFIG_FILE_READER_SUCCESS) {
                                    /* Ok continue. */
                                } else if ((readResult == CONFIG_FILE_READER_FAIL) || (readResult == CONFIG_FILE_READER_HARD_FAIL)) {
                                    /* Failed. */
                                    if (includeRequired) {
                                        /* Include file was required, but we failed to read it. */
                                        if (!reader->preload) {
                                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                                                TEXT("%sThe required configuration file, %s, was not loaded.\n%s  Referenced from: %s (line %d)"),
                                                (reader->debugIncludes ? TEXT("  ") : TEXT("")), absoluteBuffer, (reader->debugIncludes ? TEXT("  ") : TEXT("")), filename, lineNumber);
                                        }
                                        readResult = CONFIG_FILE_READER_HARD_FAIL;
                                    }
                                    if (readResult == CONFIG_FILE_READER_HARD_FAIL) {
                                        /* Can't continue. */
                                        break;
                                    } else {
                                        /* Failed but continue. */
                                        readResult = CONFIG_FILE_READER_SUCCESS;
                                    }
                                } else {
                                    _tprintf(TEXT("Unexpected load error %d\n"), readResult);
                                    /* continue. */
                                    readResult = CONFIG_FILE_READER_SUCCESS;
                                }
                            } else {
                                if (reader->debugIncludes) {
                                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                                        TEXT("  Unable to include configuration file, %s, because the max include depth was reached."), absoluteBuffer);
                                }
                            }
                            free(absoluteBuffer);
                        } else {
                            if (includeRequired) {
                                /* Include file was required, but we failed to read it. */
                                if (!reader->preload) {
                                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                                        TEXT("%sThe required configuration file, %s, was not read.\n%s  Referenced from: %s (line %d)"),
                                        (reader->debugIncludes ? TEXT("  ") : TEXT("")), expBuffer, (reader->debugIncludes ? TEXT("  ") : TEXT("")), filename, lineNumber);
                                }
                                readResult = CONFIG_FILE_READER_HARD_FAIL;
                                break;
                            }
                        }
                    } else if (_tcsstr(trimmedBuffer, TEXT("#properties.")) == trimmedBuffer) {
                        if(_tcsstr(trimmedBuffer, TEXT("#properties.on_overwrite.exit=")) == trimmedBuffer) {
                            trimmedBuffer += 30;
                            if (_tcsicmp(trimmedBuffer, TEXT("TRUE")) == 0) {
                                reader->exitOnOverwrite = TRUE;
                            } else if (_tcsicmp(trimmedBuffer, TEXT("FALSE")) == 0) {
                                reader->exitOnOverwrite = FALSE;
                            } else if (!reader->preload) {
                                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                                    TEXT("Encountered an invalid boolean value for directive #properties.on_overwrite.exit=%s (line %d).  Ignoring this directive."),
                                    trimmedBuffer, lineNumber);
                            }
                        } else if (_tcsstr(trimmedBuffer, TEXT("#properties.on_overwrite.loglevel=")) == trimmedBuffer) {
                            trimmedBuffer += 34;
                            logLevelOnOverwrite = getLogLevelForName(trimmedBuffer);
                            if (logLevelOnOverwrite >= LEVEL_NONE ) {
                                /* At least log with LEVEL_DEBUG to help support. */
                                reader->logLevelOnOverwrite = LEVEL_DEBUG;
                            } else if (logLevelOnOverwrite != LEVEL_UNKNOWN) {
                                reader->logLevelOnOverwrite = logLevelOnOverwrite;
                            } else if (!reader->preload) {
                                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                                    TEXT("Encountered an invalid value for directive #properties.on_overwrite.loglevel=%s (line %d).  Ignoring this directive."),
                                    trimmedBuffer, lineNumber);
                            }
                        } else if (strcmpIgnoreCase(trimmedBuffer, TEXT("#properties.debug")) == 0) {
                            reader->logLevelOnOverwrite = LEVEL_STATUS;
                        }
                    } else if (_tcsstr(trimmedBuffer, TEXT("#log_messages.buffer_size=")) == trimmedBuffer) {
                        trimmedBuffer += 26;
                        setThreadMessageBufferInitialSize(_ttoi(trimmedBuffer));
                    }
                }
            }
            
            /* Always free each line read. */
            free(bufferW);
        }
        lineNumber++;
    } while (c != NULL);

    /* Call the callback after reading the file completely in order to copy reader->exitOnOverwrite & reader->logLevelOnOverwrite into the properties structure. 
     *  (we want to keep these values after preload for logging potential problems on properties defined in the command line)
     *  This is needed if directives are set at the end of the file with no properties after. */
    if (reader->preload) {
        if (!(*reader->callback)(reader->callbackParam, NULL, -1, NULL, reader->exitOnOverwrite, reader->logLevelOnOverwrite)) {
            readResult = CONFIG_FILE_READER_HARD_FAIL;
        }
    }
    
    /* Close the file */
    fclose(stream);

    return readResult;
}

/**
 * Reads configuration lines from the file `filename' and calls `callback' with the line and
 *  `callbackParam' specified to its arguments.
 *
 * @param filename Name of configuration file to read.
 * @param fileRequired TRUE if the file specified by filename is required, FALSE if a missing
 *                     file will silently fail.
 * @param callback Pointer to a callback function which will be called for each line read.
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
int configFileReader(const TCHAR *filename,
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
                     int isDebugging) {
    ConfigFileReader reader;
    
    /* Initialize the reader. */
    reader.callback = callback;
    reader.callbackParam = callbackParam;
    reader.enableIncludes = enableIncludes;
    reader.preload = preload;
    reader.debugIncludes = FALSE;
    reader.exitOnOverwrite = FALSE;
    reader.logLevelOnOverwrite = LEVEL_NONE; /* on preload, don't log anything. */
    
    return configFileReader_Read(&reader, filename, fileRequired, 0, NULL, 0, argCommand, originalWorkingDir, warnedVarMap, logWarnings, logWarningLogLevel, isDebugging);
}


