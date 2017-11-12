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

#ifdef WIN32
 #include <errno.h>
 #include <io.h>
#else
 #include <glob.h>
 #include <string.h>
 #include <limits.h>
 #if defined(IRIX)
  #define PATH_MAX FILENAME_MAX
 #endif
#endif

#ifdef WIN32
 #include <winsock.h>
#endif

#include "logger.h"
#include "logger_file.h"
#include "wrapper_i18n.h"

#define FILES_CHUNK 5

#ifndef TRUE
#define TRUE -1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/**
 * Returns a valid sort mode given a name: "TIMES", "NAMES_ASC", "NAMES_DEC".
 *  In the event of an invalid value, TIMES will be returned.
 */
int loggerFileGetSortMode(const TCHAR *modeName) {
    if (strcmpIgnoreCase(modeName, TEXT("NAMES_ASC")) == 0) {
        return LOGGER_FILE_SORT_MODE_NAMES_ASC;
    } else if (strcmpIgnoreCase(modeName, TEXT("NAMES_DEC")) == 0) {
        return LOGGER_FILE_SORT_MODE_NAMES_DEC;
    } else {
        return LOGGER_FILE_SORT_MODE_TIMES;
    }
}

#ifdef WIN32
int sortFilesTimes(TCHAR **files, __time64_t *fileTimes, int cnt) {
#else
int sortFilesTimes(TCHAR **files, time_t *fileTimes, int cnt) {
#endif
    int i, j;
    TCHAR *temp;
#ifdef WIN32
    __time64_t tempTime;
#else
    time_t tempTime;
#endif

    for (i = 0; i < cnt; i++) {
        for (j = 0; j < cnt - 1; j++) {
            if (fileTimes[j] < fileTimes[j + 1]) {
                temp = files[j + 1];
                tempTime = fileTimes[j + 1];

                files[j + 1] = files[j];
                fileTimes[j + 1] = fileTimes[j];

                files[j] = temp;
                fileTimes[j] = tempTime;
            }
        }
    }

    return TRUE;
}

/**
 * Compares two strings.  Returns 0 if they are equal, -1 if file1 is bigger, 1 if file2 is bigger.
 */
int compareFileNames(const TCHAR *file1, const TCHAR *file2) {
    int pos1, pos2;
    TCHAR c1, c2;
    int numeric1, numeric2;
    long int num1, num2;
    int afterNumber = FALSE;

    pos1 = 0;
    pos2 = 0;

    while (TRUE) {
        c1 = file1[pos1];
        c2 = file2[pos2];
        /*printf("     file1[%d]=%d, file2[%d]=%d\n", pos1, c1, pos2, c2);*/

        /* Did we find the null. */
        if (c1 == 0) {
            if (c2 == 0) {
                return 0;
            } else {
                return 1;
            }
        } else {
            if (c2 == 0) {
                return -1;
            } else {
                /* Continue. */
            }
        }

        /* We have two characters. */
        numeric1 = ((c1 >= TEXT('0')) && (c1 <= TEXT('9')));
        numeric2 = ((c2 >= TEXT('0')) && (c2 <= TEXT('9')));

        /* See if one or both of the strings is numeric. */
        if (numeric1) {
            if (numeric2) {
                /* Both are numeric, we need to start comparing the two file names as integer values. */
                num1 = c1 - TEXT('0');
                c1 = file1[pos1 + 1];
                while ((c1 >= TEXT('0')) && (c1 <= TEXT('9'))) {
                    num1 = num1 * 10 + (c1 - TEXT('0'));
                    pos1++;
                    c1 = file1[pos1 + 1];
                }

                num2 = c2 - TEXT('0');
                c2 = file2[pos2 + 1];
                while ((c2 >= TEXT('0')) && (c2 <= TEXT('9'))) {
                    num2 = num2 * 10 + (c2 - TEXT('0'));
                    pos2++;
                    c2 = file2[pos2 + 1];
                }

                /*printf("     num1=%ld, num2=%ld\n", num1, num2);*/
                if (num1 > num2) {
                    return -1;
                } else if (num2 > num1) {
                    return 1;
                } else {
                    /* Equal, continue. */
                }
                afterNumber = TRUE;
            } else {
                /* 1 is numeric, 2 is not. */
                if (afterNumber) {
                    return -1;
                } else {
                    return 1;
                }
            }
        } else {
            if (numeric2) {
                /* 1 is not, 2 is numeric. */
                if (afterNumber) {
                    return 1;
                } else {
                    return -1;
                }
            } else {
                /* Neither is numeric. */
            }
        }

        /* Compare the characters as is. */
        if (c1 > c2) {
            return -1;
        } else if (c2 > c1) {
            return 1;
        } else {
            /* Equal, continue. */
            if ((c1 == TEXT('.')) || (c1 == TEXT('-')) || (c1 == TEXT('_'))) {
            } else {
                afterNumber = FALSE;
            }
        }

        pos1++;
        pos2++;
    }
}

int sortFilesNamesAsc(TCHAR **files, int cnt) {
    int i, j;
    TCHAR *temp;
    int cmp;

    for (i = 0; i < cnt; i++) {
        for (j = 0; j < cnt - 1; j++) {
            cmp = compareFileNames(files[j], files[j+1]);
            if (cmp < 0) {
                temp = files[j + 1];
                files[j + 1] = files[j];
                files[j] = temp;
            }
        }
    }

    return TRUE;
}

int sortFilesNamesDec(TCHAR **files, int cnt) {
    int i, j;
    TCHAR *temp;
    int cmp;

    for (i = 0; i < cnt; i++) {
        for (j = 0; j < cnt - 1; j++) {
            cmp = compareFileNames(files[j], files[j+1]);
            if (cmp > 0) {
                temp = files[j + 1];
                files[j + 1] = files[j];
                files[j] = temp;
            }
        }
    }

    return TRUE;
}

/**
 * Returns a NULL terminated list of file names within the specified pattern.
 *  The files will be sorted new to old for TIMES.  Then incremental ordering
 *  for NAMES.  The numeric components of the names will be treated as
 *  numbers and sorted accordingly.
 */
TCHAR** loggerFileGetFiles(const TCHAR* pattern, int sortMode) {
    int cnt;
    int filesSize;
    TCHAR **files;
#ifdef WIN32
    int i;
    size_t dirLen;
    TCHAR *c;
    TCHAR *dirPart;
    intptr_t handle;
    struct _tfinddata64_t fblock;
    size_t fileLen;
    TCHAR **newFiles;
    __time64_t *fileTimes;
    __time64_t *newFileTimes;
#else
#ifdef WRAPPER_FILE_DEBUG
    int i;
#endif
    int result;
    glob_t g;
    int findex;
    time_t *fileTimes;
    struct stat fileStat;
#endif

#ifdef WRAPPER_FILE_DEBUG
    _tprintf(TEXT("loggerFileGetFiles(%s, %d)\n"), pattern, sortMode);
#endif

#ifdef WIN32
    cnt = 0;
    /* Initialize the files array. */
    filesSize = FILES_CHUNK;
    files = malloc(sizeof(TCHAR *) * filesSize);
    if (!files) {
        outOfMemoryQueued(TEXT("WFGF"), 1);
        return NULL;
    }
    memset(files, 0, sizeof(TCHAR *) * filesSize);

    fileTimes = malloc(sizeof(__time64_t) * filesSize);
    if (!fileTimes) {
        outOfMemoryQueued(TEXT("WFGF"), 2);
        free(files);
        return NULL;
    }
    memset(fileTimes, 0, sizeof(__time64_t) * filesSize);

    /* Extract any path information from the beginning of the file */
    c = max(_tcsrchr(pattern, TEXT('\\')), _tcsrchr(pattern, TEXT('/')));
    if (c == NULL) {
        /* No directory component */
        dirPart = malloc(sizeof(TCHAR) * 1);
        if (!dirPart) {
            outOfMemoryQueued(TEXT("WFGF"), 3);
            return NULL;
        }
        dirPart[0] = TEXT('\0');
        dirLen = 0;
    } else {
        /* extract the directory. */
        dirLen = c - pattern + 1;
        dirPart = malloc(sizeof(TCHAR) * (dirLen + 1));
        if (!dirPart) {
            outOfMemoryQueued(TEXT("WFGF"), 4);
            return NULL;
        }
        _tcsncpy(dirPart, pattern, dirLen);
        dirPart[dirLen] = TEXT('\0');
    }

#ifdef WRAPPER_FILE_DEBUG
    _tprintf(TEXT("  dirPart=[%s]\n"), dirPart);
#endif

    /* Get the first file. */
#ifdef _IA64_
    /* On Itanium, the first parameter is not a "const". If you don't cast it, then you have a warning */
    if ((handle = _tfindfirst64((TCHAR *)pattern, &fblock)) > 0) {
#else
    if ((handle = _tfindfirst64(pattern, &fblock)) > 0) {
#endif
        if ((_tcscmp(fblock.name, TEXT(".")) != 0) && (_tcscmp(fblock.name, TEXT("..")) != 0)) {
            fileLen = _tcslen(fblock.name);
            files[cnt] = malloc((_tcslen(dirPart) + _tcslen(fblock.name) + 1) * sizeof(TCHAR));
            if (!files[cnt]) {
                outOfMemoryQueued(TEXT("WFGF"), 5);
                free(fileTimes);
                loggerFileFreeFiles(files);
                free(dirPart);
                return NULL;
            }
            _sntprintf(files[cnt], _tcslen(dirPart) + _tcslen(fblock.name) + 1, TEXT("%s%s"), dirPart, fblock.name);
            fileTimes[cnt] = fblock.time_write;
#ifdef WRAPPER_FILE_DEBUG
            _tprintf(TEXT("  files[%d]=%s, %ld\n"), cnt, files[cnt], fileTimes[cnt]);
#endif

            cnt++;
        }

        /* Look for additional files. */
        while (_tfindnext64(handle, &fblock) == 0) {
            if ((_tcscmp(fblock.name, TEXT(".")) != 0) && (_tcscmp(fblock.name, TEXT("..")) != 0)) {
                /* Make sure we have enough room in the files array. */
                if (cnt >= filesSize - 1) {
                    newFiles = malloc(sizeof(TCHAR *) * (filesSize + FILES_CHUNK));
                    if (!newFiles) {
                        outOfMemoryQueued(TEXT("WFGF"), 6);
                        free(fileTimes);
                        loggerFileFreeFiles(files);
                        free(dirPart);
                        return NULL;
                    }
                    memset(newFiles, 0, sizeof(TCHAR *) * (filesSize + FILES_CHUNK));
                    newFileTimes = malloc(sizeof(__time64_t) * (filesSize + FILES_CHUNK));
                    if (!newFileTimes) {
                        outOfMemoryQueued(TEXT("WFGF"), 7);
                        free(newFiles);
                        free(fileTimes);
                        loggerFileFreeFiles(files);
                        free(dirPart);
                        return NULL;
                    }
                    memset(newFileTimes, 0, sizeof(__time64_t) * (filesSize + FILES_CHUNK));
                    
                    for (i = 0; i < filesSize; i++) {
                        newFiles[i] = files[i];
                        newFileTimes[i] = fileTimes[i];
                    }
                    free(files);
                    free(fileTimes);
                    files = newFiles;
                    fileTimes = newFileTimes;
                    filesSize += FILES_CHUNK;
#ifdef WRAPPER_FILE_DEBUG
                    _tprintf(TEXT("  increased files to %d\n"), filesSize);
#endif
                }

                fileLen = _tcslen(fblock.name);
                files[cnt] = malloc((_tcslen(dirPart) + _tcslen(fblock.name) + 1) * sizeof(TCHAR));
                if (!files[cnt]) {
                    outOfMemoryQueued(TEXT("WFGF"), 8);
                    free(fileTimes);
                    loggerFileFreeFiles(files);
                    free(dirPart);
                    return NULL;
                }
                _sntprintf(files[cnt], _tcslen(dirPart) + _tcslen(fblock.name) + 1, TEXT("%s%s"), dirPart, fblock.name);
                fileTimes[cnt] = fblock.time_write;

#ifdef WRAPPER_FILE_DEBUG
                _tprintf(TEXT("  files[%d]=%s, %ld\n"), cnt, files[cnt], fileTimes[cnt]);
#endif
                cnt++;
            }
        }

        /* Close the file search */
        _findclose(handle);
    }

    if (cnt <= 0) {
        if (errno == ENOENT) {
            /* No files matched. */
#ifdef WRAPPER_FILE_DEBUG
            _tprintf(TEXT("  No files matched.\n"));
#endif
        } else {
            /* Encountered an error of some kind. */
            log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("Error listing files, %s: %s"), pattern, getLastErrorText());
            free(fileTimes);
            loggerFileFreeFiles(files);
            return NULL;
        }
    }
#else /* Unix */

#ifdef UNICODE
    char* cPattern;
    size_t req;

    req = wcstombs(NULL, pattern, 0) + 1;
    cPattern = malloc(req);
    if(!cPattern) {
        outOfMemoryQueued(TEXT("WFGF"), 8);
        return NULL;
    }
    wcstombs(cPattern, pattern, req);

    result = glob(cPattern, GLOB_MARK | GLOB_NOSORT, NULL, &g);
    free(cPattern);
#else
    result = glob(pattern, GLOB_MARK | GLOB_NOSORT, NULL, &g);
#endif
    cnt = 0;
    if (!result) {
        if (g.gl_pathc > 0) {
            filesSize = g.gl_pathc + 1;
            files = malloc(sizeof(TCHAR *) * filesSize);
            if (!files) {
                outOfMemoryQueued(TEXT("WFGF"), 9);
                return NULL;
            }
            memset(files, 0, sizeof(TCHAR *) * filesSize);
            
            fileTimes = malloc(sizeof(time_t) * filesSize);
            if (!fileTimes) {
                outOfMemoryQueued(TEXT("WFGF"), 10);
                loggerFileFreeFiles(files);
                return NULL;
            }
            memset(fileTimes, 0, sizeof(time_t) * filesSize);

            for (findex = 0; findex < g.gl_pathc; findex++) {
#ifdef UNICODE
                req = mbstowcs(NULL, g.gl_pathv[findex], 0);
                if (req == (size_t)-1) {
                    invalidMultiByteSequence(TEXT("GLET"), 1);
                }
                files[cnt] = malloc((req + 1) * sizeof(TCHAR));
                if (!files[cnt]) {
                    outOfMemoryQueued(TEXT("WFGF"), 11);
                    free(fileTimes);
                    loggerFileFreeFiles(files);
                    return NULL;
                }
                mbstowcs(files[cnt], g.gl_pathv[findex], req + 1);

#else
                files[cnt] = malloc((strlen(g.gl_pathv[findex]) + 1));
                if (!files[cnt]) {
                    outOfMemoryQueued(TEXT("WFGF"), 11);
                    free(fileTimes);
                    loggerFileFreeFiles(files);
                    return NULL;
                }

                strncpy(files[cnt], g.gl_pathv[findex], strlen(g.gl_pathv[findex]) + 1);
#endif

                /* Only try to get the modified time if it is really necessary. */
                if (sortMode == LOGGER_FILE_SORT_MODE_TIMES) {
                    if (!_tstat(files[cnt], &fileStat)) {
                        fileTimes[cnt] = fileStat.st_mtime;
                    } else {
                        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT("Failed to stat %s: %s"), files[cnt], getLastErrorText());
                    }
                }
#ifdef WRAPPER_FILE_DEBUG
                _tprintf(TEXT("  files[%d]=%s, %ld\n"), cnt, files[cnt], fileTimes[cnt]);
#endif
                cnt++;
            }
        } else {
#ifdef WRAPPER_FILE_DEBUG
            printf("  No files matched.\n");
#endif
            /* No files, but we still need the array. */
            filesSize = 1;
            files = malloc(sizeof(TCHAR *) * filesSize);
            if (!files) {
                outOfMemoryQueued(TEXT("WFGF"), 12);
                return NULL;
            }
            memset(files, 0, sizeof(TCHAR *) * filesSize);
            
            fileTimes = malloc(sizeof(time_t) * filesSize);
            if (!fileTimes) {
                free(files);
                outOfMemoryQueued(TEXT("WFGF"), 13);
                return NULL;
            }
            memset(fileTimes, 0, sizeof(time_t) * filesSize);
        }

        globfree(&g);
    } else if (result == GLOB_NOMATCH) {
#ifdef WRAPPER_FILE_DEBUG
        _tprintf(TEXT("  No files matched.\n"));
#endif
        /* No files, but we still need the array. */
        filesSize = 1;
        files = malloc(sizeof(TCHAR *) * filesSize);
        if (!files) {
            outOfMemoryQueued(TEXT("WFGF"), 14);
            return NULL;
        }
        memset(files, 0, sizeof(TCHAR *) * filesSize);

        fileTimes = malloc(sizeof(time_t) * filesSize);
        if (!fileTimes) {
            free(files);
            outOfMemoryQueued(TEXT("WFGF"), 15);
            return NULL;
        }
        memset(fileTimes, 0, sizeof(time_t) * filesSize);
    } else {
        /* Encountered an error of some kind. */
        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("Error listing files, %s: %s"), pattern, getLastErrorText());
        return NULL;
    }
#endif
    
    if (sortMode == LOGGER_FILE_SORT_MODE_TIMES) {
        if (!sortFilesTimes(files, fileTimes, cnt)) {
            /* Failed. Reported. */
            free(fileTimes);
            loggerFileFreeFiles(files);
            return NULL;
        }
    } else if (sortMode == LOGGER_FILE_SORT_MODE_NAMES_DEC) {
        if (!sortFilesNamesDec(files, cnt)) {
            /* Failed. Reported. */
            free(fileTimes);
            loggerFileFreeFiles(files);
            return NULL;
        }
    } else { /* LOGGER_FILE_SORT_MODE_NAMES_ASC */
        if (!sortFilesNamesAsc(files, cnt)) {
            /* Failed. Reported. */
            free(fileTimes);
            loggerFileFreeFiles(files);
            return NULL;
        }
    }

#ifdef WRAPPER_FILE_DEBUG
    _tprintf(TEXT("  Sorted:\n"));
    for (i = 0; i < cnt; i++) {
        _tprintf(TEXT("  files[%d]=%s, %ld\n"), i, files[i], fileTimes[i]);
    }
    _tprintf(TEXT("loggerFileGetFiles(%s, %d) END\n"), pattern, sortMode);
#endif

    free(fileTimes);

    return files;
}

/**
 * Frees the array of file names returned by loggerFileGetFiles()
 */
void loggerFileFreeFiles(TCHAR** files) {
    int i;

    i = 0;
    while (files[i]) {
        free(files[i]);
        i++;
    }
    free(files);
}

/**
 * Combines two paths and take care to add only one separator between them.
 *
 * The returned string must be freed by the caller.
 *
 * @param path1 base path
 * @param path2 relative path.
 *
 * @return The resulting path, or NULL if there were any problems.
 */
TCHAR *combinePath(const TCHAR *path1, const TCHAR *path2) {
    TCHAR* result;
    TCHAR* tempPath1 = NULL;
    TCHAR* tempPath2 = NULL;
    TCHAR* tempPath2Ptr;
    TCHAR c1;
    TCHAR c2;
    size_t len1 = _tcslen(path1); 
    size_t len2 = _tcslen(path2); 
    size_t len = len1 + len2;
    int i = 0;
#ifdef WIN32
    const TCHAR bad_sep  = TEXT('/');
    const TCHAR good_sep = TEXT('\\');
#else
    const TCHAR bad_sep  = TEXT('\\');
    const TCHAR good_sep = TEXT('/');
#endif

    if (len1 > 0) {
        tempPath1 = malloc(sizeof(TCHAR) * (len1 + 1));
        if (!tempPath1) {
            outOfMemoryQueued(TEXT("CP"), 1);
            return NULL;
        }
        _tcsncpy(tempPath1, path1, len1 + 1);
    }
    if (len2 > 0) {
        tempPath2 = malloc(sizeof(TCHAR) * (len2 + 1));
        if (!tempPath2) {
            outOfMemoryQueued(TEXT("CP"), 2);
            free(tempPath1);
            return NULL;
        }
        _tcsncpy(tempPath2, path2, len2 + 1);
    }
    if (!tempPath1 && !tempPath2) {
        result = NULL;
    } else if (tempPath1 && !tempPath2) {
        result = tempPath1;
    } else if (!tempPath1 && tempPath2) {
        result = tempPath2;
    } else {
        tempPath2Ptr = tempPath2;

        /* first replace all directory separators by their standard according to the platform. 
         *  we want to avoid that the two paths use different separators. */
        while (tempPath1[i] != TEXT('\0')) {
            if (tempPath1[i] == bad_sep) {
                tempPath1[i] = good_sep;
            }
            i++;
        }
        i = 0;
        while (tempPath2[i] != TEXT('\0')) {
            if (tempPath2[i] == bad_sep) {
                tempPath2[i] = good_sep;
            }
            i++;
        }
        
        c1 = tempPath1[len1 - 1];
        c2 = tempPath2[0];
        
        if (c1 == good_sep) {
            if (c2 == good_sep) {
                tempPath2Ptr++;
            } else {
                len += 1;
            }
            
            result = malloc(sizeof(TCHAR) * len);
            if (!result) {
                outOfMemoryQueued(TEXT("CP"), 3);
                free(tempPath1);
                free(tempPath2);
                return NULL;
            }
            _sntprintf(result, len, TEXT("%s%s"), tempPath1, tempPath2Ptr);
        } else {
            if (c2 == good_sep) {
                tempPath2Ptr++;
                len += 1;
            } else {
                len += 2;
            }
            
            result = malloc(sizeof(TCHAR) * len);
            if (!result) {
                outOfMemoryQueued(TEXT("CP"), 4);
                free(tempPath1);
                free(tempPath2);
                return NULL;
            }
            _sntprintf(result, len, TEXT("%s%c%s"), tempPath1, good_sep, tempPath2Ptr);
        }
        free(tempPath1);
        free(tempPath2);
    }
    return result;
}

/**
 * Given a path, resolve a real absolute path which has resolved all relative and symbolic links.
 *
 * The returned string must be freed by the caller.
 *
 * @param path The source path.
 * @param pathDesc A description of the path used for error messages.
 * @param errorLevel Level to log errors at.
 *
 * @return The absolute path, or NULL if there were any problems.
 */
TCHAR *getRealPath(const TCHAR *path, const TCHAR *pathDesc, int errorLevel, int useQueue) {
    TCHAR *realPath;
#ifdef WIN32
    DWORD len;
#else
    size_t len;
    TCHAR *tempPath;
#endif
    
#ifdef WIN32
    len = GetFullPathName(path, 0, NULL, NULL);
    if (!len) {
        if (errorLevel != LEVEL_NONE) {
            log_printf_queue(useQueue, WRAPPER_SOURCE_WRAPPER, errorLevel, TEXT("Unable to resolve the %s '%s': %s"), pathDesc, path, getLastErrorText());
        }
        return NULL;
    }
    realPath = malloc(sizeof(TCHAR) * len);
    if (!realPath) {
        if (useQueue) {
            outOfMemoryQueued(TEXT("GRP"), 1);
        } else {
            outOfMemory(TEXT("GRP"), 1);
        }
        return NULL;
    }
    if (!GetFullPathName(path, len, realPath, NULL)) {
        if (errorLevel != LEVEL_NONE) {
            log_printf_queue(useQueue, WRAPPER_SOURCE_WRAPPER, errorLevel, TEXT("Unable to resolve the %s '%s': %s"), pathDesc, path, getLastErrorText());
        }
        free(realPath);
        return NULL;
    }
#else
    /* The solaris implementation of realpath will return a relative path if a relative
     *  path is provided.  We always need an absolute path here.  So build up one and
     *  then use realpath to remove any .. or other relative references. */
    tempPath = malloc(sizeof(TCHAR) * (PATH_MAX + 1));
    if (!tempPath) {
        if (useQueue) {
            outOfMemoryQueued(TEXT("GRP"), 2);
        } else {
            outOfMemory(TEXT("GRP"), 2);
        }
        return NULL;
    }
    if (_trealpathN(path, tempPath, PATH_MAX + 1) == NULL) {
        if (errorLevel != LEVEL_NONE) {
            log_printf_queue(useQueue, WRAPPER_SOURCE_WRAPPER, errorLevel, TEXT("Unable to resolve the %s '%s': %s"), pathDesc, path, getLastErrorText());
        }
        free(tempPath);
        return NULL;
    }
    
    /* Now that we know how big the resulting string is, put it into a buffer of the correct size to avoid waste. */
    len = _tcslen(tempPath) + 1;
    realPath = malloc(sizeof(TCHAR) * len);
    if (!realPath) {
        if (useQueue) {
            outOfMemoryQueued(TEXT("GRP"), 3);
        } else {
            outOfMemory(TEXT("GRP"), 3);
        }
        free(tempPath);
        return NULL;
    }
    _tcsncpy(realPath, tempPath, len);
    free(tempPath);
#endif
    
    return realPath;
}

/**
 * Returns the absolute path of a file even if the file is not yet created.
 *  The folder containing the file must exist.
 *
 * The returned string must be freed by the caller.
 *
 * @param path The source path.
 * @param pathDesc A description of the path used for error messages.
 * @param errorLevel Level to log errors at.
 *
 * @return The absolute path, or NULL if there were any problems.
 */
TCHAR* getAbsolutePathOfFile(const TCHAR* path, const TCHAR *pathDesc, int errorLevel, int useQueue) {
    TCHAR* ptr;
    TCHAR* dir;
    const TCHAR* file;
    TCHAR* result = NULL;
    TCHAR* pathCpy;
    
    pathCpy = malloc(sizeof(TCHAR) * (_tcslen(path) + 1));
    if (!pathCpy) {
        outOfMemoryQueued(TEXT("GAPOF"), 1);
    } else {
        _tcsncpy(pathCpy, path, _tcslen(path) + 1);
        ptr = __max(_tcsrchr(pathCpy, TEXT('\\')), _tcsrchr(pathCpy, TEXT('/')));
        if (ptr) {
            *ptr = 0;
            ptr++;
            dir = getRealPath(pathCpy, pathDesc, errorLevel, useQueue);
            file = ptr;
        } else {
            dir = getRealPath(TEXT("."), pathDesc, errorLevel, useQueue);
            file = pathCpy;
        }
        
        if (dir) {
            result = combinePath(dir, file);
            free(dir);
        }
        free(pathCpy);
    }
    
    return result;
}

#ifdef LOGGER_FILE_DEBUG
void loggerFileTests() {
    TCHAR** files;

    printf("Start loggerFileTests\n");
    files = loggerFileGetFiles((TEXT("../logs/*.log*"), LOGGER_FILE_SORT_MODE_TIMES);
    if (files) {
        loggerFileFreeFiles(files);
    }

    files = loggerFileGetFiles(TEXT("../logs/*.log*"), LOGGER_FILE_SORT_MODE_NAMES_ASC);
    if (files) {
        loggerFileFreeFiles(files);
    }

    files = loggerFileGetFiles(TEXT("../logs/*.log*"), LOGGER_FILE_SORT_MODE_NAMES_DEC);
    if (files) {
        loggerFileFreeFiles(files);
    }
    printf("End loggerFileTests\n");
}
#endif
