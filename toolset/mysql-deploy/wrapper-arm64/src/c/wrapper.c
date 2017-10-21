/*
 * Copyright (c) 1999, 2017 Tanuki Software, Ltd.
 * http://www.tanukisoftware.com
 * All rights reserved.
 *
 * This software is the proprietary information of Tanuki Software.
 * You shall use it only in accordance with the terms of the
 * license agreement you entered into with Tanuki Software.
 * http://wrapper.tanukisoftware.com/doc/english/licenseOverview.html
 *
 *
 * Portions of the Software have been derived from source code
 * developed by Silver Egg Technology under the following license:
 *
 * Copyright (c) 2001 Silver Egg Technology
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sub-license, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 */

/**
 * Author:
 *   Leif Mortenson <leif@tanukisoftware.com>
 *   Ryan Shaw
 */

#ifdef WIN32
 /* need the 2 following includes to use IPv6 and need wsock32.lib in the makefile */
 #include <winsock2.h>
 #include <Ws2tcpip.h>
#endif

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <assert.h>

#ifdef CUNIT
#include "Basic.h"
#endif
#include "wrapper_i18n.h"
#include "wrapperinfo.h"
#include "wrapper.h"
#include "logger.h"
#include "logger_file.h"
#include "wrapper_file.h"
#ifndef WIN32
 #include "wrapper_ulimit.h"
#endif

#ifdef WIN32
 #include <direct.h>
 #include <winsock.h>
 #include <shlwapi.h>
 #include <windows.h>
 #include <io.h>

/* MS Visual Studio 8 went and deprecated the POXIX names for functions.
 *  Fixing them all would be a big headache for UNIX versions. */
 #pragma warning(disable : 4996)

/* Defines for MS Visual Studio 6 */
 #ifndef _INTPTR_T_DEFINED
  typedef long intptr_t;
  #define _INTPTR_T_DEFINED
 #endif

#else /* UNIX */
 #include <ctype.h>
 #include <string.h>
 #include <sys/wait.h>
 #include <fcntl.h>
 #include <limits.h>
 #include <signal.h>
 #include <pthread.h>
 #include <sys/socket.h>
 #include <sys/time.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <sys/resource.h>
 #define SOCKET         int
 #define HANDLE         int
 #define INVALID_HANDLE_VALUE -1
 #define INVALID_SOCKET -1
 #define SOCKET_ERROR   -1

 #if defined(SOLARIS)
  #include <sys/errno.h>
  #include <sys/fcntl.h>
 #elif defined(AIX) || defined(HPUX) || defined(MACOSX) || defined(OSF1)
 #elif defined(IRIX)
  #define PATH_MAX FILENAME_MAX
 #elif defined(FREEBSD)
  #include <sys/param.h>
  #include <errno.h>
 #else /* LINUX */
  #include <asm/errno.h>
 #endif

#endif /* WIN32 */

/* Define some common defines to make cross platform code a bit cleaner. */
#ifdef WIN32
 #define WRAPPER_EADDRINUSE  WSAEADDRINUSE
 #define WRAPPER_EWOULDBLOCK WSAEWOULDBLOCK
 #define WRAPPER_EACCES      WSAEACCES
#else
 #define WRAPPER_EADDRINUSE  EADDRINUSE
 #define WRAPPER_EWOULDBLOCK EWOULDBLOCK
 #define WRAPPER_EACCES      EACCES
#endif

WrapperConfig *wrapperData;
char          packetBufferMB[MAX_LOG_SIZE + 1];
TCHAR         packetBufferW[MAX_LOG_SIZE + 1];
TCHAR         *keyChars = TEXT("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-");

/* Properties structure loaded in from the configuration file. */
Properties              *properties;

/* Mutex for synchronization of the tick timer. */
#ifdef WIN32
HANDLE tickMutexHandle = NULL;
#else
pthread_mutex_t tickMutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/* Server Pipe Handles. */
HANDLE protocolActiveServerPipeIn = INVALID_HANDLE_VALUE;
HANDLE protocolActiveServerPipeOut = INVALID_HANDLE_VALUE;
/* Flag for indicating the connected pipes */
int protocolActiveServerPipeConnected = FALSE;

/* Server Socket. */
SOCKET protocolActiveServerSD = INVALID_SOCKET;
/* Client Socket. */
SOCKET protocolActiveBackendSD = INVALID_SOCKET;

#ifndef IN6ADDR_LOOPBACK_INIT
 /* even if I include ws2ipdef.h, it doesn't define IN6ADDR_LOOPBACK_INIT,
    so that's why I define it here */
 #define IN6ADDR_LOOPBACK_INIT { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 }
#endif
#define LOOPBACK_IPv4  "127.0.0.1"
#ifdef HPUX_IA
 /* on HPUX ia, gcc reports a warning "missing braces around initializer" 
    when using IN6ADDR_LOOPBACK_INIT (/usr/include/netinet/in6.h:241). 
    So I add braces for the struct and braces for the union. */
struct in6_addr LOOPBACK_IPv6 = {{IN6ADDR_LOOPBACK_INIT}};
#else
struct in6_addr LOOPBACK_IPv6 = IN6ADDR_LOOPBACK_INIT;  
#endif

int disposed = FALSE;
int loadConfiguration();

#define READ_BUFFER_BLOCK_SIZE 1024
static char *wrapperChildWorkBuffer = NULL;
static size_t wrapperChildWorkBufferSize = 0;
static size_t wrapperChildWorkBufferLen = 0;
static time_t wrapperChildWorkLastDataTime = 0;
static int wrapperChildWorkLastDataTimeMillis = 0;
static int wrapperChildWorkIsNewLine = TRUE;

/**
 * Constructs a tm structure from a pair of Strings like "20091116" and "1514".
 *  The time returned will be in the local time zone.  This is not 100% accurate
 *  as it doesn't take into account the time zone in which the dates were
 *  originally set.
 */
struct tm getInfoTime(const TCHAR *date, const TCHAR *time) {
    struct tm buildTM;
    TCHAR temp[5];

    memset(&buildTM, 0, sizeof(struct tm));

    /* Year */
    _tcsncpy( temp, date, 4 );
    temp[4] = 0;
    buildTM.tm_year = _ttoi( temp ) - 1900;

    /* Month */
    _tcsncpy( temp, date + 4, 2 );
    temp[2] = 0;
    buildTM.tm_mon = _ttoi( temp ) - 1;

    /* Day */
    _tcsncpy( temp, date + 6, 2 );
    temp[2] = 0;
    buildTM.tm_mday = _ttoi( temp );

    /* Hour */
    _tcsncpy( temp, time, 2 );
    temp[2] = 0;
    buildTM.tm_hour = _ttoi( temp );

    /* Minute */
    _tcsncpy( temp, time + 2, 2 );
    temp[2] = 0;
    buildTM.tm_min = _ttoi( temp );

    return buildTM;
}

struct tm wrapperGetReleaseTime() {
    return getInfoTime(wrapperReleaseDate, wrapperReleaseTime);
}

struct tm wrapperGetBuildTime() {
    return getInfoTime(wrapperBuildDate, wrapperBuildTime);
}

/**
 * Adds default properties used to set global environment variables.
 *
 * These are done by setting properties rather than call setEnv directly
 *  so that it will be impossible for users to override their values by
 *  creating a "set.XXX=NNN" property in the configuration file.
 */
void wrapperAddDefaultProperties() {
    size_t bufferLen;
    TCHAR* buffer, *confDirTemp;
#ifdef WIN32
    int work, pos2;
    TCHAR pathSep = TEXT('\\');
#else
    TCHAR pathSep = TEXT('/');
#endif
    int pos;

    /* IMPORTANT - If any new values are added here, this work buffer length may need to be calculated differently. */
    bufferLen = 1;
    bufferLen = __max(bufferLen, _tcslen(TEXT("set.WRAPPER_LANG=")) + 3 + 1);
    bufferLen = __max(bufferLen, _tcslen(TEXT("set.WRAPPER_PID=")) + 10 + 1); /* 32-bit PID would be max of 10 characters */
    bufferLen = __max(bufferLen, _tcslen(TEXT("set.WRAPPER_BITS=")) + _tcslen(wrapperBits) + 1);
    bufferLen = __max(bufferLen, _tcslen(TEXT("set.WRAPPER_ARCH=")) + _tcslen(wrapperArch) + 1);
    bufferLen = __max(bufferLen, _tcslen(TEXT("set.WRAPPER_OS=")) + _tcslen(wrapperOS) + 1);
    bufferLen = __max(bufferLen, _tcslen(TEXT("set.WRAPPER_HOSTNAME=")) + _tcslen(wrapperData->hostName) + 1);
    bufferLen = __max(bufferLen, _tcslen(TEXT("set.WRAPPER_HOST_NAME=")) + _tcslen(wrapperData->hostName) + 1);

    if (wrapperData->confDir == NULL) {
        if (_tcsrchr(wrapperData->argConfFile, pathSep) != NULL) {
            pos = (int)(_tcsrchr(wrapperData->argConfFile, pathSep) - wrapperData->argConfFile);
        } else {
            pos = -1;
        }
#ifdef WIN32
        if (_tcsrchr(wrapperData->argConfFile, TEXT('/')) != NULL) {
            pos2 = (int)(_tcsrchr(wrapperData->argConfFile, TEXT('/')) - wrapperData->argConfFile);
        } else {
            pos2 = -1;
        }
        pos = __max(pos, pos2);
#endif
        if (pos == -1) {
            confDirTemp = malloc(sizeof(TCHAR) * 2);
            if (!confDirTemp) {
                outOfMemory(TEXT("WADP"), 1);
                return;
            }
            _tcsncpy(confDirTemp, TEXT("."), 2);
        } else if (pos == 0) {
            confDirTemp = malloc(sizeof(TCHAR) * 2);
            if (!confDirTemp) {
                outOfMemory(TEXT("WADP"), 2);
                return;
            }
            _sntprintf(confDirTemp, 2, TEXT("%c"), pathSep);
        } else {
            confDirTemp = malloc(sizeof(TCHAR) * (pos + 1));
            if (!confDirTemp) {
                outOfMemory(TEXT("WADP"), 3);
                return;
            }
            _tcsncpy(confDirTemp, wrapperData->argConfFile, pos);
            confDirTemp[pos] = TEXT('\0');
        }
#ifdef WIN32
        /* Get buffer size, including '\0' */
        work = GetFullPathName(confDirTemp, 0, NULL, NULL);
        if (!work) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                TEXT("Unable to resolve the conf directory: %s"), getLastErrorText());
            free(confDirTemp);
            return;
        }
        wrapperData->confDir = malloc(sizeof(TCHAR) * work);
        if (!wrapperData->confDir) {
            outOfMemory(TEXT("WADP"), 4);
            free(confDirTemp);
            return;
        }
        if (!GetFullPathName(confDirTemp, work, wrapperData->confDir, NULL)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                TEXT("Unable to resolve the conf directory: %s"), getLastErrorText());
            free(confDirTemp);
            return;
        }
#else
        /* The solaris implementation of realpath will return a relative path if a relative
         *  path is provided.  We always need an absolute path here.  So build up one and
         *  then use realpath to remove any .. or other relative references. */
        wrapperData->confDir = malloc(sizeof(TCHAR) * (PATH_MAX + 1));
        if (!wrapperData->confDir) {
            outOfMemory(TEXT("WADP"), 5);
            free(confDirTemp);
            return;
        }
        if (_trealpathN(confDirTemp, wrapperData->confDir, PATH_MAX + 1) == NULL) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                TEXT("Unable to resolve the original working directory: %s"), getLastErrorText());
            free(confDirTemp);
            return;
        }
#endif
        setEnv(TEXT("WRAPPER_CONF_DIR"), wrapperData->confDir, ENV_SOURCE_APPLICATION);
        free(confDirTemp);
    }

    buffer = malloc(sizeof(TCHAR) * bufferLen);
    if (!buffer) {
        outOfMemory(TEXT("WADP"), 1);
        return;
    }

    _sntprintf(buffer, bufferLen, TEXT("set.WRAPPER_LANG=en"));
    addPropertyPair(properties, NULL, 0, buffer, TRUE, FALSE, TRUE);

    _sntprintf(buffer, bufferLen, TEXT("set.WRAPPER_PID=%d"), wrapperData->wrapperPID);
    addPropertyPair(properties, NULL, 0, buffer, TRUE, FALSE, TRUE);

    _sntprintf(buffer, bufferLen, TEXT("set.WRAPPER_BITS=%s"), wrapperBits);
    addPropertyPair(properties, NULL, 0, buffer, TRUE, FALSE, TRUE);

    _sntprintf(buffer, bufferLen, TEXT("set.WRAPPER_ARCH=%s"), wrapperArch);
    addPropertyPair(properties, NULL, 0, buffer, TRUE, FALSE, TRUE);

    _sntprintf(buffer, bufferLen, TEXT("set.WRAPPER_OS=%s"), wrapperOS);
    addPropertyPair(properties, NULL, 0, buffer, TRUE, FALSE, TRUE);

    _sntprintf(buffer, bufferLen, TEXT("set.WRAPPER_HOSTNAME=%s"), wrapperData->hostName);
    addPropertyPair(properties, NULL, 0, buffer, TRUE, FALSE, TRUE);

    _sntprintf(buffer, bufferLen, TEXT("set.WRAPPER_HOST_NAME=%s"), wrapperData->hostName);
    addPropertyPair(properties, NULL, 0, buffer, TRUE, FALSE, TRUE);

#ifdef WIN32
    addPropertyPair(properties, NULL, 0, TEXT("set.WRAPPER_FILE_SEPARATOR=\\"), TRUE, FALSE, TRUE);
    addPropertyPair(properties, NULL, 0, TEXT("set.WRAPPER_PATH_SEPARATOR=;"), TRUE, FALSE, TRUE);
#else
    addPropertyPair(properties, NULL, 0, TEXT("set.WRAPPER_FILE_SEPARATOR=/"), TRUE, FALSE, TRUE);
    addPropertyPair(properties, NULL, 0, TEXT("set.WRAPPER_PATH_SEPARATOR=:"), TRUE, FALSE, TRUE);
#endif

    free(buffer);
}

/**
 * This function is here to help Community Edition users who are attempting
 *  to generate a hostId.
 */
int showHostIds(int logLevel) {
    log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, TEXT(""));
    log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, TEXT("The Community Edition of the Java Service Wrapper does not implement\nHostIds."));
    log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, TEXT(""));
    log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, TEXT("If you have requested a trial license, or purchased a license, you\nmay be looking for the Standard or Professional Editions of the Java\nService Wrapper.  They can be downloaded here:"));
    log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, TEXT("  http://wrapper.tanukisoftware.com/download"));
    log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, TEXT(""));

    return FALSE;
}


/**
 * Loads the current environment into a table so we can debug it later.
 *
 * @return TRUE if there were problems, FALSE if successful.
 */
int loadEnvironment() {
    size_t len;
    TCHAR *sourcePair;
    TCHAR *pair;
    TCHAR *equal;
    TCHAR *name;
    TCHAR *value;
#ifdef WIN32
    LPTCH lpvEnv;
    LPTSTR lpszVariable;
#else
    /* The compiler won't let us reference environ directly in the for loop on OSX because it is actually a function. */
    char **environment = environ;
    int i;
#endif

#ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("Loading Environment..."));
#endif
#ifdef WIN32
    lpvEnv = GetEnvironmentStrings();
    if (!lpvEnv)
    {
        _tprintf(TEXT("GetEnvironmentStrings failed (%s)\n"), getLastErrorText());
        return TRUE;
    }
    lpszVariable = (LPTSTR)lpvEnv;
    while (lpszVariable[0] != '\0') {
            sourcePair = lpszVariable;
#else
    i = 0;
    while (environment[i]) {
        len = mbstowcs(NULL, environment[i], MBSTOWCS_QUERY_LENGTH);
        if (len == (size_t)-1) {
            /* Invalid string.  Skip. */
        } else {
            sourcePair = malloc(sizeof(TCHAR) * (len + 1));
            if (!sourcePair) {
                outOfMemory(TEXT("LE"), 1);
                _tprintf(TEXT(" Invalid character string: %s (%s)\n"), environment[i], getLastErrorText());
                return TRUE;
            }
            mbstowcs(sourcePair, environment[i], len + 1);
            sourcePair[len] = TEXT('\0'); /* Avoid bufferflows caused by badly encoded characters. */
#endif

            len = _tcslen(sourcePair);

            /* We need a copy of the variable pair so we can split it. */
            pair = malloc(sizeof(TCHAR) * (len + 1));
            if (!pair) {
                outOfMemory(TEXT("LE"), 1);
#ifdef WIN32
                FreeEnvironmentStrings(lpvEnv);
#else
                free(sourcePair);
#endif
                return TRUE;
            }
            _sntprintf(pair, len + 1, TEXT("%s"), sourcePair);

            equal = _tcschr(pair, TEXT('='));
            if (equal) {
                name = pair;
                value = &(equal[1]);
                equal[0] = TEXT('\0');

                if (_tcslen(name) <= 0) {
                    name = NULL;
                }
                if (_tcslen(value) <= 0) {
                    value = NULL;
                }

                /* It is possible that the name was empty. */
                if (name) {
                    setEnv(name, value, ENV_SOURCE_PARENT);
                }
            }

            free(pair);

#ifdef WIN32
            lpszVariable += len + 1;
#else
            free(sourcePair);
        }
        i++;
#endif
    }
#ifdef WIN32
    FreeEnvironmentStrings(lpvEnv);
#endif

#ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("Loading Environment complete."));
#endif
    return FALSE;
}

/**
 * Updates a string value by making a copy of the original.  Any old value is
 *  first freed.
 */
void updateStringValue(TCHAR **ptr, const TCHAR *value) {
    if (*ptr != NULL) {
        free(*ptr);
        *ptr = NULL;
    }

    if (value != NULL) {
        *ptr = malloc(sizeof(TCHAR) * (_tcslen(value) + 1));
        if (!(*ptr)) {
            outOfMemory(TEXT("USV"), 1);
            /* TODO: This is pretty bad.  Not sure how to recover... */
        } else {
            _tcsncpy(*ptr, value, _tcslen(value) + 1);
        }
    }
}

#ifndef WIN32 /* UNIX */
int getSignalMode(const TCHAR *modeName, int defaultMode) {
    if (!modeName) {
        return defaultMode;
    }

    if (strcmpIgnoreCase(modeName, TEXT("IGNORE")) == 0) {
        return WRAPPER_SIGNAL_MODE_IGNORE;
    } else if (strcmpIgnoreCase(modeName, TEXT("RESTART")) == 0) {
        return WRAPPER_SIGNAL_MODE_RESTART;
    } else if (strcmpIgnoreCase(modeName, TEXT("SHUTDOWN")) == 0) {
        return WRAPPER_SIGNAL_MODE_SHUTDOWN;
    } else if (strcmpIgnoreCase(modeName, TEXT("FORWARD")) == 0) {
        return WRAPPER_SIGNAL_MODE_FORWARD;
    } else if (strcmpIgnoreCase(modeName, TEXT("PAUSE")) == 0) {
        return WRAPPER_SIGNAL_MODE_PAUSE;
    } else if (strcmpIgnoreCase(modeName, TEXT("RESUME")) == 0) {
        return WRAPPER_SIGNAL_MODE_RESUME;
    } else if (strcmpIgnoreCase(modeName, TEXT("CLOSE_LOGFILE")) == 0) {
        return WRAPPER_SIGNAL_MODE_CLOSE_LOGFILE;
    } else {
        return defaultMode;
    }
}

/**
 * Return FALSE if successful, TRUE if there were problems.
 */
int wrapperBuildUnixDaemonInfo() {
    if (!wrapperData->configured) {
        /** Get the daemonize flag. */
        wrapperData->daemonize = getBooleanProperty(properties, TEXT("wrapper.daemonize"), FALSE);
        /** Configure the HUP signal handler. */
        wrapperData->signalHUPMode = getSignalMode(getStringProperty(properties, TEXT("wrapper.signal.mode.hup"), NULL), WRAPPER_SIGNAL_MODE_FORWARD);

        /** Configure the USR1 signal handler. */
        wrapperData->signalUSR1Mode = getSignalMode(getStringProperty(properties, TEXT("wrapper.signal.mode.usr1"), NULL), WRAPPER_SIGNAL_MODE_FORWARD);

        /** Configure the USR2 signal handler. */
        wrapperData->signalUSR2Mode = getSignalMode(getStringProperty(properties, TEXT("wrapper.signal.mode.usr2"), NULL), WRAPPER_SIGNAL_MODE_FORWARD);
    }

    return FALSE;
}
#endif


/**
 * Dumps the table of environment variables, and their sources.
 *
 * @param logLevel Level at which to log the output.
 */
void dumpEnvironment(int logLevel) {
    EnvSrc *envSrc;
    TCHAR *envVal;

    log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, TEXT(""));
    log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, TEXT("Environment variables (Source | Name=Value) BEGIN:"));

    envSrc = baseEnvSrc;
    while (envSrc) {
        envVal = _tgetenv(envSrc->name);

        log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, TEXT("  %c%c%c%c%c | %s=%s"),
            (envSrc->source & ENV_SOURCE_PARENT ? TEXT('P') : TEXT('-')),
#ifdef WIN32
            (envSrc->source & ENV_SOURCE_REG_SYSTEM ? TEXT('S') : TEXT('-')),
            (envSrc->source & ENV_SOURCE_REG_ACCOUNT ? TEXT('A') : TEXT('-')),
#else
            TEXT('-'),
            TEXT('-'),
#endif
            (envSrc->source & ENV_SOURCE_APPLICATION ? TEXT('W') : TEXT('-')),
            (envSrc->source & ENV_SOURCE_CONFIG ? TEXT('C') : TEXT('-')),
            envSrc->name,
            (envVal ? envVal : TEXT("<null>"))
        );

#if !defined(WIN32) && defined(UNICODE)
        if (envVal) {
            free(envVal);
        }
#endif

        envSrc = envSrc->next;
    }
    log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, TEXT("Environment variables END:"));
    log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, TEXT(""));
}

#ifdef WIN32
/**
 * Check if the Wrapper is running under cygwin terminal.
 * I'm looking for the environment variable TERM to be equal to "xterm".
 * I tried with OSTYPE and MACHTYPE, but _tgetenv always returns NULL.
 * @return TRUE if under cygwin, otherwise returns FALSE
 */
int isCygwin() {
    TCHAR *osType;
    int retVal = FALSE;
    
    osType = _tgetenv(TEXT("TERM"));
    
    if ((osType != NULL) && (_tcscmp(osType, TEXT("xterm")) == 0)) {
        retVal = TRUE;
    } 

    return retVal;
}
#endif

void wrapperLoadLoggingProperties(int preload) {
    const TCHAR *logfilePath;
    int logfileRollMode;
    int underCygwin = FALSE;
    int defaultFlushTimeOut = 1;
#ifdef WIN32
    int silent;
#endif
    
    setLoggingIsPreload(preload);
    
    setLogPropertyWarnings(properties, !preload);
    
    setLogPropertyWarningLogLevel(properties, getLogLevelForName(getStringProperty(properties, TEXT("wrapper.property_warning.loglevel"), TEXT("WARN"))));
    
    setPropertiesDumpLogLevel(properties, getLogLevelForName(getStringProperty(properties, TEXT("wrapper.properties.dump.loglevel"), TEXT("DEBUG"))));

    setLogWarningThreshold(getIntProperty(properties, TEXT("wrapper.log.warning.threshold"), 0));
    wrapperData->logLFDelayThreshold = propIntMax(propIntMin(getIntProperty(properties, TEXT("wrapper.log.lf_delay.threshold"), 500), 3600000), 0);

    if (resolveDefaultLogFilePath()) {
        /* The error has already been logged. This is not fatal, we will continue with the relative path. */
    }
    
    logfilePath = getFileSafeStringProperty(properties, TEXT("wrapper.logfile"), TEXT("wrapper.log"));
    setLogfilePath(logfilePath, TRUE);

    logfileRollMode = getLogfileRollModeForName(getStringProperty(properties, TEXT("wrapper.logfile.rollmode"), TEXT("SIZE")));
    if (logfileRollMode == ROLL_MODE_UNKNOWN) {
        if (!preload) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            TEXT("wrapper.logfile.rollmode invalid.  Disabling log file rolling."));
        }
        logfileRollMode = ROLL_MODE_NONE;
    } else if (logfileRollMode == ROLL_MODE_DATE) {
        if (!_tcsstr(logfilePath, ROLL_MODE_DATE_TOKEN)) {
            if (!preload) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                    TEXT("wrapper.logfile must contain \"%s\" for a roll mode of DATE.  Disabling log file rolling."),
                    ROLL_MODE_DATE_TOKEN);
            }
            logfileRollMode = ROLL_MODE_NONE;
        }
    }
    setLogfileRollMode(logfileRollMode);

    /* Load log file format */
    setLogfileFormat(getStringProperty(properties, TEXT("wrapper.logfile.format"), LOG_FORMAT_LOGFILE_DEFAULT));

    /* Load log file log level */
    setLogfileLevel(getStringProperty(properties, TEXT("wrapper.logfile.loglevel"), TEXT("INFO")));

    /* Load max log filesize log level */
    setLogfileMaxFileSize(getStringProperty(properties, TEXT("wrapper.logfile.maxsize"), TEXT("0")));

    /* Load log files level */
    setLogfileMaxLogFiles(getIntProperty(properties, TEXT("wrapper.logfile.maxfiles"), 0));

    /* Load log file purge pattern */
    setLogfilePurgePattern(getFileSafeStringProperty(properties, TEXT("wrapper.logfile.purge.pattern"), TEXT("")));

    /* Load log file purge sort */
    setLogfilePurgeSortMode(loggerFileGetSortMode(getStringProperty(properties, TEXT("wrapper.logfile.purge.sort"), TEXT("TIMES"))));

    /* Get the close timeout. */
    wrapperData->logfileCloseTimeout = propIntMax(propIntMin(getIntProperty(properties, TEXT("wrapper.logfile.close.timeout"), getIntProperty(properties, TEXT("wrapper.logfile.inactivity.timeout"), 1)), 3600), -1);
    setLogfileAutoClose(wrapperData->logfileCloseTimeout == 0);
    
    /* Get the flush timeout. */
    wrapperData->logfileFlushTimeout = propIntMax(propIntMin(getIntProperty(properties, TEXT("wrapper.logfile.flush.timeout"), defaultFlushTimeOut), 3600), 0);
    setLogfileAutoFlush(wrapperData->logfileFlushTimeout == 0);

    /* Load console format */
    setConsoleLogFormat(getStringProperty(properties, TEXT("wrapper.console.format"), LOG_FORMAT_CONSOLE_DEFAULT));

    setConsoleLogLevel(getStringProperty(properties, TEXT("wrapper.console.loglevel"), TEXT("INFO")));

#ifdef WIN32
    /* check if the current instance of the Wrapper is running under Cygwin */
    underCygwin = isCygwin();
#endif

    /* Load the console flush flag. */
    setConsoleFlush(getBooleanProperty(properties, TEXT("wrapper.console.flush"), FALSE || underCygwin));

#ifdef WIN32
    /* Load the console direct flag. */
    setConsoleDirect(getBooleanProperty(properties, TEXT("wrapper.console.direct"), TRUE));
#endif

    /* Load the console loglevel targets. */
    setConsoleFatalToStdErr(getBooleanProperty(properties, TEXT("wrapper.console.fatal_to_stderr"), TRUE));
    setConsoleErrorToStdErr(getBooleanProperty(properties, TEXT("wrapper.console.error_to_stderr"), TRUE));
    setConsoleWarnToStdErr(getBooleanProperty(properties, TEXT("wrapper.console.warn_to_stderr"), FALSE));


    /* Load syslog log level */
    setSyslogLevel(getStringProperty(properties, TEXT("wrapper.syslog.loglevel"), TEXT("NONE")));
    
    /* Load syslog split messages flag. */
    setSyslogSplitMessages(getBooleanProperty(properties, TEXT("wrapper.syslog.split_messages"), FALSE));

#ifndef WIN32
    /* Load syslog facility */
    setSyslogFacility(getStringProperty(properties, TEXT("wrapper.syslog.facility"), TEXT("USER")));
#endif

    /* Load syslog event source name */
    setSyslogEventSourceName(getStringProperty(properties, TEXT("wrapper.syslog.ident"), getStringProperty(properties, TEXT("wrapper.name"), getStringProperty(properties, TEXT("wrapper.ntservice.name"), TEXT("wrapper")))));
#ifdef WIN32
    setSyslogRegister(getBooleanProperty(properties, TEXT("wrapper.syslog.ident.enable"), TRUE));
#endif


#ifdef WIN32
    /* Register or unregister an event source depending on the value of wrapper.syslog.ident.enable.
     *  The syslog will be disabled if the application is not registered after calling the functions
     *  to register or unregister, so this has to be done on preload. This has to be done again each
     *  time the configuration is reloaded because wrapper.syslog.ident.enable may potentially change. */
    /* Make sure we are not running in setup, teardown or install mode. 
     *  - Setup is automatically executed when installing a service - no need to do it here.
     *  - TearDown is not executed when removing the service as this would remove the source 
     *    of existing messages in the event log. */
    if (strcmpIgnoreCase(wrapperData->argCommand, TEXT("su")) && strcmpIgnoreCase(wrapperData->argCommand, TEXT("-setup")) &&
        strcmpIgnoreCase(wrapperData->argCommand, TEXT("td")) && strcmpIgnoreCase(wrapperData->argCommand, TEXT("-teardown")) &&
        strcmpIgnoreCase(wrapperData->argCommand, TEXT("i"))  && strcmpIgnoreCase(wrapperData->argCommand, TEXT("-install")) &&
        strcmpIgnoreCase(wrapperData->argCommand, TEXT("it")) && strcmpIgnoreCase(wrapperData->argCommand, TEXT("-installstart"))) {
        /* The functions below need to be called even if we don't have the permission to edit the registry.
         *  They will eventually disable event logging if the application is not registered.
         *  On preload use the silent mode to avoid double log outputs. */
        silent = preload || !strcmpIgnoreCase(wrapperData->argCommand, TEXT("r")) || !strcmpIgnoreCase(wrapperData->argCommand, TEXT("-remove"))
                         || !strcmpIgnoreCase(wrapperData->argCommand, TEXT("p")) || !strcmpIgnoreCase(wrapperData->argCommand, TEXT("-stop"));
        if (getSyslogRegister()) {
            /* Register the syslog message */
            registerSyslogMessageFile(FALSE, silent);
        } else {
            /* Unregister the syslog message */
            unregisterSyslogMessageFile(silent);
        }
    }
#endif

    /* Get the debug status (Property is deprecated but flag is still used) */
    wrapperData->isDebugging = getBooleanProperty(properties, TEXT("wrapper.debug"), FALSE);
    if (wrapperData->isDebugging) {
        /* For backwards compatability */
        setConsoleLogLevelInt(LEVEL_DEBUG);
        setLogfileLevelInt(LEVEL_DEBUG);
    } else {
        if (getLowLogLevel() <= LEVEL_DEBUG) {
            wrapperData->isDebugging = TRUE;
        }
    }
}


/**
 * This function provides a log file after proloading the properties.
 *  It will load all configurations related to the logging (loglevel, format, etc.).
 *  For standard editions, it helps us to resolve the language, if specified in the conf file.
 *
 * @param logLevelOnOverwriteProperties : use this parameter to keep the value of the last #properties.on_overwrite.loglevel found during the preload phase
 * @param exitOnOverwriteProperties     : use this parameter to keep the value of the last #properties.on_overwrite.exit found during the preload phase
 *
 * @return TRUE if something failed.
 */
int wrapperPreLoadConfigurationProperties(int *logLevelOnOverwriteProperties, int *exitOnOverwriteProperties) {
    int returnVal = FALSE;
#ifdef WIN32
    /* For the community edition, always try to display the system errors in English. */
    setLogSysLangId((SUBLANG_ENGLISH_US << 10) + LANG_ENGLISH);
#endif
    
    /* Load log file */
    wrapperLoadLoggingProperties(TRUE);

    /* As soon as the logging is loaded see if we are in a translate call.  If so we need to reset the log levels to silent mode. */
    if (!strcmpIgnoreCase(wrapperData->argCommand, TEXT("-translate"))) {
        setSilentLogLevels();
    }
    
    wrapperAddDefaultProperties();
    if (wrapperData->workingDir && wrapperData->originalWorkingDir) {
        if (wrapperSetWorkingDir(wrapperData->originalWorkingDir, FALSE)) {
            /* Failed to restore the working dir.  Shutdown the Wrapper */
            returnVal = TRUE;
        }
    }
    
    if (!returnVal) {
        *logLevelOnOverwriteProperties = properties->logLevelOnOverwrite;
        *exitOnOverwriteProperties = properties->exitOnOverwrite;
    }

    if (properties) {
        disposeProperties(properties);
        properties = NULL;
    }
    return returnVal;
}

/**
 * Load the configuration.
 *
 * @param preload TRUE if the configuration is being preloaded.
 *
 * Return TRUE if there were any problems.
 */
int wrapperLoadConfigurationProperties(int preload) {
    static int logLevelOnOverwriteProperties = LEVEL_NONE;
    static int exitOnOverwriteProperties = FALSE;
    int i;
    int firstCall;
#ifdef WIN32
    int work;
    int defaultUMask;
#else 
    mode_t defaultUMask;
#endif
    const TCHAR* prop;

    /* Unless this is the first call, we need to dispose the previous properties object. */
    if (properties) {
        firstCall = FALSE;
        disposeProperties(properties);
        properties = NULL;
    } else {
        firstCall = TRUE;
        if (wrapperData->originalWorkingDir) {
            free(wrapperData->originalWorkingDir);
        }
        /* This is the first time, so preserve the working directory. */
#ifdef WIN32
        /* Get buffer size, including '\0' */
        work = GetFullPathName(TEXT("."), 0, NULL, NULL);
        if (!work) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                TEXT("Unable to resolve the original working directory: %s"), getLastErrorText());
            return TRUE;
        }
        wrapperData->originalWorkingDir = malloc(sizeof(TCHAR) * work);
        if (!wrapperData->originalWorkingDir) {
            outOfMemory(TEXT("WLCP"), 3);
            return TRUE;
        }
        if (!GetFullPathName(TEXT("."), work, wrapperData->originalWorkingDir, NULL)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                TEXT("Unable to resolve the original working directory: %s"), getLastErrorText());
            return TRUE;
        }
#else
        /* The solaris implementation of realpath will return a relative path if a relative
         *  path is provided.  We always need an absolute path here.  So build up one and
         *  then use realpath to remove any .. or other relative references. */
        wrapperData->originalWorkingDir = malloc(sizeof(TCHAR) * (PATH_MAX + 1));
        if (!wrapperData->originalWorkingDir) {
            outOfMemory(TEXT("WLCP"), 4);
            return TRUE;
        }
        if (_trealpathN(TEXT("."), wrapperData->originalWorkingDir, PATH_MAX + 1) == NULL) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                TEXT("Unable to resolve the original working directory: %s"), getLastErrorText());
            return TRUE;
        }
#endif
        if (wrapperData->configFile) {
            free(wrapperData->configFile);
        }
        /* This is the first time, so preserve the full canonical location of the
         *  configuration file. */
#ifdef WIN32
        work = GetFullPathName(wrapperData->argConfFile, 0, NULL, NULL);
        if (!work) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                TEXT("Unable to resolve the full path of the configuration file, %s: %s"),
                wrapperData->argConfFile, getLastErrorText());
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                TEXT("Current working directory is: %s"), wrapperData->originalWorkingDir);
            return TRUE;
        }
        wrapperData->configFile = malloc(sizeof(TCHAR) * work);
        if (!wrapperData->configFile) {
            outOfMemory(TEXT("WLCP"), 1);
            return TRUE;
        }
        if (!GetFullPathName(wrapperData->argConfFile, work, wrapperData->configFile, NULL)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                TEXT("Unable to resolve the full path of the configuration file, %s: %s"),
                wrapperData->argConfFile, getLastErrorText());
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT(
                "Current working directory is: %s"), wrapperData->originalWorkingDir);
            return TRUE;
        }
#else
        /* The solaris implementation of realpath will return a relative path if a relative
         *  path is provided.  We always need an absolute path here.  So build up one and
         *  then use realpath to remove any .. or other relative references. */
        wrapperData->configFile = malloc(sizeof(TCHAR) * (PATH_MAX + 1));
        if (!wrapperData->configFile) {
            outOfMemory(TEXT("WLCP"), 2);
            return TRUE;
        }
        if (_trealpathN(wrapperData->argConfFile, wrapperData->configFile, PATH_MAX + 1) == NULL) {
            /* Most likely the file does not exist.  The wrapperData->configFile has the first
             *  file that could not be found.  May not be the config file directly if symbolic
             *  links are involved. */
            if (wrapperData->argConfFileDefault) {
                /* The output buffer is likely to contain undefined data.
                 * To be on the safe side and in order to report the error
                 *  below correctly we need to override the data first.*/
                _sntprintf(wrapperData->configFile, PATH_MAX + 1, TEXT("%s"), wrapperData->argConfFile);
                /* This was the default config file name.  We know that the working directory
                 *  could be resolved so the problem must be that the default config file does
                 *  not exist.  This problem will be reported later and the wrapperData->configFile
                 *  variable will have the correct full path.
                 * Fall through for now and the user will get a better error later. */
            } else {
                if (!preload) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT(
                        "Unable to open configuration file: %s (%s)\n  Current working directory: %s"),
                        wrapperData->argConfFile, getLastErrorText(), wrapperData->originalWorkingDir);
                }
                return TRUE;
            }
        }
#endif
    }

    /* Create a Properties structure. */
    properties = createProperties(!preload && firstCall, logLevelOnOverwriteProperties, exitOnOverwriteProperties);
    logLevelOnOverwriteProperties = LEVEL_NONE;
    exitOnOverwriteProperties = FALSE;
    if (!properties) {
        return TRUE;
    }
    
    setLogPropertyWarnings(properties, !preload);
    
    /* Not sure we need to call again wrapperAddDefaultProperties() here. The function was already called on preload.
    Use properties->logLevelOnOverwrite to see the concerned properties */
    wrapperAddDefaultProperties();


    /* The argument prior to the argBase will be the configuration file, followed
     *  by 0 or more command line properties.  The command line properties need to be
     *  loaded first, followed by the configuration file. */
    if (strcmpIgnoreCase(wrapperData->argCommand, TEXT("-translate")) != 0) {
        for (i = 0; i < wrapperData->argCount; i++) {
            if (addPropertyPair(properties, NULL, 0, wrapperData->argValues[i], TRUE, TRUE, FALSE)) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                    TEXT("The argument '%s' is not a valid property name-value pair."),
                    wrapperData->argValues[i]);
                return TRUE;
            }
        }
    }

    /* Now load the configuration file.
     *  When this happens, the working directory MUST be set to the original working dir. */
#ifdef WIN32
    if (loadProperties(properties, wrapperData->configFile, preload, wrapperData->argCommand, wrapperData->originalWorkingDir, wrapperData->isDebugging)) {
#else
    if (loadProperties(properties, wrapperData->configFile, (preload | wrapperData->daemonize), wrapperData->argCommand, wrapperData->originalWorkingDir, wrapperData->isDebugging)) {
#endif
        /* File not found. */
        /* If this was a default file name then we don't want to show this as
         *  an error here.  It will be handled by the caller. */
        /* Debug is not yet available as the config file is not yet loaded. */
        if ((!preload) && (!wrapperData->argConfFileDefault)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Failed to load configuration: %s"), wrapperData->configFile);
        }
        return TRUE;
    }

    /* Config file found. */
    wrapperData->argConfFileFound = TRUE;

    /* The properties have just been loaded. */
    if (preload == TRUE) {
        /* If we are in preload mode, we want to enable log warning messages here so everything below this point has propper warnings. */
        setLogPropertyWarnings(properties, TRUE);
    } else if (properties->overwrittenPropertyCausedExit) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Found duplicated properties."));
        return TRUE; /* will cause the wrapper to exit with error code 1 */
    }

    if (firstCall) {
        /* If the working dir was configured, we need to extract it and preserve its value.
         *  This must be done after the configuration has been completely loaded. */
        prop = getStringProperty(properties, TEXT("wrapper.working.dir"), TEXT("."));
        if (prop && (_tcslen(prop) > 0)) {
            if (wrapperData->workingDir) {
                free(wrapperData->workingDir);
            }
#ifdef WIN32
            work = GetFullPathName(prop, 0, NULL, NULL);
            if (!work) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                    TEXT("Unable to resolve the working directory %s: %s"), prop, getLastErrorText());
                return TRUE;
            }
            wrapperData->workingDir = malloc(sizeof(TCHAR) * work);
            if (!wrapperData->workingDir) {
                outOfMemory(TEXT("WLCP"), 5);
                return TRUE;
            }
            if (!GetFullPathName(prop, work, wrapperData->workingDir, NULL)) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                    TEXT("Unable to resolve the working directory %s: %s"), prop, getLastErrorText());
                return TRUE;
            }
#else
            /* The solaris implementation of realpath will return a relative path if a relative
             *  path is provided.  We always need an absolute path here.  So build up one and
             *  then use realpath to remove any .. or other relative references. */
            wrapperData->workingDir = malloc(sizeof(TCHAR) * (PATH_MAX + 1));
            if (!wrapperData->workingDir) {
                outOfMemory(TEXT("WLCP"), 6);
                return TRUE;
            }
            if (_trealpathN(prop, wrapperData->workingDir, PATH_MAX + 1) == NULL) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                    TEXT("Unable to resolve the working directory %s: %s"), prop, getLastErrorText());
                return TRUE;
            }
#endif
        }
    }

    /* Now that the configuration is loaded, we need to update the working directory if the user specified one.
     *  This must be done now so that anything that references the working directory, including the log file
     *  and language pack locations will work correctly. */
    if (wrapperData->workingDir && wrapperSetWorkingDir(wrapperData->workingDir, !preload)) {
        return TRUE;
    }
    
    /* We are only preloading */
    if (preload == TRUE) {
        /* this affects basically language specific variables */
        if (wrapperData->umask == -1) {
        /** Get the umask value for the various files. */
#ifdef WIN32
            defaultUMask = _umask(0);
            _umask(defaultUMask);
#else
            defaultUMask = umask((mode_t)0);
            umask(defaultUMask);
#endif    
            wrapperData->umask = getIntProperty(properties, TEXT("wrapper.umask"), defaultUMask);
        }
        wrapperData->javaUmask = getIntProperty(properties, TEXT("wrapper.java.umask"), wrapperData->umask);
        wrapperData->pidFileUmask = getIntProperty(properties, TEXT("wrapper.pidfile.umask"), wrapperData->umask);
        wrapperData->lockFileUmask = getIntProperty(properties, TEXT("wrapper.lockfile.umask"), wrapperData->umask);
        wrapperData->javaPidFileUmask = getIntProperty(properties, TEXT("wrapper.java.pidfile.umask"), wrapperData->umask);
        wrapperData->javaIdFileUmask = getIntProperty(properties, TEXT("wrapper.java.idfile.umask"), wrapperData->umask);
        wrapperData->statusFileUmask = getIntProperty(properties, TEXT("wrapper.statusfile.umask"), wrapperData->umask);
        wrapperData->javaStatusFileUmask = getIntProperty(properties, TEXT("wrapper.java.statusfile.umask"), wrapperData->umask);
        wrapperData->anchorFileUmask = getIntProperty(properties, TEXT("wrapper.anchorfile.umask"), wrapperData->umask);
        setLogfileUmask(getIntProperty(properties, TEXT("wrapper.logfile.umask"), wrapperData->umask));
        return wrapperPreLoadConfigurationProperties(&logLevelOnOverwriteProperties, &exitOnOverwriteProperties);
    }
#ifndef WIN32
    /** If in the first call here and the wrapper will daemonize, then we don't need
     * to proceed any further anymore as the properties will be loaded properly at
     * the second time...
     */
    if ((firstCall == TRUE) && (!wrapperBuildUnixDaemonInfo()) && wrapperData->daemonize) {
        return FALSE;
    }
#endif
    /* Load the configuration. */
    if ((strcmpIgnoreCase(wrapperData->argCommand, TEXT("-translate")) != 0) && loadConfiguration()) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
            TEXT("Problem loading the Wrapper configuration file: %s"), wrapperData->configFile);
        return TRUE;
    }
    
    return FALSE;
}

void wrapperGetCurrentTime(struct timeb *timeBuffer) {
#ifdef WIN32
    ftime(timeBuffer);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    timeBuffer->time = (time_t)tv.tv_sec;
    timeBuffer->millitm = (unsigned short)(tv.tv_usec / 1000);
#endif
}

/**
 *  This function stops the pipes (quite in a brutal way)
 */
void protocolStopServerPipe() {
    if (protocolActiveServerPipeIn != INVALID_HANDLE_VALUE) {
#ifdef WIN32
        CloseHandle(protocolActiveServerPipeIn);
#else
        close(protocolActiveServerPipeIn);
#endif
        protocolActiveServerPipeIn = INVALID_HANDLE_VALUE;
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_INFO, TEXT("backend pipe closed."));
    }
    if (protocolActiveServerPipeOut != INVALID_HANDLE_VALUE) {
#ifdef WIN32
        CloseHandle(protocolActiveServerPipeOut);
#else
        close(protocolActiveServerPipeOut);
#endif
        protocolActiveServerPipeOut = INVALID_HANDLE_VALUE;
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_INFO, TEXT("backend pipe closed."));
    }
}

/**
 * There is no difference between closing a socket IPv4 vs a socket IPv6
 */
void protocolStopServerSocket() {
    int rc;

    /* Close the socket. */
    if (protocolActiveServerSD != INVALID_SOCKET) {
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, TEXT("closing backend server."));
        }
#ifdef WIN32
        rc = closesocket(protocolActiveServerSD);
#else /* UNIX */
        rc = close(protocolActiveServerSD);
#endif
        if (rc == SOCKET_ERROR) {
            if (wrapperData->isDebugging) {
                log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, TEXT("server socket close failed. (%d)"), wrapperGetLastError());
            }
        }
        protocolActiveServerSD = INVALID_SOCKET;
    }

    wrapperData->actualPort = 0;
}

void protocolStopServer() {
    if (wrapperData->backendType == WRAPPER_BACKEND_TYPE_PIPE) {
        protocolStopServerPipe();
    } else {
        protocolStopServerSocket();
    }
}

int protocolActiveServerPipeStarted = FALSE;

int protocolStartServerPipe() {
    size_t pipeNameLen;
    TCHAR *pipeName;

#ifdef WIN32
    pipeNameLen = 17 + 10 + 1 + 10 + 3;
#else
    pipeNameLen = 12 + 10 + 1 + 10 + 3;
#endif
    pipeName = malloc(sizeof(TCHAR) * (pipeNameLen + 1));
    if (!pipeName) {
        outOfMemory(TEXT("PSSP"), 1);
        return TRUE;
    }
#ifdef WIN32
    _sntprintf(pipeName, pipeNameLen, TEXT("\\\\.\\pipe\\wrapper-%d-%d-out"), wrapperData->wrapperPID, wrapperData->jvmRestarts + 1);
    if ((protocolActiveServerPipeOut = CreateNamedPipe(pipeName,
                                                    PIPE_ACCESS_OUTBOUND,/* + FILE_FLAG_FIRST_PIPE_INSTANCE, */
                                                    PIPE_TYPE_MESSAGE |       /* message type pipe */
                                                    PIPE_READMODE_MESSAGE |   /* message-read mode */
                                                    PIPE_NOWAIT,              /* nonblocking mode */
                                                    1,  /* only allow 1 connection at a time */
                                                    32768,
                                                    32768,
                                                    0,
                                                    NULL)) == INVALID_HANDLE_VALUE) {
#else
    _sntprintf(pipeName, pipeNameLen, TEXT("/tmp/wrapper-%d-%d-out"), wrapperData->wrapperPID, wrapperData->jvmRestarts + 1);
    if (_tmkfifo(pipeName, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH) == INVALID_HANDLE_VALUE) {

#endif

        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_ERROR, TEXT("Unable to create backend pipe: %s"), getLastErrorText());
        free(pipeName);
        return TRUE;
    }
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, TEXT("server listening on pipe %s."), pipeName);
    }
#ifdef WIN32
    _sntprintf(pipeName, pipeNameLen, TEXT("\\\\.\\pipe\\wrapper-%d-%d-in"), wrapperData->wrapperPID, wrapperData->jvmRestarts + 1);
    if ((protocolActiveServerPipeIn = CreateNamedPipe(pipeName,
                                                    PIPE_ACCESS_INBOUND,/* + FILE_FLAG_FIRST_PIPE_INSTANCE,*/
                                                    PIPE_TYPE_MESSAGE |       /* message type pipe */
                                                    PIPE_READMODE_MESSAGE |   /* message-read mode*/
                                                    PIPE_NOWAIT,              /* nonblocking mode*/
                                                    1,
                                                    32768,
                                                    32768,
                                                    0,
                                                    NULL)) == INVALID_HANDLE_VALUE) {
#else
    _sntprintf(pipeName, pipeNameLen, TEXT("/tmp/wrapper-%d-%d-in"), wrapperData->wrapperPID, wrapperData->jvmRestarts + 1);
    if (_tmkfifo(pipeName, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH) == INVALID_HANDLE_VALUE) {
#endif
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_ERROR, TEXT("Unable to create backend pipe: %s"), getLastErrorText());
        free(pipeName);
        return TRUE;
    }
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, TEXT("server listening on pipe %s."), pipeName);
    }
    protocolActiveServerPipeStarted = TRUE;
    free(pipeName);

    return FALSE;
}

#ifdef WIN32
/**
 * This function doesn't exist on Windows. Similar to inet_ntop that you can find on unix.
 * Convert IPv4 and IPv6 addresses from binary to text form 
 * @param af The family type (AF_INET or AF_INET6)
 * @param src Network address structure
 * @param dst Pointer where to write the text form of the address
 * @return On success, inet_ntop() returns a non-NULL pointer to dst. NULL is returned if there was an error.
 */
const char* inet_ntop(int af, const void* src, char* dst, socklen_t cnt) {
    int result;
    struct sockaddr_in6 addr6;
    struct sockaddr_in  addr4;

    if (af == AF_INET) {
        memset(&addr4, 0, sizeof(struct sockaddr_in));
        memcpy(&(addr4.sin_addr), src, sizeof(addr4.sin_addr));
        addr4.sin_family = af;

        /* here is the function to return a TCHAR. I keep it commented out just as a note */
        /* result = WSAAddressToString((struct sockaddr*) &addr4, sizeof(struct sockaddr_in), 0, dst, (LPDWORD) &cnt); */
        result = getnameinfo((struct sockaddr *)&addr4, sizeof(struct sockaddr_in), dst, cnt, NULL, 0, NI_NUMERICHOST);
    } else {
        memset(&addr6, 0, sizeof(struct sockaddr_in));
        memcpy(&(addr6.sin6_addr), src, sizeof(addr6.sin6_addr));
        addr6.sin6_family = af;

        /* here is the function to return a TCHAR. I keep it commented out just as a note */
        /* result = WSAAddressToString((struct sockaddr*) &addr6, sizeof(struct sockaddr_in6), 0, dst, (LPDWORD) &cnt); */
        result = getnameinfo((struct sockaddr *)&addr6, sizeof(struct sockaddr_in6), dst, cnt, NULL, 0, NI_NUMERICHOST);
    }

    if (result != 0) {
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, TEXT("getnameinfo failed (%d): (%s)"), result, getLastErrorText());
        return NULL;
    }
    return dst;
}

/**
 * This function doesn't exist on Windows. Similar to inet_pton that you can find on unix.
 * convert IPv4 and IPv6 addresses from text to binary form
 * @param af The family type (AF_INET or AF_INET6)
 * @param src The adress in text
 * @param dst Pointer where to write the binary form of the address
 * @return FALSE if no error.
 */
int inet_pton(int af, const char *src, void *dst) {
        struct addrinfo hints;
        struct addrinfo *res, *ressave;
        int result;
        int i = 0;
        struct sockaddr_in6 *sockaddr_ipv6;
        struct sockaddr_in  *sockaddr_ipv4;
        
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = af;

        result = getaddrinfo(src, NULL, &hints, &res);
        if (result != 0) {
            log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, TEXT("getaddrinfo failed (%d): (%s)"), result, getLastErrorText());
            return TRUE;
        }
        
        ressave = res;

        while (res) {
            switch (res->ai_family) {
                case AF_INET:
                    sockaddr_ipv4 = (struct sockaddr_in *) res->ai_addr;
                    memcpy(dst, &sockaddr_ipv4->sin_addr, sizeof(struct  in_addr));
                    result = FALSE;
                    break;
                case AF_INET6:
                     sockaddr_ipv6 = (struct sockaddr_in6 *) res->ai_addr;
                     memcpy(dst, &sockaddr_ipv6->sin6_addr, sizeof(struct  in6_addr));
                     result = FALSE;
                    break;
                default:
                    log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, TEXT("Ignore unsupported family type: %d"), res->ai_family);
                    break;
            }

            res = res->ai_next;
        }

        freeaddrinfo(ressave);
        return result;
}

#endif

/**
 * Start server using a socket
 * @param IPv4 if true then we use IPv4, otherwise we use IPv6
 * @return FALSE if socket is created successfully. For some specific error, returns WRAPPER_BACKEND_ERROR_NEXT, for all the other errors returns TRUE
 */
int protocolStartServerSocket(int IPv4) {
    struct sockaddr_in  addr_srv4;
    struct sockaddr_in6 addr_srv6;
    
    int rc;
    int port;
    int fixedPort;
#ifdef UNICODE
    char* tempAddress;
    size_t len;
#endif

    /*int optVal;*/
#ifdef WIN32
    u_long dwNoBlock = TRUE;
#endif

    /* Create the server socket. */
    if (IPv4) {
        protocolActiveServerSD = socket(AF_INET, SOCK_STREAM, 0);
    } else {
        protocolActiveServerSD = socket(AF_INET6, SOCK_STREAM, 0);
    }
    
    if (protocolActiveServerSD == INVALID_SOCKET) {
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_ERROR,
            TEXT("server socket creation failed. (%s)"), getLastErrorText());
        return WRAPPER_BACKEND_ERROR_NEXT;
    }

    /* Make sure the socket is reused. */
    /* We actually do not want to do this as it makes it possible for more than one Wrapper
     *  instance to bind to the same port.  The second instance succeeds to bind, but any
     *  attempts to connect to that port will go to the first Wrapper.  This would of course
     *  cause attempts to launch the second JVM to fail.
     * Leave this code here as a future development note.
    optVal = 1;
#ifdef WIN32
    if (setsockopt(protocolActiveServerSD, SOL_SOCKET, SO_REUSEADDR, (TCHAR *)&optVal, sizeof(optVal)) < 0) {
#else
    if (setsockopt(protocolActiveServerSD, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal)) < 0) {
#endif
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_ERROR,
            "server socket SO_REUSEADDR failed. (%s)", getLastErrorText());
        wrapperProtocolClose();
        protocolStopServer();
        return;
    }
    */

    /* Make the socket non-blocking */
#ifdef WIN32
    rc = ioctlsocket(protocolActiveServerSD, FIONBIO, &dwNoBlock);
#else /* UNIX  */
    rc = fcntl(protocolActiveServerSD, F_SETFL, O_NONBLOCK);
#endif

    if (rc == SOCKET_ERROR) {
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_ERROR,
            TEXT("server socket ioctlsocket failed. (%s)"), getLastErrorText());
        wrapperProtocolClose();
        protocolStopServer();
        return TRUE;
    }

    /* If a port was specified in the configuration file then we want to
     *  try to use that port or find the next available port.  If 0 was
     *  specified, then we will silently start looking for an available
     *  port starting at 32000. */
    port = wrapperData->port;
    if (port <= 0) {
        port = wrapperData->portMin;
        fixedPort = FALSE;
    } else {
        fixedPort = TRUE;
    }

  tryagain:
    /* Cleanup the socket first */
    if (IPv4) {
        memset(&addr_srv4, 0, sizeof(addr_srv4));
        addr_srv4.sin_family = AF_INET;
        addr_srv4.sin_port = htons((u_short)port);
    } else {
        memset(&addr_srv6, 0, sizeof(addr_srv6));
        addr_srv6.sin6_family = AF_INET6;
        addr_srv6.sin6_flowinfo = 0;
        /* htons switch the 2 bytes. For example:
           32000 in binary: 1111101 00000000
           After swap: 00000000 1111101 which is 125 in decimal */
        addr_srv6.sin6_port = htons((u_short)port);
    }
    
    if (wrapperData->portAddress == NULL) {
        /* the user hasn't defined any address, so we use the loopback address */
        if (IPv4) {
            addr_srv4.sin_addr.s_addr = inet_addr(LOOPBACK_IPv4);
        } else {
            addr_srv6.sin6_addr = LOOPBACK_IPv6;
        }
    } else {
#ifdef UNICODE
#ifdef WIN32
        len = WideCharToMultiByte(CP_OEMCP, 0, wrapperData->portAddress, -1, NULL, 0, NULL, NULL);
        if (len <= 0) {
            log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_WARN,
                TEXT("Invalid multibyte sequence in port address \"%s\" : %s"), wrapperData->portAddress, getLastErrorText());
            return TRUE;
        }
        tempAddress = malloc(len);
        if (!tempAddress) {
            outOfMemory(TEXT("PSSS"), 1);
            return TRUE;
        }
        WideCharToMultiByte(CP_OEMCP, 0, wrapperData->portAddress, -1, tempAddress, (int)len, NULL, NULL);
#else
        len = wcstombs(NULL, wrapperData->portAddress, 0);
        if (len == (size_t)-1) {
            log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_WARN,
                TEXT("Invalid multibyte sequence in port address \"%s\" : %s"), wrapperData->portAddress, getLastErrorText());
            return TRUE;
        }
        tempAddress = malloc(len + 1);
        if (!tempAddress) {
            outOfMemory(TEXT("PSSS"), 2);
            return TRUE;
        }
        wcstombs(tempAddress, wrapperData->portAddress, len + 1);
#endif
        
        /* convert the adress from text to binary form */
        if (IPv4) {
            inet_pton(AF_INET, (const char *)tempAddress, &(addr_srv4.sin_addr));
        } else {
            inet_pton(AF_INET6, (const char *)tempAddress, &(addr_srv6.sin6_addr));
        }

        free(tempAddress);
#else 
        
        if (IPv4) {
            inet_pton(AF_INET, (const char *)wrapperData->portAddress, &(addr_srv4.sin_addr));
        } else {
            inet_pton(AF_INET6, (const char *)wrapperData->portAddress, &(addr_srv6.sin6_addr));
        }

#endif
    }


#ifdef WIN32
    if (IPv4) {
        rc = bind(protocolActiveServerSD, (struct sockaddr FAR *)&addr_srv4, sizeof(addr_srv4));
    } else {
        rc = bind(protocolActiveServerSD, (struct sockaddr FAR *)&addr_srv6, sizeof(addr_srv6));
    }
#else /* UNIX */
    if (IPv4) {
        rc = bind(protocolActiveServerSD, (struct sockaddr *)&addr_srv4, sizeof(addr_srv4));
    } else {
        rc = bind(protocolActiveServerSD, (struct sockaddr *)&addr_srv6, sizeof(addr_srv6));
    }
#endif

    if (rc == SOCKET_ERROR) {
        rc = wrapperGetLastError();

        /* The specified port could not be bound. */
        if ((rc == WRAPPER_EADDRINUSE) || (rc == WRAPPER_EACCES)) {

            /* Address in use, try looking at the next one. */
            if (fixedPort) {
                /* The last port checked was the defined fixed port, switch to the dynamic range. */
                port = wrapperData->portMin;
                fixedPort = FALSE;
                goto tryagain;
            } else {
                port++;
                if (port <= wrapperData->portMax) {
                    goto tryagain;
                }
            }
        }

        /* Log an error.  This is fatal, so die. */
        if (wrapperData->port <= 0) {
            log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_FATAL,
                TEXT("unable to bind listener to any port in the range %d to %d. (%s)"),
                wrapperData->portMin, wrapperData->portMax, getLastErrorText());
        } else {
            log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_FATAL,
                TEXT("unable to bind listener port %d, or any port in the range %d to %d. (%s)"),
                wrapperData->port, wrapperData->portMin, wrapperData->portMax, getLastErrorText());
        }

        wrapperStopProcess(getLastError(), TRUE);
        wrapperProtocolClose();
        protocolStopServer();
        wrapperData->exitRequested = TRUE;
        wrapperData->restartRequested = WRAPPER_RESTART_REQUESTED_NO;
        return TRUE;
    }

    /* If we got here, then we are bound to the port */
    if ((wrapperData->port > 0) && (port != wrapperData->port)) {
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_INFO, TEXT("port %d already in use, using port %d instead."), wrapperData->port, port);
    }
    wrapperData->actualPort = port;
    
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, TEXT("server listening on port %d."), wrapperData->actualPort);
    }

    /* Tell the socket to start listening. */
    rc = listen(protocolActiveServerSD, 1);
    if (rc == SOCKET_ERROR) {
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_ERROR, TEXT("server socket listen failed. (%d)"), wrapperGetLastError());
        wrapperProtocolClose();
        protocolStopServer();
        return TRUE;
    }

    return FALSE;
}

/**
 * if backendType is 'auto', then it will try in this order:
 *   - socket IPv4
 *   - socket IPv6
 *   - pipe
 * if backendType is 'socket', then it will try in this order:
 *   - socket IPv4
 *   - socket IPv6
 */
void protocolStartServer() {
    int useFallbackAuto = FALSE;
    int useFallbackSocket = FALSE;
    int result;
    
    if (wrapperData->backendType == WRAPPER_BACKEND_TYPE_AUTO) {
        useFallbackAuto = TRUE;
    }

    if (wrapperData->backendType == WRAPPER_BACKEND_TYPE_SOCKET) {
        useFallbackSocket = TRUE;
    }

    if (wrapperData->backendType & WRAPPER_BACKEND_TYPE_SOCKET_V4) {
        result = protocolStartServerSocket(TRUE);
        if (result == WRAPPER_BACKEND_ERROR_NEXT && (useFallbackAuto || useFallbackSocket)) {
            /* we failed to use Ipv4, so lets try with IPv6 */
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Failed to start server using socket IPv4, will try with socket IPv6..."));
        } else if (result == FALSE) {
            /* success */
            wrapperData->backendType = WRAPPER_BACKEND_TYPE_SOCKET_V4;
            return;
        } else {
            /* error message should have already be printed in protocolStartServerSocket */
        }
    }

    if (wrapperData->backendType & WRAPPER_BACKEND_TYPE_SOCKET_V6) {
        result = protocolStartServerSocket(FALSE);
        if (result == WRAPPER_BACKEND_ERROR_NEXT && useFallbackAuto) {
            /* we failed to use Ipv6, so lets try with pipe */
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Failed to start server socket IPv6, will try with Pipe..."));
        } else if (result == FALSE) {
            /* success */
            wrapperData->backendType = WRAPPER_BACKEND_TYPE_SOCKET_V6;
            return;
        } else {
            /* error message should have already be printed in protocolStartServerSocket */
        }
    }

    if ( wrapperData->backendType & WRAPPER_BACKEND_TYPE_PIPE) {
        result = protocolStartServerPipe();
        if (result == WRAPPER_BACKEND_ERROR_NEXT && useFallbackAuto) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Failed to start server socket when trying with socket (IPv4 and IPv6) and pipe..."));
        } else if (result == FALSE) {
            /* success */
            wrapperData->backendType = WRAPPER_BACKEND_TYPE_PIPE;
            return;
        } else {
            /* error message should have already be printed in protocolStartServerPipe */
        }
    }

    /* if we reach this code, it means we couldn't create a socket or a pipe */
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("Unable to start server socket."));
    if (!useFallbackAuto) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE, TEXT("You can set wrapper.backend.type=AUTO, so the wrapper will try to connect to the JVM using ipv4, ipv6 and pipe."));

        if (!useFallbackSocket) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE, TEXT("You can set wrapper.backend.type=SOCKET, so the wrapper will try to connect to the JVM using ipv4 and ipv6."));
        }
    }
    
    return;
}

/* this functions connects the pipes once the other end is there */
void protocolOpenPipe() {
#ifdef WIN32
    int result;
    result = ConnectNamedPipe(protocolActiveServerPipeOut, NULL);

    if (GetLastError() == ERROR_PIPE_LISTENING) {
        return;
    }

    result = ConnectNamedPipe(protocolActiveServerPipeIn, NULL);
    if (GetLastError() == ERROR_PIPE_LISTENING) {
        return;
    }
    if ((result == 0) && (GetLastError() != ERROR_PIPE_CONNECTED) && (GetLastError() != ERROR_NO_DATA)) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Pipe connect failed: %s"), getLastErrorText());
        return;
    }
#else
    size_t pipeNameLen;
    TCHAR *pipeName;
    pipeNameLen = 12 + 10 + 1 + 10 + 3;
    pipeName = malloc(sizeof(TCHAR) * (pipeNameLen + 1));
    if (!pipeName) {
        outOfMemory(TEXT("PSSP"), 1);
        return;
    }
    _sntprintf(pipeName, pipeNameLen, TEXT("/tmp/wrapper-%d-%d-out"), wrapperData->wrapperPID, wrapperData->jvmRestarts);
    protocolActiveServerPipeOut = _topen(pipeName, O_WRONLY | O_NONBLOCK, S_IWUSR | S_IRUSR);

    if (protocolActiveServerPipeOut == INVALID_HANDLE_VALUE) {
        free(pipeName);
        return;
    }

    _sntprintf(pipeName, pipeNameLen, TEXT("/tmp/wrapper-%d-%d-in"), wrapperData->wrapperPID, wrapperData->jvmRestarts);
    protocolActiveServerPipeIn = _topen(pipeName, O_RDONLY  | O_NONBLOCK,  S_IRUSR);
    if (protocolActiveServerPipeIn == INVALID_HANDLE_VALUE) {
        free(pipeName);
        return;
    }
    free(pipeName);
#endif

    protocolActiveServerPipeConnected = TRUE;
}

/**
 * @param IPv4 if true then we use IPv4, otherwise we use IPv6
 */
void protocolOpenSocket(int IPv4) {
    struct sockaddr_in6 addr_srv6;
    struct sockaddr_in  addr_srv4;
    int rc;
    TCHAR* socketSource;
    int req;
#if defined(WIN32)
    u_long dwNoBlock = TRUE;
    u_long addr_srv_len;
#elif (defined(HPUX) && !defined(ARCH_IA)) || defined(OSF1) || defined(IRIX)
    int addr_srv_len;
#else
    socklen_t addr_srv_len;
#endif
    SOCKET newBackendSD = INVALID_SOCKET;

    char  straddr[256] = {0};
    int port;

    /* Is the server socket open? */
    if (protocolActiveServerSD == INVALID_SOCKET) {
        /* can't do anything yet. */
        return;
    }

    /* Try accepting a socket. */
    if (IPv4) {
        addr_srv_len = sizeof(addr_srv4);
    } else {
        addr_srv_len = sizeof(addr_srv6);
    }

#ifdef WIN32
    if (IPv4) {
        newBackendSD = accept(protocolActiveServerSD, (struct sockaddr FAR *)&addr_srv4, &addr_srv_len);
    } else {
        newBackendSD = accept(protocolActiveServerSD, (struct sockaddr FAR *)&addr_srv6, &addr_srv_len);
    }
#else /* UNIX */
    if (IPv4) {
        newBackendSD = accept(protocolActiveServerSD, (struct sockaddr *)&addr_srv4, &addr_srv_len);
    } else {
        newBackendSD = accept(protocolActiveServerSD, (struct sockaddr *)&addr_srv6, &addr_srv_len);
    }
#endif

    if (newBackendSD == INVALID_SOCKET) {
        rc = wrapperGetLastError();
        /* EWOULDBLOCK != EAGAIN on some platforms. */
        if ((rc == WRAPPER_EWOULDBLOCK) || (rc == EAGAIN)) {
            /* There are no incomming sockets right now. */
            return;
        } else {
            if (wrapperData->isDebugging) {
                log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG,
                    TEXT("socket creation failed. (%s)"), getLastErrorText());
            }
            return;
        }
    }

    /* get a human readable version of the address */
    if (IPv4) {
        inet_ntop(AF_INET, &addr_srv4.sin_addr, straddr, sizeof(straddr));
    } else {
        inet_ntop(AF_INET6, &addr_srv6.sin6_addr, straddr, sizeof(straddr));
    }

    /* convert the address */
#ifdef WIN32
    req = MultiByteToWideChar(CP_OEMCP, 0, straddr, -1, NULL, 0);
    if (req <= 0) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                TEXT("Invalid multibyte sequence in %s: %s"), TEXT("network address"), getLastErrorText());
        return;
    }
    socketSource = malloc(sizeof(TCHAR) * (req + 1));
    if (!socketSource) {
        outOfMemory(TEXT("PO"), 1);
        return;
    }
    MultiByteToWideChar(CP_OEMCP, 0, straddr, -1, socketSource, req + 1);
#else
    req = mbstowcs(NULL, straddr, MBSTOWCS_QUERY_LENGTH);
    if (req == (size_t)-1) {
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_WARN,
                TEXT("Invalid multibyte sequence in %s: %s"), TEXT("network address"), getLastErrorText());
        return;
    }
    socketSource = malloc(sizeof(TCHAR) * (req + 1));
    if (!socketSource) {
        outOfMemory(TEXT("PO"), 2);
        return;
    }
    mbstowcs(socketSource, straddr, req + 1);
    socketSource[req] = TEXT('\0'); /* Avoid bufferflows caused by badly encoded characters. */
#endif


    /* Is it already open? */
    if (protocolActiveBackendSD != INVALID_SOCKET) {
        if (IPv4) {
            port = ntohs(addr_srv4.sin_port);
        } else {
            port = ntohs(addr_srv6.sin6_port);
        }

        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_WARN, TEXT("Ignoring unexpected backend socket connection from %s on port %d"), socketSource, port);
        free(socketSource);
#ifdef WIN32
        rc = closesocket(newBackendSD);
#else /* UNIX */
        rc = close(newBackendSD);
#endif
        if (rc == SOCKET_ERROR) {
            if (wrapperData->isDebugging) {
                log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, TEXT("socket close failed. (%d)"), wrapperGetLastError());
            }
        }
        return;
    }

    /* New connection, so continue. */
    protocolActiveBackendSD = newBackendSD;

    /* Collect information about the remote end of the socket. */
    if (wrapperData->isDebugging) {
        if (IPv4) {
            port = ntohs(addr_srv4.sin_port);
        } else {
            port = ntohs(addr_srv6.sin6_port);
        }
        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, TEXT("accepted a socket on port %d from %s at port %d"), wrapperData->actualPort, socketSource, port);
    }
    
    free(socketSource);
    
    /* Make the socket non-blocking */
#ifdef WIN32
    rc = ioctlsocket(protocolActiveBackendSD, FIONBIO, &dwNoBlock);
#else /* UNIX */
    rc = fcntl(protocolActiveBackendSD, F_SETFL, O_NONBLOCK);
#endif
    if (rc == SOCKET_ERROR) {
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG,
                TEXT("socket ioctlsocket failed. (%s)"), getLastErrorText());
        }
        wrapperProtocolClose();
        return;
    }
    
    /* We got an incoming connection, so close down the listener to prevent further connections. */
    protocolStopServer();
}

/**
 * Attempt to accept a connection from a JVM client.
 */
void protocolOpen() {
    if (wrapperData->backendType == WRAPPER_BACKEND_TYPE_PIPE) {
        protocolOpenPipe();
    } else if (wrapperData->backendType == WRAPPER_BACKEND_TYPE_SOCKET_V6) {
        protocolOpenSocket(FALSE);
    } else {
        protocolOpenSocket(TRUE);
    }
}

void protocolClosePipe() {
#ifndef WIN32
    size_t pipeNameLen;
    TCHAR *pipeName;

   
    pipeNameLen = 12 + 10 + 1 + 10 + 3;
#endif
    if (protocolActiveServerPipeConnected) {
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, TEXT("Closing backend pipe."));
        }
#ifdef WIN32
        if (protocolActiveServerPipeIn != INVALID_HANDLE_VALUE && !CloseHandle(protocolActiveServerPipeIn)) {
#else
        if (close(protocolActiveServerPipeIn) == -1) {
#endif
            log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_ERROR, TEXT("Failed to close backend pipe: %s"), getLastErrorText());
        }

#ifdef WIN32
        if (protocolActiveServerPipeOut != INVALID_HANDLE_VALUE && !CloseHandle(protocolActiveServerPipeOut)) {
#else
        if (close(protocolActiveServerPipeOut) == -1) {
#endif
            log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_ERROR, TEXT("Failed to close backend pipe: %s"), getLastErrorText());
        }
        
#ifndef WIN32
        pipeName = malloc(sizeof(TCHAR) * (pipeNameLen + 1));
        if (!pipeName) {
            outOfMemory(TEXT("PCP"), 1);
            return;
        }

        _sntprintf(pipeName, pipeNameLen, TEXT("/tmp/wrapper-%d-%d-in"), wrapperData->wrapperPID, wrapperData->jvmRestarts);
        _tunlink(pipeName);
        _sntprintf(pipeName, pipeNameLen, TEXT("/tmp/wrapper-%d-%d-out"), wrapperData->wrapperPID, wrapperData->jvmRestarts);
        _tunlink(pipeName);
#endif

        protocolActiveServerPipeConnected = FALSE;
        protocolActiveServerPipeStarted = FALSE;
        protocolActiveServerPipeIn = INVALID_HANDLE_VALUE;
        protocolActiveServerPipeOut = INVALID_HANDLE_VALUE;
    }
}

void protocolCloseSocket() {
    int rc;

    /* Close the socket. */
    if (protocolActiveBackendSD != INVALID_SOCKET) {
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, TEXT("Closing backend socket."));
        }
#ifdef WIN32
        rc = closesocket(protocolActiveBackendSD);
#else /* UNIX */
        rc = close(protocolActiveBackendSD);
#endif
        if (rc == SOCKET_ERROR) {
            if (wrapperData->isDebugging) {
                log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, TEXT("socket close failed. (%d)"), wrapperGetLastError());
            }
        }
        protocolActiveBackendSD = INVALID_SOCKET;
    }
}

/**
 * Close the backend socket.
 */
void wrapperProtocolClose() {
    if (wrapperData->backendType == WRAPPER_BACKEND_TYPE_PIPE) {
        protocolClosePipe();
    } else {
        protocolCloseSocket();
    }
}

/**
 * Returns the name of a given function code for debug purposes.
 */
TCHAR *wrapperProtocolGetCodeName(char code) {
    static TCHAR unknownBuffer[14];
    TCHAR *name;

    switch (code) {
    case WRAPPER_MSG_START:
        name = TEXT("START");
        break;

    case WRAPPER_MSG_STOP:
        name = TEXT("STOP");
        break;

    case WRAPPER_MSG_RESTART:
        name = TEXT("RESTART");
        break;

    case WRAPPER_MSG_PING:
        name = TEXT("PING");
        break;

    case WRAPPER_MSG_STOP_PENDING:
        name = TEXT("STOP_PENDING");
        break;

    case WRAPPER_MSG_START_PENDING:
        name = TEXT("START_PENDING");
        break;

    case WRAPPER_MSG_STARTED:
        name = TEXT("STARTED");
        break;

    case WRAPPER_MSG_STOPPED:
        name = TEXT("STOPPED");
        break;

    case WRAPPER_MSG_KEY:
        name = TEXT("KEY");
        break;

    case WRAPPER_MSG_BADKEY:
        name = TEXT("BADKEY");
        break;

    case WRAPPER_MSG_LOW_LOG_LEVEL:
        name = TEXT("LOW_LOG_LEVEL");
        break;

    case WRAPPER_MSG_PING_TIMEOUT: /* No longer used. */
        name = TEXT("PING_TIMEOUT");
        break;

    case WRAPPER_MSG_SERVICE_CONTROL_CODE:
        name = TEXT("SERVICE_CONTROL_CODE");
        break;

    case WRAPPER_MSG_PROPERTIES:
        name = TEXT("PROPERTIES");
        break;

    case WRAPPER_MSG_LOG + LEVEL_DEBUG:
        name = TEXT("LOG(DEBUG)");
        break;

    case WRAPPER_MSG_LOG + LEVEL_INFO:
        name = TEXT("LOG(INFO)");
        break;

    case WRAPPER_MSG_LOG + LEVEL_STATUS:
        name = TEXT("LOG(STATUS)");
        break;

    case WRAPPER_MSG_LOG + LEVEL_WARN:
        name = TEXT("LOG(WARN)");
        break;

    case WRAPPER_MSG_LOG + LEVEL_ERROR:
        name = TEXT("LOG(ERROR)");
        break;

    case WRAPPER_MSG_LOG + LEVEL_FATAL:
        name = TEXT("LOG(FATAL)");
        break;

    case WRAPPER_MSG_LOG + LEVEL_ADVICE:
        name = TEXT("LOG(ADVICE)");
        break;

    case WRAPPER_MSG_LOG + LEVEL_NOTICE:
        name = TEXT("LOG(NOTICE)");
        break;

    case WRAPPER_MSG_LOGFILE:
        name = TEXT("LOGFILE");
        break;

    case WRAPPER_MSG_APPEAR_ORPHAN: /* No longer used. */
        name = TEXT("APPEAR_ORPHAN");
        break;

    case WRAPPER_MSG_PAUSE:
        name = TEXT("PAUSE");
        break;

    case WRAPPER_MSG_RESUME:
        name = TEXT("RESUME");
        break;

    case WRAPPER_MSG_GC:
        name = TEXT("GC");
        break;
        
    default:
        _sntprintf(unknownBuffer, 14, TEXT("UNKNOWN(%d)"), code);
        name = unknownBuffer;
        break;
    }
    return name;
}

/* Mutex for synchronization of the wrapperProtocolFunction function. */
#ifdef WIN32
HANDLE protocolMutexHandle = NULL;
#else
pthread_mutex_t protocolMutex = PTHREAD_MUTEX_INITIALIZER;
#endif


/** Obtains a lock on the protocol mutex. */
int lockProtocolMutex() {
#ifdef WIN32
    switch (WaitForSingleObject(protocolMutexHandle, INFINITE)) {
    case WAIT_ABANDONED:
        _tprintf(TEXT("Protocol mutex was abandoned.\n"));
        fflush(NULL);
        return -1;
    case WAIT_FAILED:
        _tprintf(TEXT("Protocol mutex wait failed.\n"));
        fflush(NULL);
        return -1;
    case WAIT_TIMEOUT:
        _tprintf(TEXT("Protocol mutex wait timed out.\n"));
        fflush(NULL);
        return -1;
    default:
        /* Ok */
        break;
    }
#else
    if (pthread_mutex_lock(&protocolMutex)) {
        _tprintf(TEXT("Failed to lock the Protocol mutex. %s\n"), getLastErrorText());
        return -1;
    }
#endif

    return 0;
}

/** Releases a lock on the protocol mutex. */
int releaseProtocolMutex() {
#ifdef WIN32
    if (!ReleaseMutex(protocolMutexHandle)) {
        _tprintf(TEXT("Failed to release Protocol mutex. %s\n"), getLastErrorText());
        fflush(NULL);
        return -1;
    }
#else
    if (pthread_mutex_unlock(&protocolMutex)) {
        _tprintf(TEXT("Failed to unlock the Protocol mutex. %s\n"), getLastErrorText());
        return -1;
    }
#endif
    return 0;
}

size_t protocolSendBufferSize = 0;
char *protocolSendBuffer = NULL;
/**
 * Sends a command to the JVM process.
 *
 * @param function The command to send.  (This is intentionally an 8-bit char.)
 * @param message Message to send along with the command.
 *
 * @return TRUE if there were any problems.
 */
int wrapperProtocolFunction(char function, const TCHAR *messageW) {
#ifdef UNICODE
 #ifdef WIN32
    TCHAR buffer[16];
    UINT cp;
 #endif
#endif
    int rc;
    int cnt;
    int sendCnt;
    int inWritten;
    size_t len;
    TCHAR *logMsgW;
    const TCHAR *messageTemplate;
    char *messageMB = NULL;
    int returnVal = FALSE;
    int ok = TRUE;
    size_t sent;
#ifdef WIN32
    int maxSendSize;
#endif

    /* It is important than there is never more than one thread allowed in here at a time. */
    if (lockProtocolMutex()) {
        return TRUE;
    }

    if (ok) {
        /* We will be trasmitting a MultiByte string of characters.  So we need to convert the messageW. */
        if (messageW) {
#ifdef UNICODE
 #ifdef WIN32
            GetLocaleInfo(GetThreadLocale(), LOCALE_IDEFAULTANSICODEPAGE, buffer, sizeof(buffer));
            cp = _ttoi(buffer);
            len = WideCharToMultiByte(CP_OEMCP, 0, messageW, -1, NULL, 0, NULL, NULL);
            if (len <= 0) {
                log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_WARN,
                    TEXT("Invalid multibyte sequence in %s \"%s\" : %s"), TEXT("protocol message"), messageW, getLastErrorText());
                returnVal = TRUE;
                ok = FALSE;
            } else {
                messageMB = malloc(len);
                if (!messageMB) {
                    outOfMemory(TEXT("WPF"), 2);
                    returnVal = TRUE;
                    ok = FALSE;
                } else {
                    WideCharToMultiByte(CP_OEMCP, 0, messageW, -1, messageMB, (int)len, NULL, NULL);
                }
            }
 #else
            len = wcstombs(NULL, messageW, 0);
            if (len == (size_t)-1) {
                log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_WARN,
                    TEXT("Invalid multibyte sequence in %s \"%s\" : %s"), TEXT("protocol message"), messageW, getLastErrorText());
                returnVal = TRUE;
                ok = FALSE;
            } else {
                messageMB = malloc(len + 1);
                if (!messageMB) {
                    outOfMemory(TEXT("WPF"), 3);
                    returnVal = TRUE;
                    ok = FALSE;
                } else {
                    wcstombs(messageMB, messageW, len + 1);
                }
            }
 #endif
#else
            len = _tscslen(messageW) + 1;
            messageMB = malloc(len);
            if (!messageMB) {
                outOfMemory(TEXT("WPF"), 4);
                returnVal = TRUE;
                ok = FALSE;
            } else {
                _tcsncpy(messageMB, messageW, len);
            }
#endif
        } else {
            messageMB = NULL;
        }
    }

    /* We don't want to show the full properties log message.  It is quite long and distracting. */
    if (function == WRAPPER_MSG_PROPERTIES) {
        messageTemplate = TEXT("(Property Values, Size=%d)");
        len = _tcslen(messageTemplate) + 16 + 1;
        logMsgW = malloc(sizeof(TCHAR) * len);
        if (!logMsgW) {
            outOfMemory(TEXT("WPF"), 1);
            /* Fallback to raw message.  Not ideal but Ok. */
            logMsgW = (TCHAR*)messageW; /* Strip the const, but will never be modified. */
        } else {
            /* messageMB should never be NULL, but for code checker be careful. */
            _sntprintf(logMsgW, len, messageTemplate, (messageMB ? strlen(messageMB) : 0));
        }
    } else {
        logMsgW = (TCHAR*)messageW; /* Strip the const, but will never be modified. */
    }

    if (ok) {
        /* We need to construct a single string that will be used to transmit the command + message. */
        if (messageMB) {
            len = 1 + strlen(messageMB) + 1;
        } else {
            len = 2;
        }
        if (protocolSendBufferSize < len) {
            if (protocolSendBuffer) {
                free(protocolSendBuffer);
            }
            protocolSendBuffer = malloc(sizeof(char) * len);
            if (!protocolSendBuffer) {
                outOfMemory(TEXT("WPF"), 4);
                returnVal = TRUE;
                ok = FALSE;
            } else {
                /* Build the packet */
                protocolSendBuffer[0] = function;
                if (messageMB) {
                    strncpy(&(protocolSendBuffer[1]), messageMB, len - 1);
                } else {
                    protocolSendBuffer[1] = 0;
                }
            }
        }
        if (messageMB) {
            free(messageMB);
        }
    }

    if (ok) {
        if (((protocolActiveBackendSD == INVALID_SOCKET) && (wrapperData->backendType == WRAPPER_BACKEND_TYPE_SOCKET))
            || ((protocolActiveServerPipeConnected == FALSE) && (wrapperData->backendType == WRAPPER_BACKEND_TYPE_PIPE))) {
            /* A socket was not opened */
            if (wrapperData->isDebugging) {
                log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG,
                    TEXT("Socket not open, so packet not sent %s : %s"),
                    wrapperProtocolGetCodeName(function), (logMsgW == NULL ? TEXT("NULL") : logMsgW));
            }
            returnVal = TRUE;
        } else {
            if (wrapperData->isDebugging) {
                if ((function == WRAPPER_MSG_PING) && messageW && (_tcsstr(messageW, TEXT("silent")) == messageW)) {
                    /*
                    log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG,
                        TEXT("Send a silent ping packet %s : %s"),
                        wrapperProtocolGetCodeName(function), (logMsgW == NULL ? TEXT("NULL") : logMsgW));
                    */
                } else {
                    log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG,
                        TEXT("Send a packet %s : %s"),
                        wrapperProtocolGetCodeName(function), (logMsgW == NULL ? TEXT("NULL") : logMsgW));
                }
            }

            /* When actually sending the packet, we need to be careful to make sure that the entire packet gets sent.
             *  There isssues on both sockets and Pipes where the send will fail if the packet is too large.
             *  In such cases, it needs to be broken up into multiple calls.
             *  This is currently only an issue with the PROPERTIES packet. */
            if (wrapperData->backendType == WRAPPER_BACKEND_TYPE_PIPE) {
                sent = 0;
                cnt = 0;
                sendCnt = 0;
#ifdef WIN32
                maxSendSize = 40000;
#endif
                while ((sent < len) && (cnt < 200)) {
                    if (cnt > 0) {
                        wrapperSleep(10);
                    }

#ifdef WIN32
                    /* Send a maximum of 32000 characters per call as larger values appear to fail without error. */
                    if (WriteFile(protocolActiveServerPipeOut, protocolSendBuffer + sent, __min(maxSendSize, (int)(sizeof(char) * (len - sent))), &inWritten, NULL) == FALSE) {
#else
                    if ((inWritten = write(protocolActiveServerPipeOut, protocolSendBuffer + sent, sizeof(char) * (int)(len - sent))) == -1) {
#endif
                        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_FATAL, TEXT("Writing to the backend pipe failed (%d): %s"), wrapperGetLastError(), getLastErrorText());
                        returnVal = TRUE;
                        break;
#ifdef WIN32
                    } else if (inWritten == 0) {
                        /* Didn't write anything, but not an error.
                         *  Have not found this documented anywhere, but it happens if the size is larger than some hidden limit. */
                        maxSendSize = __max(512, (int)(maxSendSize * 0.90));
                        sendCnt++;
#endif
                    } else {
                        /* Write N characters */
                        if (((sent + inWritten < len) || (sendCnt > 0)) && wrapperData->isDebugging) {
                            log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, TEXT("  Sent %d bytes, %d remaining."), inWritten, len - sent - inWritten );
                        }
                        sent += inWritten;
                        sendCnt++;
                    }
                    
                    cnt++;
                }
                if (sent < len) {
                    log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_ERROR, TEXT("%s send failed.  Incomplete.  Sent %d of %d bytes."), TEXT("Pipe"), sent, len);
                    returnVal = TRUE;
                }
            } else {
                sent = 0;
                cnt = 0;
                sendCnt = 0;
                rc = 0;
                while ((sent < len) && (cnt < 200)) {
                    if (cnt > 0) {
                        wrapperSleep(10);
                    }

                    rc = send(protocolActiveBackendSD, protocolSendBuffer + sent, sizeof(char) * (int)(len - sent), 0);
                    if (rc == SOCKET_ERROR) {
                        if (wrapperGetLastError() == WRAPPER_EWOULDBLOCK) {
                            /* The output buffer is simply full right now.  Try again in a bit. */
                        } else {
                            break;
                        }
                    } else if (rc < 0) {
                        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_ERROR, TEXT("Send unexpectedly returned %d"), rc);
                        break;
                    } else {
                        /* Wrote N characters. */
                        if (((sent + rc < len) || (sendCnt > 0)) && wrapperData->isDebugging) {
                            log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, TEXT("  Sent %d bytes, %d remaining."), rc, len - sent - rc );
                        }
                        sent += rc;
                        sendCnt++;
                    }

                    cnt++;
                }
                if (rc == SOCKET_ERROR) {
                    if (wrapperGetLastError() == WRAPPER_EWOULDBLOCK) {
                        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_WARN, TEXT("Socket send failed.  Blocked for 2 seconds.  %s"), getLastErrorText());
#ifdef WIN32
                    } else if (wrapperGetLastError() == WSAECONNRESET) {
                        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_ERROR, TEXT("Socket send failed.  %s"), getLastErrorText());
#endif
                    } else {
                        if (wrapperData->isDebugging) {
                            log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, TEXT("Socket send failed.  %s"), getLastErrorText());
                        }
                    }
                    wrapperProtocolClose();
                    returnVal = TRUE;
                } else if (sent < len) {
                    log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_ERROR, TEXT("%s send failed.  Incomplete.  Sent %d of %d bytes."), TEXT("Socket"), sent, len);
                    returnVal = TRUE;
                } else {
                    returnVal = FALSE;
                }
            }
        }
    }
    
    /* Free up the logMsgW if we allocated it. */
    if (logMsgW != messageW) {
        free(logMsgW);
    }

    /* Always make sure the mutex is released. */
    if (releaseProtocolMutex()) {
        returnVal = TRUE;
    }
    return returnVal;
}

/**
 * Checks the status of the server backend.
 *
 * The backend will be initialized if the JVM is in a state where it should
 *  be up, otherwise the backend will be left alone.
 *
 * If the forceOpen flag is set then an attempt will be made to initialize
 *  the backend regardless of the JVM state.
 *
 * Returns TRUE if the backend is open and ready on return, FALSE if not.
 */
int wrapperCheckServerBackend(int forceOpen) {
    if (((wrapperData->backendType & WRAPPER_BACKEND_TYPE_SOCKET) && (protocolActiveServerSD == INVALID_SOCKET)) ||
        ((wrapperData->backendType == WRAPPER_BACKEND_TYPE_PIPE) && (protocolActiveServerPipeStarted == FALSE)) ) {
        /* The backend is not currently open and needs to be started,
         *  unless the JVM is DOWN or in a state where it is not needed. */
        if ((!forceOpen) &&
            ((wrapperData->jState == WRAPPER_JSTATE_DOWN_CLEAN) ||
             (wrapperData->jState == WRAPPER_JSTATE_LAUNCH_DELAY) ||
             (wrapperData->jState == WRAPPER_JSTATE_RESTART) ||
             (wrapperData->jState == WRAPPER_JSTATE_STOPPED) ||
             (wrapperData->jState == WRAPPER_JSTATE_KILLING) ||
             (wrapperData->jState == WRAPPER_JSTATE_KILL) ||
             (wrapperData->jState == WRAPPER_JSTATE_KILLED) ||
             (wrapperData->jState == WRAPPER_JSTATE_DOWN_CHECK) ||
             (wrapperData->jState == WRAPPER_JSTATE_DOWN_FLUSH))) {
            /* The JVM is down or in a state where the backend is not needed. */
            return FALSE;
        } else {
            /* The backend should be open, try doing so. */
            protocolStartServer();
            if ( ((wrapperData->backendType & WRAPPER_BACKEND_TYPE_SOCKET) && (protocolActiveServerSD == INVALID_SOCKET)) ||
                 ((wrapperData->backendType == WRAPPER_BACKEND_TYPE_PIPE) && (protocolActiveServerPipeStarted == FALSE)) ) {
                /* Failed. */
                return FALSE;

            } else {
                return TRUE;
            }
        }
    } else {
        /* Backend is ready. */
        return TRUE;
    }
}

/**
 * Simple function to parse hexidecimal numbers into a TICKS
 */
TICKS hexToTICKS(TCHAR *buffer) {
    TICKS value = 0;
    TCHAR c;
    int pos = 0;
    
    while (TRUE) {
        c = buffer[pos];
        
        if ((c >= TEXT('a')) && (c <= TEXT('f'))) {
            value = (value << 4) + (10 + c - TEXT('a'));
        } else if ((c >= TEXT('A')) && (c <= TEXT('F'))) {
            value = (value << 4) + (10 + c - TEXT('A'));
        } else if ((c >= TEXT('0')) && (c <= TEXT('9'))) {
            value = (value << 4) + (c - TEXT('0'));
        } else {
            /* Any other character or null is the end of the number. */
            return value;
        }
        
        pos++;
    }
}

/**
 * Read any data sent from the JVM.  This function will loop and read as many
 *  packets are available.  The loop will only be allowed to go for 250ms to
 *  ensure that other functions are handled correctly.
 *
 * Returns 0 if all available data has been read, 1 if more data is waiting.
 */
int wrapperProtocolRead() {
    char c;
    char code;
    int len;
#ifdef WIN32
    int maxlen;
#endif
    int pos;
    TCHAR *tc;
    int err;
    struct timeb timeBuffer;
    time_t startTime;
    int startTimeMillis;
    time_t now;
    int nowMillis;
    time_t durr;
#ifdef WIN32
    TCHAR cpBuffer[16];
    UINT cp;
#endif
    size_t req;

    wrapperGetCurrentTime(&timeBuffer);
    startTime = now = timeBuffer.time;
    startTimeMillis = nowMillis = timeBuffer.millitm;

    /*
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("now=%ld, nowMillis=%d"), now, nowMillis);
    */
    while((durr = (now - startTime) * 1000 + (nowMillis - startTimeMillis)) < 250) {
        /*
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("durr=%ld"), durr);
        */

        /* If we have an open client backend, then use it. */
        if (((wrapperData->backendType & WRAPPER_BACKEND_TYPE_SOCKET) && (protocolActiveBackendSD == INVALID_SOCKET)) ||
            ((wrapperData->backendType == WRAPPER_BACKEND_TYPE_PIPE) && (protocolActiveServerPipeConnected == FALSE))) {
            /* A Client backend is not open */
            /* Is the server backend open? */
            if (!wrapperCheckServerBackend(FALSE)) {
                /* Backend is down.  We can not read any packets. */
                return 0;
            }
            /* Try accepting a connection */
            protocolOpen();
            if (((wrapperData->backendType & WRAPPER_BACKEND_TYPE_SOCKET) && (protocolActiveBackendSD == INVALID_SOCKET)) ||
                ((wrapperData->backendType == WRAPPER_BACKEND_TYPE_PIPE) && (protocolActiveServerPipeConnected == FALSE))) {
                return 0;
            }
        }

        if (wrapperData->backendType & WRAPPER_BACKEND_TYPE_SOCKET) {
            /* Try receiving a packet code */
            
            len = recv(protocolActiveBackendSD, (void*) &c, 1, 0);
            if (len == SOCKET_ERROR) {
                err = wrapperGetLastError();
                if ((err != WRAPPER_EWOULDBLOCK) &&  /* Windows - Would block. */
                    (err != EAGAIN)) {       /* UNIX - Would block. */
                    if (wrapperData->isDebugging) {
                        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, TEXT("socket read failed. (%s)"), getLastErrorText());
                    }
                    wrapperProtocolClose();
                }
                return 0;
            } else if (len != 1) {
                if (wrapperData->isDebugging) {
                    log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, TEXT("socket read no code (closed?)."));
                }
                wrapperProtocolClose();
                return 0;
            }

            code = (char)c;

            /* Read in any message */
            pos = 0;
            do {
                len = recv(protocolActiveBackendSD, (void*) &c, 1, 0);
                if (len == 1) {
                    if (c == 0) {
                        /* End of string */
                        len = 0;
                    } else if (pos < MAX_LOG_SIZE) {
                        packetBufferMB[pos] = c;
                        pos++;
                    }
                } else {
                    len = 0;
                }
            } while (len == 1);

            /* terminate the string; */
            packetBufferMB[pos] = '\0';
        } else if (wrapperData->backendType == WRAPPER_BACKEND_TYPE_PIPE) {
#ifdef WIN32
            err = PeekNamedPipe(protocolActiveServerPipeIn, NULL, 0, NULL, &maxlen, NULL);
            if ((err == 0) && (GetLastError() == ERROR_BROKEN_PIPE)) {
                /* ERROR_BROKEN_PIPE - the client has closed the pipe. So most likely it just exited */
                protocolActiveServerPipeIn = INVALID_HANDLE_VALUE;
            }
            if (maxlen == 0) {
                /*no data available */
                return 0;
            }
            if (ReadFile(protocolActiveServerPipeIn, &c, 1, &len, NULL) == TRUE || GetLastError() == ERROR_MORE_DATA) {
                code = (char)c;
                --maxlen;
                pos = 0;
                do {
                    ReadFile(protocolActiveServerPipeIn, &c, 1, &len, NULL);
                    if (len == 1) {
                        if (c == 0) {
                            /* End of string */
                            len = 0;
                        } else if (pos < MAX_LOG_SIZE) {
                            packetBufferMB[pos] = c;
                            pos++;
                        }
                    } else {
                        len = 0;
                    }
                } while (len == 1 && maxlen-- >= 0);
                packetBufferMB[pos] = '\0';
            } else {
                if (GetLastError() == ERROR_INVALID_HANDLE) {
                    return 0;
                } else {
                    wrapperProtocolClose();
                    return 0;
                }
            }            
#else
            len = read(protocolActiveServerPipeIn, (void*) &c, 1);
            if (len == SOCKET_ERROR) {
                err = wrapperGetLastError();
                if ((err != WRAPPER_EWOULDBLOCK) &&  /* Windows - Would block. */
                    (err != EAGAIN)) {       /* UNIX - Would block. */
                    if (wrapperData->isDebugging) {
                        log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, TEXT("pipe read failed. (%s)"), getLastErrorText());
                    }
                    wrapperProtocolClose();
                }
                return 0;
            } else if (len == 0) {
                /*nothing read...*/
                return 0;
            }
            code = (char)c;

            /* Read in any message */
            pos = 0;
            do {
                len = read(protocolActiveServerPipeIn, (void*) &c, 1);
                if (len == 1) {
                    if (c == 0) {
                        /* End of string */
                        len = 0;
                    } else if (pos < MAX_LOG_SIZE) {
                        packetBufferMB[pos] = c;
                        pos++;
                    }
                } else {
                    len = 0;
                }
            } while (len == 1);
            /* terminate the string; */
            packetBufferMB[pos] = '\0';
#endif
        } else {
            /* Should not reach this part because wrapperData->backendType should always have a valid value */
            return 0;
        }

        /* Convert the multi-byte packetBufferMB buffer into a wide-character string. */
        /* Source message is always smaller than the MAX_LOG_SIZE so the output will be as well. */
#ifdef WIN32
        GetLocaleInfo(GetThreadLocale(), LOCALE_IDEFAULTANSICODEPAGE, cpBuffer, sizeof(cpBuffer));
        cp = _ttoi(cpBuffer);
        req = MultiByteToWideChar(cp, 0, packetBufferMB, -1, packetBufferW, MAX_LOG_SIZE + 1);
        if (req <= 0) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                    TEXT("Invalid multibyte sequence in %s: %s"), TEXT("protocol message"), getLastErrorText());
            packetBufferW[0] = TEXT('\0');
        }
#else
        req = mbstowcs(packetBufferW, packetBufferMB, MAX_LOG_SIZE + 1);
        if (req == (size_t)-1) {
            log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_WARN,
                    TEXT("Invalid multibyte sequence in %s: %s"), TEXT("protocol message"), getLastErrorText());
            packetBufferW[0] = TEXT('\0');
        }
#endif

        if (wrapperData->isDebugging) {
            if ((code == WRAPPER_MSG_PING) && (_tcsstr(packetBufferW, TEXT("silent")) == packetBufferW)) {
                /*
                log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, TEXT("read a silent ping packet %s : %s"),
                    wrapperProtocolGetCodeName(code), packetBufferW);
                */
            } else {
                log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, TEXT("read a packet %s : %s"),
                    wrapperProtocolGetCodeName(code), packetBufferW);
            }
        }

        switch (code) {
        case WRAPPER_MSG_STOP:
            wrapperStopRequested(_ttoi(packetBufferW));
            break;

        case WRAPPER_MSG_RESTART:
            wrapperRestartRequested();
            break;

        case WRAPPER_MSG_PING:
            /* Because all versions of the wrapper.jar simply bounce back the ping message, the pingSendTicks should always exist. */
            tc = _tcschr(packetBufferW, TEXT(' '));
            if (tc) {
                /* A pingSendTicks should exist. Parse the id following the space. It will be in the format 0xffffffff. */
                wrapperPingResponded(hexToTICKS(&tc[1]), TRUE);
            } else {
                /* Should not happen, but just in case use the current ticks. */
                wrapperPingResponded(wrapperGetTicks(), FALSE);
            }
            break;

        case WRAPPER_MSG_STOP_PENDING:
            wrapperStopPendingSignaled(_ttoi(packetBufferW));
            break;

        case WRAPPER_MSG_STOPPED:
            wrapperStoppedSignaled();
            break;

        case WRAPPER_MSG_START_PENDING:
            wrapperStartPendingSignaled(_ttoi(packetBufferW));
            break;

        case WRAPPER_MSG_STARTED:
            wrapperStartedSignaled();
            break;

        case WRAPPER_MSG_KEY:
            wrapperKeyRegistered(packetBufferW);
            break;

        case WRAPPER_MSG_LOG + LEVEL_DEBUG:
        case WRAPPER_MSG_LOG + LEVEL_INFO:
        case WRAPPER_MSG_LOG + LEVEL_STATUS:
        case WRAPPER_MSG_LOG + LEVEL_WARN:
        case WRAPPER_MSG_LOG + LEVEL_ERROR:
        case WRAPPER_MSG_LOG + LEVEL_FATAL:
            wrapperLogSignaled(code - WRAPPER_MSG_LOG, packetBufferW);
            break;

        case WRAPPER_MSG_APPEAR_ORPHAN:
            /* No longer used.  This is still here in case a mix of versions are used. */
            break;

        default:
            if (wrapperData->isDebugging) {
                log_printf(WRAPPER_SOURCE_PROTOCOL, LEVEL_DEBUG, TEXT("received unknown packet (%d:%s)"), code, packetBufferW);
            }
            break;
        }

        /* Get the time again */
        wrapperGetCurrentTime(&timeBuffer);
        now = timeBuffer.time;
        nowMillis = timeBuffer.millitm;
    }
    /*
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("done durr=%ld"), durr);
    */
    if ((durr = (now - startTime) * 1000 + (nowMillis - startTimeMillis)) < 250) {
        return 0;
    } else {
        return 1;
    }
}


/******************************************************************************
 * Wrapper inner methods.
 *****************************************************************************/
/**
 * IMPORTANT - Any logging done in here needs to be queued or it would cause a recursion problem.
 *
 * It is also critical that this is NEVER called from within the protocol function because it
 *  would cause a deadlock with the protocol semaphore.  This means that it can never be called
 *  from within log_printf(...).
 */
void wrapperLogFileChanged(const TCHAR *logFile) {
    if (wrapperData->isDebugging) {
        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("active log file changed: %s"), logFile);
    }

    /* On startup, this function will always be called the first time the log file is set,
     *  we don't want to send the command in this case as it clutters the debug log output.
     *  Besides, the JVM will not be running anyway. */
    if (wrapperData->jState != WRAPPER_JSTATE_DOWN_CLEAN) {
        wrapperProtocolFunction(WRAPPER_MSG_LOGFILE, logFile);
    }
}

/**
 * Return the size of the PID
 */
int wrapperGetPidSize(int pid, int minSize) {
    if (pid < pow(10, minSize + 1)) {
        return minSize;
    }

    /* Slower method but it should not happen often. */
    return (int)floor (log10 ((double)abs (pid))) + 1;
}

/* In the future, we may load this value from configuration properties. */
#define PID_DEFAULT_SIZE   5
static int jPidSize = PID_DEFAULT_SIZE;
static int wPidSize = 0;

/**
 * Calculates the size required to display the Wrapper PID column or the Java PID column.
 *  The size of each column can grow on its own as needed and never go back down.
 */
int wrapperLogFormatCount(const TCHAR format, size_t *reqSize) {
    switch( format ) {
    case TEXT('J'):
    case TEXT('j'):
        jPidSize = wrapperGetPidSize((int)wrapperData->javaPID, jPidSize);
        *reqSize += jPidSize + 3;
        return 1;

    case TEXT('W'):
    case TEXT('w'):
        if (wPidSize == 0) {
            /* The Wrapper PID can't change. Calculate its width only one time. */
            wPidSize = wrapperGetPidSize((int)wrapperData->wrapperPID, PID_DEFAULT_SIZE);
        }
        *reqSize += wPidSize + 3;
        return 1;
    }
    
    return 0;
}

/**
 * Print the Wrapper PID column or the Java PID column.
 */
int wrapperLogFormatPrint(const TCHAR format, size_t printSize, TCHAR** pBuffer) {
    switch( format ) {
    case TEXT('J'):
    case TEXT('j'):
        if (wrapperData->javaPID) {
            return _sntprintf( *pBuffer, printSize, TEXT("%*d"), jPidSize, wrapperData->javaPID);
        } else {
            return _sntprintf( *pBuffer, printSize, TEXT("-----"));
        }

    case TEXT('W'):
    case TEXT('w'):
        return _sntprintf( *pBuffer, printSize, TEXT("%*d"), wPidSize, wrapperData->wrapperPID);
    }
    
    return 0;
}

/**
 * Pre initialize the wrapper.
 */
int wrapperInitialize() {
#ifdef WIN32
    int maxPathLen = _MAX_PATH;
#else
    int maxPathLen = PATH_MAX;
#endif

    /* Initialize the properties variable. */
    properties = NULL;

    /* Initialize the random seed. */
    srand((unsigned)time(NULL));

    /* Make sure all values are reliably set to 0. All required values should also be
     *  set below, but this extra step will protect against future changes.  Some
     *  platforms appear to initialize maloc'd memory to 0 while others do not. */
    wrapperData = malloc(sizeof(WrapperConfig));
    if (!wrapperData) {
        _tprintf(TEXT("Out of memory (%s)\n"), TEXT("WIZ1"));
        return 1;
    }
    memset(wrapperData, 0, sizeof(WrapperConfig));
    /* Setup the initial values of required properties. */
    wrapperData->configured = FALSE;
    wrapperData->isConsole = TRUE;
    wrapperSetWrapperState(WRAPPER_WSTATE_STARTING);
    wrapperSetJavaState(WRAPPER_JSTATE_DOWN_CLEAN, 0, -1);
    wrapperData->lastPingTicks = wrapperGetTicks();
    wrapperData->lastLoggedPingTicks = wrapperGetTicks();
    wrapperData->jvmVersionCommand = NULL;
    wrapperData->jvmCommand = NULL;
    wrapperData->exitRequested = FALSE;
    wrapperData->restartRequested = WRAPPER_RESTART_REQUESTED_INITIAL; /* The first JVM needs to be started. */
    wrapperData->exitCode = 0;
    wrapperData->errorExitCode = 1;
    wrapperData->jvmRestarts = 0;
    wrapperData->jvmLaunchTicks = wrapperGetTicks();
    wrapperData->failedInvocationCount = 0;
    wrapperData->originalWorkingDir = NULL;
    wrapperData->configFile = NULL;
    wrapperData->workingDir = NULL;
    wrapperData->outputFilterCount = 0;
    wrapperData->confDir = NULL;
    wrapperData->umask = -1;
    wrapperData->language = NULL;
    wrapperData->portAddress = NULL;
    wrapperData->pingTimedOut = FALSE;
    wrapperData->shutdownActionPropertyName = NULL;
#ifdef WIN32
    if (!(tickMutexHandle = CreateMutex(NULL, FALSE, NULL))) {
        printf("Failed to create tick mutex. %s\n", getLastErrorText());
        return 1;
    }

    /* Initialize control code queue. */
    wrapperData->ctrlCodeQueue = malloc(sizeof(int) * CTRL_CODE_QUEUE_SIZE);
    if (!wrapperData->ctrlCodeQueue) {
        _tprintf(TEXT("Out of memory (%s)\n"), TEXT("WIZ2"));
        return 1;
    }
    wrapperData->ctrlCodeQueueWriteIndex = 0;
    wrapperData->ctrlCodeQueueReadIndex = 0;
    wrapperData->ctrlCodeQueueWrapped = FALSE;
#endif

    if (initLogging(wrapperLogFileChanged)) {
        return 1;
    }

    /* This will only be called by the main thread on startup.
     * Immediately register this thread with the logger.
     * This has to happen after the logging is initialized. */
    logRegisterThread(WRAPPER_THREAD_MAIN);
    
    logRegisterFormatCallbacks(wrapperLogFormatCount, wrapperLogFormatPrint);

    setLogfilePath(TEXT("wrapper.log"), FALSE);
    setLogfileRollMode(ROLL_MODE_SIZE);
    setLogfileFormat(TEXT("LPTM"));
    setLogfileLevelInt(LEVEL_DEBUG);
    setLogfileAutoClose(FALSE);
    setConsoleLogFormat(TEXT("LPM"));
    setConsoleLogLevelInt(LEVEL_DEBUG);
    setConsoleFlush(TRUE);  /* Always flush immediately until the logfile is configured to make sure that problems are in a consistent location. */
    setSyslogLevelInt(LEVEL_NONE);

#ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("Wrapper Initializing...  Minimum logging configured."));
#endif

    /** Remember what the initial user directory was when the Wrapper was launched. */
    wrapperData->initialPath = (TCHAR *)malloc((maxPathLen + 1) * sizeof(TCHAR));
    if (!wrapperData->initialPath) {
        outOfMemory(TEXT("WIZ"), 3);
        return 1;
    } else {
        if (!(wrapperData->initialPath = _tgetcwd((TCHAR*)wrapperData->initialPath, maxPathLen + 1))) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Failed to get the initial directory. (%s)"), getLastErrorText());
            return 1;
        }
    }
    /* Set a variable to the initial working directory. */
    setEnv(TEXT("WRAPPER_INIT_DIR"), wrapperData->initialPath, ENV_SOURCE_APPLICATION);

#ifdef WIN32
    if (!(protocolMutexHandle = CreateMutex(NULL, FALSE, NULL))) {
        _tprintf(TEXT("Failed to create protocol mutex. %s\n"), getLastErrorText());
        fflush(NULL);
        return 1;
    }
#endif

    /* This is a sanity check to make sure that the datatype used for tick counts is correct. */
    if (sizeof(TICKS) != 4) {
        printf("Tick size incorrect %d != 4\n", (int)sizeof(TICKS));
        fflush(NULL);
        return 1;
    }

    if (loadEnvironment()) {
        return 1;
    }

#ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("Wrapper Initialization complete."));
#endif
    return 0;
}

void wrapperDataDispose() {
    int i;
    
    if (wrapperData->workingDir) {
        free(wrapperData->workingDir);
        wrapperData->workingDir = NULL;
    }
    if (wrapperData->originalWorkingDir) {
        free(wrapperData->originalWorkingDir);
        wrapperData->originalWorkingDir = NULL;
    }
    if (wrapperData->configFile) {
        free(wrapperData->configFile);
        wrapperData->configFile = NULL;
    }
    if (wrapperData->initialPath) {
        free(wrapperData->initialPath);
        wrapperData->initialPath = NULL;
    }
    if (wrapperData->classpath) {
        free(wrapperData->classpath);
        wrapperData->classpath = NULL;
    }
    if (wrapperData->portAddress) {
        free(wrapperData->portAddress);
        wrapperData->portAddress = NULL;
    }
#ifdef WIN32
    if (wrapperData->jvmVersionCommand) {
        free(wrapperData->jvmVersionCommand);
        wrapperData->jvmVersionCommand = NULL;
    }
    if (wrapperData->jvmCommand) {
        free(wrapperData->jvmCommand);
        wrapperData->jvmCommand = NULL;
    }
    if (wrapperData->userName) {
        free(wrapperData->userName);
        wrapperData->userName = NULL;
    }
    if (wrapperData->domainName) {
        free(wrapperData->domainName);
        wrapperData->domainName = NULL;
    }
    if (wrapperData->ntServiceLoadOrderGroup) {
        free(wrapperData->ntServiceLoadOrderGroup);
        wrapperData->ntServiceLoadOrderGroup = NULL;
    }
    if (wrapperData->ntServiceDependencies) {
        free(wrapperData->ntServiceDependencies);
        wrapperData->ntServiceDependencies = NULL;
    }
    if (wrapperData->ntServiceAccount) {
        free(wrapperData->ntServiceAccount);
        wrapperData->ntServiceAccount = NULL;
    }
    if (wrapperData->ntServicePassword) {
        free(wrapperData->ntServicePassword);
        wrapperData->ntServicePassword = NULL;
    }
    if (wrapperData->ctrlCodeQueue) {
        free(wrapperData->ctrlCodeQueue);
        wrapperData->ctrlCodeQueue = NULL;
    }
#else
    if(wrapperData->jvmVersionCommand) {
        for (i = 0; wrapperData->jvmVersionCommand[i] != NULL; i++) {
            free(wrapperData->jvmVersionCommand[i]);
            wrapperData->jvmVersionCommand[i] = NULL;
        }
        free(wrapperData->jvmVersionCommand);
        wrapperData->jvmVersionCommand = NULL;
    }
    if(wrapperData->jvmCommand) {
        for (i = 0; wrapperData->jvmCommand[i] != NULL; i++) {
            free(wrapperData->jvmCommand[i]);
            wrapperData->jvmCommand[i] = NULL;
        }
        free(wrapperData->jvmCommand);
        wrapperData->jvmCommand = NULL;
    }
#endif
    if(wrapperData->shutdownActionPropertyName) {
        free(wrapperData->shutdownActionPropertyName);
        wrapperData->shutdownActionPropertyName = NULL;
    }
    if (wrapperData->outputFilterCount > 0) {
        for (i = 0; i < wrapperData->outputFilterCount; i++) {
            if (wrapperData->outputFilters[i]) {
                free(wrapperData->outputFilters[i]);
                wrapperData->outputFilters[i] = NULL;
            }
            if (wrapperData->outputFilterActionLists[i]) {
                free(wrapperData->outputFilterActionLists[i]);
                wrapperData->outputFilterActionLists[i] = NULL;
            }
        }
        if (wrapperData->outputFilters) {
            free(wrapperData->outputFilters);
            wrapperData->outputFilters = NULL;
        }
        if (wrapperData->outputFilterActionLists) {
            free(wrapperData->outputFilterActionLists);
            wrapperData->outputFilterActionLists = NULL;
        }
        if (wrapperData->outputFilterMessages) {
            free(wrapperData->outputFilterMessages);
            wrapperData->outputFilterMessages = NULL;
        }
        if (wrapperData->outputFilterAllowWildFlags) {
            free(wrapperData->outputFilterAllowWildFlags);
            wrapperData->outputFilterAllowWildFlags = NULL;
        }
        if (wrapperData->outputFilterMinLens) {
            free(wrapperData->outputFilterMinLens);
            wrapperData->outputFilterMinLens = NULL;
        }
    }

    if (wrapperData->pidFilename) {
        free(wrapperData->pidFilename);
        wrapperData->pidFilename = NULL;
    }
    if (wrapperData->lockFilename) {
        free(wrapperData->lockFilename);
        wrapperData->lockFilename = NULL;
    }
    if (wrapperData->javaPidFilename) {
        free(wrapperData->javaPidFilename);
        wrapperData->javaPidFilename = NULL;
    }
    if (wrapperData->javaIdFilename) {
        free(wrapperData->javaIdFilename);
        wrapperData->javaIdFilename = NULL;
    }
    if (wrapperData->statusFilename) {
        free(wrapperData->statusFilename);
        wrapperData->statusFilename = NULL;
    }
    if (wrapperData->javaStatusFilename) {
        free(wrapperData->javaStatusFilename);
        wrapperData->javaStatusFilename = NULL;
    }
    if (wrapperData->commandFilename) {
        free(wrapperData->commandFilename);
        wrapperData->commandFilename = NULL;
    }
    if (wrapperData->consoleTitle) {
        free(wrapperData->consoleTitle);
        wrapperData->consoleTitle = NULL;
    }
    if (wrapperData->serviceName) {
        free(wrapperData->serviceName);
        wrapperData->serviceName = NULL;
    }
    if (wrapperData->serviceDisplayName) {
        free(wrapperData->serviceDisplayName);
        wrapperData->serviceDisplayName = NULL;
    }
    if (wrapperData->serviceDescription) {
        free(wrapperData->serviceDescription);
        wrapperData->serviceDescription = NULL;
    }
    if (wrapperData->hostName) {
        free(wrapperData->hostName);
        wrapperData->hostName = NULL;
    }
    if (wrapperData->confDir) {
        free(wrapperData->confDir);
        wrapperData->confDir = NULL;
    }
    if (wrapperData->argConfFileDefault && wrapperData->argConfFile) {
        free(wrapperData->argConfFile);
        wrapperData->argConfFile = NULL;
    }

    if (wrapperData) {
        free(wrapperData);
        wrapperData = NULL;
    }

}



/** Common wrapper cleanup code. */
void wrapperDispose() {
    /* Make sure not to dispose twice.  This should not happen, but check for safety. */
    if (disposed) {
        /* Don't use log_printf here as the second call may have already disposed logging. */
        _tprintf(TEXT("wrapperDispose was called more than once.\n"));
        return;
    }
    disposed = TRUE;

#ifdef WIN32
    if (protocolMutexHandle) {
        if (!CloseHandle(protocolMutexHandle)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Unable to close protocol mutex handle. %s"), getLastErrorText());
        }
    }
    
    /* Make sure that the startup thread has completed. */
    disposeStartup();
#endif

    
    /* Clean up the javaIO thread. This should be done before the timer thread. */
    if (wrapperData->useJavaIOThread) {
        disposeJavaIO();
    }

    /* Clean up the timer thread. */
    if (!wrapperData->useSystemTime) {
        disposeTimer();
    }

    /* Clean up the properties structure. */
    disposeProperties(properties);
    properties = NULL;

    disposeEnvironment();
    if (wrapperChildWorkBuffer) {
        free(wrapperChildWorkBuffer);
        wrapperChildWorkBuffer = NULL;
    }
    if (protocolSendBuffer) {
        free(protocolSendBuffer);
        protocolSendBuffer = NULL;
    }

    /* Clean up the logging system.  Should happen near last. */
    disposeLogging();

    /* clean up the main wrapper data structure. This must be done last.*/
    wrapperDataDispose();
}

/**
 * Returns the file name base as a newly malloced TCHAR *.  The resulting
 *  base file name will have any path and extension stripped.
 *
 * baseName should be long enough to always contain the base name.
 *  (_tcslen(fileName) + 1) is safe.
 */
void wrapperGetFileBase(const TCHAR *fileName, TCHAR *baseName) {
    const TCHAR *start;
    const TCHAR *end;
    const TCHAR *c;

    start = fileName;
    end = &fileName[_tcslen(fileName)];

    /* Strip off any path. */
#ifdef WIN32
    c = _tcsrchr(start, TEXT('\\'));
#else
    c = _tcsrchr(start, TEXT('/'));
#endif
    if (c) {
        start = &c[1];
    }

    /* Strip off any extension. */
    c = _tcsrchr(start, TEXT('.'));
    if (c) {
        end = c;
    }

    /* Now create the new base name. */
    _tcsncpy(baseName, start, end - start);
    baseName[end - start] = TEXT('\0');
}

/**
 * Returns a buffer containing a multi-line version banner.  It is the responsibility of the caller
 *  to make sure it gets freed.
 */
TCHAR *generateVersionBanner() {
    TCHAR *banner = TEXT("Java Service Wrapper %s Edition %s-bit %s\n  Copyright (C) 1999-%s Tanuki Software, Ltd. All Rights Reserved.\n    http://wrapper.tanukisoftware.com");
    TCHAR *product = TEXT("Community");
    TCHAR *copyright = TEXT("2017");
    TCHAR *buffer;
    size_t len;

    len = _tcslen(banner) + _tcslen(product) + _tcslen(wrapperBits) + _tcslen(wrapperVersionRoot) + _tcslen(copyright) + 1;
    buffer = malloc(sizeof(TCHAR) * len);
    if (!buffer) {
        outOfMemory(TEXT("GVB"), 1);
        return NULL;
    }

    _sntprintf(buffer, len, banner, product, wrapperBits, wrapperVersionRoot, copyright);

    return buffer;
}

/**
 * Output the version.
 */
void wrapperVersionBanner() {
    TCHAR *banner = generateVersionBanner();
    if (!banner) {
        return;
    }
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, banner);
    free(banner);
}

/**
 * Output the application usage.
 */
void wrapperUsage(TCHAR *appName) {
    TCHAR *confFileBase;

    confFileBase = malloc(sizeof(TCHAR) * (_tcslen(appName) + 1));
    if (!confFileBase) {
        outOfMemory(TEXT("WU"), 1);
        return;
    }
    wrapperGetFileBase(appName, confFileBase);

    setSimpleLogLevels();

    wrapperVersionBanner();
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT(""));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("Usage:"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  %s <command> <configuration file> [configuration properties] [...]"), appName);
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  %s <configuration file> [configuration properties] [...]"), appName);
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("     (<command> implicitly '-c')"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  %s <command>"), appName);
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("     (<configuration file> implicitly '%s.conf')"), confFileBase);
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  %s"), appName);
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("     (<command> implicitly '-c' and <configuration file> '%s.conf')"), confFileBase);
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT(""));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("where <command> can be one of:"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  -c  --console run as a Console application"));
#ifdef WIN32
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  -su --setup   SetUp the Wrapper"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  -td --teardown TearDown the Wrapper"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  -t  --start   starT an NT service"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  -a  --pause   pAuse a running NT service"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  -e  --resume  rEsume a paused NT service"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  -p  --stop    stoP a started NT service"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  -i  --install Install as an NT service"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  -it --installstart Install and sTart as an NT service"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  -r  --remove  Uninstall/Remove as an NT service"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  -l=<code> --controlcode=<code> send a user controL Code to a running NT service"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  -d  --dump    request a thread Dump"));
    /** Return mask: installed:1 running:2 interactive:4 automatic:8 manual:16 disabled:32 */
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  -q  --query   Query the current status of the service"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  -qs --querysilent Silently Query the current status of the service"));
    /* Omit '-s' option from help as it is only used by the service manager. */
    /*log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  -s  --service used by service manager")); */
#endif
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  -v  --version print the Wrapper's Version information"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  -?  --help    print this help message"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  -- <args>     mark the end of Wrapper arguments.  All arguments after the\n                '--' will be passed through unmodified to the java application."));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT(""));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("<configuration file> is the wrapper.conf to use.  Name must be absolute or relative"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  to the location of %s"), appName);
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT(""));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("[configuration properties] are configuration name-value pairs which override values"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  in wrapper.conf.  For example:"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  wrapper.debug=true"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT(""));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  Please note that any file references must be absolute or relative to the location\n  of the Wrapper executable."));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT(""));

    free(confFileBase);
}

/**
 * Parse the main arguments.
 *
 * Returns FALSE if the application should exit with an error.  A message will
 *  already have been logged.
 */
int wrapperParseArguments(int argc, TCHAR **argv) {
    TCHAR *argConfFileBase;
    TCHAR *c;
    int delimiter, wrapperArgCount;
    wrapperData->javaArgValueCount = 0;
    delimiter = 1;

    if (argc > 1
        ) {
        for (delimiter = 0; delimiter < argc ; delimiter++) {
            if ( _tcscmp(argv[delimiter], TEXT("--")) == 0) {
#if !defined(WIN32) && defined(UNICODE)
                free(argv[delimiter]);
#endif
                argv[delimiter] = NULL;

                wrapperData->javaArgValueCount = argc - delimiter - 1;
                if (delimiter + 1 < argc) {
                    wrapperData->javaArgValues = &argv[delimiter + 1];
                }
                break;
            }
        }
    }

    wrapperArgCount = delimiter ;
    if (wrapperArgCount > 1) {

        /* Store the name of the binary.*/
        wrapperData->argBinary = argv[0];
        
        if (argv[1][0] == TEXT('-')) {
            /* Syntax 1 or 3 */

            /* A command appears to have been specified. */
            wrapperData->argCommand = &argv[1][1]; /* Strip off the '-' */
            if (wrapperData->argCommand[0] == TEXT('\0')) {
                wrapperUsage(argv[0]);
                return FALSE;
            }

            /* Does the argument have a value? */
            c = _tcschr(wrapperData->argCommand, TEXT('='));
            if (c == NULL) {
                wrapperData->argCommandArg = NULL;
            } else {
                wrapperData->argCommandArg = (TCHAR *)(c + 1);
                c[0] = TEXT('\0');
            }

            if (wrapperArgCount > 2) {
                if (_tcsncmp(wrapperData->argCommand, TEXT("-translate"), 5) == 0) {
                    if (wrapperArgCount > 3) {
                        wrapperData->argConfFile = argv[3];
                        wrapperData->argCount = wrapperArgCount - 4;
                        wrapperData->argValues = &argv[4];
                    }
                    return TRUE;
                }
                /* Syntax 1 */
                /* A command and conf file were specified. */
                wrapperData->argConfFile = argv[2];
                wrapperData->argCount = wrapperArgCount - 3;
                wrapperData->argValues = &argv[3];
            } else {
                /* Syntax 3 */
                /* Only a command was specified.  Assume a default config file name. */
                    argConfFileBase = malloc(sizeof(TCHAR) * (_tcslen(argv[0]) + 1));
                    if (!argConfFileBase) {
                        outOfMemory(TEXT("WPA"), 1);
                        return FALSE;
                    }
                    wrapperGetFileBase(argv[0], argConfFileBase);

                    /* The following malloc is only called once, but is never freed. */
                    wrapperData->argConfFile = malloc((_tcslen(argConfFileBase) + 5 + 1) * sizeof(TCHAR));
                    if (!wrapperData->argConfFile) {
                        outOfMemory(TEXT("WPA"), 2);
                        free(argConfFileBase);
                        return FALSE;
                    }
                    _sntprintf(wrapperData->argConfFile, _tcslen(argConfFileBase) + 5 + 1, TEXT("%s.conf"), argConfFileBase);

                    free(argConfFileBase);

                wrapperData->argConfFileDefault = TRUE;
                wrapperData->argCount = wrapperArgCount - 2;
                wrapperData->argValues = &argv[2];
            }
        } else {
            /* Syntax 2 */
            /* A command was not specified, but there may be a config file. */
            wrapperData->argCommand = TEXT("c");
            wrapperData->argCommandArg = NULL;
            wrapperData->argConfFile = argv[1];
            wrapperData->argCount = wrapperArgCount - 2;
            wrapperData->argValues = &argv[2];
        }
    } else {
        /* Systax 4 */
        /* A config file was not specified.  Assume a default config file name. */
        wrapperData->argCommand = TEXT("c");
        wrapperData->argCommandArg = NULL;
            argConfFileBase = malloc(sizeof(TCHAR) * (_tcslen(argv[0]) + 1));
            if (!argConfFileBase) {
                outOfMemory(TEXT("WPA"), 3);
                return FALSE;
            }
            wrapperGetFileBase(argv[0], argConfFileBase);

            /* The following malloc is only called once, but is never freed. */
            wrapperData->argConfFile = malloc((_tcslen(argConfFileBase) + 5 + 1) * sizeof(TCHAR));
            if (!wrapperData->argConfFile) {
                outOfMemory(TEXT("WPA"), 4);
                free(argConfFileBase);
                return FALSE;
            }
            _sntprintf(wrapperData->argConfFile, _tcslen(argConfFileBase) + 5 + 1, TEXT("%s.conf"), argConfFileBase);

            free(argConfFileBase);
        wrapperData->argConfFileDefault = TRUE;
        wrapperData->argCount = wrapperArgCount - 1;
            wrapperData->argValues = &argv[1];
    }

    return TRUE;
}

/**
 * Performs the specified action,
 *
 * @param actionList An array of action Ids ending with a value ACTION_LIST_END.
 *                   Negative values are standard actions, positive are user
 *                   custom events.
 * @param triggerMsg The reason the actions are being fired.
 * @param actionSourceCode Tracks where the action originated.
 * @param actionPropertyIndex Index of the property where the action was configured. Ignored if the type of action is not configured with a <n>-component property.
 * @param logForActionNone Flag stating whether or not a message should be logged
 *                         for the NONE action.
 * @param exitCode Error code to use in case the action results in a shutdown.
 */
void wrapperProcessActionList(int *actionList, const TCHAR *triggerMsg, int actionSourceCode, int actionPropertyIndex, int logForActionNone, int exitCode) {
    int i;
    int action;
    TCHAR propertyName[32];

    if (actionList) {
        i = 0;
        while ((action = actionList[i]) != ACTION_LIST_END) {
                switch(action) {
                case ACTION_RESTART:
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("%s  %s"), triggerMsg, wrapperGetRestartProcessMessage());
                    wrapperRestartProcess();
                    if (actionSourceCode == WRAPPER_ACTION_SOURCE_CODE_PING_TIMEOUT) {
                        wrapperKillProcess(TRUE);
                    }
                    break;

                case ACTION_SHUTDOWN:
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("%s  Shutting down."), triggerMsg);
                    wrapperData->shutdownActionTriggered = TRUE;
                    switch (actionSourceCode) {
                    case WRAPPER_ACTION_SOURCE_CODE_FILTER:
                        _sntprintf(propertyName, 32, TEXT("wrapper.filter.action.%d"), actionPropertyIndex);
                        break;
                    case WRAPPER_ACTION_SOURCE_CODE_PING_TIMEOUT:
                        _sntprintf(propertyName, 32, TEXT("wrapper.ping.timeout.action"));
                        break;
                    default:
                        _sntprintf(propertyName, 32, TEXT(""));
                    }
                    updateStringValue(&(wrapperData->shutdownActionPropertyName), propertyName);
                    if (actionSourceCode == WRAPPER_ACTION_SOURCE_CODE_PING_TIMEOUT) {
                        wrapperData->exitCode = exitCode;
                        wrapperKillProcess(TRUE);
                    } else {
                        wrapperStopProcess(exitCode, FALSE);
                    }
                    break;

                case ACTION_DUMP:
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("%s  Requesting thread dump."), triggerMsg);
                    wrapperRequestDumpJVMState();
                    break;

                case ACTION_DEBUG:
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("%s  Debugging."), triggerMsg);
                    break;

                case ACTION_PAUSE:
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("%s  %s"), triggerMsg, wrapperGetPauseProcessMessage());
                    wrapperPauseProcess(actionSourceCode);
                    break;

                case ACTION_RESUME:
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("%s  %s"), triggerMsg, wrapperGetResumeProcessMessage());
                    wrapperResumeProcess(actionSourceCode);
                    break;

#if defined(MACOSX)
                case ACTION_ADVICE_NIL_SERVER:
                    if (wrapperData->isAdviserEnabled) {
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE, TEXT(""));
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE, TEXT(
                            "--------------------------------------------------------------------"));
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE, TEXT(
                            "Advice:"));
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE, TEXT(
                            "MACOSX is known to have problems displaying GUIs from processes\nrunning as a daemon launched from launchd.  The above\n\"Returning nil _server\" means that you are encountering this\nproblem.  This usually results in a long timeout which is affecting\nthe performance of your application."));
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE, TEXT(
                            "--------------------------------------------------------------------"));
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE, TEXT(""));
                    }
                    break;
#endif

                case ACTION_NONE:
                    if (logForActionNone) {
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("%s"), triggerMsg);
                    }
                    /* Do nothing but masks later filters */
                    break;

                case ACTION_SUCCESS:
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("%s  Application has signaled success, consider this application started successful..."), triggerMsg);
                    wrapperData->failedInvocationCount = 0;
                    break;

                case ACTION_GC:
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("%s  Requesting GC..."), triggerMsg);
                    wrapperRequestJVMGC(actionSourceCode);
                    break;

                default:
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT("Unknown action type: %d"), action);
                    break;
                }

            i++;
        }
    }
}

/**
 * Function that will recursively attempt to match two strings where the
 *  pattern can contain '?' or '*' wildcard characters.  This function requires
 *  that the pattern be matched from the beginning of the text.
 *
 * @param text Text to be searched.
 * @param textLen Length of the text.
 * @param pattern Pattern to search for.
 * @param patternLen Length of the pattern.
 * @param minTextLen Minimum number of characters that the text needs to possibly match the pattern.
 *
 * @return TRUE if found, FALSE otherwise.
 *
 * 1)     text=abcdefg  textLen=7  pattern=a*d*efg  patternLen=7  minTextLen=5
 * 1.1)   text=bcdefg   textLen=6  pattern=d*efg    patternLen=5  minTextLen=4
 * 1.2)   text=cdefg    textLen=5  pattern=d*efg    patternLen=5  minTextLen=4
 * 1.3)   text=defg     textLen=4  pattern=d*efg    patternLen=5  minTextLen=4
 * 1.3.1) text=efg      textLen=3  pattern=efg      patternLen=3  minTextLen=3
 */
int wildcardMatchInner(const TCHAR *text, size_t textLen, const TCHAR *pattern, size_t patternLen, size_t minTextLen) {
    size_t textIndex;
    size_t patternIndex;
    TCHAR patternChar;
    size_t textIndex2;
    TCHAR textChar;

    /*log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("  wildcardMatchInner(\"%s\", %d, \"%s\", %d, %d)"), text, textLen, pattern, patternLen, minTextLen);*/

    textIndex = 0;
    patternIndex = 0;

    while ((textIndex < textLen) && (patternIndex < patternLen)) {
        patternChar = pattern[patternIndex];

        if (patternChar == TEXT('*')) {
            /* The pattern '*' can match 0 or more characters.  This requires a bit of recursion to work it out. */
            textIndex2 = textIndex;
            /* Loop over all possible starting locations.  We know how many characters are needed to match (minTextLen - patternIndex) so we can stop there. */
            while (textIndex2 < textLen - (minTextLen - (patternIndex + 1))) {
                if (wildcardMatchInner(&(text[textIndex2]), textLen - textIndex2, &(pattern[patternIndex + 1]), patternLen - (patternIndex + 1), minTextLen - patternIndex)) {
                    /* Got a match in recursion. */
                    /*log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("  wildcardMatchInner(\"%s\", %d, \"%s\", %d, %d) -> HERE1 textIndex=%d, patternIndex=%d, textIndex2=%d TRUE"), text, textLen, pattern, patternLen, minTextLen, textIndex, patternIndex, textIndex2);*/
                    return TRUE;
                } else {
                    /* Failed to match.  Try matching one more character against the '*'. */
                    textIndex2++;
                }
            }
            /* If we get here then all possible starting locations failed. */
            /*log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("  wildcardMatchInner(\"%s\", %d, \"%s\", %d, %d) -> HERE2 textIndex=%d, patternIndex=%d, textIndex2=%d FALSE"), text, textLen, pattern, patternLen, minTextLen, textIndex, patternIndex, textIndex2);*/
            return FALSE;
        } else if (patternChar == TEXT('?')) {
            /* Match any character. */
            patternIndex++;
            textIndex++;
        } else {
            textChar = text[textIndex];
            if (patternChar == textChar) {
                /* Characters match. */
                patternIndex++;
                textIndex++;
            } else {
                /* Characters do not match.  We are done. */
                /*log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("  wildcardMatchInner(\"%s\", %d, \"%s\", %d, %d) -> HERE3 textIndex=%d, patternIndex=%d FALSE"), text, textLen, pattern, patternLen, minTextLen, textIndex, patternIndex);*/
                return FALSE;
            }
        }
    }

    /* It is ok if there are text characters left over as we only need to match a substring, not the whole string. */

    /* If there are any pattern chars left.  Make sure that they are all wildcards. */
    while (patternIndex < patternLen) {
        if (pattern[patternIndex] != TEXT('*')) {
            /*log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("  wildcardMatchInner(\"%s\", %d, \"%s\", %d, %d) -> HERE4 pattern[%d]=%c FALSE"), text, textLen, pattern, patternLen, minTextLen, patternIndex, pattern[patternIndex]);*/
            return FALSE;
        }
        patternIndex++;
    }

    /*log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("  wildcardMatchInner(\"%s\", %d, \"%s\", %d, %d) -> HERE5 textIndex=%d, patternIndex=%d TRUE"), text, textLen, pattern, patternLen, minTextLen, textIndex, patternIndex);*/
    return TRUE;
}
    
/**
 * Does any necessary post processing on the command string.
 *  This function assumes that command has been malloced.  It will either return
 *  the string as is, or return a modified string.  When a modified string is
 *  returned the orignal command buffer will always be freed.
 *
 * 1) Replace the first instance of the %WRAPPER_COMMAND_FILLER_N% environment
 *    variable so that the total command length will be equal to or greater than
 *    the length specified by N.  The padding will be the length plus a series of
 *    Xs terminated by a single Y.  This is mainly for testing.
 *
 * @param command The original command.
 *
 * @return The modifed command.
 */
TCHAR *wrapperPostProcessCommandElement(TCHAR *command) {
    TCHAR *pos1;
    TCHAR *pos2;
    size_t commandLen;
    size_t commandLen2;
    size_t commandLenLen;
    TCHAR commandLenBuffer[8];
    size_t fillerLen;
    TCHAR *tempCommand;
    size_t index;
    
    /* If the special WRAPPER_COMMAND_FILLER_N environment variable is being used then expand it.
     *  This is mainly used for testing. */
    pos1 = _tcsstr(command, TEXT("%WRAPPER_COMMAND_FILLER_"));
    if (pos1 == NULL) {
        return command;
    }
    
    pos2 = _tcsstr(pos1 + 1, TEXT("%"));
    if (pos2 == NULL) {
        return command;
    }
    
    commandLen = _tcslen(command);
    commandLenLen = pos2 - pos1 - 24;
    if (commandLenLen >= 8) {
        /* Too long. invalid. */
        return command;
    }

    memcpy(commandLenBuffer, pos1 + 24, sizeof(TCHAR) * commandLenLen);
    commandLenBuffer[commandLenLen] = TEXT('\0');
    commandLen2 = __max((int)(commandLen - commandLenLen) - 25, __min(_ttoi(commandLenBuffer), 9999999));
    
    fillerLen = commandLen2 - commandLen + commandLenLen + 25;
    
    tempCommand = malloc(sizeof(TCHAR) * (commandLen - commandLenLen - 25 + fillerLen + 1));
    if (!tempCommand) {
        outOfMemory(TEXT("WBJC"), 3);
        return command;
    }
    
    memcpy(tempCommand, command, (pos1 - command) * sizeof(TCHAR));
    index = pos1 - command;
    if (fillerLen > 11) {
        _sntprintf(&(tempCommand[index]), commandLen2 + 1 - index, TEXT("FILL-%d-"), fillerLen);
        fillerLen -= _tcslen(&tempCommand[index]);
        index += _tcslen(&tempCommand[index]);
    }
    while (fillerLen > 1) {
        tempCommand[index] = TEXT('X');
        index++;
        fillerLen--;
    }
    if (fillerLen > 0) {
        tempCommand[index] = TEXT('Y');
        index++;
        fillerLen--;
    }
    memcpy(&(tempCommand[index]), pos2 + 1, sizeof(TCHAR) * _tcslen(pos2 + 1));
    tempCommand[commandLen2] = TEXT('\0');
    
    free(command);
    
    return tempCommand;
}

/**
 * Test function to pause the current thread for the specified amount of time.
 *  This is used to test how the rest of the Wrapper behaves when a particular
 *  thread blocks for any reason.
 *
 * @param pauseTime Number of seconds to pause for.  -1 will pause indefinitely.
 * @param threadName Name of the thread that will be logged prior to pausing.
 */
void wrapperPauseThread(int pauseTime, const TCHAR *threadName) {
    int i;
    
    if (pauseTime > 0) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT("Pausing the \"%s\" thread for %d seconds..."), threadName, pauseTime);
        for (i = 0; i < pauseTime; i++) {
            wrapperSleep(1000);
        }
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT("Resuming the \"%s\" thread..."), threadName);
    } else if (pauseTime < 0) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT("Pausing the \"%s\" thread indefinitely."), threadName);
        while(TRUE) {
            wrapperSleep(1000);
        }
    }
}

/**
 * Function that will recursively attempt to match two strings where the
 *  pattern can contain '?' or '*' wildcard characters.
 *
 * @param text Text to be searched.
 * @param pattern Pattern to search for.
 * @param patternLen Length of the pattern.
 * @param minTextLen Minimum number of characters that the text needs to possibly match the pattern.
 *
 * @return TRUE if found, FALSE otherwise.
 */
int wrapperWildcardMatch(const TCHAR *text, const TCHAR *pattern, size_t minTextLen) {
    size_t textLen;
    size_t patternLen;
    size_t textIndex;

    /*log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("wrapperWildcardMatch(\"%s\", \"%s\", %d)"), text, pattern, minTextLen);*/

    textLen = _tcslen(text);
    if (textLen < minTextLen) {
        return FALSE;
    }

    patternLen = _tcslen(pattern);
    /*log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("  textLen=%d, patternLen=%d"), textLen, patternLen);*/

    textIndex = 0;
    while (textIndex <= textLen - minTextLen) {
        if (wildcardMatchInner(&(text[textIndex]), textLen - textIndex, pattern, patternLen, minTextLen)) {
            return TRUE;
        }
        textIndex++;
    }

    return FALSE;
}

/**
 * Calculates the minimum text length which could be matched by the specified pattern.
 *  Patterns can contain '*' or '?' wildcards.
 *  '*' matches 0 or more characters.
 *  '?' matches exactly one character.
 *
 * @param pattern Pattern to calculate.
 *
 * @return The minimum text length of the pattern.
 */
size_t wrapperGetMinimumTextLengthForPattern(const TCHAR *pattern) {
    size_t patternLen;
    size_t patternIndex;
    size_t minLen;

    /*log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("wrapperGetMinimumTextLengthForPattern(%s)"), pattern);*/

    patternLen = _tcslen(pattern);
    minLen = 0;
    for (patternIndex = 0; patternIndex < patternLen; patternIndex++) {
        if (pattern[patternIndex] == TEXT('*')) {
            /* Matches 0 or more characters, so don't increment the minLen */
        } else {
            minLen++;
        }
    }

    /*log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("wrapperGetMinimumTextLengthForPattern(%s) -> %d"), pattern, minLen);*/

    return minLen;
}

/**
 * Trims any whitespace from the beginning and end of the in string
 *  and places the results in the out buffer.  Assumes that the out
 *  buffer is at least as large as the in buffer. */
void trim(const TCHAR *in, TCHAR *out) {
    size_t len;
    size_t first;
    size_t last;

    len = _tcslen(in);
    if (len > 0) {
        first = 0;
        last = len - 1;

        /* Left Trim */
        while (((in[first] == ' ') || (in[first] == '\t')) && (first < last)) {
            first++;
        }
        /* Right Trim */
        while ((last > first) && ((in[last] == ' ') || (in[last] == '\t'))) {
            last--;
        }

        /* Copy over what is left. */
        len = last - first + 1;
        if (len > 0) {
            _tcsncpy(out, in + first, len);
        }
    }
    out[len] = TEXT('\0');
}

void logApplyFilters(const TCHAR *log) {
    int i;
    const TCHAR *filter;
    const TCHAR *filterMessage;
    int matched;

    /* Look for output filters in the output.  Only match the first. */
    for (i = 0; i < wrapperData->outputFilterCount; i++) {
        if (_tcslen(wrapperData->outputFilters[i]) > 0) {
            /* The filter is defined. */
            matched = FALSE;
            filter = wrapperData->outputFilters[i];

            if (wrapperData->outputFilterAllowWildFlags[i]) {
                if (wrapperWildcardMatch(log, filter, wrapperData->outputFilterMinLens[i])) {
                    matched = TRUE;
                }
            } else {
                /* Do a simple check to see if the pattern is found exactly as is. */
                if (_tcsstr(log, filter)) {
                    /* Found an exact match for the pattern. */
                    /* Any wildcards in the pattern can be matched exactly if they exist in the output.  This is by design. */
                    matched = TRUE;
                }
            }

            if (matched) {
                filterMessage = wrapperData->outputFilterMessages[i];
                if ((!filterMessage) || (_tcslen(filterMessage) <= 0)) {
                    filterMessage = TEXT("Filter trigger matched.");
                }
                wrapperProcessActionList(wrapperData->outputFilterActionLists[i], filterMessage, WRAPPER_ACTION_SOURCE_CODE_FILTER, i, FALSE, wrapperData->errorExitCode);

                /* break out of the loop */
                break;
            }
        }
    }
}

/**
 * Logs a single line of child output allowing any filtering
 *  to be done in a common location.
 */
void logChildOutput(const char* log) {
    TCHAR* tlog;
#ifdef UNICODE
    int size;

 #ifdef WIN32
    TCHAR buffer[16];
    UINT cp;
 #endif
#endif

#ifdef UNICODE
 #ifdef WIN32
    GetLocaleInfo(GetThreadLocale(), LOCALE_IDEFAULTANSICODEPAGE, buffer, sizeof(buffer));
    cp = _ttoi(buffer);
    size = MultiByteToWideChar(cp, 0, log, -1 , NULL, 0);
    if (size <= 0) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                    TEXT("Invalid multibyte sequence in %s: %s"), TEXT("JVM console output"), getLastErrorText());
        return;
    }

    tlog = (TCHAR*)malloc((size + 1) * sizeof(TCHAR));
    if (!tlog) {
        outOfMemory(TEXT("WLCO"), 1);
        return;
    }
    MultiByteToWideChar(cp, 0, log, -1, tlog, size + 1);
 #else
    size = mbstowcs(NULL, log, MBSTOWCS_QUERY_LENGTH);
    if (size == (size_t)-1) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                    TEXT("Invalid multibyte sequence in %s: %s"), TEXT("JVM console output"), getLastErrorText());
        return;
    }
    tlog = malloc(sizeof(TCHAR) * (size + 1));
    if (!tlog) {
        outOfMemory(TEXT("WLCO"), 1);
        return;
    }
    mbstowcs(tlog, log, size + 1);
    tlog[size] = TEXT('\0'); /* Avoid bufferflows caused by badly encoded characters. */
 #endif
#else
    tlog = (TCHAR*)log;
#endif
    log_printf(wrapperData->jvmRestarts, LEVEL_INFO, tlog);

    /* Look for output filters in the output.  Only match the first. */
    logApplyFilters(tlog);

#ifdef UNICODE
    free(tlog);
#endif
}


/**
 * This function is for moving a buffer inside itself.
 *
 * This implementation exists because the standard memcpy is not reliable on
 *  some platforms when the source and target buffer are the same.  Most likely the
 *  problem was caused by internal optimizations, but it was leading to crashes.
 */
void safeMemCpy(char *buffer, size_t target, size_t src, size_t nbyte) {
    size_t i;
    for (i = 0; i < nbyte; i++) {
        buffer[target + i] = buffer[src + i];
    }
}

#define CHAR_LF 0x0a

/**
 * Read and process any output from the child JVM Process.
 *
 * When maxTimeMS is non-zero this function will only be allowed to run for that maximum
 *  amount of time.  This is done to make sure the calling function is allowed CPU for
 *  other activities.   When timing out for this reason when there is more data in the
 *  pipe, this function will return TRUE to let the calling code know that it should
 *  not to any unnecessary sleeps.  Otherwise FALSE will be returned.
 *
 * @param maxTimeMS The maximum number of milliseconds that this function will be allowed
 *                  to run without returning.  In reality no new reads will happen after
 *                  this time, but actual processing may take longer.
 *
 * @return TRUE if the calling code should call this function again as soon as possible.
 */
int wrapperReadChildOutput(int maxTimeMS) {
    struct timeb timeBuffer;
    time_t startTime;
    int startTimeMillis;
    time_t now;
    int nowMillis;
    time_t durr;
    char *tempBuffer;
    char *cLF;
    int currentBlockRead;
    size_t loggedOffset;
    int defer = FALSE;
    int readThisPass = FALSE;
    size_t i;

    if (!wrapperChildWorkBuffer) {
        /* Initialize the wrapperChildWorkBuffer.  Set its initial size to the block size + 1.
         *  This is so that we can always add a \0 to the end of it. */
        wrapperChildWorkBuffer = malloc(sizeof(char) * ((READ_BUFFER_BLOCK_SIZE * 2) + 1));
        if (!wrapperChildWorkBuffer) {
            outOfMemory(TEXT("WRCO"), 1);
            return FALSE;
        }
        wrapperChildWorkBufferSize = READ_BUFFER_BLOCK_SIZE * 2;
    }

    wrapperGetCurrentTime(&timeBuffer);
    startTime = now = timeBuffer.time;
    startTimeMillis = nowMillis = timeBuffer.millitm;

#ifdef DEBUG_CHILD_OUTPUT
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("wrapperReadChildOutput() BEGIN"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("now=%ld, nowMillis=%d"), now, nowMillis);
#endif

    /* Loop and read in CHILD_BLOCK_SIZE characters at a time.
     *
     * To keep a JVM outputting lots of content from freezing the Wrapper, we force a return every 250ms. */
    while ((maxTimeMS <= 0) || ((durr = (now - startTime) * 1000 + (nowMillis - startTimeMillis)) < maxTimeMS)) {
#ifdef DEBUG_CHILD_OUTPUT
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("durr=%ld"), durr);
#endif

        /* If there is not enough space in the work buffer to read in a full block then it needs to be extended. */
        if (wrapperChildWorkBufferLen + READ_BUFFER_BLOCK_SIZE > wrapperChildWorkBufferSize) {
#ifdef DEBUG_CHILD_OUTPUT
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("Expand buffer."));
#endif
            /* Increase the buffer quickly, but try not to get too big.  Increase to a size that is the
             *  greater of size + 1024 or size * 1.1.
             * Also make sure the new buffer is larger than the buffer len.  This should not be necessary
             *  but is safer. */
            wrapperChildWorkBufferSize = __max(wrapperChildWorkBufferLen + 1, __max(wrapperChildWorkBufferSize + READ_BUFFER_BLOCK_SIZE, wrapperChildWorkBufferSize + wrapperChildWorkBufferSize / 10));
            
            tempBuffer = malloc(wrapperChildWorkBufferSize + 1);
            if (!tempBuffer) {
                outOfMemory(TEXT("WRCO"), 2);
                return FALSE;
            }
            memcpy(tempBuffer, wrapperChildWorkBuffer, wrapperChildWorkBufferLen);
            tempBuffer[wrapperChildWorkBufferLen] = '\0';
            free(wrapperChildWorkBuffer);
            wrapperChildWorkBuffer = tempBuffer;
#ifdef DEBUG_CHILD_OUTPUT
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("buffer now %d bytes"), wrapperChildWorkBufferSize);
#endif
        }

#ifdef DEBUG_CHILD_OUTPUT
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("Try reading from pipe.  totalBuffLen=%d, buffSize=%d"), wrapperChildWorkBufferLen, wrapperChildWorkBufferSize);
#endif
        if (wrapperReadChildOutputBlock(wrapperChildWorkBuffer + (wrapperChildWorkBufferLen), (int)(wrapperChildWorkBufferSize - wrapperChildWorkBufferLen), &currentBlockRead)) {
            /* Error already reported. */
            return FALSE;
        }

        if (currentBlockRead > 0) {
            /* We read in a block, so increase the length. */
            wrapperChildWorkBufferLen += currentBlockRead;
            if (wrapperChildWorkIsNewLine) {
                wrapperChildWorkLastDataTime = now;
                wrapperChildWorkLastDataTimeMillis = nowMillis;
                wrapperChildWorkIsNewLine = FALSE;
            }
            readThisPass = TRUE;
#ifdef DEBUG_CHILD_OUTPUT
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("  Read %d bytes of new output.  totalBuffLen=%d, buffSize=%d"), currentBlockRead, wrapperChildWorkBufferLen, wrapperChildWorkBufferSize);
#endif
        }

        /* Terminate the string just to avoid errors.  The buffer has an extra character to handle this. */
        wrapperChildWorkBuffer[wrapperChildWorkBufferLen] = '\0';
        
        /* Loop over the contents of the buffer and try and extract as many lines as possible.
         *  Keep track of where we are to avoid unnecessary memory copies.
         *  At this point, the entire buffer will always be unlogged. */
        loggedOffset = 0;
        defer = FALSE;
        while ((wrapperChildWorkBufferLen > loggedOffset) && (!defer)) {
#ifdef DEBUG_CHILD_OUTPUT
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("Inner loop.  totalBuffLen=%d, loggedOffset=%d, unloggedBuffLen=%d, buffSize=%d"), wrapperChildWorkBufferLen, loggedOffset, wrapperChildWorkBufferLen - loggedOffset, wrapperChildWorkBufferSize);
#endif
            /* We have something in the buffer.  Loop and see if we have a complete line to log.
             * We will always find a LF at the end of the line.  On Windows there may be a CR immediately before it. */
            cLF = NULL;
            for (i = loggedOffset; i < wrapperChildWorkBufferLen; i++) {
                /* If there is a null character, replace it with a question mark (\0 is not a termination character in Java). */
                if (wrapperChildWorkBuffer[i] == 0) {
                    wrapperChildWorkBuffer[i] = '?';
                } else if (wrapperChildWorkBuffer[i] == (char)CHAR_LF) {
                    cLF = &wrapperChildWorkBuffer[i];
                    break;
                }
            }
            
            if (cLF != NULL) {
                /* We found a valid LF so we know that a full line is ready to be logged. */
#ifdef WIN32
                if ((cLF > wrapperChildWorkBuffer) && ((cLF - sizeof(char))[0] == 0x0d)) {
 #ifdef DEBUG_CHILD_OUTPUT
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("Found CR+LF"));
 #endif
                    /* Replace the CR with a NULL */
                    (cLF - sizeof(char))[0] = 0;
                } else {
#endif
#ifdef DEBUG_CHILD_OUTPUT
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("Found LF"));
#endif
#ifdef WIN32
                }
#endif
                /* Replace the LF with a NULL */
                cLF[0] = '\0';

                /* We have a string to log. */
#ifdef DEBUG_CHILD_OUTPUT
 #ifdef UNICODE
                /* It is not easy to log the string as is because they are not wide chars. Send it only to stdout. */
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("Log: (see stdout)"));
  #ifdef WIN32
                wprintf(TEXT("Log: [%S]\n"), wrapperChildWorkBuffer + loggedOffset);
  #else
                wprintf(TEXT("Log: [%s]\n"), wrapperChildWorkBuffer + loggedOffset);
  #endif
 #else
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("Log: [%s]"), wrapperChildWorkBuffer + loggedOffset);
 #endif
#endif
                /* Actually log the individual line of output. */
                logChildOutput(wrapperChildWorkBuffer + loggedOffset);
                
                /* Update the offset so we know how far we've logged. */
                loggedOffset = cLF - wrapperChildWorkBuffer + 1;
                wrapperChildWorkIsNewLine = TRUE;
#ifdef DEBUG_CHILD_OUTPUT
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("loggedOffset: %d"), loggedOffset);
#endif
            } else {
                /* If we read this pass or if the last character is a CR on Windows then we always want to defer. */
                if (readThisPass
#ifdef WIN32
                        || (wrapperChildWorkBuffer[wrapperChildWorkBufferLen - 1] == 0x0d)
#endif
                        /* Avoid dumping partial lines because we call this funtion too quickly more than once.
                         *  Never let the line be partial unless more than the LF-Delay threshold has expired. */
                        || (wrapperData->logLFDelayThreshold == 0)
                        || (((now - wrapperChildWorkLastDataTime) * 1000 + (nowMillis - wrapperChildWorkLastDataTimeMillis)) < wrapperData->logLFDelayThreshold)
                    ) {
#ifdef DEBUG_CHILD_OUTPUT
 #ifdef UNICODE
                    /* It is not easy to log the string as is because they are not wide chars. Send it only to stdout. */
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("Incomplete line.  Defer: (see stdout)  Age: %d"),
                        (now - wrapperChildWorkLastDataTime) * 1000 + (nowMillis - wrapperChildWorkLastDataTimeMillis));
  #ifdef WIN32
                    wprintf(TEXT("Defer Log: [%S]\n"), wrapperChildWorkBuffer + loggedOffset);
  #else
                    wprintf(TEXT("Defer Log: [%s]\n"), wrapperChildWorkBuffer + loggedOffset);
  #endif
 #else
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("Incomplete line.  Defer: [%s]  Age: %d"), wrapperChildWorkBuffer,
                        (now - wrapperChildWorkLastDataTime) * 1000 + (nowMillis - wrapperChildWorkLastDataTimeMillis));
 #endif
#endif
                    defer = TRUE;
                } else {
                    /* We have an incomplete line, but it was from a previous pass and is old enough, so we want to log it as it may be a prompt.
                     *  This will always be the complete buffer. */
#ifdef DEBUG_CHILD_OUTPUT
 #ifdef UNICODE
                    /* It is not easy to log the string as is because they are not wide chars. Send it only to stdout. */
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("Incomplete line, but log now: (see stdout)  Age: %d"),
                        (now - wrapperChildWorkLastDataTime) * 1000 + (nowMillis - wrapperChildWorkLastDataTimeMillis));
  #ifdef WIN32
                    wprintf(TEXT("Log: [%S]\n"), wrapperChildWorkBuffer + loggedOffset);
  #else
                    wprintf(TEXT("Log: [%s]\n"), wrapperChildWorkBuffer + loggedOffset);
  #endif
 #else
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("Incomplete line, but log now: [%s]  Age: %d"), wrapperChildWorkBuffer,
                        (now - wrapperChildWorkLastDataTime) * 1000 + (nowMillis - wrapperChildWorkLastDataTimeMillis));
 #endif
#endif
                    logChildOutput(wrapperChildWorkBuffer + loggedOffset);
                    
                    /* We know we read everything so we can safely reset the loggedOffset and clear the buffer. */
                    wrapperChildWorkBuffer[0] = '\0';
                    wrapperChildWorkBufferLen = 0;
                    loggedOffset = 0;
                    wrapperChildWorkIsNewLine = TRUE;
                }
            }
        }
        
        /* We have read as many lines from the buffered output as possible.
         *  If we still have any partial lines, then we need to make sure they are moved to the beginning of the buffer so we can read in another block. */
        if (loggedOffset > 0) {
            if (loggedOffset >= wrapperChildWorkBufferLen) {
                /* We know we have read everything in.  So we can efficiently clear the buffer. */
#ifdef DEBUG_CHILD_OUTPUT
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("Cleared Buffer as everything was logged."));
#endif
                wrapperChildWorkBuffer[0] = '\0';
                wrapperChildWorkBufferLen = 0;
                /* loggedOffset = 0; Not needed. */
            } else {
                /* We have logged one or more lines from the buffer, but unlogged content still exists.  It needs to be moved to the head of the buffer. */
#ifdef DEBUG_CHILD_OUTPUT
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("Moving %d bytes in buffer for next cycle."), wrapperChildWorkBufferLen - loggedOffset);
#endif
                /* NOTE - This line intentionally does the copy within the same memory space.  It is safe the way it is working however. */
                wrapperChildWorkBufferLen = wrapperChildWorkBufferLen - loggedOffset;
                safeMemCpy(wrapperChildWorkBuffer, 0, loggedOffset, wrapperChildWorkBufferLen);
                /* Shouldn't be needed, but just to make sure the buffer has been ended properly */
                wrapperChildWorkBuffer[wrapperChildWorkBufferLen] = 0;
                /* loggedOffset = 0; Not needed. */
            }
        } else {
        }

        if (currentBlockRead <= 0) {
            /* All done for now. */
            if (wrapperChildWorkBufferLen > 0) {
#ifdef DEBUG_CHILD_OUTPUT
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("wrapperReadChildOutput() END (Incomplete)"));
#endif
            } else {
#ifdef DEBUG_CHILD_OUTPUT
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("wrapperReadChildOutput() END"));
#endif
            }
            return FALSE;
        }
        
        /* Get the time again */
        wrapperGetCurrentTime(&timeBuffer);
        now = timeBuffer.time;
        nowMillis = timeBuffer.millitm;
    }

    /* If we got here then we timed out. */
#ifdef DEBUG_CHILD_OUTPUT
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("wrapperReadChildOutput() END TIMEOUT"));
#endif
    return TRUE;
}

/**
 * Immediately after a JVM is launched and whenever the log file name changes,
 *  the log file name is sent to the JVM where it can be referenced by applications.
 */
void sendLogFileName() {
    TCHAR *currentLogFilePath;

    currentLogFilePath = getCurrentLogfilePath();

    if (currentLogFilePath) {
        wrapperProtocolFunction(WRAPPER_MSG_LOGFILE, currentLogFilePath);
        free(currentLogFilePath);
    }
}

/**
 * Immediately after a JVM is launched, the wrapper configuration is sent to the
 *  JVM where it can be used as a properties object.
 */
void sendProperties() {
    TCHAR *buffer;

    buffer = linearizeProperties(properties, TEXT('\t'));
    if (buffer) {
        wrapperProtocolFunction(WRAPPER_MSG_PROPERTIES, buffer);

        free(buffer);
    }
}

/**
 * Common cleanup code which should get called when we first decide that the JVM was down.
 */
void wrapperJVMDownCleanup(int setState) {
    /* Only set the state to DOWN_CHECK if we are not already in a state which reflects this. */
    if (setState) {
        if (wrapperData->jvmCleanupTimeout > 0) {
            wrapperSetJavaState(WRAPPER_JSTATE_DOWN_CHECK, wrapperGetTicks(), wrapperData->jvmCleanupTimeout);
        } else {
            wrapperSetJavaState(WRAPPER_JSTATE_DOWN_CHECK, wrapperGetTicks(), -1);
        }
    }

    /* Remove java pid file if it was registered and created by this process. */
    if (wrapperData->javaPidFilename) {
        _tunlink(wrapperData->javaPidFilename);
    }

#ifdef WIN32
    if (!CloseHandle(wrapperData->javaProcess)) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
            TEXT("Failed to close the Java process handle: %s"), getLastErrorText());
    }
    wrapperData->javaProcess = NULL;
#endif

    /* Close any open socket to the JVM */
    if (wrapperData->stoppedPacketReceived) {
        wrapperProtocolClose();
    } else {
        /* Leave the socket open so the Wrapper has the chance to read any outstanding packets. */
    }
}

/**
 * Immediately kill the JVM process and set the JVM state to
 *  WRAPPER_JSTATE_DOWN_CHECK.
 */
int wrapperKillProcessNow() {
#ifdef WIN32
    int ret;
#endif
    TCHAR errorMessage[512];

    /* Check to make sure that the JVM process is still running */
#ifdef WIN32
    ret = WaitForSingleObject(wrapperData->javaProcess, 0);
    if (ret == WAIT_TIMEOUT) {
#else
    if (waitpid(wrapperData->javaPID, NULL, WNOHANG) == 0) {
#endif
        /* JVM is still up when it should have already stopped itself. */

        /* The JVM process is not responding so the only choice we have is to
         *  kill it. */
#ifdef WIN32
        /* The TerminateProcess funtion will kill the process, but it
         *  does not correctly notify the process's DLLs that it is shutting
         *  down.  Ideally, we would call ExitProcess, but that can only be
         *  called from within the process being killed. */
        if (TerminateProcess(wrapperData->javaProcess, 0)) {
    
#else
        if (kill(wrapperData->javaPID, SIGKILL) == 0) { 
#endif
            if (!wrapperData->jvmSilentKill) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("JVM did not exit on request, termination requested."));
            }
            return FALSE;
        } else {
            if (!wrapperData->jvmSilentKill) {
                _sntprintf(errorMessage, 512, TEXT("  Attempt to terminate process failed: %s"), getLastErrorText());
                errorMessage[511] = 0;
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("JVM did not exit on request."));
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, errorMessage);
            } else {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                    TEXT("Attempt to terminate the JVM failed: %s"), getLastErrorText());
            }
            /* Terminating the current JVM failed. Cancel pending restart requests */
            wrapperJVMDownCleanup(TRUE);
            wrapperData->exitCode = wrapperData->errorExitCode;
            return TRUE;
        }
    }
    wrapperJVMDownCleanup(TRUE);
    return FALSE;
}

/**
 * Puts the Wrapper into a state where the JVM will be killed at the soonest
 *  possible opportunity.  It is necessary to wait a moment if a final thread
 *  dump is to be requested.  This call will always set the JVM state to
 *  WRAPPER_JSTATE_KILLING.
 *
 * @param silent TRUE to skip messages saying that the JVM did not exit on request.
 *               This is useful in certain cases where we kill the JVM without trying
 *               to shut it down cleanly.
 */
void wrapperKillProcess(int silent) {
#ifdef WIN32
    int ret;
#endif
    int delay = 0;

    if ((wrapperData->jState == WRAPPER_JSTATE_DOWN_CLEAN) ||
        (wrapperData->jState == WRAPPER_JSTATE_LAUNCH_DELAY) ||
        (wrapperData->jState == WRAPPER_JSTATE_DOWN_CHECK) ||
        (wrapperData->jState == WRAPPER_JSTATE_DOWN_FLUSH)) {
        /* Already down. */
        if (wrapperData->jState == WRAPPER_JSTATE_LAUNCH_DELAY) {
            wrapperSetJavaState(WRAPPER_JSTATE_DOWN_CLEAN, wrapperGetTicks(), 0);
        }
        return;
    }

    /* Check to make sure that the JVM process is still running */
#ifdef WIN32
    ret = WaitForSingleObject(wrapperData->javaProcess, 0);
    if (ret == WAIT_TIMEOUT) {
#else
    if (waitpid(wrapperData->javaPID, NULL, WNOHANG) == 0) {
#endif
        /* JVM is still up when it should have already stopped itself. */
        if (wrapperData->requestThreadDumpOnFailedJVMExit) {
            wrapperRequestDumpJVMState();

            delay = wrapperData->requestThreadDumpOnFailedJVMExitDelay;
        }
    }

    wrapperSetJavaState(WRAPPER_JSTATE_KILLING, wrapperGetTicks(), delay);
    wrapperData->jvmSilentKill = silent;
}


/**
 * Add some checks of the properties to try to catch the case where the user is making use of TestWrapper scripts.
 *
 * @return TRUE if there is such a missconfiguration.  FALSE if all is Ok.
 */
int checkForTestWrapperScripts() {
    const TCHAR* prop;

    prop = getStringProperty(properties, TEXT("wrapper.java.mainclass"), NULL);
    if (prop) {
        if (_tcscmp(prop, TEXT("org.tanukisoftware.wrapper.test.Main")) == 0) {
            /* This is the TestWrapper app.  So don't check. */
        } else {
            /* This is a user application, so make sure that they are not using the TestWrapper scripts. */
            prop = getStringProperty(properties, TEXT("wrapper.app.parameter.2"), NULL);
            if (prop) {
                if (_tcscmp(prop, TEXT("{{TestWrapperBat}}")) == 0) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT(
                        ""));
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT(
                        "--------------------------------------------------------------------"));
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT(
                        "We have detected that you are making use of the sample batch files\nthat are designed for the TestWrapper Example Application.  When\nsetting up your own application, please copy fresh files over from\nthe Wrapper's src\\bin directory."));
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT(
                        ""));
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT(
                        "Shutting down as this will likely cause problems with your\napplication startup."));
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT(
                        ""));
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT(
                        "Please see the integration section of the documentation for more\ninformation."));
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT(
                        "  http://wrapper.tanukisoftware.com/integrate"));
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT(
                        "--------------------------------------------------------------------"));
                    return TRUE;
                } else if (_tcscmp(prop, TEXT("{{TestWrapperSh}}")) == 0) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT(
                        ""));
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT(
                        "--------------------------------------------------------------------"));
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT(
                        "We have detected that you are making use of the sample shell scripts\nthat are designed for the TestWrapper Example Application.  When\nsetting up your own application, please copy fresh files over from\nthe Wrapper's src/bin directory."));
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT(
                        ""));
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT(
                        "Shutting down as this will likely cause problems with your\napplication startup."));
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT(
                        ""));
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT(
                        "Please see the integration section of the documentation for more\ninformation."));
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT(
                        "  http://wrapper.tanukisoftware.com/integrate"));
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT(
                        "--------------------------------------------------------------------"));
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

#ifdef WIN32

/**
 * Creates a human readable representation of the Windows OS the Wrapper is run on.
 *
 * @param pszOS the buffer the information gets stored to
 * @return FALSE if error or no information could be retrieved. TRUE otherwise.
 */
BOOL GetOSDisplayString(TCHAR** pszOS) {
    OSVERSIONINFOEX osvi;
    SYSTEM_INFO si;
    FARPROC pGNSI;
    FARPROC pGPI;
    DWORD dwType;
    TCHAR buf[80];

    ZeroMemory(&si, sizeof(SYSTEM_INFO));
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

#pragma warning(push)
#pragma warning(disable : 4996) /* Visual Studio 2013 deprecates GetVersionEx but we still want to use it. */
    if (!GetVersionEx((OSVERSIONINFO*) &osvi)) {
         return FALSE;
    }
#pragma warning(pop)

    /* Call GetNativeSystemInfo if supported or GetSystemInfo otherwise.*/

    pGNSI = GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetNativeSystemInfo");
    if (NULL != pGNSI) {
        pGNSI(&si);
    } else {
        GetSystemInfo(&si);
    }

    if ((VER_PLATFORM_WIN32_NT == osvi.dwPlatformId) && (osvi.dwMajorVersion > 4)) {
        _tcsncpy(*pszOS, TEXT("Microsoft "), OSBUFSIZE);

        /* Test for the specific product. */
        if (osvi.dwMajorVersion == 10) {
            if (osvi.dwMinorVersion == 0 ) {
                if (osvi.wProductType == VER_NT_WORKSTATION) {
                    _tcsncat(*pszOS, TEXT("Windows 10 "), OSBUFSIZE);
                } else {
                    _tcsncat(*pszOS, TEXT("Windows Server 2016 "), OSBUFSIZE);
                }
            }
        } else if (osvi.dwMajorVersion == 6) {
            if (osvi.dwMinorVersion == 0 ) {
                if (osvi.wProductType == VER_NT_WORKSTATION) {
                    _tcsncat(*pszOS, TEXT("Windows Vista "), OSBUFSIZE);
                } else {
                    _tcsncat(*pszOS, TEXT("Windows Server 2008 "), OSBUFSIZE);
                }
            } else if (osvi.dwMinorVersion == 1) {
                if (osvi.wProductType == VER_NT_WORKSTATION) {
                    _tcsncat(*pszOS, TEXT("Windows 7 "), OSBUFSIZE);
                } else {
                    _tcsncat(*pszOS, TEXT("Windows Server 2008 R2 "), OSBUFSIZE);
                }
            } else if ( osvi.dwMinorVersion == 2 ) {
                if( osvi.wProductType == VER_NT_WORKSTATION ) {
                    _tcsncat(*pszOS, TEXT("Windows 8 "), OSBUFSIZE);
                } else {
                    _tcsncat(*pszOS, TEXT("Windows Server 2012 "), OSBUFSIZE);
                }
            } else if ( osvi.dwMinorVersion == 3 ) {
                if( osvi.wProductType == VER_NT_WORKSTATION ) {
                    _tcsncat(*pszOS, TEXT("Windows 8.1 "), OSBUFSIZE);
                } else {
                    _tcsncat(*pszOS, TEXT("Windows Server 2012 R2 "), OSBUFSIZE);
                }
            }
        } else if ((osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion == 2)) {
            if (GetSystemMetrics(89)) {
                _tcsncat(*pszOS, TEXT("Windows Server 2003 R2, "), OSBUFSIZE);
            } else if (osvi.wSuiteMask & 8192) {
                _tcsncat(*pszOS, TEXT("Windows Storage Server 2003"), OSBUFSIZE);
            } else if (osvi.wSuiteMask & 32768) {
                _tcsncat(*pszOS, TEXT("Windows Home Server"), OSBUFSIZE);
            } else if (osvi.wProductType == VER_NT_WORKSTATION && si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64) {
                _tcsncat(*pszOS, TEXT("Windows XP Professional x64 Edition"), OSBUFSIZE);
            } else {
                _tcsncat(*pszOS, TEXT("Windows Server 2003, "), OSBUFSIZE);
            }

            /* Test for the server type. */
            if (osvi.wProductType != VER_NT_WORKSTATION) {
                if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64) {
                    if (osvi.wSuiteMask & VER_SUITE_DATACENTER) {
                        _tcsncat(*pszOS, TEXT("Datacenter Edition for Itanium-based Systems"), OSBUFSIZE);
                    } else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE) {
                        _tcsncat(*pszOS, TEXT("Enterprise Edition for Itanium-based Systems"), OSBUFSIZE);
                    }
                } else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
                    if (osvi.wSuiteMask & VER_SUITE_DATACENTER) {
                        _tcsncat(*pszOS, TEXT("Datacenter x64 Edition"), OSBUFSIZE);
                    } else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE) {
                        _tcsncat(*pszOS, TEXT("Enterprise x64 Edition"), OSBUFSIZE);
                    } else {
                        _tcsncat(*pszOS, TEXT("Standard x64 Edition"), OSBUFSIZE);
                    }
                } else {
                    if (osvi.wSuiteMask & VER_SUITE_COMPUTE_SERVER) {
                        _tcsncat(*pszOS, TEXT("Compute Cluster Edition"), OSBUFSIZE);
                    } else if (osvi.wSuiteMask & VER_SUITE_DATACENTER) {
                        _tcsncat(*pszOS, TEXT("Datacenter Edition"), OSBUFSIZE);
                    } else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE) {
                        _tcsncat(*pszOS, TEXT("Enterprise Edition"), OSBUFSIZE);
                    } else if (osvi.wSuiteMask & VER_SUITE_BLADE) {
                        _tcsncat(*pszOS, TEXT("Web Edition" ), OSBUFSIZE);
                    } else {
                        _tcsncat(*pszOS, TEXT("Standard Edition"), OSBUFSIZE);
                    }
                }
            }
        } else if ((osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion == 1)) {
            _tcsncat(*pszOS, TEXT("Windows XP "), OSBUFSIZE);
            if (osvi.wSuiteMask & VER_SUITE_PERSONAL) {
                _tcsncat(*pszOS, TEXT("Home Edition"), OSBUFSIZE);
            } else {
                _tcsncat(*pszOS, TEXT("Professional"), OSBUFSIZE);
            }
        } else if ((osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion == 0)) {
            _tcsncat(*pszOS, TEXT("Windows 2000 "), OSBUFSIZE);
            if (osvi.wProductType == VER_NT_WORKSTATION) {
                _tcsncat(*pszOS, TEXT("Professional"), OSBUFSIZE);
            } else {
                if (osvi.wSuiteMask & VER_SUITE_DATACENTER) {
                    _tcsncat(*pszOS, TEXT("Datacenter Server"), OSBUFSIZE);
                } else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE) {
                    _tcsncat(*pszOS, TEXT("Advanced Server"), OSBUFSIZE);
                } else {
                    _tcsncat(*pszOS, TEXT("Server"), OSBUFSIZE);
                }
            }
        }

        if (osvi.dwMajorVersion >= 6) {
            pGPI = GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetProductInfo");

            pGPI(osvi.dwMajorVersion, osvi.dwMinorVersion, 0, 0, &dwType);

            switch (dwType) {
                case 1:
                    _tcsncat(*pszOS, TEXT("Ultimate Edition" ), OSBUFSIZE);
                    break;
                case 48:
                    _tcsncat(*pszOS, TEXT("Professional"), OSBUFSIZE);
                    break;
                case 3:
                    _tcsncat(*pszOS, TEXT("Home Premium Edition"), OSBUFSIZE);
                    break;
                case 67:
                    _tcsncat(*pszOS, TEXT("Home Basic Edition"), OSBUFSIZE);
                    break;
                case 4:
                    _tcsncat(*pszOS, TEXT("Enterprise Edition"), OSBUFSIZE);
                    break;
                case 6:
                    _tcsncat(*pszOS, TEXT("Business Edition"), OSBUFSIZE);
                    break;
                case 11:
                    _tcsncat(*pszOS, TEXT("Starter Edition"), OSBUFSIZE);
                    break;
                case 18:
                    _tcsncat(*pszOS, TEXT("Cluster Server Edition"), OSBUFSIZE);
                    break;
                case 8:
                    _tcsncat(*pszOS, TEXT("Datacenter Edition"), OSBUFSIZE);
                    break;
                case 12:
                    _tcsncat(*pszOS, TEXT("Datacenter Edition (core installation)"), OSBUFSIZE);
                    break;
                case 10:
                    _tcsncat(*pszOS, TEXT("Enterprise Edition"), OSBUFSIZE);
                    break;
                case 14:
                    _tcsncat(*pszOS, TEXT("Enterprise Edition (core installation)"), OSBUFSIZE);
                    break;
                case 15:
                    _tcsncat(*pszOS, TEXT("Enterprise Edition for Itanium-based Systems"), OSBUFSIZE);
                    break;
                case 9:
                    _tcsncat(*pszOS, TEXT("Small Business Server"), OSBUFSIZE);
                    break;
                case 25:
                    _tcsncat(*pszOS, TEXT("Small Business Server Premium Edition"), OSBUFSIZE);
                    break;
                case 7:
                    _tcsncat(*pszOS, TEXT("Standard Edition"), OSBUFSIZE);
                    break;
                case 13:
                    _tcsncat(*pszOS, TEXT("Standard Edition (core installation)"), OSBUFSIZE);
                    break;
                case 17:
                    _tcsncat(*pszOS, TEXT("Web Server Edition"), OSBUFSIZE);
                    break;
                case 101:
                    _tcsncat(*pszOS, TEXT("Home"), OSBUFSIZE);
                    break;
                case 98:
                    _tcsncat(*pszOS, TEXT("Home N"), OSBUFSIZE);
                    break;
                case 99:
                    _tcsncat(*pszOS, TEXT("Home China"), OSBUFSIZE);
                    break;
                case 100:
                    _tcsncat(*pszOS, TEXT("Home Single Language"), OSBUFSIZE);
                    break;
                case 104:
                    _tcsncat(*pszOS, TEXT("Mobile"), OSBUFSIZE);
                    break;
                case 133:
                    _tcsncat(*pszOS, TEXT("Mobile Enterprise"), OSBUFSIZE);
                    break;
                case 121:
                    _tcsncat(*pszOS, TEXT("Education"), OSBUFSIZE);
                    break;
                case 122:
                    _tcsncat(*pszOS, TEXT("Education N"), OSBUFSIZE);
                    break;
                case 70:
                    _tcsncat(*pszOS, TEXT("Enterprise E"), OSBUFSIZE);
                    break;
                case 84:
                    _tcsncat(*pszOS, TEXT("Enterprise N (evaluation installation)"), OSBUFSIZE);
                    break;
                case 27:
                    _tcsncat(*pszOS, TEXT("Enterprise N"), OSBUFSIZE);
                    break;
                case 72:
                    _tcsncat(*pszOS, TEXT("Enterprise (evaluation installation)"), OSBUFSIZE);
                    break;
            }
        }

        /* Include service pack (if any) and build number. */
        if (_tcslen(osvi.szCSDVersion) > 0) {
            _tcsncat(*pszOS, TEXT(" "), OSBUFSIZE);
            _tcsncat(*pszOS, osvi.szCSDVersion, OSBUFSIZE);
        }
        _sntprintf(buf, 80, TEXT(" (build %d)"), osvi.dwBuildNumber);
        _tcsncat(*pszOS, buf, OSBUFSIZE);

        if (osvi.dwMajorVersion >= 6) {
            if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
                _tcsncat(*pszOS, TEXT(", 64-bit"), OSBUFSIZE);
            } else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
                _tcsncat(*pszOS, TEXT(", 32-bit"), OSBUFSIZE);
            }
        }
        return TRUE;
    } else {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Unknown Windows Version"));
        return FALSE;
    }
}
#endif

/**
 * Launch common setup code.
 */
int wrapperRunCommonInner() {
    const TCHAR *prop;
#ifdef WIN32
    TCHAR* szOS;
#endif
    struct tm timeTM;
    TCHAR* tz1;
    TCHAR* tz2;
#if defined(UNICODE)
    size_t req;
#endif

    /* Make sure the tick timer is working correctly. */
    if (wrapperTickAssertions()) {
        return 1;
    }

    /* Log a startup banner. */
    wrapperVersionBanner();

    /* The following code will display a licensed to block if a license key is found
     *  in the Wrapper configuration.  This piece of code is required as is for
     *  Development License owners to be in complience with their development license.
     *  This code does not do any validation of the license keys and works differently
     *  from the license code found in the Standard and Professional Editions of the
     *  Wrapper. */
    prop = getStringProperty(properties, TEXT("wrapper.license.type"), TEXT(""));
    if (strcmpIgnoreCase(prop, TEXT("DEV")) == 0) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
            TEXT("  Licensed to %s for %s"),
            getStringProperty(properties, TEXT("wrapper.license.licensee"), TEXT("(LICENSE INVALID)")),
            getStringProperty(properties, TEXT("wrapper.license.dev_application"), TEXT("(LICENSE INVALID)")));
    }
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT(""));

    if (checkForTestWrapperScripts()) {
        return 1;
    }

#ifdef WIN32
    if (initializeStartup()) {
        return 1;
    }
#endif

    if (wrapperData->isDebugging) {
        timeTM = wrapperGetReleaseTime();
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Release time: %04d/%02d/%02d %02d:%02d:%02d"),
                timeTM.tm_year + 1900, timeTM.tm_mon + 1, timeTM.tm_mday,
                timeTM.tm_hour, timeTM.tm_min, timeTM.tm_sec );

        timeTM = wrapperGetBuildTime();
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Build time:   %04d/%02d/%02d %02d:%02d:%02d"),
                timeTM.tm_year + 1900, timeTM.tm_mon + 1, timeTM.tm_mday,
                timeTM.tm_hour, timeTM.tm_min, timeTM.tm_sec );

        /* Display timezone information. */
        tzset();
#if defined(UNICODE)
 #if !defined(WIN32)
        req = mbstowcs(NULL, tzname[0], MBSTOWCS_QUERY_LENGTH);
        if (req == (size_t)-1) {
            return 1;
        }
        tz1 = malloc(sizeof(TCHAR) * (req + 1));
        if (!tz1) {
            outOfMemory(TEXT("LHN"), 1);
        } else {
            mbstowcs(tz1, tzname[0], req + 1);
            tz1[req] = TEXT('\0'); /* Avoid bufferflows caused by badly encoded characters. */
            
            req = mbstowcs(NULL, tzname[1], MBSTOWCS_QUERY_LENGTH);
            if (req == (size_t)-1) {
                free(tz1);
                return 1;
            }
            tz2 = malloc(sizeof(TCHAR) * (req + 1));
            if (!tz2) {
                outOfMemory(TEXT("LHN"), 2);
                free(tz1);
            } else {
                mbstowcs(tz2, tzname[1], req + 1);
                tz2[req] = TEXT('\0'); /* Avoid bufferflows caused by badly encoded characters. */
 #else
        req = MultiByteToWideChar(CP_OEMCP, 0, tzname[0], -1, NULL, 0);
        if (req <= 0) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                TEXT("Invalid multibyte sequence in port address \"%s\" : %s"), tzname[0], getLastErrorText());
            return 1;
        }

        tz1 = malloc((req + 1) * sizeof(TCHAR));
        if (!tz1) {
            outOfMemory(TEXT("LHN"), 1);
        } else {
            MultiByteToWideChar(CP_OEMCP,0, tzname[0], -1, tz1, (int)req + 1);
            req = MultiByteToWideChar(CP_OEMCP, 0, tzname[1], -1, NULL, 0);
            if (req <= 0) {
                free(tz1);
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                    TEXT("Invalid multibyte sequence in port address \"%s\" : %s"), tzname[1], getLastErrorText());
                return 1;
            }
            tz2 = malloc((req  + 1) * sizeof(TCHAR));
            if (!tz2) {
                free(tz1);
                outOfMemory(TEXT("LHN"), 2);
            } else {
                MultiByteToWideChar(CP_OEMCP,0, tzname[1], -1, tz2, (int)req + 1);
 #endif

#else
        tz1 = tzname[0];
        tz2 = tzname[1];
#endif
#ifndef FREEBSD
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Timezone:     %s (%s) Offset: %ld, hasDaylight: %d"),
                        tz1, tz2, timezone, daylight);
#else
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Timezone:     %s (%s) Offset: %ld"),
                        tz1, tz2, timezone);
#endif
                if (wrapperData->useSystemTime) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Using system timer."));
                } else {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Using tick timer."));
                }
#ifdef UNICODE
                free(tz1);
                free(tz2);
            }
        }
#endif
        
        /* Log the Wrapper's PID. */
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("PID:          %d"), wrapperData->wrapperPID);
    }

#ifdef WIN32
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Current User: %s  Domain: %s"), (wrapperData->userName ? wrapperData->userName : TEXT("N/A")), (wrapperData->domainName ? wrapperData->domainName : TEXT("N/A")));
        szOS = calloc(OSBUFSIZE, sizeof(TCHAR));
        if (szOS) {
            if (GetOSDisplayString(&szOS)) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Operating System ID: %s"), szOS);
            }
            free(szOS);
        }
        
        if (isCygwin()) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Cygwin detected"));
        }
        
        if (_tcscmp(wrapperBits, TEXT("32")) == 0) {
            if (wrapperData->DEPStatus) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("DEP status: Enabled"));
            } else if (!wrapperData->DEPApiAvailable) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("DEP status: Not supported"));
            } else if (wrapperData->DEPError == 5) {
                /* If the operating system is configured to always use DEP for all processes,
                 *  then SetProcessDEPPolicy() will return an Access Denied Error (5), but 
                 *  that doesn't mean DEP is Disabled.*/
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("DEP status: Unchanged (set by the OS)"));
            } else if (wrapperData->DEPError) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("DEP status: Disabled (0x%x)"), wrapperData->DEPError);
            } else {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("DEP status: Disabled"));
            }
        }
    }
#endif

#ifdef FREEBSD
    /* log the iconv library in use. */
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Iconv library: %s"), getIconvLibName());
    }
#endif

    /* Dump the configured properties */
    dumpProperties(properties);

    /* Should we dump the environment variables?
     * If the the user specifically wants the environment, show it as the status log level, otherwise include it in debug output if enabled. */
    if (getBooleanProperty(properties, TEXT("wrapper.environment.dump"), FALSE)) {
        dumpEnvironment(LEVEL_INFO);
    } else if (wrapperData->isDebugging) {
        dumpEnvironment(LEVEL_DEBUG);
    }
    
#ifndef WIN32
    showResourceslimits();
#endif

#ifdef _DEBUG
    /* Multi-line logging tests. */
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("----- Should be 5 lines -----"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("\nLINE2:\n\nLINE4:\n"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("----- Next is one line ------"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT(""));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("----- Next is two lines -----"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("\n"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("----- Next is two lines -----"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("ABC\nDEF"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("-----------------------------"));
#endif

#ifdef WRAPPER_FILE_DEBUG
    wrapperFileTests();
#endif

    return 0;
}

int wrapperRunCommon(const TCHAR *runMode) {
    int exitCode;

    /* Setup the wrapperData structure. */
    wrapperSetWrapperState(WRAPPER_WSTATE_STARTING);
    wrapperSetJavaState(WRAPPER_JSTATE_DOWN_CLEAN, 0, -1);

    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("--> Wrapper Started as %s"), runMode);

    /* Initialize the wrapper */
    exitCode = wrapperInitializeRun();
    if (exitCode == 0) {
        if (!wrapperRunCommonInner()) {
            /* Enter main event loop */
            wrapperEventLoop();
        
            /* Clean up any open sockets. */
            wrapperProtocolClose();
            protocolStopServer();
            
            exitCode = wrapperData->exitCode;
        } else {
            exitCode = wrapperData->errorExitCode;
        }
    } else {
        exitCode = wrapperData->errorExitCode;
    }

    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("<-- Wrapper Stopped"));

    return exitCode;
}
        
/**
 * Launch the wrapper as a console application.
 */
int wrapperRunConsole() {
    return wrapperRunCommon(TEXT("Console"));
}

/**
 * Launch the wrapper as a service application.
 */
int wrapperRunService() {
    return wrapperRunCommon(
#ifdef WIN32
        TEXT("Service")
#else
        TEXT("Daemon")
#endif
        );
}

/**
 * Used to ask the state engine to shut down the JVM and Wrapper.
 *
 * @param exitCode Exit code to use when shutting down.
 * @param force True to force the Wrapper to shutdown even if some configuration
 *              had previously asked that the JVM be restarted.  This will reset
 *              any existing restart requests, but it will still be possible for
 *              later actions to request a restart.
 */
void wrapperStopProcess(int exitCode, int force) {
    /* If we are pausing or paused, cancel it. */
    if ((wrapperData->wState == WRAPPER_WSTATE_PAUSING) ||
        (wrapperData->wState == WRAPPER_WSTATE_PAUSED)) {
        wrapperSetWrapperState(WRAPPER_WSTATE_STARTED);
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                TEXT("wrapperStopProcess(%d, %s) called while pausing or being paused."), exitCode, (force ? TEXT("TRUE") : TEXT("FALSE")));
        }
    }
    
    /* If we are are not aready shutting down, then do so. */
    if ((wrapperData->wState == WRAPPER_WSTATE_STOPPING) ||
        (wrapperData->wState == WRAPPER_WSTATE_STOPPED)) {
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                TEXT("wrapperStopProcess(%d, %s) called while stopping.  (IGNORED)"), exitCode, (force ? TEXT("TRUE") : TEXT("FALSE")));
        }
    } else {
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                TEXT("wrapperStopProcess(%d, %s) called."), exitCode, (force ? TEXT("TRUE") : TEXT("FALSE")));
        }
        /* If it has not already been set, set the exit request flag. */
        if (wrapperData->exitRequested ||
            (wrapperData->jState == WRAPPER_JSTATE_DOWN_CLEAN) ||
            (wrapperData->jState == WRAPPER_JSTATE_STOP) ||
            (wrapperData->jState == WRAPPER_JSTATE_STOPPING) ||
            (wrapperData->jState == WRAPPER_JSTATE_STOPPED) ||
            (wrapperData->jState == WRAPPER_JSTATE_KILLING) ||
            (wrapperData->jState == WRAPPER_JSTATE_KILLED) ||
            (wrapperData->jState == WRAPPER_JSTATE_KILL) ||
            (wrapperData->jState == WRAPPER_JSTATE_DOWN_CHECK) ||
            (wrapperData->jState == WRAPPER_JSTATE_DOWN_FLUSH)) {
            /* JVM is already down or going down. */
        } else {
            wrapperData->exitRequested = TRUE;
        }

        wrapperData->exitCode = exitCode;

        if (force) {
            /* Make sure that further restarts are disabled. */
            wrapperData->restartRequested = WRAPPER_RESTART_REQUESTED_NO;

            /* Do not call wrapperSetWrapperState(WRAPPER_WSTATE_STOPPING) here.
             *  It will be called by the wrappereventloop.c.jStateDown once the
             *  the JVM is completely down.  Calling it here will make it
             *  impossible to trap and restart based on exit codes or other
             *  Wrapper configurations. */

            if (wrapperData->isDebugging) {
                if ((wrapperData->restartRequested == WRAPPER_RESTART_REQUESTED_AUTOMATIC) || (wrapperData->restartRequested == WRAPPER_RESTART_REQUESTED_CONFIGURED)) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("  Overriding request to restart JVM."));
                }
            }
        } else {
            /* Do not call wrapperSetWrapperState(WRAPPER_WSTATE_STOPPING) here.
             *  It will be called by the wrappereventloop.c.jStateDown once the
             *  the JVM is completely down.  Calling it here will make it
             *  impossible to trap and restart based on exit codes. */
            if (wrapperData->isDebugging) {
                if ((wrapperData->restartRequested == WRAPPER_RESTART_REQUESTED_AUTOMATIC) || (wrapperData->restartRequested == WRAPPER_RESTART_REQUESTED_CONFIGURED)) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("  Stop ignored.  Continuing to restart JVM."));
                }
            }
        }
    }
}

/**
 * Depending on the current state, we want to change the exact message displayed when restarting the JVM.
 *
 * The logic here needs to match that in wrapperRestartProcess.
 */
const TCHAR *wrapperGetRestartProcessMessage() {
    if ((wrapperData->jState == WRAPPER_JSTATE_DOWN_CLEAN) ||
        (wrapperData->jState == WRAPPER_JSTATE_STOP) ||
        (wrapperData->jState == WRAPPER_JSTATE_STOPPING) ||
        (wrapperData->jState == WRAPPER_JSTATE_STOPPED) ||
        (wrapperData->jState == WRAPPER_JSTATE_KILLING) ||
        (wrapperData->jState == WRAPPER_JSTATE_KILL) ||
        (wrapperData->jState == WRAPPER_JSTATE_DOWN_CHECK) ||
        (wrapperData->jState == WRAPPER_JSTATE_DOWN_FLUSH) ||
        (wrapperData->jState == WRAPPER_JSTATE_LAUNCH_DELAY)) {
        if (wrapperData->restartRequested || (wrapperData->jState == WRAPPER_JSTATE_LAUNCH_DELAY)) {
            return TEXT("Restart JVM (Ignoring, already restarting).");
        } else {
            return TEXT("Restart JVM (Ignoring, already shutting down).");
        }
    } else if (wrapperData->exitRequested || wrapperData->restartRequested) {
        return TEXT("Restart JVM (Ignoring, already restarting).");
    } else {
        return TEXT("Restarting JVM.");
    }
}

/**
 * Depending on the current state, we want to change the exact message displayed when pausing the Wrapper.
 *
 * The logic here needs to match that in wrapperPauseProcess.
 */
const TCHAR *wrapperGetPauseProcessMessage() {
    if (!wrapperData->pausable) {
        return TEXT("Pause (Ignoring, the Wrapper was not set pausable).");
    } else if ((wrapperData->wState == WRAPPER_WSTATE_STOPPING) ||
               (wrapperData->wState == WRAPPER_WSTATE_STOPPED)) {
        return TEXT("Pause (Ignoring, already stopping)."); 
    } else if (wrapperData->wState == WRAPPER_WSTATE_PAUSING) {
        return TEXT("Pause (Ignoring, already pausing)."); 
    } else if (wrapperData->wState == WRAPPER_WSTATE_PAUSED) {
        return TEXT("Pause (Ignoring, already paused)."); 
    } else {
        return TEXT("Pausing..."); 
    }
}

/**
 * Depending on the current state, we want to change the exact message displayed when resuming the Wrapper.
 *
 * The logic here needs to match that in wrapperResumeProcess.
 */
const TCHAR *wrapperGetResumeProcessMessage() {
    if ((wrapperData->wState == WRAPPER_WSTATE_STOPPING) ||
        (wrapperData->wState == WRAPPER_WSTATE_STOPPED)) {
        return TEXT("Resume (Ignoring, already stopping)."); 
    } else if (wrapperData->wState == WRAPPER_WSTATE_STARTING) {
        return TEXT("Resume (Ignoring, already starting)."); 
    } else if (wrapperData->wState == WRAPPER_WSTATE_STARTED) {
        return TEXT("Resume (Ignoring, already started)."); 
    } else if (wrapperData->wState == WRAPPER_WSTATE_RESUMING) {
        return TEXT("Resume (Ignoring, already resuming)."); 
    } else {
        return TEXT("Resuming..."); 
    }
}

/**
 * Used to ask the state engine to shut down the JVM.  This are always intentional restart requests.
 */
void wrapperRestartProcess() {
    /* If it has not already been set, set the restart request flag in the wrapper data. */
    if (wrapperData->exitRequested || wrapperData->restartRequested ||
        (wrapperData->jState == WRAPPER_JSTATE_DOWN_CLEAN) ||
        (wrapperData->jState == WRAPPER_JSTATE_STOP) ||
        (wrapperData->jState == WRAPPER_JSTATE_STOPPING) ||
        (wrapperData->jState == WRAPPER_JSTATE_STOPPED) ||
        (wrapperData->jState == WRAPPER_JSTATE_KILLING) ||
        (wrapperData->jState == WRAPPER_JSTATE_KILL) ||
        (wrapperData->jState == WRAPPER_JSTATE_DOWN_CHECK) ||
        (wrapperData->jState == WRAPPER_JSTATE_DOWN_FLUSH) ||
        (wrapperData->jState == WRAPPER_JSTATE_LAUNCH_DELAY)) { /* Down but not yet restarted. */

        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                TEXT("wrapperRestartProcess() called.  (IGNORED)"));
        }
    } else {
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                TEXT("wrapperRestartProcess() called."));
        }

        wrapperData->exitRequested = TRUE;
        wrapperData->restartRequested = WRAPPER_RESTART_REQUESTED_CONFIGURED;
    }
}

/**
 * Used to ask the state engine to pause the JVM.
 *
 * @param actionSourceCode Tracks where the action originated.
 */
void wrapperPauseProcess(int actionSourceCode) {
    TCHAR msgBuffer[10];

    if (!wrapperData->pausable) {
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT(
                "wrapperPauseProcess() called but wrapper.pausable is FALSE.  (IGNORED)"));
        }
        return;
    }

    if ((wrapperData->wState == WRAPPER_WSTATE_STOPPING) ||
        (wrapperData->wState == WRAPPER_WSTATE_STOPPED)) {
        /* If we are already shutting down, then ignore and continue to do so. */

        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT(
                "wrapperPauseProcess() called while stopping.  (IGNORED)"));
        }
    } else if (wrapperData->wState == WRAPPER_WSTATE_PAUSING) {
        /* If we are currently being paused, then ignore and continue to do so. */

        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT(
                "wrapperPauseProcess() called while pausing.  (IGNORED)"));
        }
    } else if (wrapperData->wState == WRAPPER_WSTATE_PAUSED) {
        /* If we are currently paused, then ignore and continue to do so. */

        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT(
                "wrapperPauseProcess() called while paused.  (IGNORED)"));
        }
    } else {
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT(
                "wrapperPauseProcess() called."));
        }

        wrapperSetWrapperState(WRAPPER_WSTATE_PAUSING);

        if (!wrapperData->pausableStopJVM) {
            /* Notify the Java process. */
            _sntprintf(msgBuffer, 10, TEXT("%d"), actionSourceCode);
            wrapperProtocolFunction(WRAPPER_MSG_PAUSE, msgBuffer);
        }
    }
}

/**
 * Used to ask the state engine to resume a paused the JVM.
 *
 * @param actionSourceCode Tracks where the action originated.
 */
void wrapperResumeProcess(int actionSourceCode) {
    TCHAR msgBuffer[10];

    if ((wrapperData->wState == WRAPPER_WSTATE_STOPPING) ||
        (wrapperData->wState == WRAPPER_WSTATE_STOPPED)) {
        /* If we are already shutting down, then ignore and continue to do so. */

        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT(
                "wrapperResumeProcess() called while stopping.  (IGNORED)"));
        }
    } else if (wrapperData->wState == WRAPPER_WSTATE_STARTING) {
        /* If we are currently being started, then ignore and continue to do so. */

        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT(
                "wrapperResumeProcess() called while starting.  (IGNORED)"));
        }
    } else if (wrapperData->wState == WRAPPER_WSTATE_STARTED) {
        /* If we are currently started, then ignore and continue to do so. */

        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT(
                "wrapperResumeProcess() called while started.  (IGNORED)"));
        }
    } else if (wrapperData->wState == WRAPPER_WSTATE_RESUMING) {
        /* If we are currently being continued, then ignore and continue to do so. */

        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT(
                "wrapperResumeProcess() called while resuming.  (IGNORED)"));
        }
    } else {
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT(
                "wrapperResumeProcess() called."));
        }

        /* If we were configured to stop the JVM then we want to reset its failed
         *  invocation count as the current stoppage was expected. */
        if (wrapperData->pausableStopJVM) {
            wrapperData->failedInvocationCount = 0;
        }

        wrapperSetWrapperState(WRAPPER_WSTATE_RESUMING);

        if (!wrapperData->pausableStopJVM) {
            /* Notify the Java process. */
            _sntprintf(msgBuffer, 10, TEXT("%d"), actionSourceCode);
            wrapperProtocolFunction(WRAPPER_MSG_RESUME, msgBuffer);
        }
    }
}

/**
 * Sends a command off to the JVM asking it to perform a garbage collection sweep.
 *
 * @param actionSourceCode Tracks where the action originated.
 */
void wrapperRequestJVMGC(int actionSourceCode) {
    TCHAR msgBuffer[10];
    
    /* Notify the Java process. */
    _sntprintf(msgBuffer, 10, TEXT("%d"), actionSourceCode);
    wrapperProtocolFunction(WRAPPER_MSG_GC, msgBuffer);
}

/**
 * Loops over and strips all double quotes from prop and places the
 *  stripped version into propStripped.
 *
 * The exception is double quotes that are preceeded by a backslash
 *  in this case the backslash is stripped.
 *
 * If two backslashes are found in a row, then the first escapes the
 *  second and the second is removed.
 */
static size_t wrapperStripQuotesInner(const TCHAR *prop, size_t propLen, TCHAR *propStripped) {
    size_t len;
    int i, j;

    len = propLen;
    j = 0;
    for (i = 0; i < (int)len; i++) {
        if ((prop[i] == TEXT('\\')) && (i < (int)len - 1)) {
            if (prop[i + 1] == TEXT('\\')) {
                /* Double backslash.  Keep the first, and skip the second. */
                propStripped[j] = prop[i];
                j++;
                i++;
            } else if (prop[i + 1] == TEXT('\"')) {
                /* Escaped quote.  Keep the quote. */
                propStripped[j] = prop[i + 1];
                j++;
                i++;
            } else {
                /* Include the backslash as normal. */
                propStripped[j] = prop[i];
                j++;
            }
        } else if (prop[i] == TEXT('\"')) {
            /* Quote.  Skip it. */
        } else {
            propStripped[j] = prop[i];
            j++;
        }
    }
    return j;
}

/**
 * Stripped quotes out of the prop argument.
 * The resulting value in propStripped will always be equal or shorter in length
 *  so the propStripped buffer should always be equal to the prop buffer in length.
 */
void wrapperStripQuotes(const TCHAR *prop, TCHAR *propStripped) {
    size_t len;

    len = wrapperStripQuotesInner(prop, _tcslen(prop), propStripped);
    propStripped[len] = TEXT('\0');
}

/**
 * Adds quotes around the specified string in such a way that everything is
 *  escaped correctly.  If the bufferSize is not large enough then the
 *  required size will be returned.  0 is returned if successful.
 */
size_t wrapperQuoteValue(const TCHAR* value, TCHAR *buffer, size_t bufferSize) {
    size_t len = _tcslen(value);
    size_t in = 0;
    size_t out = 0;
    size_t in2;
    int escape;

    /* Initial quote. */
    if (out < bufferSize) {
        buffer[out] = TEXT('"');
    }
    out++;

    /* Copy over characters of value. */
    while ((in < len) && (value[in] != TEXT('\0'))) {
        escape = FALSE;
        if (value[in] == TEXT('\\')) {
            /* All '\' characters in a row prior to a '"' or the end of the string need to be
             *  escaped */
            in2 = in + 1;
            while ((in2 < len) && (value[in2] == TEXT('\\'))) {
                in2++;
            }
            escape = ((in2 >= len) || (value[in2] == TEXT('"')));
        } else if (value[in] == TEXT('"')) {
            escape = TRUE;
        }

        if (escape) {
            /* Needs to be escaped. */
            if (out < bufferSize) {
                buffer[out] = TEXT('\\');
            }
            out++;
        }
        if (out < bufferSize) {
            buffer[out] = value[in];
        }
        out++;
        in++;
    }

    /* Trailing quote. */
    if (out < bufferSize) {
        buffer[out] = TEXT('"');
    }
    out++;

    /* Null terminate. */
    if (out < bufferSize) {
        buffer[out] = TEXT('\0');
    }
    out++;

    if (out <= bufferSize) {
        return 0;
    } else {
        return out;
    }
}

/**
 * Checks the quotes in the value and displays an error if there are any problems.
 * This can be useful to help users debug quote problems.
 */
int wrapperCheckQuotes(const TCHAR *value, const TCHAR *propName) {
    size_t len = _tcslen(value);
    size_t in = 0;
    size_t in2 = 0;
    int inQuote = FALSE;
    int escaped;

    while (in < len) {
        if (value[in] == TEXT('"')) {
            /* Decide whether or not this '"' is escaped. */
            escaped = FALSE;
            if (in > 0) {
                in2 = in - 1;
                while (value[in2] == TEXT('\\')) {
                    escaped = !escaped;
                    if (in2 > 0) {
                        in2--;
                    } else {
                        break;
                    }
                }
            }
            if (!escaped) {
                inQuote = !inQuote;
            }
        } else if (inQuote) {
            /* Quoted text. */
        } else {
            /* Unquoted. white space is bad. */
            if (value[in] == TEXT(' ')) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                    TEXT("The value of property '%s', '%s' contains unquoted spaces and will most likely result in an invalid java command line."),
                    propName, value);
                return 1;
            }
        }

        in++;
    }
    if (inQuote) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
            TEXT("The value of property '%s', '%s' contains an unterminated quote and will most likely result in an invalid java command line."),
            propName, value);
        return 1;
    }
    return 0;
}

#ifndef WIN32
int checkIfExecutable(const TCHAR *filename) {
    int result;
#if defined(WIN32) && !defined(WIN64)
    struct _stat64i32 statInfo;
#else
    struct stat statInfo;
#endif
    result = _tstat(filename, &statInfo);
    if (result < 0) {
        return 0;
    }

    if (!S_ISREG(statInfo.st_mode)) {
        return 0;
    }
    if (statInfo.st_uid == geteuid()) {
        return statInfo.st_mode & S_IXUSR;
    }
    if (statInfo.st_gid == getegid()) {
        return statInfo.st_mode & S_IXGRP;
    }
    return statInfo.st_mode & S_IXOTH;

}
#endif

int checkIfBinary(const TCHAR *filename) {
    FILE* f;
    unsigned char head[5];
    int r;
    f = _tfopen(filename, TEXT("rb"));
    if (!f) { /*couldnt find the java command... wrapper will moan later*/
       return 1;
    } else {
        r = (int)fread( head,1, 4, f);
        if (r != 4)
        {
            fclose(f);
            return 0;
        }
        fclose(f);
        head[4] = '\0';
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Magic number for file %s: 0x%02x%02x%02x%02x"), filename, head[0], head[1], head[2], head[3]);
        }

#if defined(LINUX) || defined(FREEBSD) || defined(SOLARIS) 
        if (head[1] == 'E' && head[2] == 'L' && head[3] == 'F') {
            return 1; /*ELF */
#elif defined(AIX)
        /* http://en.wikipedia.org/wiki/XCOFF */
        if (head[0] == 0x01 && head[1] == 0xf7 && head[2] == 0x00) { /* 0x01f700NN */
            return 1; /*xcoff 64*/
        } else if (head[0] == 0x01 && head[1] == 0xdf && head[2] == 0x00) { /* 0x01df00NN */
            return 1; /*xcoff 32*/
#elif defined(MACOSX)
        if (head[0] == 0xca && head[1] == 0xfe && head[2] == 0xba && head[3] == 0xbe) { /* 0xcafebabe */
            return 1; /*MACOS Universal binary*/
        } else if (head[0] == 0xcf && head[1] == 0xfa && head[2] == 0xed && head[3] == 0xfe) { /* 0xcffaedfe */
            return 1; /*MACOS x86_64 binary*/
        } else if (head[0] == 0xce && head[1] == 0xfa && head[2] == 0xed && head[3] == 0xfe) { /* 0xcefaedfe */
            return 1; /*MACOS i386 binary*/
        } else if (head[0] == 0xfe && head[1] == 0xed && head[2] == 0xfa && head[3] == 0xce) { /* 0xfeedface */
            return 1; /*MACOS ppc, ppc64 binary*/
#elif defined(HPUX)
        if (head[0] == 0x02 && head[1] == 0x10 && head[2] == 0x01 && head[3] == 0x08) { /* 0x02100108 PA-RISC 1.1 */
            return 1; /*HP UX PA RISC 32*/
        } else if (head[0] == 0x02 && head[1] == 0x14 && head[2] == 0x01 && head[3] == 0x07) { /* 0x02140107 PA-RISC 2.0 */
            return 1; /*HP UX PA RISC 32*/
        } else if (head[1] == 'E' && head[2] == 'L' && head[3] == 'F') {
            return 1; /*ELF */
#elif defined(WIN32)
        if (head[0] == 'M' && head[1] == 'Z') {
            return 1; /* MS */
#else
        if (FALSE) {
 #error I dont know what to do for this host type. (in checkIfBinary())
#endif
        } else {
            return 0;
        }
    }
}


#ifndef WIN32
TCHAR* findPathOf(const TCHAR *exe, const TCHAR *name) {
    TCHAR *searchPath;
    TCHAR *beg, *end;
    int stop, found;
    TCHAR pth[PATH_MAX + 1];
    TCHAR *ret;
    TCHAR resolvedPath[PATH_MAX + 1];

    if (exe[0] == TEXT('/')) {
        /* This is an absolute reference. */
        if (_trealpathN(exe, resolvedPath, PATH_MAX + 1)) {
            _tcsncpy(pth, resolvedPath, PATH_MAX + 1);
            if (checkIfExecutable(pth)) {
                ret = malloc((_tcslen(pth) + 1) * sizeof(TCHAR));
                if (!ret) {
                    outOfMemory(TEXT("FPO"), 1);
                    return NULL;
                }
                _tcsncpy(ret, pth, _tcslen(pth) + 1);
                if (wrapperData->isDebugging) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Resolved the real path of %s as an absolute reference: %s"), name, ret);
                }
                return ret;
            }
        } else if (wrapperData->isDebugging) {
            if (_tcslen(resolvedPath)) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Unable to resolve the real path of %s as an absolute reference: %s. %s (Problem at: %s)"), name, exe, getLastErrorText(), resolvedPath);
            } else {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Unable to resolve the real path of %s as an absolute reference: %s. %s"), name, exe, getLastErrorText());
            }
        }

        return NULL;
    }

    /* This is a non-absolute reference.  See if it is a relative reference. */
    if (_trealpathN(exe, resolvedPath, PATH_MAX + 1)) {
        /* Resolved.  See if the file exists. */
        _tcsncpy(pth, resolvedPath, PATH_MAX + 1);
        if (checkIfExecutable(pth)) {
            ret = malloc((_tcslen(pth) + 1) * sizeof(TCHAR));
            if (!ret) {
                outOfMemory(TEXT("FPO"), 2);
                return NULL;
            }
            _tcsncpy(ret, pth, _tcslen(pth) + 1);
            if (wrapperData->isDebugging) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Resolved the real path of %s as a relative reference: %s"), name, ret);
            }
            return ret;
        }
    } else if (wrapperData->isDebugging) {
        if (_tcslen(resolvedPath)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Unable to resolve the real path of %s as a relative reference: %s. %s (Problem at: %s)"), name, exe, getLastErrorText(), resolvedPath);
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Unable to resolve the real path of %s as a relative reference: %s. %s"), name, exe, getLastErrorText());
        }
    }

    /* The file was not a direct relative reference.   If and only if it does not contain any relative path components, we can search the PATH. */
    if (_tcschr(exe, TEXT('/')) == NULL) {
        searchPath = _tgetenv(TEXT("PATH"));
        if (searchPath && (_tcslen(searchPath) <= 0)) {
#if !defined(WIN32) && defined(UNICODE)
            free(searchPath);
#endif
            searchPath = NULL;
        }
        if (searchPath) {
            if (wrapperData->isDebugging) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Attempt to locate %s on system PATH: %s"), name, exe);
            }

            beg = searchPath;
            stop = 0; found = 0;
            do {
                end = _tcschr(beg, TEXT(':'));
                if (end == NULL) {
                    /* This is the last element in the PATH, so we want the whole thing. */
                    stop = 1;
                    _tcsncpy(pth, beg, PATH_MAX + 1);
                } else {
                    /* Copy the single path entry. */
                    _tcsncpy(pth, beg, end - beg);
                    pth[end - beg] = TEXT('\0');
                }
                if (pth[_tcslen(pth) - 1] != TEXT('/')) {
                    _tcsncat(pth, TEXT("/"), PATH_MAX + 1);
                }
                _tcsncat(pth, exe, PATH_MAX + 1);

                /* The file can exist on the path, but via a symbolic link, so we need to expand it.  Ignore errors here. */
#ifdef _DEBUG
                if (wrapperData->isDebugging) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("  Check PATH entry: %s"), pth);
                }
#endif
                if (_trealpathN(pth, resolvedPath, PATH_MAX + 1) != NULL) {
                    /* Copy over the result. */
                    _tcsncpy(pth, resolvedPath, PATH_MAX + 1);
                    found = checkIfExecutable(pth);
                } else if (wrapperData->isDebugging) {
                    if (_tcslen(resolvedPath)) {
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Unable to resolve the path %s: %s (Problem at: %s)"), pth, getLastErrorText(), resolvedPath);
                    } else {
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Unable to resolve the path %s: %s"), pth, getLastErrorText());
                    }
                    stop = TRUE;
                }

                if (!stop) {
                    beg = end + 1;
                }
            } while (!stop && !found);

#if !defined(WIN32) && defined(UNICODE)
            free(searchPath);
#endif

            if (found) {
                ret = malloc((_tcslen(pth) + 1) * sizeof(TCHAR));
                if (!ret) {
                    outOfMemory(TEXT("FPO"), 3);
                    return NULL;
                }
                _tcsncpy(ret, pth, _tcslen(pth) + 1);
                if (wrapperData->isDebugging) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Resolved the real path of %s from system PATH: %s"), name, ret);
                }
                return ret;
            } else {
                if (wrapperData->isDebugging) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Unable to resolve the real path of %s on the system PATH: %s"), name, exe);
                }
            }
        }
    }

    /* Still did not find the file.  So it must not exist. */
    return NULL;
}
#endif

/**
 * Checks to see if the speicified executable is a regular binary.   This will continue
 *  in either case, but a warning will be logged if the binary is invalid.
 *
 * @param para The binary to check.  On UNIX, the para memory may be freed and reallocated by this call.
 */
void checkIfRegularExe(TCHAR** para) {
    TCHAR* path;
#ifdef WIN32
    int len, start;
#endif

#ifdef WIN32
    if (_tcschr(*para, TEXT('\"')) != NULL){
        start = 1;
        len = (int)_tcslen(*para) - 2;
    } else {
        start = 0;
        len = (int)_tcslen(*para);
    }
    path = malloc(sizeof(TCHAR) * (len + 1));
    if (!path){
        outOfMemory(TEXT("CIRE"), 1);
    } else {
        _tcsncpy(path, (*para) + start, len);
        path[len] = TEXT('\0');
#else
    int replacePath;
    path = findPathOf(*para, TEXT("wrapper.java.command"));
    if (!path) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT("The configured wrapper.java.command could not be found, attempting to launch anyway: %s"), *para);
    } else {
        replacePath = getBooleanProperty(properties, TEXT("wrapper.java.command.resolve"), TRUE);
        if (replacePath == TRUE) {
            free(*para);
            *para = malloc((_tcslen(path) + 1) * sizeof(TCHAR));
            if (!(*para)) {
                outOfMemory(TEXT("CIRE"), 2);
                free(path);
                return;
            }
            _tcsncpy(*para, path, _tcslen(path) + 1);
        }
#endif
        if (!checkIfBinary(path)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT("The value of wrapper.java.command does not appear to be a java binary."));
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT("The use of scripts is not supported. Trying to continue, but some features may not work correctly.."));
        }
        free(path);
    }
}


/**
 * Builds up the java command section of the Java command line.
 *
 * @return The final index into the strings array, or -1 if there were any problems.
 */
int wrapperBuildJavaCommandArrayJavaCommand(TCHAR **strings, int addQuotes, int detectDebugJVM, int index) {
    const TCHAR *prop;
    TCHAR *c;
#ifdef WIN32
    TCHAR cpPath[512];
    int found;
#endif

    if (strings) {
        prop = getStringProperty(properties, TEXT("wrapper.java.command"), TEXT("java"));

#ifdef WIN32
        found = 0;

        if (_tcscmp(prop, TEXT("")) == 0) {
            /* If the java command is an empty string, we want to look for the
             *  the java command in the windows registry. */
            if (wrapperGetJavaHomeFromWindowsRegistry(cpPath)) {
                if (wrapperData->isDebugging) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                        TEXT("Loaded java home from registry: %s"), cpPath);
                }

                addProperty(properties, NULL, 0, TEXT("set.WRAPPER_JAVA_HOME"), cpPath, TRUE, FALSE, FALSE, TRUE);

                _tcsncat(cpPath, TEXT("\\bin\\java.exe"), 512);
                if (wrapperData->isDebugging) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                        TEXT("Found Java Runtime Environment home directory in system registry."));
                }
                found = 1;
            } else {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                    TEXT("The Java Runtime Environment home directory could not be located in the system registry."));
                found = 0;
                return -1;
            }
        } else {
            /* To avoid problems on Windows XP systems, the '/' characters must
             *  be replaced by '\' characters in the specified path.
             * prop is supposed to be constant, but allow this change as it is
             *  the actual value that we want. */
            wrapperCorrectWindowsPath((TCHAR *)prop);

            /* If the full path to the java command was not specified, then we
             *  need to try and resolve it here to avoid problems later when
             *  calling CreateProcess.  CreateProcess will look in the windows
             *  system directory before searching the PATH.  This can lead to
             *  the wrong JVM being run. */
            _sntprintf(cpPath, 512, TEXT("%s"), prop);
            if ((PathFindOnPath((TCHAR*)cpPath, (TCHAR**)wrapperGetSystemPath())) && (!PathIsDirectory(cpPath))) {
                /*printf("Found %s on path.\n", cpPath); */
                found = 1;
            } else {
                /*printf("Could not find %s on path.\n", cpPath); */

                /* Try adding .exe to the end */
                _sntprintf(cpPath, 512, TEXT("%s.exe"), prop);
                if ((PathFindOnPath(cpPath, wrapperGetSystemPath())) && (!PathIsDirectory(cpPath))) {
                    /*printf("Found %s on path.\n", cpPath); */
                    found = 1;
                } else {
                    /*printf("Could not find %s on path.\n", cpPath); */
                }
            }
        }

        if (found) {
            strings[index] = malloc(sizeof(TCHAR) * (_tcslen(cpPath) + 2 + 1));
            if (!strings[index]) {
                outOfMemory(TEXT("WBJCAJC"), 1);
                return -1;
            }
            if (addQuotes) {
                _sntprintf(strings[index], _tcslen(cpPath) + 2 + 1, TEXT("\"%s\""), cpPath);
            } else {
                _sntprintf(strings[index], _tcslen(cpPath) + 2 + 1, TEXT("%s"), cpPath);
            }
        } else {
            strings[index] = malloc(sizeof(TCHAR) * (_tcslen(prop) + 2 + 1));
            if (!strings[index]) {
                outOfMemory(TEXT("WBJCAJC"), 2);
                return -1;
            }
            if (addQuotes) {
                _sntprintf(strings[index], _tcslen(prop) + 2 + 1, TEXT("\"%s\""), prop);
            } else {
                _sntprintf(strings[index], _tcslen(prop) + 2 + 1, TEXT("%s"), prop);
            }
        }

        if (addQuotes) {
            wrapperCheckQuotes(strings[index], TEXT("wrapper.java.command"));
        }

#else /* UNIX */

        strings[index] = malloc(sizeof(TCHAR) * (_tcslen(prop) + 2 + 1));
        if (!strings[index]) {
            outOfMemory(TEXT("WBJCAJC"), 3);
            return -1;
        }
        if (addQuotes) {
            _sntprintf(strings[index], _tcslen(prop) + 2 + 1, TEXT("\"%s\""), prop);
        } else {
            _sntprintf(strings[index], _tcslen(prop) + 2 + 1, TEXT("%s"), prop);
        }
#endif
        checkIfRegularExe(&strings[index]);
        if (detectDebugJVM) {
            c = _tcsstr(strings[index], TEXT("jdb"));
            if (c && ((unsigned int)(c - strings[index]) == _tcslen(strings[index]) - 3 - 1)) {
                /* Ends with "jdb".  The jdb debugger is being used directly.  go into debug JVM mode. */
                wrapperData->debugJVM = TRUE;
            } else {
                c = _tcsstr(strings[index], TEXT("jdb.exe"));
                if (c && ((unsigned int)(c - strings[index]) == _tcslen(strings[index]) - 7 - 1)) {
                    /* Ends with "jdb".  The jdb debugger is being used directly.  go into debug JVM mode. */
                    wrapperData->debugJVM = TRUE;
                }
            }
        }
    }

    index++;

    return index;
}

/**
 * Builds up the additional section of the Java command line.
 *
 * @return The final index into the strings array, or -1 if there were any problems.
 */
int wrapperBuildJavaCommandArrayJavaAdditional(TCHAR **strings, int addQuotes, int detectDebugJVM, int index) {
    const TCHAR *prop;
    int i;
    size_t len;
    TCHAR paramBuffer2[128];
    int quotable;
    int defaultStripQuote;
    int stripQuote;
    TCHAR *propStripped;
    TCHAR **propertyNames;
    TCHAR **propertyValues;
    long unsigned int *propertyIndices;

    if (getStringProperties(properties, TEXT("wrapper.java.additional."), TEXT(""), wrapperData->ignoreSequenceGaps, FALSE, &propertyNames, &propertyValues, &propertyIndices)) {
        /* Failed */
        return -1;
    }

    defaultStripQuote = getBooleanProperty(properties, TEXT("wrapper.java.additional.default.stripquotes"), FALSE);
    i = 0;
    while (propertyNames[i]) {
        prop = propertyValues[i];
        if (prop) {
            if (_tcslen(prop) > 0) {
                /* All additional parameters must begin with a - or they will be interpretted
                 *  as the being the main class name by Java. */
                if (!((_tcsstr(prop, TEXT("-")) == prop) || (_tcsstr(prop, TEXT("\"-")) == prop))) {
                    /* Only log the message on the second pass. */
                    if (strings) {
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                            TEXT("The value of property '%s', '%s' is not a valid argument to the JVM.  Skipping."),
                            propertyNames[i], prop);
                    }
                } else {
                    if (strings) {
                        /* is quotable also changes the value of the property!
                           therefore prop can potentially point to free'd memory*/
                        quotable = isQuotableProperty(properties, propertyNames[i]);
                        prop = getStringProperty(properties, propertyNames[i], NULL);
                        propertyValues[i] = (TCHAR*) prop;
                        if (prop == NULL) {
                            freeStringProperties(propertyNames, propertyValues, propertyIndices);
                            return -1;
                        }
                        _sntprintf(paramBuffer2, 128, TEXT("wrapper.java.additional.%lu.stripquotes"), propertyIndices[i]);
                        if (addQuotes) {
                            stripQuote = FALSE;
                        } else {
                            stripQuote = getBooleanProperty(properties, paramBuffer2, defaultStripQuote);
                        }
                        if (stripQuote) {
                            propStripped = malloc(sizeof(TCHAR) * (_tcslen(prop) + 1));
                            if (!propStripped) {
                                freeStringProperties(propertyNames, propertyValues, propertyIndices);
                                outOfMemory(TEXT("WBJCAJA"), 2);
                                return -1;
                            }
                            wrapperStripQuotes(prop, propStripped);
                        } else {
                            propStripped = (TCHAR *)prop;
                        }

                        if (addQuotes && quotable && _tcschr(propStripped, TEXT(' '))) {
                            len = wrapperQuoteValue(propStripped, NULL, 0);
                            strings[index] = malloc(sizeof(TCHAR) * len);
                            if (!strings[index]) {
                                outOfMemory(TEXT("WBJCAJA"), 3);
                                freeStringProperties(propertyNames, propertyValues, propertyIndices);
                                if (stripQuote) {
                                    free(propStripped);
                                }
                                return -1;
                            }
                            wrapperQuoteValue(propStripped, strings[index], len);
                        } else {
                            strings[index] = malloc(sizeof(TCHAR) * (_tcslen(propStripped) + 1));
                            if (!strings[index]) {
                                outOfMemory(TEXT("WBJCAJA"), 4);
                                freeStringProperties(propertyNames, propertyValues, propertyIndices);
                                if (stripQuote) {
                                    free(propStripped);
                                }
                                return -1;
                            }
                            _sntprintf(strings[index], _tcslen(propStripped) + 1, TEXT("%s"), propStripped);
                        }

                        if (addQuotes) {
                            wrapperCheckQuotes(strings[index], propertyNames[i]);
                        }

                        if (stripQuote) {
                            free(propStripped);
                            propStripped = NULL;
                        }

                        /* Set if this paremeter enables debugging. */
                        if (detectDebugJVM) {
                            if (_tcsstr(strings[index], TEXT("-Xdebug")) == strings[index]) {
                                wrapperData->debugJVM = TRUE;
                            }
                        }
                    }

                    index++;
                }
            }
            i++;
        }
    }
    freeStringProperties(propertyNames, propertyValues, propertyIndices);

    return index;
}

/**
 * Java command line callback.
 *
 * @return FALSE if there were any problems.
 */
static int loadParameterFileCallbackParam_AddArg(LoadParameterFileCallbackParam *param, TCHAR *arg, size_t argLen) {
    TCHAR *argTerm;
    TCHAR *argStripped;
    TCHAR argExpanded[MAX_PROPERTY_VALUE_LENGTH];
    size_t len;

    /* The incoming arg can not be considered to be null terminated so we need a local copy. */
    if (!(argTerm = malloc(sizeof(TCHAR) * (argLen + 1)))) {
        outOfMemory(TEXT("LPFCPAA"), 1);
        return FALSE;
    }
    memcpy(argTerm, arg, sizeof(TCHAR) * argLen);
    argTerm[argLen] = TEXT('\0');
#ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_NOTICE, TEXT("    :> %s"), argTerm);
#endif

    if (param->isJVMParam == TRUE) {
        /* As in wrapperBuildJavaCommandArrayJavaAdditional(), skip an
           argument which does not begin with '-'. */
        if ((argTerm[0] != TEXT('-')) && !((argTerm[0] == TEXT('"')) && (argTerm[1] == TEXT('-')))) {
            if (param->strings) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                           TEXT("The value '%s' is not a valid argument to the JVM.  Skipping."), argTerm);
            }
            free(argTerm);
            return TRUE;
        }
    }

    if (param->strings) {
        /* Create a buffer to hold the stripped copy of argTerm. */
        len = _tcslen(argTerm);
        if (!(argStripped = malloc(sizeof(TCHAR) * (len + 1)))) {
            outOfMemory(TEXT("LPFCPAA"), 2);
            free(argTerm);
            return FALSE;
        }
        
        if (!param->stripQuote) {
            /* Nothing to strip, simply copy the string. */
            _tcsncpy(argStripped, argTerm, len + 1);
        } else {
            /* Strip the quotes. */
            wrapperStripQuotes(argTerm, argStripped);
        }
        
        /* No longer needed. */
        free(argTerm);
        
        /* Just in case the string contains and environment variable references, make sure they are all evaluated.
         *  argExpanded needs to be static because there is no way to know how long it will be in advance. */
        evaluateEnvironmentVariables(argStripped, argExpanded, MAX_PROPERTY_VALUE_LENGTH, properties->logWarnings, properties->warnedVarMap, properties->logWarningLogLevel);
        
        /* No longer needed. */
        free(argStripped);
        
        len = _tcslen(argExpanded);
        param->strings[param->index] = malloc(sizeof(TCHAR) * (len + 1));
        if (!param->strings[param->index]) {
            return FALSE;
        }
        _tcsncpy(param->strings[param->index], argExpanded, len + 1);
    } else {
        free(argTerm);
    }
    
    param->index++;
    return TRUE;
}

/*exitOnOverwrite and logLevelOnOverwrite are not used here but the function must implement the signature of ConfigFileReader_Callback. */
static int loadParameterFileCallback(void *callbackParam, const TCHAR *fileName, int lineNumber, TCHAR *config, int exitOnOverwrite, int logLevelOnOverwrite) {
    LoadParameterFileCallbackParam *param = (LoadParameterFileCallbackParam *)callbackParam;
    TCHAR *tail_bound;
    TCHAR *arg;
    TCHAR *s;
    int InDelim = FALSE;
    int InQuotes = FALSE;
    int Escaped = FALSE;

    /* special case where the callback should do nothing */
    if ((fileName == NULL) && (lineNumber == -1) && (config == NULL)) {
        return TRUE;
    }

#ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_NOTICE, TEXT("    : %s"), config);
#endif

    /* Assume that the line `config' has no white spaces at its
       beginning and end. */
    assert(config && _tcslen(config) > 0);
    assert(config[0] != TEXT(' ') && config[_tcslen(config) - 1] != TEXT(' '));

    tail_bound = config + _tcslen(config) + 1;
    for (arg = s = config; s < tail_bound; s++) {
        switch (*s) {
        case TEXT('\0'):
            if (!loadParameterFileCallbackParam_AddArg(param, arg, s - arg)) {
                outOfMemory(TEXT("LJAC"), 1);
                return FALSE;
            }
            break;
        case TEXT(' '):
            Escaped = FALSE;
            if (!InDelim && !InQuotes) {
                InDelim = TRUE;
                if (!loadParameterFileCallbackParam_AddArg(param, arg, s - arg)) {
                    outOfMemory(TEXT("LJAC"), 2);
                    return FALSE;
                }
            }
            break;
        case TEXT('"'):
            if (!Escaped) {
                InQuotes = !InQuotes;
            }
            Escaped = FALSE;
            if (InDelim) {
                InDelim = FALSE;
                arg = s;
            }
            break;
        case TEXT('\\'):
            Escaped = !Escaped;
            if (InDelim) {
                InDelim = FALSE;
                arg = s;
            }
            break;
        default:
            Escaped = FALSE;
            if (InDelim) {
                InDelim = FALSE;
                arg = s;
            }
            break;
        }
    }

    return TRUE;
}

/**
 * Builds up the additional section of the Java command line.
 *
 * @return The final index into the strings array, or -1 if there were any problems.
 */
int wrapperLoadParameterFile(TCHAR **strings, int addQuotes, int detectDebugJVM, int index, TCHAR *parameterName, int isJVMParameter) {
    const TCHAR *parameterFilePath;
    LoadParameterFileCallbackParam callbackParam;
    int readResult;
    TCHAR prop[256];

    parameterFilePath = getFileSafeStringProperty(properties, parameterName, TEXT(""));
#ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_NOTICE,
           TEXT("%s=%s"), parameterName, parameterFilePath ? parameterFilePath : TEXT(""));
#endif
    if (_tcslen(parameterFilePath) == 0) {
        return index;
    }

    if (addQuotes) {
        callbackParam.stripQuote = FALSE;
    } else {
        _sntprintf(prop, 256, TEXT("%s.stripquotes"), parameterName);
        callbackParam.stripQuote = getBooleanProperty(properties, prop, FALSE);
    }
    callbackParam.strings = strings;
    callbackParam.index = index;
    callbackParam.isJVMParam = isJVMParameter;

    readResult = configFileReader(parameterFilePath, TRUE, loadParameterFileCallback, &callbackParam, FALSE, FALSE, wrapperData->argCommand, wrapperData->originalWorkingDir, properties->warnedVarMap, properties->logWarnings, properties->logWarningLogLevel, wrapperData->isDebugging);
    switch (readResult) {
    case CONFIG_FILE_READER_SUCCESS:
        return callbackParam.index;
    case CONFIG_FILE_READER_FAIL:
    case CONFIG_FILE_READER_HARD_FAIL:
        return -1;
    default:
        _tprintf(TEXT("Unexpected read error %d\n"), readResult);
        return index;
    };
}

/**
 * Builds up the library path section of the Java command line.
 *
 * @return The final index into the strings array, or -1 if there were any problems.
 */
int wrapperBuildJavaCommandArrayLibraryPath(TCHAR **strings, int addQuotes, int index) {
    const TCHAR *prop;
    int i, j;
    size_t len2;
    size_t cpLen, cpLenAlloc;
    TCHAR *tmpString;
    TCHAR *systemPath;
    TCHAR **propertyNames;
    TCHAR **propertyValues;
    long unsigned int *propertyIndices;

    if (strings) {
        if (wrapperData->libraryPathAppendPath) {
            /* We are going to want to append the full system path to
             *  whatever library path is generated. */
#ifdef WIN32
            systemPath = _tgetenv(TEXT("PATH"));
#else
            systemPath = _tgetenv(TEXT("LD_LIBRARY_PATH"));
#endif
            if (systemPath) {
                /* If we are going to add our own quotes then we need to make sure that the system
                 *  PATH doesn't contain any of its own.  Windows allows users to do this... */
                if (addQuotes) {
                    i = 0;
                    j = 0;
                    do {
                        if (systemPath[i] != TEXT('"') ) {
                            systemPath[j] = systemPath[i];
                            j++;
                        }
                        i++;
                    } while (systemPath[j] != TEXT('\0'));
                }
            }
        } else {
            systemPath = NULL;
        }

        prop = getStringProperty(properties, TEXT("wrapper.java.library.path"), NULL);
        if (prop) {
            /* An old style library path was specified.
             * If quotes are being added, check the last character before the
             *  closing quote. If it is a backslash then Windows will use it to
             *  escape the quote.  To make things work correctly, we need to add
             *  another backslash first so it will result in a single backslash
             *  before the quote. */
            if (systemPath) {
                strings[index] = malloc(sizeof(TCHAR) * (22 + _tcslen(prop) + 1 + _tcslen(systemPath) + 1 + 1));
                if (!strings[index]) {
                    outOfMemory(TEXT("WBJCALP"), 1);
#if !defined(WIN32) && defined(UNICODE)
                    free(systemPath);
#endif
                    return -1;
                }
                if (addQuotes) {
                    if ((_tcslen(systemPath) > 1) && (systemPath[_tcslen(systemPath) - 1] == TEXT('\\'))) {
                        _sntprintf(strings[index], 22 + _tcslen(prop) + 1 + _tcslen(systemPath) + 1 + 1, TEXT("-Djava.library.path=\"%s%c%s\\\""), prop, wrapperClasspathSeparator, systemPath);
                    } else {
                        _sntprintf(strings[index], 22 + _tcslen(prop) + 1 + _tcslen(systemPath) + 1 + 1, TEXT("-Djava.library.path=\"%s%c%s\""), prop, wrapperClasspathSeparator, systemPath);
                    }
                } else {
                    _sntprintf(strings[index], 22 + _tcslen(prop) + 1 + _tcslen(systemPath) + 1 + 1, TEXT("-Djava.library.path=%s%c%s"), prop, wrapperClasspathSeparator, systemPath);
                }
            } else {
                strings[index] = malloc(sizeof(TCHAR) * (22 + _tcslen(prop) + 1 + 1));
                if (!strings[index]) {
                    outOfMemory(TEXT("WBJCALP"), 2);
                    return -1;
                }
                if (addQuotes) {
                    if ((_tcslen(prop) > 1) && (prop[_tcslen(prop) - 1] == TEXT('\\'))) {
                        _sntprintf(strings[index], 22 + _tcslen(prop) + 1 + 1, TEXT("-Djava.library.path=\"%s\\\""), prop);
                    } else {
                        _sntprintf(strings[index], 22 + _tcslen(prop) + 1 + 1, TEXT("-Djava.library.path=\"%s\""), prop);
                    }
                } else {
                    _sntprintf(strings[index], 22 + _tcslen(prop) + 1 + 1, TEXT("-Djava.library.path=%s"), prop);
                }
            }

            if (addQuotes) {
                wrapperCheckQuotes(strings[index], TEXT("wrapper.java.library.path"));
            }
        } else {
            /* Look for a multiline library path. */
            cpLen = 0;
            cpLenAlloc = 1024;
            strings[index] = malloc(sizeof(TCHAR) * cpLenAlloc);
            if (!strings[index]) {
                outOfMemory(TEXT("WBJCALP"), 3);
#if !defined(WIN32) && defined(UNICODE)
                if (systemPath) {
                    free(systemPath);
                }
#endif
                return -1;
            }

            /* Start with the property value. */
            _sntprintf(&(strings[index][cpLen]), cpLenAlloc - cpLen, TEXT("-Djava.library.path="));
            cpLen += 20;

            /* Add an open quote to the library path */
            if (addQuotes) {
                _sntprintf(&(strings[index][cpLen]), cpLenAlloc - cpLen, TEXT("\""));
                cpLen++;
            }

            /* Loop over the library path entries adding each one */
            if (getStringProperties(properties, TEXT("wrapper.java.library.path."), TEXT(""), wrapperData->ignoreSequenceGaps, FALSE, &propertyNames, &propertyValues, &propertyIndices)) {
                /* Failed */
#if !defined(WIN32) && defined(UNICODE)
                if (systemPath) {
                    free(systemPath);
                }
#endif
                return -1;
            }

            i = 0;
            j = 0;
            while (propertyNames[i]) {
                prop = propertyValues[i];
                if (prop) {
                    len2 = _tcslen(prop);
                    if (len2 > 0) {
                        /* Is there room for the entry? */
                        while (cpLen + len2 + 3 > cpLenAlloc) {
                            /* Resize the buffer */
                            tmpString = strings[index];
                            cpLenAlloc += 1024;
                            strings[index] = malloc(sizeof(TCHAR) * cpLenAlloc);
                            if (!strings[index]) {
                                outOfMemory(TEXT("WBJCALP"), 4);
#if !defined(WIN32) && defined(UNICODE)
                                if (systemPath) {
                                    free(systemPath);
                                }
#endif
                                return -1;
                            }
                            _sntprintf(strings[index], cpLenAlloc, TEXT("%s"), tmpString);
                            free(tmpString);
                            tmpString = NULL;
                        }

                        if (j > 0) {
                            strings[index][cpLen++] = wrapperClasspathSeparator; /* separator */
                        }
                        _sntprintf(&(strings[index][cpLen]), cpLenAlloc - cpLen, TEXT("%s"), prop);
                        cpLen += len2;
                        j++;
                    }
                    i++;
                }
            }
            freeStringProperties(propertyNames, propertyValues, propertyIndices);

            if (systemPath) {
                /* We need to append the system path. */
                len2 = _tcslen(systemPath);
                if (len2 > 0) {
                    /* Is there room for the entry? */
                    while (cpLen + len2 + 3 > cpLenAlloc) {
                        /* Resize the buffer */
                        tmpString = strings[index];
                        cpLenAlloc += 1024;
                        strings[index] = malloc(sizeof(TCHAR) * cpLenAlloc);
                        if (!strings[index]) {
                            outOfMemory(TEXT("WBJCALP"), 5);
#if !defined(WIN32) && defined(UNICODE)
                            free(systemPath);
#endif
                            return -1;
                        }
                        _sntprintf(strings[index], cpLenAlloc, TEXT("%s"), tmpString);
                        free(tmpString);
                        tmpString = NULL;
                    }

                    if (j > 0) {
                        strings[index][cpLen++] = wrapperClasspathSeparator; /* separator */
                    }
                    _sntprintf(&(strings[index][cpLen]), cpLenAlloc - cpLen, TEXT("%s"), systemPath);
                    cpLen += len2;
                    j++;
                }
            }

            if (j == 0) {
                /* No library path, use default. always room */
                _sntprintf(&(strings[index][cpLen]), cpLenAlloc - cpLen, TEXT("./"));
                cpLen++;
            }
            /* Add ending quote.  If the previous character is a backslash then
             *  Windows will use it to escape the quote.  To make things work
             *  correctly, we need to add another backslash first so it will
             *  result in a single backslash before the quote. */
            if (addQuotes) {
                if (strings[index][cpLen - 1] == TEXT('\\')) {
                    _sntprintf(&(strings[index][cpLen]), cpLenAlloc - cpLen, TEXT("\\"));
                    cpLen++;
                }
                _sntprintf(&(strings[index][cpLen]), cpLenAlloc - cpLen, TEXT("\""));
                cpLen++;
            }

            if (addQuotes) {
                wrapperCheckQuotes(strings[index], TEXT("wrapper.java.library.path.<n>"));
            }
        }

#if !defined(WIN32) && defined(UNICODE)
        if (systemPath) {
            free(systemPath);
        }
#endif
    }
    index++;
    return index;
}


/**
 * Builds up the java classpath.
 *
 * @return 0 if successful, or -1 if there were any problems.
 */
int wrapperBuildJavaClasspath(TCHAR **classpath) {
    const TCHAR *prop;
    TCHAR *propStripped;
    TCHAR *propBaseDir;
    int i, j;
    size_t cpLen, cpLenAlloc;
    size_t len2;
    TCHAR *tmpString;
#if defined(WIN32) && !defined(WIN64)
    struct _stat64i32 statBuffer;
#else
    struct stat statBuffer;
#endif
    TCHAR **propertyNames;
    TCHAR **propertyValues;
    long unsigned int *propertyIndices;
    TCHAR **files;
    int cnt;
    int missingLogLevel;

    /* Build a classpath */
    cpLen = 0;
    cpLenAlloc = 1024;
    *classpath = malloc(sizeof(TCHAR) * cpLenAlloc);
    if (!*classpath) {
        outOfMemory(TEXT("WBJCP"), 1);
        return -1;
    }

    /* Loop over the classpath entries adding each one. */
    if (getStringProperties(properties, TEXT("wrapper.java.classpath."), TEXT(""), wrapperData->ignoreSequenceGaps, FALSE, &propertyNames, &propertyValues, &propertyIndices)) {
        /* Failed */
        return -1;
    }
    
    /* Get the loglevel to display warnings about missing classpath elements. */
    missingLogLevel = getLogLevelForName(getStringProperty(properties, TEXT("wrapper.java.classpath.missing.loglevel"), TEXT("DEBUG")));

    i = 0;
    j = 0;
    while (propertyNames[i]) {
        prop = propertyValues[i];

        /* Does this contain any quotes? */
        if (_tcschr(prop, TEXT('"'))) {
            propStripped = malloc(sizeof(TCHAR) * (_tcslen(prop) + 1));
            if (!propStripped) {
                outOfMemory(TEXT("WBJCP"), 2);
                freeStringProperties(propertyNames, propertyValues, propertyIndices);
                return -1;
            }
            wrapperStripQuotes(prop, propStripped);
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT(
                "Classpath element, %s, should not contain quotes: %s, stripping and continuing: %s"), propertyNames[i], prop, propStripped);
        } else {
            propStripped = (TCHAR *)prop;
        }

        len2 = _tcslen(propStripped);
        if (len2 > 0) {
            /* Does this contain wildcards? */
            if ((_tcsrchr(propStripped, TEXT('*')) != NULL) || (_tcschr(propStripped, TEXT('?')) != NULL)) {
                /* Need to do a wildcard search */
                files = loggerFileGetFiles(propStripped, LOGGER_FILE_SORT_MODE_NAMES_ASC);
                if (!files) {
                    /* Failed */
                    if (propStripped != prop) {
                        free(propStripped);
                    }
                    freeStringProperties(propertyNames, propertyValues, propertyIndices);
                    return -1;
                }

                /* Loop over the files. */
                cnt = 0;
                while (files[cnt]) {
                    len2 = _tcslen(files[cnt]);

                    /* Is there room for the entry? */
                    if (cpLen + len2 + 3 > cpLenAlloc) {
                        /* Resize the buffer */
                        tmpString = *classpath;
                        cpLenAlloc += len2 + 3;
                        *classpath = malloc(sizeof(TCHAR) * cpLenAlloc);
                        if (!*classpath) {
                            if (propStripped != prop) {
                                free(propStripped);
                            }
                            loggerFileFreeFiles(files);
                            freeStringProperties(propertyNames, propertyValues, propertyIndices);
                            outOfMemory(TEXT("WBJCP"), 2);
                            return -1;
                        }
                        if (j > 0) {
                            _sntprintf(*classpath, cpLenAlloc, TEXT("%s"), tmpString);
                        }
                        free(tmpString);
                        tmpString = NULL;
                    }

                    if (j > 0) {
                        (*classpath)[cpLen++] = wrapperClasspathSeparator; /* separator */
                    }
                    _sntprintf(&((*classpath)[cpLen]), cpLenAlloc - cpLen, TEXT("%s"), files[cnt]);
                    cpLen += len2;
                    j++;
                    cnt++;
                }
                loggerFileFreeFiles(files);
            } else {
                /* This classpath entry does not contain any wildcards. */

                /* If the path element is a directory then we want to strip the trailing slash if it exists. */
                propBaseDir = (TCHAR*)propStripped;
                if ((propStripped[_tcslen(propStripped) - 1] == TEXT('/')) || (propStripped[_tcslen(propStripped) - 1] == TEXT('\\'))) {
                    propBaseDir = malloc(sizeof(TCHAR) * _tcslen(propStripped));
                    if (!propBaseDir) {
                        outOfMemory(TEXT("WBJCP"), 3);
                        if (propStripped != prop) {
                            free(propStripped);
                        }
                        freeStringProperties(propertyNames, propertyValues, propertyIndices);
                        return -1;
                    }
                    _tcsncpy(propBaseDir, propStripped, _tcslen(propStripped) - 1);
                    propBaseDir[_tcslen(propStripped) - 1] = TEXT('\0');
                }

                /* See if it exists so we can display a debug warning if it does not. */
                if (_tstat(propBaseDir, &statBuffer)) {
                    /* Encountered an error of some kind. */
                    if ((errno == ENOENT) || (errno == 3)) {
                        log_printf(WRAPPER_SOURCE_WRAPPER, missingLogLevel, TEXT(
                            "Classpath element, %s, does not exist: %s"), propertyNames[i], propStripped);
                    } else {
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT(
                            "Unable to get information of classpath element: %s (%s)"),
                            propStripped, getLastErrorText());
                    }
                } else {
                    /* Got the stat info. */
                }

                /* If we allocated the propBaseDir buffer then free it up. */
                if (propBaseDir != propStripped) {
                    free(propBaseDir);
                }
                propBaseDir = NULL;

                /* Is there room for the entry? */
                if (cpLen + len2 + 3 > cpLenAlloc) {
                    /* Resize the buffer */
                    tmpString = *classpath;
                    cpLenAlloc += len2 + 3;
                    *classpath = malloc(sizeof(TCHAR) * cpLenAlloc);
                    if (!*classpath) {
                        outOfMemory(TEXT("WBJCP"), 4);
                        if (propStripped != prop) {
                            free(propStripped);
                        }
                        freeStringProperties(propertyNames, propertyValues, propertyIndices);
                        return -1;
                    }
                    if (j > 0) {
                        _sntprintf(*classpath, cpLenAlloc, TEXT("%s"), tmpString);
                    }
                    free(tmpString);
                    tmpString = NULL;
                }

                if (j > 0) {
                    (*classpath)[cpLen++] = wrapperClasspathSeparator; /* separator */
                }
                _sntprintf(&((*classpath)[cpLen]), cpLenAlloc - cpLen, TEXT("%s"), propStripped);
                cpLen += len2;
                j++;
            }
        }

        /* If we allocated the propStripped buffer then free it up. */
        if (propStripped != prop) {
            free(propStripped);
        }
        propStripped = NULL;

        i++;
    }
    freeStringProperties(propertyNames, propertyValues, propertyIndices);
    if (j == 0) {
        /* No classpath, use default. always room */
        _sntprintf(&(*classpath[cpLen]), cpLenAlloc - cpLen, TEXT("./"));
        cpLen++;
    }

    return 0;
}


/**
 * Builds up the java classpath section of the Java command line.
 *
 * @return The final index into the strings array, or -1 if there were any problems.
 */
int wrapperBuildJavaCommandArrayClasspath(TCHAR **strings, int addQuotes, int index, const TCHAR *classpath) {
    size_t len;
    size_t cpLen;

    /* Store the classpath */
    if (strings) {
        strings[index] = malloc(sizeof(TCHAR) * (10 + 1));
        if (!strings[index]) {
            outOfMemory(TEXT("WBJCAC"), 1);
            return -1;
        }
        _sntprintf(strings[index], 10 + 1, TEXT("-classpath"));
    }
    index++;
    if (strings) {
        cpLen = 0;

        len = _tcslen(classpath);
        strings[index] = malloc(sizeof(TCHAR) * (len + 4));
        if (!strings[index]) {
            outOfMemory(TEXT("WBJCAC"), 2);
            return -1;
        }

        /* Add an open quote the classpath */
        if (addQuotes) {
            _sntprintf(&(strings[index][cpLen]), len + 4 - cpLen, TEXT("\""));
            cpLen++;
        }

        _sntprintf(&(strings[index][cpLen]), len + 4 - cpLen, TEXT("%s"), classpath);
        cpLen += len;

        /* Add ending quote.  If the previous character is a backslash then
         *  Windows will use it to escape the quote.  To make things work
         *  correctly, we need to add another backslash first so it will
         *  result in a single backslash before the quote. */
        if (addQuotes) {
            if (strings[index][cpLen - 1] == TEXT('\\')) {
                _sntprintf(&(strings[index][cpLen]), len + 4 - cpLen, TEXT("\\"));
                cpLen++;
            }
            _sntprintf(&(strings[index][cpLen]), len + 4 - cpLen, TEXT("\""));
            cpLen++;
        }

        if (addQuotes) {
            wrapperCheckQuotes(strings[index], TEXT("wrapper.java.classpath.<n>"));
        }
    }
    index++;

    return index;
}


/**
 * Builds up the app parameters section of the Java command line.
 *
 * @return The final index into the strings array, or -1 if there were any problems.
 */
int wrapperBuildJavaCommandArrayAppParameters(TCHAR **strings, int addQuotes, int index, int thisIsTestWrapper) {
    const TCHAR *prop;
    int i;
    int quotable;
    TCHAR *propStripped;
    int defaultStripQuote;
    int stripQuote;
    TCHAR paramBuffer2[128];
    size_t len;
    TCHAR **propertyNames;
    TCHAR **propertyValues;
    long unsigned int *propertyIndices;

    if (getStringProperties(properties, TEXT("wrapper.app.parameter."), TEXT(""), wrapperData->ignoreSequenceGaps, FALSE, &propertyNames, &propertyValues, &propertyIndices)) {
        /* Failed */
        return -1;
    }

    defaultStripQuote = getBooleanProperty(properties, TEXT("wrapper.app.parameter.default.stripquotes"), FALSE);
    i = 0;
    while (propertyNames[i]) {
        prop = propertyValues[i];
        if (_tcslen(prop) > 0) {
            if (thisIsTestWrapper && (i == 1) && ((_tcscmp(prop, TEXT("{{TestWrapperBat}}")) == 0) || (_tcscmp(prop, TEXT("{{TestWrapperSh}}")) == 0))) {
                /* This is the TestWrapper dummy parameter.  Simply skip over it so it doesn't get put into the command line. */
            } else {
                if (strings) {
                    quotable = isQuotableProperty(properties, propertyNames[i]);
                    _sntprintf(paramBuffer2, 128, TEXT("wrapper.app.parameter.%lu.stripquotes"), propertyIndices[i]);
                    if (addQuotes) {
                        stripQuote = FALSE;
                    } else {
                        stripQuote = getBooleanProperty(properties, paramBuffer2, defaultStripQuote);
                    }
                    if (stripQuote) {
                        propStripped = malloc(sizeof(TCHAR) * (_tcslen(prop) + 1));
                        if (!propStripped) {
                            outOfMemory(TEXT("WBJCAAP"), 1);
                            freeStringProperties(propertyNames, propertyValues, propertyIndices);
                            return -1;
                        }
                        wrapperStripQuotes(prop, propStripped);
                    } else {
                        propStripped = (TCHAR *)prop;
                    }

                    if (addQuotes && quotable && _tcschr(propStripped, TEXT(' '))) {
                        len = wrapperQuoteValue(propStripped, NULL, 0);
                        strings[index] = malloc(sizeof(TCHAR) * len);
                        if (!strings[index]) {
                            outOfMemory(TEXT("WBJCAAP"), 2);
                            if (stripQuote) {
                                free(propStripped);
                            }
                            freeStringProperties(propertyNames, propertyValues, propertyIndices);
                            return -1;
                        }
                        wrapperQuoteValue(propStripped, strings[index], len);
                    } else {
                        strings[index] = malloc(sizeof(TCHAR) * (_tcslen(propStripped) + 1));
                        if (!strings[index]) {
                            if (stripQuote) {
                                free(propStripped);
                            }
                            freeStringProperties(propertyNames, propertyValues, propertyIndices);
                            outOfMemory(TEXT("WBJCAAP"), 3);
                            return -1;
                        }
                        _sntprintf(strings[index], _tcslen(propStripped) + 1, TEXT("%s"), propStripped);
                    }

                    if (addQuotes) {
                        wrapperCheckQuotes(strings[index], propertyNames[i]);
                    }

                    if (stripQuote) {
                        free(propStripped);
                        propStripped = NULL;
                    }
                }
                index++;
            }
        }
        i++;
    }
    freeStringProperties(propertyNames, propertyValues, propertyIndices);

    /* precede command line parameters */
    if (wrapperData->javaArgValueCount > 0) {
        for (i = 0; i < wrapperData->javaArgValueCount; i++) {
            if (strings) {
                if (addQuotes && _tcschr(wrapperData->javaArgValues[i], TEXT(' '))) {
                    len = wrapperQuoteValue(wrapperData->javaArgValues[i], NULL, 0);
                    strings[index] = malloc(sizeof(TCHAR) * len);
                    if (!strings[index]) {
                        outOfMemory(TEXT("WBJCAAP"), 4);
                        return -1;
                    }
                    wrapperQuoteValue(wrapperData->javaArgValues[i], strings[index], len);
                 } else {
                    strings[index] = malloc(sizeof(TCHAR) * (_tcslen(wrapperData->javaArgValues[i]) + 1));
                    if (!strings[index]) {
                        outOfMemory(TEXT("WBJCAAP"), 5);
                        return -1;
                    }
                    _sntprintf(strings[index], _tcslen(wrapperData->javaArgValues[i]) + 1, TEXT("%s"), wrapperData->javaArgValues[i]);
                }
            }
            index++;
        }
    }
    return index;
}

/**
 * Loops over and stores all necessary commands into an array which
 *  can be used to launch a process.
 * This method will only count the elements if stringsPtr is NULL.
 *
 * Note - Next Out Of Memory is #47
 *
 * @return The final index into the strings array, or -1 if there were any problems.
 */
int wrapperBuildJavaCommandArrayInner(TCHAR **strings, int addQuotes, const TCHAR *classpath) {
    int index;
    int detectDebugJVM;
    const TCHAR *prop;
    int initMemory = 0;
    int maxMemory;
    int thisIsTestWrapper;

    setLogPropertyWarnings(properties, strings != NULL);
    index = 0;

    detectDebugJVM = getBooleanProperty(properties, TEXT("wrapper.java.detect_debug_jvm"), TRUE);

    /* Java commnd */
    if ((index = wrapperBuildJavaCommandArrayJavaCommand(strings, addQuotes, detectDebugJVM, index)) < 0) {
        return -1;
    }

    /* See if the auto bits parameter is set.  Ignored by all but the following platforms. */

#if /*defined(WIN32) || defined(LINUX) ||*/ defined(HPUX) || defined(MACOSX) || defined(SOLARIS) || defined(FREEBSD)


    if (getBooleanProperty(properties,
/*#ifdef WIN32
                              TEXT("wrapper.java.additional.auto_bits.windows"),
#elif defined(LINUX)
                              TEXT("wrapper.java.additional.auto_bits.linux"),
*/
#if defined(HPUX)
                              TEXT("wrapper.java.additional.auto_bits.hpux"),
#elif defined(SOLARIS)
                              TEXT("wrapper.java.additional.auto_bits.solaris"),
#elif defined(FREEBSD)
                              TEXT("wrapper.java.additional.auto_bits.freebsd"),
#elif defined(MACOSX)
                              TEXT("wrapper.java.additional.auto_bits.macosx"),
#endif
                              getBooleanProperty(properties, TEXT("wrapper.java.additional.auto_bits"), FALSE))) {
        if (strings) {
            strings[index] = malloc(sizeof(TCHAR) * 5);
            if (!strings[index]) {
                outOfMemory(TEXT("WBJCAI"), 46);
                return -1;
            }
            _sntprintf(strings[index], 5, TEXT("-d%s"), wrapperBits);
        }
        index++;
    }
#endif

    /* Store additional java parameters */
    if ((index = wrapperBuildJavaCommandArrayJavaAdditional(strings, addQuotes, detectDebugJVM, index)) < 0) {
        return -1;
    }

    /* Store additional java parameters specified in the parameter file */
    if ((index = wrapperLoadParameterFile(strings, addQuotes, detectDebugJVM, index, TEXT("wrapper.java.additional_file"), TRUE)) < 0) {
        return -1;
    }

    /* Initial JVM memory */
    initMemory = getIntProperty(properties, TEXT("wrapper.java.initmemory"), 0);
    if (initMemory > 0) {
        if (strings) {
            initMemory = __max(initMemory, 1); /* 1 <= n */
            strings[index] = malloc(sizeof(TCHAR) * (5 + 10 + 1));  /* Allow up to 10 digits. */
            if (!strings[index]) {
                outOfMemory(TEXT("WBJCAI"), 8);
                return -1;
            }
            _sntprintf(strings[index], 5 + 10 + 1, TEXT("-Xms%dm"), initMemory);
        }
        index++;
    } else {
            /* Set the initMemory so the checks in the maxMemory section below will work correctly. */
            initMemory = 3;
    }

    /* Maximum JVM memory */
    maxMemory = getIntProperty(properties, TEXT("wrapper.java.maxmemory"), 0);
    if (maxMemory > 0) {
        if (strings) {
            maxMemory = __max(maxMemory, initMemory);  /* initMemory <= n */
            strings[index] = malloc(sizeof(TCHAR) * (5 + 10 + 1));  /* Allow up to 10 digits. */
            if (!strings[index]) {
                outOfMemory(TEXT("WBJCAI"), 10);
                return -1;
            }
            _sntprintf(strings[index], 5 + 10 + 1, TEXT("-Xmx%dm"), maxMemory);
        }
        index++;
    }

    /* Library Path */
    if ((index = wrapperBuildJavaCommandArrayLibraryPath(strings, addQuotes, index)) < 0) {
        return -1;
    }

    /* Classpath */
    if (!wrapperData->environmentClasspath) {
        if ((index = wrapperBuildJavaCommandArrayClasspath(strings, addQuotes, index, classpath)) < 0) {
            return -1;
        }
    }

    /* Store the Wrapper key */
    if (strings) {
        strings[index] = malloc(sizeof(TCHAR) * (16 + _tcslen(wrapperData->key) + 1));
        if (!strings[index]) {
            outOfMemory(TEXT("WBJCAI"), 24);
            return -1;
        }
        if (addQuotes) {
            _sntprintf(strings[index], 16 + _tcslen(wrapperData->key) + 1, TEXT("-Dwrapper.key=\"%s\""), wrapperData->key);
        } else {
            _sntprintf(strings[index], 16 + _tcslen(wrapperData->key) + 1, TEXT("-Dwrapper.key=%s"), wrapperData->key);
        }
    }
    index++;
    
    /* Store the backend connection information. */
    if (wrapperData->backendType == WRAPPER_BACKEND_TYPE_PIPE) {
        if (strings) {
            strings[index] = malloc(sizeof(TCHAR) * (22 + 1));
            if (!strings[index]) {
                outOfMemory(TEXT("WBJCAI"), 25);
                return -1;
            }
            _sntprintf(strings[index], 22 + 1, TEXT("-Dwrapper.backend=pipe"));
        }
        index++;
    } else {

        /* default is socket ipv4, so we have to specify ipv6 if it's the case */
        if (wrapperData->backendType == WRAPPER_BACKEND_TYPE_SOCKET_V6) {
            if (strings) {
                strings[index] = malloc(sizeof(TCHAR) * (29 + 1));
                if (!strings[index]) {
                    outOfMemory(TEXT("WBJCAI"), 261);
                    return -1;
                }
                _sntprintf(strings[index], 29 + 1, TEXT("-Dwrapper.backend=socket_ipv6"));
            }
            index++;

            /* specify to the JVM to change the preference and use IPv6 addresses over IPv4 ones where possible. */
            if (strings) {
                strings[index] = malloc(sizeof(TCHAR) * (35 + 1));
                if (!strings[index]) {
                    outOfMemory(TEXT("WBJCAI"), 262);
                    return -1;
                }
                _sntprintf(strings[index], 35 + 1, TEXT("-Djava.net.preferIPv6Addresses=TRUE"));
            }
            index++;
        }

        /* Store the Wrapper server port */
        if (strings) {
            strings[index] = malloc(sizeof(TCHAR) * (15 + 5 + 1));  /* Port up to 5 characters */
            if (!strings[index]) {
                outOfMemory(TEXT("WBJCAI"), 26);
                return -1;
            }
            _sntprintf(strings[index], 15 + 5 + 1, TEXT("-Dwrapper.port=%d"), (int)wrapperData->actualPort);
        }
        index++;
    }

    /* Store the Wrapper jvm min and max ports. */
    if ((wrapperData->backendType == WRAPPER_BACKEND_TYPE_SOCKET_V4) || (wrapperData->backendType == WRAPPER_BACKEND_TYPE_SOCKET_V6)) {
        if (wrapperData->portAddress != NULL) {
            if (strings) {
                strings[index] = malloc(sizeof(TCHAR) * (_tcslen(TEXT("-Dwrapper.port.address=")) + _tcslen(wrapperData->portAddress) + 1));
                if (!strings[index]) {
                    outOfMemory(TEXT("WBJCAI"), 27);
                    return -1;
                }
                _sntprintf(strings[index], _tcslen(TEXT("-Dwrapper.port.address=")) + _tcslen(wrapperData->portAddress) + 1, TEXT("-Dwrapper.port.address=%s"), wrapperData->portAddress);
            }
            index++;
        }
        
        if (wrapperData->jvmPort >= 0) {
            if (strings) {
                strings[index] = malloc(sizeof(TCHAR) * (19 + 5 + 1));  /* Port up to 5 characters */
                if (!strings[index]) {
                    outOfMemory(TEXT("WBJCAI"), 28);
                    return -1;
                }
                _sntprintf(strings[index], 19 + 5 + 1, TEXT("-Dwrapper.jvm.port=%d"), wrapperData->jvmPort);
            }
            index++;
        }
        
        if (strings) {
            strings[index] = malloc(sizeof(TCHAR) * (23 + 5 + 1));  /* Port up to 5 characters */
            if (!strings[index]) {
                outOfMemory(TEXT("WBJCAI"), 29);
                return -1;
            }
            _sntprintf(strings[index], 23 + 5 + 1, TEXT("-Dwrapper.jvm.port.min=%d"), wrapperData->jvmPortMin);
        }
        index++;
        
        if (strings) {
            strings[index] = malloc(sizeof(TCHAR) * (23 + 5 + 1));  /* Port up to 5 characters */
            if (!strings[index]) {
                outOfMemory(TEXT("WBJCAI"), 30);
                return -1;
            }
            _sntprintf(strings[index], 23 + 5 + 1, TEXT("-Dwrapper.jvm.port.max=%d"), wrapperData->jvmPortMax);
        }
        index++;
    }
    /* Store the Wrapper debug flag */
    if (wrapperData->isDebugging) {
        if (strings) {
            strings[index] = malloc(sizeof(TCHAR) * (22 + 1));
            if (!strings[index]) {
                outOfMemory(TEXT("WBJCAI"), 31);
                return -1;
            }
            if (addQuotes) {
                _sntprintf(strings[index], 22 + 1, TEXT("-Dwrapper.debug=\"TRUE\""));
            } else {
                _sntprintf(strings[index], 22 + 1, TEXT("-Dwrapper.debug=TRUE"));
            }
        }
        index++;
    }

    /* Store the Wrapper disable console input flag. */
    if (getBooleanProperty(properties, TEXT("wrapper.disable_console_input"),
#ifdef WIN32
            FALSE
#else
            wrapperData->daemonize /* We want to disable console input by default when daemonized. */
#endif
        )) {
        if (strings) {
            strings[index] = malloc(sizeof(TCHAR) * (38 + 1));
            if (!strings[index]) {
                outOfMemory(TEXT("WBJCAI"), 32);
                return -1;
            }
            if (addQuotes) {
                _sntprintf(strings[index], 38 + 1, TEXT("-Dwrapper.disable_console_input=\"TRUE\""));
            } else {
                _sntprintf(strings[index], 38 + 1, TEXT("-Dwrapper.disable_console_input=TRUE"));
            }
        }
        index++;
    }

    /* Store the Wrapper listener force stop flag. */
    if (getBooleanProperty(properties, TEXT("wrapper.listener.force_stop"), FALSE)) {
        if (strings) {
            strings[index] = malloc(sizeof(TCHAR) * (38 + 1));
            if (!strings[index]) {
                outOfMemory(TEXT("WBJCAI"), 33);
                return -1;
            }
            if (addQuotes) {
                _sntprintf(strings[index], 38 + 1, TEXT("-Dwrapper.listener.force_stop=\"TRUE\""));
            } else {
                _sntprintf(strings[index], 38 + 1, TEXT("-Dwrapper.listener.force_stop=TRUE"));
            }
        }
        index++;
    }

    /* Store the Wrapper PID */
    if (strings) {
        strings[index] = malloc(sizeof(TCHAR) * (24 + 1)); /* Pid up to 10 characters */
        if (!strings[index]) {
            outOfMemory(TEXT("WBJCAI"), 34);
            return -1;
        }
#if defined(SOLARIS) && (!defined(_LP64))
        _sntprintf(strings[index], 24 + 1, TEXT("-Dwrapper.pid=%ld"), wrapperData->wrapperPID);
#else
        _sntprintf(strings[index], 24 + 1, TEXT("-Dwrapper.pid=%d"), wrapperData->wrapperPID);
#endif
    }
    index++;

    /* Store a flag telling the JVM to use the system clock. */
    if (wrapperData->useSystemTime) {
        if (strings) {
            strings[index] = malloc(sizeof(TCHAR) * (32 + 1));
            if (!strings[index]) {
                outOfMemory(TEXT("WBJCAI"), 35);
                return -1;
            }
            if (addQuotes) {
                _sntprintf(strings[index], 32 + 1, TEXT("-Dwrapper.use_system_time=\"TRUE\""));
            } else {
                _sntprintf(strings[index], 32 + 1, TEXT("-Dwrapper.use_system_time=TRUE"));
            }
        }
        index++;
    } else {
        /* Only pass the timer fast and slow thresholds to the JVM if they are not default.
         *  These are only used if the system time is not being used. */
        if (wrapperData->timerFastThreshold != WRAPPER_TIMER_FAST_THRESHOLD) {
            if (strings) {
                strings[index] = malloc(sizeof(TCHAR) * (43 + 1)); /* Allow for 10 digits */
                if (!strings[index]) {
                    outOfMemory(TEXT("WBJCAI"), 36);
                    return -1;
                }
                if (addQuotes) {
                    _sntprintf(strings[index], 43 + 1, TEXT("-Dwrapper.timer_fast_threshold=\"%d\""), wrapperData->timerFastThreshold * WRAPPER_TICK_MS / 1000);
                } else {
                    _sntprintf(strings[index], 43 + 1, TEXT("-Dwrapper.timer_fast_threshold=%d"), wrapperData->timerFastThreshold * WRAPPER_TICK_MS / 1000);
                }
            }
            index++;
        }
        if (wrapperData->timerSlowThreshold != WRAPPER_TIMER_SLOW_THRESHOLD) {
            if (strings) {
                strings[index] = malloc(sizeof(TCHAR) * (43 + 1)); /* Allow for 10 digits */
                if (!strings[index]) {
                    outOfMemory(TEXT("WBJCAI"), 37);
                    return -1;
                }
                if (addQuotes) {
                    _sntprintf(strings[index], 43 + 1, TEXT("-Dwrapper.timer_slow_threshold=\"%d\""), wrapperData->timerSlowThreshold * WRAPPER_TICK_MS / 1000);
                } else {
                    _sntprintf(strings[index], 43 + 1, TEXT("-Dwrapper.timer_slow_threshold=%d"), wrapperData->timerSlowThreshold * WRAPPER_TICK_MS / 1000);
                }
            }
            index++;
        }
    }

    /* Always write the version of the wrapper binary as a property.  The
     *  WrapperManager class uses it to verify that the version matches. */
    if (strings) {
        strings[index] = malloc(sizeof(TCHAR) * (20 + _tcslen(wrapperVersion) + 1));
        if (!strings[index]) {
            outOfMemory(TEXT("WBJCAI"), 38);
            return -1;
        }
        if (addQuotes) {
            _sntprintf(strings[index], 20 + _tcslen(wrapperVersion) + 1, TEXT("-Dwrapper.version=\"%s\""), wrapperVersion);
        } else {
            _sntprintf(strings[index], 20 + _tcslen(wrapperVersion) + 1, TEXT("-Dwrapper.version=%s"), wrapperVersion);
        }
    }
    index++;

    /* Store the base name of the native library. */
    if (strings) {
        strings[index] = malloc(sizeof(TCHAR) * (27 + _tcslen(wrapperData->nativeLibrary) + 1));
        if (!strings[index]) {
            outOfMemory(TEXT("WBJCAI"), 39);
            return -1;
        }
        if (addQuotes) {
            _sntprintf(strings[index], 27 + _tcslen(wrapperData->nativeLibrary) + 1, TEXT("-Dwrapper.native_library=\"%s\""), wrapperData->nativeLibrary);
        } else {
            _sntprintf(strings[index], 27 + _tcslen(wrapperData->nativeLibrary) + 1, TEXT("-Dwrapper.native_library=%s"), wrapperData->nativeLibrary);
        }
    }
    index++;

    /* Store the arch name of the wrapper. */
    if (strings) {
        strings[index] = malloc(sizeof(TCHAR) * (17 + _tcslen(wrapperArch) + 1));
        if (!strings[index]) {
            outOfMemory(TEXT("WBJCAI"), 40);
            return -1;
        }
        if (addQuotes) {
            _sntprintf(strings[index], 17 + _tcslen(wrapperArch) + 1, TEXT("-Dwrapper.arch=\"%s\""), wrapperArch);
        } else {
            _sntprintf(strings[index], 17 + _tcslen(wrapperArch) + 1, TEXT("-Dwrapper.arch=%s"), wrapperArch);
        }
    }
    index++;

    /* Store the ignore signals flag if configured to do so */
    if (wrapperData->ignoreSignals & WRAPPER_IGNORE_SIGNALS_JAVA) {
        if (strings) {
            strings[index] = malloc(sizeof(TCHAR) * (31 + 1));
            if (!strings[index]) {
                outOfMemory(TEXT("WBJCAI"), 41);
                return -1;
            }
            if (addQuotes) {
                _sntprintf(strings[index], 31 + 1, TEXT("-Dwrapper.ignore_signals=\"TRUE\""));
            } else {
                _sntprintf(strings[index], 31 + 1, TEXT("-Dwrapper.ignore_signals=TRUE"));
            }
        }
        index++;
    }

    /* If this is being run as a service, add a service flag. */
    if (!wrapperData->isConsole) {
        if (strings) {
            strings[index] = malloc(sizeof(TCHAR) * (24 + 1));
            if (!strings[index]) {
                outOfMemory(TEXT("WBJCAI"), 42);
                return -1;
            }
            if (addQuotes) {
                _sntprintf(strings[index], 24 + 1, TEXT("-Dwrapper.service=\"TRUE\""));
            } else {
                _sntprintf(strings[index], 24 + 1, TEXT("-Dwrapper.service=TRUE"));
            }
        }
        index++;
    }

    /* Store the Disable Tests flag */
    if (wrapperData->isTestsDisabled) {
        if (strings) {
            strings[index] = malloc(sizeof(TCHAR) * (30 + 1));
            if (!strings[index]) {
                outOfMemory(TEXT("WBJCAI"), 43);
                return -1;
            }
            if (addQuotes) {
                _sntprintf(strings[index], 30 + 1, TEXT("-Dwrapper.disable_tests=\"TRUE\""));
            } else {
                _sntprintf(strings[index], 30 + 1, TEXT("-Dwrapper.disable_tests=TRUE"));
            }
        }
        index++;
    }

    /* Store the Disable Shutdown Hook flag */
    if (wrapperData->isShutdownHookDisabled) {
        if (strings) {
            strings[index] = malloc(sizeof(TCHAR) * (38 + 1));
            if (!strings[index]) {
                outOfMemory(TEXT("WBJCAI"), 44);
                return -1;
            }
            if (addQuotes) {
                _sntprintf(strings[index], 38 + 1, TEXT("-Dwrapper.disable_shutdown_hook=\"TRUE\""));
            } else {
                _sntprintf(strings[index], 38 + 1, TEXT("-Dwrapper.disable_shutdown_hook=TRUE"));
            }
        }
        index++;
    }

    /* Store the CPU Timeout value */
    if (strings) {
        /* Just to be safe, allow 20 characters for the timeout value */
        strings[index] = malloc(sizeof(TCHAR) * (24 + 20 + 1));
        if (!strings[index]) {
            outOfMemory(TEXT("WBJCAI"), 45);
            return -1;
        }
        if (addQuotes) {
            _sntprintf(strings[index], 24 + 20 + 1, TEXT("-Dwrapper.cpu.timeout=\"%d\""), wrapperData->cpuTimeout);
        } else {
            _sntprintf(strings[index], 24 + 20 + 1, TEXT("-Dwrapper.cpu.timeout=%d"), wrapperData->cpuTimeout);
        }
    }
    index++;

    if ((prop = getStringProperty(properties, TEXT("wrapper.java.outfile"), NULL))) {
        if (strings) {
            strings[index] = malloc(sizeof(TCHAR) * (25 + _tcslen(prop) + 1));
            if (!strings[index]) {
                outOfMemory(TEXT("WBJCAI"), 47);
                return -1;
            }
            if (addQuotes) {
                _sntprintf(strings[index], 25 + _tcslen(prop) + 1, TEXT("-Dwrapper.java.outfile=\"%s\""), prop);
            } else {
                _sntprintf(strings[index], 25 + _tcslen(prop) + 1, TEXT("-Dwrapper.java.outfile=%s"), prop);
            }
        }
        index++;
    }

    if ((prop = getStringProperty(properties, TEXT("wrapper.java.errfile"), NULL))) {
        if (strings) {
            strings[index] = malloc(sizeof(TCHAR) * (25 + _tcslen(prop) + 1));
            if (!strings[index]) {
                outOfMemory(TEXT("WBJCAI"), 48);
                return -1;
            }
            if (addQuotes) {
                _sntprintf(strings[index],  25 + _tcslen(prop) + 1, TEXT("-Dwrapper.java.errfile=\"%s\""), prop);
            } else {
                _sntprintf(strings[index], 25 + _tcslen(prop) + 1, TEXT("-Dwrapper.java.errfile=%s"), prop);
            }
        }
        index++;
    }

    /* Store the Wrapper JVM ID.  (Get here before incremented) */
    if (strings) {
        strings[index] = malloc(sizeof(TCHAR) * (16 + 5 + 1));  /* jvmid up to 5 characters */
        if (!strings[index]) {
            outOfMemory(TEXT("WBJCAI"), 49);
            return -1;
        }
        _sntprintf(strings[index], 16 + 5 + 1, TEXT("-Dwrapper.jvmid=%d"), (wrapperData->jvmRestarts + 1));
    }
    index++;


    /* If this JVM will be detached after startup, it needs to know that. */
    if (wrapperData->detachStarted) {
        if (strings) {
            strings[index] = malloc(sizeof(TCHAR) * (30 + 1));
            if (!strings[index]) {
                outOfMemory(TEXT("WBJCAI"), 52);
                return -1;
            }
            if (addQuotes) {
                _sntprintf(strings[index], 30 + 1, TEXT("-Dwrapper.detachStarted=\"TRUE\""));
            } else {
                _sntprintf(strings[index], 30 + 1, TEXT("-Dwrapper.detachStarted=TRUE"));
            }
        }
        index++;
    }

    /* Store the main class */
    prop = getStringProperty(properties, TEXT("wrapper.java.mainclass"), TEXT("Main"));
    if (_tcscmp(prop, TEXT("org.tanukisoftware.wrapper.test.Main")) == 0) {
        thisIsTestWrapper = TRUE;
    } else {
        thisIsTestWrapper = FALSE;
    }
    if (strings) {
        strings[index] = malloc(sizeof(TCHAR) * (_tcslen(prop) + 1));
        if (!strings[index]) {
            outOfMemory(TEXT("WBJCAI"), 53);
            return -1;
        }
        _sntprintf(strings[index], _tcslen(prop) + 1, TEXT("%s"), prop);
    }
    index++;

    /* Store any application parameters */
    if ((index = wrapperBuildJavaCommandArrayAppParameters(strings, addQuotes, index, thisIsTestWrapper)) < 0) {
        return -1;
    }

    if ((index = wrapperLoadParameterFile(strings, addQuotes, detectDebugJVM, index, TEXT("wrapper.app.parameter_file"), FALSE)) < 0) {
        return -1;
    }

    return index;
}

/**
 * command is a pointer to a pointer of an array of character strings.
 * length is the number of strings in the above array.
 *
 * @return TRUE if there were any problems.
 */
int wrapperBuildJavaCommandArray(TCHAR ***stringsPtr, int *length, int addQuotes, const TCHAR *classpath) {
    int reqLen;

    /* Reset the flag stating that the JVM is a debug JVM. */
    wrapperData->debugJVM = FALSE;
    wrapperData->debugJVMTimeoutNotified = FALSE;

    /* Find out how long the array needs to be first. */
    reqLen = wrapperBuildJavaCommandArrayInner(NULL, addQuotes, classpath);
    if (reqLen < 0) {
        return TRUE;
    }
    *length = reqLen;

    /* Allocate the correct amount of memory */
    *stringsPtr = malloc((*length) * sizeof **stringsPtr );
    if (!(*stringsPtr)) {
        outOfMemory(TEXT("WBJCA"), 1);
        return TRUE;
    }

    /* Now actually fill in the strings */
    reqLen = wrapperBuildJavaCommandArrayInner(*stringsPtr, addQuotes, classpath);
    if (reqLen < 0) {
        return TRUE;
    }

    if (wrapperData->debugJVM) {
        if ((wrapperData->startupTimeout > 0) || (wrapperData->pingTimeout > 0) ||
            (wrapperData->shutdownTimeout > 0) || (wrapperData->jvmExitTimeout > 0)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                TEXT("---------------------------------------------------------------------") );
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                TEXT(
                     "The JVM is being launched with a debugger enabled and could possibly\nbe suspended.  To avoid unwanted shutdowns, timeouts will be\ndisabled, removing the ability to detect and restart frozen JVMs."));
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                TEXT("---------------------------------------------------------------------") );
        }
    }

    return FALSE;
}

void wrapperFreeJavaCommandArray(TCHAR **strings, int length) {
    int i;

    if (strings != NULL) {
        /* Loop over and free each of the strings in the array */
        for (i = 0; i < length; i++) {
            if (strings[i] != NULL) {
                free(strings[i]);
                strings[i] = NULL;
            }
        }
        free(strings);
        strings = NULL;
    }
}

/**
 * Called when the Wrapper detects that the JVM process has exited.
 *  Contains code common to all platforms.
 */
void wrapperJVMProcessExited(TICKS nowTicks, int exitCode) {
    int setState = TRUE;
#ifdef WIN32
    const TCHAR* errorDescriptionWrapper;
    const TCHAR* errorDescriptionJVM;
    HMODULE handle;
#endif

    if (exitCode == 0) {
        /* The JVM exit code was 0, so leave any current exit code as is. */
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
            TEXT("JVM process exited with a code of %d, leaving the Wrapper exit code set to %d."),
            exitCode, wrapperData->exitCode);

    } else if (wrapperData->exitCode == 0) {
        /* Update the Wrapper exitCode. */
        wrapperData->exitCode = exitCode;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
            TEXT("JVM process exited with a code of %d, setting the Wrapper exit code to %d."),
            exitCode, wrapperData->exitCode);

    } else {
        /* The Wrapper exit code was already non-zero, so leave it as is. */
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
            TEXT("JVM process exited with a code of %d, however the Wrapper exit code was already %d."),
            exitCode, wrapperData->exitCode);
    }

    /* lets display a description of the error codes */
#ifdef WIN32
    if ((exitCode != 0) || (wrapperData->exitCode != 0)) {
        handle = LoadLibrary(TEXT("ntdll.dll"));
        if (handle) {
            if (exitCode != 0) {
                errorDescriptionJVM = getErrorText(exitCode, handle);
                /* error 317 means the message failed to be formatted */
                if(GetLastError() != 317 && errorDescriptionJVM[0] != TEXT('\0')) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                        TEXT("  %d: %s"),
                        exitCode, errorDescriptionJVM);
                }
            }
            
            if ((wrapperData->exitCode != 0) && (wrapperData->exitCode != exitCode)) {
                errorDescriptionWrapper = getErrorText(wrapperData->exitCode, handle);
                /* error 317 means the message failed to be formatted */
                if(GetLastError() != 317 && errorDescriptionWrapper[0] != TEXT('\0')) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                        TEXT("  %d: %s"),
                        wrapperData->exitCode, errorDescriptionWrapper);
                }
            }
            FreeLibrary(handle);
        }
    }
#endif

    switch(wrapperData->jState) {
    case WRAPPER_JSTATE_DOWN_CLEAN:
    case WRAPPER_JSTATE_DOWN_CHECK:
    case WRAPPER_JSTATE_DOWN_FLUSH:
        /* Shouldn't be called in this state.  But just in case. */
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                TEXT("JVM already down."));
        }
        setState = FALSE;
        break;

    case WRAPPER_JSTATE_LAUNCH_DELAY:
        /* We got a message that the JVM process died when we already thought is was down.
         *  Most likely this was caused by a SIGCHLD signal.  We are already in the expected
         *  state so go ahead and ignore it.  Do NOT go back to DOWN or the restart flag
         *  and all restart counts will have be lost */
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                TEXT("Received a message that the JVM is down when in the LAUNCH(DELAY) state."));
        }
        setState = FALSE;
        break;

    case WRAPPER_JSTATE_RESTART:
        /* We got a message that the JVM process died when we already thought is was down.
         *  Most likely this was caused by a SIGCHLD signal.  We are already in the expected
         *  state so go ahead and ignore it.  Do NOT go back to DOWN or the restart flag
         *  and all restart counts will have be lost */
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                TEXT("Received a message that the JVM is down when in the RESTART state."));
        }
        setState = FALSE;
        break;

    case WRAPPER_JSTATE_LAUNCH:
        /* We got a message that the JVM process died when we already thought is was down.
         *  Most likely this was caused by a SIGCHLD signal.  We are already in the expected
         *  state so go ahead and ignore it.  Do NOT go back to DOWN or the restart flag
         *  and all restart counts will have be lost.
         * This can happen if the Java process dies Immediately after it is launched.  It
         *  is very rare if Java is launched, but can happen if the configuration is set to
         *  launch something else. */
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                TEXT("Received a message that the JVM is down when in the LAUNCH state."));
        }
        setState = FALSE;
        break;

    case WRAPPER_JSTATE_LAUNCHING:
        wrapperData->restartRequested = WRAPPER_RESTART_REQUESTED_AUTOMATIC;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
            TEXT("JVM exited while loading the application."));
        break;

    case WRAPPER_JSTATE_LAUNCHED:
        /* Shouldn't be called in this state, but just in case. */
        wrapperData->restartRequested = WRAPPER_RESTART_REQUESTED_AUTOMATIC;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
           TEXT("JVM exited before starting the application."));
        break;

    case WRAPPER_JSTATE_STARTING:
        wrapperData->restartRequested = WRAPPER_RESTART_REQUESTED_AUTOMATIC;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
            TEXT("JVM exited while starting the application."));
        break;

    case WRAPPER_JSTATE_STARTED:
        wrapperData->restartRequested = WRAPPER_RESTART_REQUESTED_AUTOMATIC;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
            TEXT("JVM exited unexpectedly."));
        break;

    case WRAPPER_JSTATE_STOP:
    case WRAPPER_JSTATE_STOPPING:
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            TEXT("JVM exited unexpectedly while stopping the application."));
        break;

    case WRAPPER_JSTATE_STOPPED:
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                TEXT("JVM exited normally."));
        }
        break;

    case WRAPPER_JSTATE_KILLING:
    case WRAPPER_JSTATE_KILL:
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO,
            TEXT("JVM exited on its own while waiting to kill the application."));
        break;

    case WRAPPER_JSTATE_KILLED:
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
            TEXT("JVM exited after being requested to terminate."));
        break;

    default:
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            TEXT("Unexpected jState=%d in wrapperJVMProcessExited."), wrapperData->jState);
        break;
    }

    wrapperJVMDownCleanup(setState);
}

void wrapperBuildKey() {
    int i;
    size_t kcNum;
    size_t num;
    static int seeded = FALSE;

    /* Seed the randomizer */
    if (!seeded) {
        srand((unsigned)time(NULL));
        seeded = TRUE;
    }

    /* Start by generating a key */
    num = _tcslen(keyChars);

    for (i = 0; i < 16; i++) {
        /* The way rand works, this will sometimes equal num, which is too big.
         *  This is rare so just round those cases down. */

        /* Some platforms use very large RAND_MAX values that cause overflow problems in our math */
        if (RAND_MAX > 0x10000) {
            kcNum = (size_t)((rand() >> 8) * num / (RAND_MAX >> 8));
        } else {
            kcNum = (size_t)(rand() * num / RAND_MAX);
        }

        if (kcNum >= num) {
            kcNum = num - 1;
        }

        wrapperData->key[i] = keyChars[kcNum];
    }
    wrapperData->key[16] = TEXT('\0');

    /*
    printf("  Key=%s Len=%lu\n", wrapperData->key, _tcslen(wrapperData->key));
    */
}

#ifdef WIN32

/* The ABOVE and BELOW normal priority class constants are not defined in MFVC 6.0 headers. */
#ifndef ABOVE_NORMAL_PRIORITY_CLASS
#define ABOVE_NORMAL_PRIORITY_CLASS 0x00008000
#endif
#ifndef BELOW_NORMAL_PRIORITY_CLASS
#define BELOW_NORMAL_PRIORITY_CLASS 0x00004000
#endif

/**
 * Return FALSE if successful, TRUE if there were problems.
 */
int wrapperBuildNTServiceInfo() {
    TCHAR *work;
    const TCHAR *priority;
    size_t len;
    size_t valLen;
    size_t workLen;
    int i;
    TCHAR **propertyNames;
    TCHAR **propertyValues;
    long unsigned int *propertyIndices;

    if (!wrapperData->configured) {
        /* Load the service load order group */
        updateStringValue(&wrapperData->ntServiceLoadOrderGroup, getStringProperty(properties, TEXT("wrapper.ntservice.load_order_group"), TEXT("")));

        if (getStringProperties(properties, TEXT("wrapper.ntservice.dependency."), TEXT(""), wrapperData->ignoreSequenceGaps, FALSE, &propertyNames, &propertyValues, &propertyIndices)) {
            /* Failed */
            return TRUE;
        }

        /* Build the dependency list.  Decide how large the list needs to be */
        len = 0;
        i = 0;
        while (propertyNames[i]) {
            valLen = _tcslen(propertyValues[i]);
            if (valLen > 0) {
                len += valLen + 1;
            }
            i++;
        }
        /* List must end with a double '\0'.  If the list is not empty then it will end with 3.  But that is fine. */
        len += 2;

        /* Actually build the buffer */
        if (wrapperData->ntServiceDependencies) {
            /** This is a reload, so free up the old data. */
            free(wrapperData->ntServiceDependencies);
            wrapperData->ntServiceDependencies = NULL;
        }
        work = wrapperData->ntServiceDependencies = malloc(sizeof(TCHAR) * len);
        if (!work) {
            outOfMemory(TEXT("WBNTSI"), 1);
            return TRUE;
        }
        workLen = len;

        /* Now actually build up the list. Each value is separated with a '\0'. */
        i = 0;
        while (propertyNames[i]) {
            valLen = _tcslen(propertyValues[i]);
            if (valLen > 0) {
                _tcsncpy(work, propertyValues[i], workLen);
                work += valLen + 1;
                workLen -= valLen + 1;
            }
            i++;
        }
        /* Add two more nulls to the end of the list. */
        work[0] = TEXT('\0');
        work[1] = TEXT('\0');

        /* Memory allocated in work is stored in wrapperData.  The memory should not be released here. */
        work = NULL;

        freeStringProperties(propertyNames, propertyValues, propertyIndices);

        /* Set the service start type */
        if (strcmpIgnoreCase(getStringProperty(properties, TEXT("wrapper.ntservice.starttype"), TEXT("DEMAND_START")), TEXT("AUTO_START")) == 0) {
            wrapperData->ntServiceStartType = SERVICE_AUTO_START;
        } else {
            wrapperData->ntServiceStartType = SERVICE_DEMAND_START;
        }

        /* Set the service priority class */
        priority = getStringProperty(properties, TEXT("wrapper.ntservice.process_priority"), TEXT("NORMAL"));
        if ( (strcmpIgnoreCase(priority, TEXT("LOW")) == 0) || (strcmpIgnoreCase(priority, TEXT("IDLE")) == 0) ) {
            wrapperData->ntServicePriorityClass = IDLE_PRIORITY_CLASS;
        } else if (strcmpIgnoreCase(priority, TEXT("HIGH")) == 0) {
            wrapperData->ntServicePriorityClass = HIGH_PRIORITY_CLASS;
        } else if (strcmpIgnoreCase(priority, TEXT("REALTIME")) == 0) {
            wrapperData->ntServicePriorityClass = REALTIME_PRIORITY_CLASS;
        } else if (strcmpIgnoreCase(priority, TEXT("ABOVE_NORMAL")) == 0) {
            wrapperData->ntServicePriorityClass = ABOVE_NORMAL_PRIORITY_CLASS;
        } else if (strcmpIgnoreCase(priority, TEXT("BELOW_NORMAL")) == 0) {
            wrapperData->ntServicePriorityClass = BELOW_NORMAL_PRIORITY_CLASS;
        } else {
            wrapperData->ntServicePriorityClass = NORMAL_PRIORITY_CLASS;
        }

        /* Account name */
        updateStringValue(&wrapperData->ntServiceAccount, getStringProperty(properties, TEXT("wrapper.ntservice.account"), NULL));
        if (wrapperData->ntServiceAccount && (_tcslen(wrapperData->ntServiceAccount) <= 0)) {
            wrapperData->ntServiceAccount = NULL;
        }

        /* Account password */
        wrapperData->ntServicePrompt = getBooleanProperty(properties, TEXT("wrapper.ntservice.account.prompt"), FALSE);
        if (wrapperData->ntServicePrompt == TRUE) {
            wrapperData->ntServicePasswordPrompt = TRUE;
        } else {
            wrapperData->ntServicePasswordPrompt = getBooleanProperty(properties, TEXT("wrapper.ntservice.password.prompt"), FALSE);
        }
        wrapperData->ntServicePasswordPromptMask = getBooleanProperty(properties, TEXT("wrapper.ntservice.password.prompt.mask"), TRUE);
        updateStringValue(&wrapperData->ntServicePassword, getStringProperty(properties, TEXT("wrapper.ntservice.password"), NULL));
        if ( wrapperData->ntServicePassword && ( _tcslen( wrapperData->ntServicePassword ) <= 0 ) ) {
            wrapperData->ntServicePassword = NULL;
        }
        if (!wrapperData->ntServiceAccount) {
            /* If there is not account name, then the password must not be set. */
            wrapperData->ntServicePassword = NULL;
        }

        /* Interactive */
        wrapperData->ntServiceInteractive = getBooleanProperty(properties, TEXT("wrapper.ntservice.interactive"), FALSE);
        /* The interactive flag can not be set if an account is also set. */
        if (wrapperData->ntServiceAccount && wrapperData->ntServiceInteractive) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                TEXT("Ignoring the wrapper.ntservice.interactive property because it can not be set when wrapper.ntservice.account is also set."));
            wrapperData->ntServiceInteractive = FALSE;
        }

        /* Display a Console Window. */
        wrapperData->ntAllocConsole = getBooleanProperty(properties, TEXT("wrapper.ntservice.console"), FALSE);
        /* Set the default hide wrapper console flag to the inverse of the alloc console flag. */
        wrapperData->ntHideWrapperConsole = !wrapperData->ntAllocConsole;

        /* Hide the JVM Console Window. */
        wrapperData->ntHideJVMConsole = getBooleanProperty(properties, TEXT("wrapper.ntservice.hide_console"), TRUE);

        /* Make sure that a console is always generated to support thread dumps */
        wrapperData->generateConsole = getBooleanProperty(properties, TEXT("wrapper.ntservice.generate_console"), TRUE);
        
#ifndef WINIA
        /* Accept preshutdown control code? */
        wrapperData->ntPreshutdown = getBooleanProperty(properties, TEXT("wrapper.ntservice.preshutdown"), FALSE);
        if (wrapperData->ntPreshutdown) {
            if (!isVista()) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                    TEXT("Property %s is ignored on this system."), TEXT("wrapper.ntservice.preshutdown"));
                removeProperty(properties, TEXT("wrapper.ntservice.preshutdown"));
                wrapperData->ntPreshutdown = FALSE;
            } else {
                wrapperData->ntPreshutdownTimeout = getIntProperty(properties, TEXT("wrapper.ntservice.preshutdown.timeout"), 180);
                if (wrapperData->ntPreshutdownTimeout > 3600) {
                    wrapperData->ntPreshutdownTimeout = 3600;
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                        TEXT("The value of %s must be at most %d hour (%d second(s)).  Changing to %d."), TEXT("wrapper.ntservice.preshutdown.timeout"), 1, 3600, wrapperData->ntPreshutdownTimeout);
                } else if (wrapperData->ntPreshutdownTimeout < 1) {
                    wrapperData->ntPreshutdownTimeout = 1;
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                        TEXT("The value of %s must be at least %d second(s).  Changing to %d."), TEXT("wrapper.ntservice.preshutdown.timeout"), 1, wrapperData->ntPreshutdownTimeout);
                }
            }
        }
#else
        if (getBooleanProperty(properties, TEXT("wrapper.ntservice.preshutdown"), FALSE)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                TEXT("Property %s is not supported on Windows Itanium."), TEXT("wrapper.ntservice.preshutdown"));
            removeProperty(properties, TEXT("wrapper.ntservice.preshutdown"));
        }
#endif
        
        /* Wait hint used when reporting STARTING, RESUMING, PAUSING or STOPPING status to the service controller. */
        wrapperData->ntStartupWaitHint = getIntProperty(properties, TEXT("wrapper.ntservice.startup.waithint"), 30);
        wrapperData->ntShutdownWaitHint = getIntProperty(properties, TEXT("wrapper.ntservice.shutdown.waithint"), 30);
        
        /* Set the range of valid values for the wait hints to [2secs, 1min] */
        if (wrapperData->ntStartupWaitHint > 60) {
            wrapperData->ntStartupWaitHint = 60;
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                TEXT("The value of %s must be at most %d second(s).  Changing to %d."), TEXT("wrapper.ntservice.startup.waithint"), 60, wrapperData->ntStartupWaitHint);
        } else if (wrapperData->ntStartupWaitHint < 0) {
            wrapperData->ntStartupWaitHint = 2;
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                TEXT("The value of %s must be at least %2 second(s).  Changing to %d."), TEXT("wrapper.ntservice.startup.waithint"), 1, wrapperData->ntStartupWaitHint);
        }
        if (wrapperData->ntShutdownWaitHint > 60) {
            wrapperData->ntShutdownWaitHint = 60;
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                TEXT("The value of %s must be at most %d second(s).  Changing to %d."), TEXT("wrapper.ntservice.shutdown.waithint"), 60, wrapperData->ntStartupWaitHint);
        } else if (wrapperData->ntShutdownWaitHint < 0) {
            wrapperData->ntShutdownWaitHint = 2;
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                TEXT("The value of %s must be at least %2 second(s).  Changing to %d."), TEXT("wrapper.ntservice.shutdown.waithint"), 1, wrapperData->ntStartupWaitHint);
        }
    }

    /* Set the single invocation flag. */
    wrapperData->isSingleInvocation = getBooleanProperty(properties, TEXT("wrapper.single_invocation"), FALSE);

    wrapperData->threadDumpControlCode = getIntProperty(properties, TEXT("wrapper.thread_dump_control_code"), 255);
    if (wrapperData->threadDumpControlCode <= 0) {
        /* Disabled */
    } else if ((wrapperData->threadDumpControlCode < 128) || (wrapperData->threadDumpControlCode > 255)) {
        wrapperData->threadDumpControlCode = 255;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            TEXT("Ignoring the wrapper.thread_dump_control_code property because it must be in the range 128-255 or 0."));
    }

    return FALSE;
}
#endif

int validateTimeout(const TCHAR* propertyName, int value) {
    int okValue;
    if (value <= 0) {
        okValue = 0;
    } else if (value > WRAPPER_TIMEOUT_MAX) {
        okValue = WRAPPER_TIMEOUT_MAX;
    } else {
        okValue = value;
    }

    if (okValue != value) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            TEXT("The value of %s must be in the range 1 to %d seconds (%d days), or 0 to disable.  Changing to %d."),
            propertyName, WRAPPER_TIMEOUT_MAX, WRAPPER_TIMEOUT_MAX / 86400, okValue);
    }

    return okValue;
}

void wrapperLoadHostName() {
    char hostName[80];
    TCHAR* hostName2;
#ifdef UNICODE
    int len;
#endif

    if (gethostname(hostName, sizeof(hostName))) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT("Unable to obtain host name. %s"),
            getLastErrorText());
    } else {
#ifdef UNICODE
 #ifdef WIN32
        len = MultiByteToWideChar(CP_OEMCP, 0, hostName, -1, NULL, 0);
        if (len <= 0) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                TEXT("Invalid multibyte sequence in port address \"%s\" : %s"), hostName, getLastErrorText());
            return;
        }

        hostName2 = malloc(sizeof(LPWSTR) * (len + 1));
        if (!hostName2) {
            outOfMemory(TEXT("LHN"), 1);
            return;
        }
        MultiByteToWideChar(CP_OEMCP,0, hostName, -1, hostName2, len + 1);
 #else
        len = mbstowcs(NULL, hostName, MBSTOWCS_QUERY_LENGTH);
        if (len == (size_t)-1) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                TEXT("Invalid multibyte sequence in port address \"%s\" : %s"), hostName, getLastErrorText());
            return;
        }
        hostName2 = malloc(sizeof(TCHAR) * (len + 1));
        if (!hostName2) {
            outOfMemory(TEXT("LHN"), 2);
            return;
        }
        mbstowcs(hostName2, hostName, len + 1);
        hostName2[len] = TEXT('\0'); /* Avoid bufferflows caused by badly encoded characters. */
 #endif
#else
        /* No conversion needed.  Do an extra malloc here to keep the code simple below. */
        len = _tcslen(hostName);
        hostName2 = malloc(sizeof(TCHAR) * (len + 1));
        if (!hostName2) {
            outOfMemory(TEXT("LHN"), 3);
            return;
        }
        _tcsncpy(hostName2, hostName, len + 1);
#endif

        wrapperData->hostName = malloc(sizeof(TCHAR) * (_tcslen(hostName2) + 1));
        if (!wrapperData->hostName) {
            outOfMemory(TEXT("LHN"), 4);
            free(hostName2);
            return;
        }
        _tcsncpy(wrapperData->hostName, hostName2, _tcslen(hostName2) + 1);

        free(hostName2);
    }
}

/**
 * Resolves an action name into an actionId.
 *
 * @param actionName Action to be resolved.  (Contents of buffer will be converted to upper case.)
 * @param propertyName The name of the property where the action name originated.
 * @param logErrors TRUE if errors should be logged.
 *
 * @return The action Id, or 0 if it was unknown.
 */
int getActionForName(TCHAR *actionName, const TCHAR *propertyName, int logErrors) {
    size_t len;
    size_t i;
    int action;

    /* We need the actionName in upper case. */
    len = _tcslen(actionName);
    for (i = 0; i < len; i++) {
        actionName[i] = _totupper(actionName[i]);
    }

    if (_tcscmp(actionName, TEXT("RESTART")) == 0) {
        action = ACTION_RESTART;
    } else if (_tcscmp(actionName, TEXT("SHUTDOWN")) == 0) {
        action = ACTION_SHUTDOWN;
    } else if (_tcscmp(actionName, TEXT("DUMP")) == 0) {
        action = ACTION_DUMP;
    } else if (_tcscmp(actionName, TEXT("NONE")) == 0) {
        action = ACTION_NONE;
    } else if (_tcscmp(actionName, TEXT("DEBUG")) == 0) {
        action = ACTION_DEBUG;
    } else if (_tcscmp(actionName, TEXT("SUCCESS")) == 0) {
        action = ACTION_SUCCESS;
    } else if (_tcscmp(actionName, TEXT("GC")) == 0) {
        action = ACTION_GC;
    } else if (_tcscmp(actionName, TEXT("PAUSE")) == 0) {
        if (logErrors) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT("Pause actions require the Standard Edition.  Ignoring action '%s' in the %s property."), actionName, propertyName);
        }
        action = 0;
    } else if (_tcscmp(actionName, TEXT("RESUME")) == 0) {
        if (logErrors) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT("Resume actions require the Standard Edition.  Ignoring action '%s' in the %s property."), actionName, propertyName);
        }
        action = 0;
    } else if (_tcsstr(actionName, TEXT("USER_")) == actionName) {
        if (logErrors) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT("User actions require the Professional Edition.  Ignoring action '%s' in the %s property."), actionName, propertyName);
        }
        action = 0;
    } else {
        if (logErrors) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT("Encountered an unknown action '%s' in the %s property.  Skipping."), actionName, propertyName);
        }
        action = 0;
    }

    return action;
}

/**
 * Parses a list of actions for an action property.
 *
 * @param actionNameList A space separated list of action names.
 * @param propertyName The name of the property where the action name originated.
 *
 * @return an array of integer action ids, or NULL if there were any problems.
 */
int *wrapperGetActionListForNames(const TCHAR *actionNameList, const TCHAR *propertyName) {
    size_t len;
    TCHAR *workBuffer;
    TCHAR *token;
    int actionCount;
    int action;
    int *actionList = NULL;
#if defined(UNICODE) && !defined(WIN32)
    TCHAR *state;
#endif

#if _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("wrapperGetActionListForNames(%s, %s)"), actionNameList, propertyName);
#endif

    /* First get a count of the number of valid actions. */
    len = _tcslen(actionNameList);
    workBuffer = malloc(sizeof(TCHAR) * (len + 1));
    if (!workBuffer) {
        outOfMemory(TEXT("GALFN"), 1);
    } else {
        actionCount = 0;
        _tcsncpy(workBuffer, actionNameList, len + 1);
        token = _tcstok(workBuffer, TEXT(" ,")
#if defined(UNICODE) && !defined(WIN32)
            , &state
#endif
);
        while (token != NULL) {
            action = getActionForName(token, propertyName, TRUE);
            if (action == 0) {
                /* Unknown action */
            } else {
                actionCount++;
            }
#if _DEBUG
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("  action='%s' -> %d"), token, action);
#endif
            token = _tcstok(NULL, TEXT(" ,")
#if defined(UNICODE) && !defined(WIN32)
            , &state
#endif
);
        }
        /* Add ACTION_LIST_END */
        actionCount++;

        /* Create the action list to return. */
        actionList = malloc(sizeof(int) * actionCount);
        if (!actionList) {
            outOfMemory(TEXT("GALFN"), 2);
        } else {
            /* Now actually pull out the actions */
            actionCount = 0;
            _tcsncpy(workBuffer, actionNameList, len + 1);
            token = _tcstok(workBuffer, TEXT(" ,")
#if defined(UNICODE) && !defined(WIN32)
            , &state
#endif
);
            while (token != NULL) {
                action = getActionForName(token, propertyName, FALSE);
                if (action == 0) {
                    /* Unknown action */
                } else {
                    actionList[actionCount] = action;
                    actionCount++;
                }
                token = _tcstok(NULL, TEXT(" ,")
#if defined(UNICODE) && !defined(WIN32)
            , &state
#endif
);
            }
            /* Add ACTION_LIST_END */
            actionList[actionCount] = ACTION_LIST_END;
            actionCount++;

            /* actionList returned, so don't free it. */
        }

        free(workBuffer);
    }

    return actionList;
}

/**
 * Loads in the configuration triggers.
 *
 * @return Returns FALSE if successful, TRUE if there were any problems.
 */
int loadConfigurationTriggers() {
    const TCHAR *prop;
    TCHAR propName[256];
    int i;
    TCHAR **propertyNames;
    TCHAR **propertyValues;
    long unsigned int *propertyIndices;
#ifdef _DEBUG
    int j;
#endif

    /* To support reloading, we need to free up any previously loaded filters. */
    if (wrapperData->outputFilterCount > 0) {
        for (i = 0; i < wrapperData->outputFilterCount; i++) {
            free(wrapperData->outputFilters[i]);
            wrapperData->outputFilters[i] = NULL;
        }
        free(wrapperData->outputFilters);
        wrapperData->outputFilters = NULL;

        if (wrapperData->outputFilterActionLists) {
            for (i = 0; i < wrapperData->outputFilterCount; i++) {
                free(wrapperData->outputFilterActionLists[i]);
                wrapperData->outputFilterActionLists[i] = NULL;
            }
            free(wrapperData->outputFilterActionLists);
            wrapperData->outputFilterActionLists = NULL;
        }

        /* Individual messages are references to property values and are not malloced. */
        free(wrapperData->outputFilterMessages);
        wrapperData->outputFilterMessages = NULL;

        free(wrapperData->outputFilterAllowWildFlags);
        wrapperData->outputFilterAllowWildFlags = NULL;

        free(wrapperData->outputFilterMinLens);
        wrapperData->outputFilterMinLens = NULL;
    }

    wrapperData->outputFilterCount = 0;
    if (getStringProperties(properties, TEXT("wrapper.filter.trigger."), TEXT(""), wrapperData->ignoreSequenceGaps, FALSE, &propertyNames, &propertyValues, &propertyIndices)) {
        /* Failed */
        return TRUE;
    }

    /* Loop over the properties and count how many triggers there are. */
    i = 0;
    while (propertyNames[i]) {
        wrapperData->outputFilterCount++;
        i++;
    }
#if defined(MACOSX)
    wrapperData->outputFilterCount++;
    i++;
#endif

    /* Now that a count is known, allocate memory to hold the filters and actions and load them in. */
    if (wrapperData->outputFilterCount > 0) {
        wrapperData->outputFilters = malloc(sizeof(TCHAR *) * wrapperData->outputFilterCount);
        if (!wrapperData->outputFilters) {
            outOfMemory(TEXT("LC"), 1);
            return TRUE;
        }
        memset(wrapperData->outputFilters, 0, sizeof(TCHAR *) * wrapperData->outputFilterCount);

        wrapperData->outputFilterActionLists = malloc(sizeof(int*) * wrapperData->outputFilterCount);
        if (!wrapperData->outputFilterActionLists) {
            outOfMemory(TEXT("LC"), 2);
            return TRUE;
        }
        memset(wrapperData->outputFilterActionLists, 0, sizeof(int*) * wrapperData->outputFilterCount);

        wrapperData->outputFilterMessages = malloc(sizeof(TCHAR *) * wrapperData->outputFilterCount);
        if (!wrapperData->outputFilterMessages) {
            outOfMemory(TEXT("LC"), 3);
            return TRUE;
        }

        wrapperData->outputFilterAllowWildFlags = malloc(sizeof(int) * wrapperData->outputFilterCount);
        if (!wrapperData->outputFilterAllowWildFlags) {
            outOfMemory(TEXT("LC"), 4);
            return TRUE;
        }
        memset(wrapperData->outputFilterAllowWildFlags, 0, sizeof(int) * wrapperData->outputFilterCount);

        wrapperData->outputFilterMinLens = malloc(sizeof(size_t) * wrapperData->outputFilterCount);
        if (!wrapperData->outputFilterMinLens) {
            outOfMemory(TEXT("LC"), 5);
            return TRUE;
        }
        memset(wrapperData->outputFilterMinLens, 0, sizeof(size_t) * wrapperData->outputFilterCount);

        i = 0;
        while (propertyNames[i]) {
            prop = propertyValues[i];

            wrapperData->outputFilters[i] = malloc(sizeof(TCHAR) * (_tcslen(prop) + 1));
            if (!wrapperData->outputFilters[i]) {
                outOfMemory(TEXT("LC"), 3);
                return TRUE;
            }
            _tcsncpy(wrapperData->outputFilters[i], prop, _tcslen(prop) + 1);

            /* Get the action */
            _sntprintf(propName, 256, TEXT("wrapper.filter.action.%lu"), propertyIndices[i]);
            prop = getStringProperty(properties, propName, TEXT("RESTART"));
            wrapperData->outputFilterActionLists[i] = wrapperGetActionListForNames(prop, propName);

            /* Get the message */
            _sntprintf(propName, 256, TEXT("wrapper.filter.message.%lu"), propertyIndices[i]);
            prop = getStringProperty(properties, propName, NULL);
            wrapperData->outputFilterMessages[i] = (TCHAR *)prop;

            /* Get the wildcard flags. */
            _sntprintf(propName, 256, TEXT("wrapper.filter.allow_wildcards.%lu"), propertyIndices[i]);
            wrapperData->outputFilterAllowWildFlags[i] = getBooleanProperty(properties, propName, FALSE);
            if (wrapperData->outputFilterAllowWildFlags[i]) {
                /* Calculate the minimum text length. */
                wrapperData->outputFilterMinLens[i] = wrapperGetMinimumTextLengthForPattern(wrapperData->outputFilters[i]);
            }

#ifdef _DEBUG
            _tprintf(TEXT("filter #%lu, actions=("), propertyIndices[i]);
            if (wrapperData->outputFilterActionLists[i]) {
                j = 0;
                while (wrapperData->outputFilterActionLists[i][j]) {
                    if (j > 0) {
                        _tprintf(TEXT(","));
                    }
                    _tprintf(TEXT("%d"), wrapperData->outputFilterActionLists[i][j]);
                    j++;
                }
            }
            _tprintf(TEXT("), filter='%s'\n"), wrapperData->outputFilters[i]);
#endif
            i++;
        }

#if defined(MACOSX)
        wrapperData->outputFilters[i] = malloc(sizeof(TCHAR) * (_tcslen(TRIGGER_ADVICE_NIL_SERVER) + 1));
        if (!wrapperData->outputFilters[i]) {
            outOfMemory(TEXT("LC"), 4);
            return TRUE;
        }
        _tcsncpy(wrapperData->outputFilters[i], TRIGGER_ADVICE_NIL_SERVER, _tcslen(TRIGGER_ADVICE_NIL_SERVER) + 1);
        wrapperData->outputFilterActionLists[i] = malloc(sizeof(int) * 2);
        if (!wrapperData->outputFilters[i]) {
            outOfMemory(TEXT("LC"), 5);
            return TRUE;
        }
        wrapperData->outputFilterActionLists[i][0] = ACTION_ADVICE_NIL_SERVER;
        wrapperData->outputFilterActionLists[i][1] = ACTION_LIST_END;
        wrapperData->outputFilterMessages[i] = NULL;
        wrapperData->outputFilterAllowWildFlags[i] = FALSE;
        wrapperData->outputFilterMinLens[i] = 0;
        i++;
#endif
    }
    freeStringProperties(propertyNames, propertyValues, propertyIndices);

    return FALSE;
}

int getBackendTypeForName(const TCHAR *typeName) {
    if (strcmpIgnoreCase(typeName, TEXT("SOCKET")) == 0) {
        return WRAPPER_BACKEND_TYPE_SOCKET;
    } else if (strcmpIgnoreCase(typeName, TEXT("SOCKET_IPv4")) == 0) {
        return WRAPPER_BACKEND_TYPE_SOCKET_V4;
    } else if (strcmpIgnoreCase(typeName, TEXT("SOCKET_IPv6")) == 0) {
        return WRAPPER_BACKEND_TYPE_SOCKET_V6;
    } else if (strcmpIgnoreCase(typeName, TEXT("PIPE")) == 0) {
        return WRAPPER_BACKEND_TYPE_PIPE;
    } else if (strcmpIgnoreCase(typeName, TEXT("AUTO")) == 0) {
        return WRAPPER_BACKEND_TYPE_AUTO;
    } else {
        return WRAPPER_BACKEND_TYPE_UNKNOWN;
    }
}

/**
 * Return FALSE if successful, TRUE if there were problems.
 */
int loadConfiguration() {
    TCHAR propName[256];
    const TCHAR* val;
    int startupDelay;

    wrapperLoadLoggingProperties(FALSE);
    
    /* Decide on the error exit code */
    wrapperData->errorExitCode = getIntProperty(properties, TEXT("wrapper.exit_code.error"), 1);
    if (wrapperData->errorExitCode < 1 || wrapperData->errorExitCode > 255) {
        wrapperData->errorExitCode = 1;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            TEXT("%s must be in the range %d to %d.  Changing to %d."), TEXT("wrapper.exit_code.error"), 1, 255, 1);
    }

    /* Decide on the backend type to use. */    
    val = getStringProperty(properties, TEXT("wrapper.backend.type"), TEXT("AUTO"));
    wrapperData->backendType = getBackendTypeForName(val);
    if (wrapperData->backendType == WRAPPER_BACKEND_TYPE_UNKNOWN) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Unknown value for wrapper.backend.type: %s. Setting it to AUTO."), val);
        wrapperData->backendType = WRAPPER_BACKEND_TYPE_AUTO;
    }

    /* Decide whether the classpath should be passed via the environment. */
    wrapperData->environmentClasspath = getBooleanProperty(properties, TEXT("wrapper.java.classpath.use_environment"), FALSE);

    /* Decide how sequence gaps should be handled before any other properties are loaded. */
    wrapperData->ignoreSequenceGaps = getBooleanProperty(properties, TEXT("wrapper.ignore_sequence_gaps"), FALSE);
    
    /* Get the adviser status */
    wrapperData->isAdviserEnabled = getBooleanProperty(properties, TEXT("wrapper.adviser"), TRUE);
    /* The adviser is always enabled if debug is enabled. */
    if (wrapperData->isDebugging) {
        wrapperData->isAdviserEnabled = TRUE;
    }

    /* Make sure that the configured log file directory is accessible. */
    checkLogfileDir();
    /* To make configuration reloading work correctly with changes to the log file,
     *  it needs to be closed here. */
    closeLogfile();

    /* Maintain the logger just in case we wrote any queued errors. */
    maintainLogger();
    /* Because the first call could cause errors as well, do it again to clear them out.
     *  This is only a one-time thing on startup as we test the new logfile configuration. */
    maintainLogger();

    /* Initialize some values not loaded */
    wrapperData->exitCode = 0;

    updateStringValue(&wrapperData->portAddress, getStringProperty(properties, TEXT("wrapper.port.address"), NULL));
    /* Get the port. The int will wrap within the 0-65535 valid range, so no need to test the value. */
    wrapperData->port = getIntProperty(properties, TEXT("wrapper.port"), 0);
    wrapperData->portMin = getIntProperty(properties, TEXT("wrapper.port.min"), 32000);
    if ((wrapperData->portMin < 1) || (wrapperData->portMin > 65535)) {
        wrapperData->portMin = 32000;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            TEXT("%s must be in the range %d to %d.  Changing to %d."), TEXT("wrapper.port.min"), 1, 65535, wrapperData->portMin);
    }
    wrapperData->portMax = getIntProperty(properties, TEXT("wrapper.port.max"), 32999);
    if ((wrapperData->portMax < 1) || (wrapperData->portMax > 65535)) {
        wrapperData->portMax = __min(wrapperData->portMin + 999, 65535);
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            TEXT("%s must be in the range %d to %d.  Changing to %d."), TEXT("wrapper.port.max"), 1, 65535, wrapperData->portMax);
    } else if (wrapperData->portMax < wrapperData->portMin) {
        wrapperData->portMax = __min(wrapperData->portMin + 999, 65535);
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            TEXT("%s must be greater than or equal to %s.  Changing to %d."), TEXT("wrapper.port.max"), TEXT("wrapper.port.min"), wrapperData->portMax);
    }

    /* Get the port for the JVM side of the socket. */
    wrapperData->jvmPort = getIntProperty(properties, TEXT("wrapper.jvm.port"), -1);
    if (wrapperData->jvmPort > 0) {
        if (wrapperData->jvmPort == wrapperData->port) {
            wrapperData->jvmPort = -1;
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                TEXT("wrapper.jvm.port must not equal wrapper.port.  Changing to the default."));
        }
    }
    wrapperData->jvmPortMin = getIntProperty(properties, TEXT("wrapper.jvm.port.min"), 31000);
    if ((wrapperData->jvmPortMin < 1) || (wrapperData->jvmPortMin > 65535)) {
        wrapperData->jvmPortMin = 31000;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            TEXT("%s must be in the range %d to %d.  Changing to %d."), TEXT("wrapper.jvm.port.min"), 1, 65535, wrapperData->jvmPortMin);
    }
    wrapperData->jvmPortMax = getIntProperty(properties, TEXT("wrapper.jvm.port.max"), 31999);
    if ((wrapperData->jvmPortMax < 1) || (wrapperData->jvmPortMax > 65535)) {
        wrapperData->jvmPortMax = __min(wrapperData->jvmPortMin + 999, 65535);
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            TEXT("%s must be in the range %d to %d.  Changing to %d."), TEXT("wrapper.jvm.port.max"), 1, 65535, wrapperData->jvmPortMax);
    } else if (wrapperData->jvmPortMax < wrapperData->jvmPortMin) {
        wrapperData->jvmPortMax = __min(wrapperData->jvmPortMin + 999, 65535);
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            TEXT("%s must be greater than or equal to %s.  Changing to %d."), TEXT("wrapper.jvm.port.max"), TEXT("wrapper.jvm.port.min"), wrapperData->jvmPortMax);
    }

    wrapperData->printJVMVersion = getBooleanProperty(properties, TEXT("wrapper.java.version.output"), FALSE);
    /* Get the wrapper command log level. */
    wrapperData->commandLogLevel = getLogLevelForName(
        getStringProperty(properties, TEXT("wrapper.java.command.loglevel"), TEXT("DEBUG")));
    if (wrapperData->commandLogLevel >= LEVEL_NONE || wrapperData->commandLogLevel == LEVEL_UNKNOWN) {
        /* Should never be possible to completely disable the java command as this would make it very difficult to support. */
        wrapperData->commandLogLevel = LEVEL_DEBUG;
    }
    
    /* Should we detach the JVM on startup. */
    if (wrapperData->isConsole) {
        wrapperData->detachStarted = getBooleanProperty(properties, TEXT("wrapper.jvm_detach_started"), FALSE);
    }

    /* Get the use system time flag. */
    if (!wrapperData->configured) {
        wrapperData->useSystemTime = getBooleanProperty(properties, TEXT("wrapper.use_system_time"), FALSE);
    }
    
    if (!wrapperData->configured) {
        wrapperData->logBufferGrowth = getBooleanProperty(properties, TEXT("wrapper.log_buffer_growth"), FALSE);
        setLogBufferGrowth(wrapperData->logBufferGrowth);
    }
    
#ifdef WIN32
    /* Get the use javaio buffer size. */
    if (!wrapperData->configured) {
        wrapperData->javaIOBufferSize = getIntProperty(properties, TEXT("wrapper.javaio.buffer_size"), WRAPPER_JAVAIO_BUFFER_SIZE_DEFAULT);
        if (wrapperData->javaIOBufferSize == WRAPPER_JAVAIO_BUFFER_SIZE_SYSTEM_DEFAULT) {
            /* Ok. System default buffer size. */
        } else if (wrapperData->javaIOBufferSize < WRAPPER_JAVAIO_BUFFER_SIZE_MIN) {
            wrapperData->javaIOBufferSize = WRAPPER_JAVAIO_BUFFER_SIZE_MIN;
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                TEXT("%s must be in the range %d to %d or %d.  Changing to %d."), TEXT("wrapper.javaio.buffer_size"), WRAPPER_JAVAIO_BUFFER_SIZE_MIN, WRAPPER_JAVAIO_BUFFER_SIZE_MAX, WRAPPER_JAVAIO_BUFFER_SIZE_SYSTEM_DEFAULT, wrapperData->javaIOBufferSize);
        } else if (wrapperData->javaIOBufferSize > WRAPPER_JAVAIO_BUFFER_SIZE_MAX) {
            wrapperData->javaIOBufferSize = WRAPPER_JAVAIO_BUFFER_SIZE_MAX;
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                TEXT("%s must be in the range %d to %d or %d.  Changing to %d."), TEXT("wrapper.javaio.buffer_size"), WRAPPER_JAVAIO_BUFFER_SIZE_MIN, WRAPPER_JAVAIO_BUFFER_SIZE_MAX, WRAPPER_JAVAIO_BUFFER_SIZE_SYSTEM_DEFAULT, wrapperData->javaIOBufferSize);
        }
    }
#endif
    
    /* Get the use javaio thread flag. */
    if (!wrapperData->configured) {
        wrapperData->useJavaIOThread = getBooleanProperty(properties, TEXT("wrapper.javaio.use_thread"), getBooleanProperty(properties, TEXT("wrapper.use_javaio_thread"), FALSE));
    }
    
    /* Decide whether or not a mutex should be used to protect the tick timer. */
    if (!wrapperData->configured) {
        wrapperData->useTickMutex = getBooleanProperty(properties, TEXT("wrapper.use_tick_mutex"), FALSE);
    }
    
    /* Get the timer thresholds. Properties are in seconds, but internally we use ticks. */
    wrapperData->timerFastThreshold = getIntProperty(properties, TEXT("wrapper.timer_fast_threshold"), WRAPPER_TIMER_FAST_THRESHOLD * WRAPPER_TICK_MS / 1000) * 1000 / WRAPPER_TICK_MS;
    wrapperData->timerSlowThreshold = getIntProperty(properties, TEXT("wrapper.timer_slow_threshold"), WRAPPER_TIMER_SLOW_THRESHOLD * WRAPPER_TICK_MS / 1000) * 1000 / WRAPPER_TICK_MS;

    /* Load the name of the native library to be loaded. */
    wrapperData->nativeLibrary = getStringProperty(properties, TEXT("wrapper.native_library"), TEXT("wrapper"));

    /* Get the append PATH to library path flag. */
    wrapperData->libraryPathAppendPath = getBooleanProperty(properties, TEXT("wrapper.java.library.path.append_system_path"), FALSE);

    /* Get the state output status. */
    wrapperData->isStateOutputEnabled = getBooleanProperty(properties, TEXT("wrapper.state_output"), FALSE);

    /* Get the tick output status. */
    wrapperData->isTickOutputEnabled = getBooleanProperty(properties, TEXT("wrapper.tick_output"), FALSE);

    /* Get the loop debug output status. */
    wrapperData->isLoopOutputEnabled = getBooleanProperty(properties, TEXT("wrapper.loop_output"), FALSE);

    /* Get the sleep debug output status. */
    wrapperData->isSleepOutputEnabled = getBooleanProperty(properties, TEXT("wrapper.sleep_output"), FALSE);

    /* Get the memory output status. */
    wrapperData->isMemoryOutputEnabled = getBooleanProperty(properties, TEXT("wrapper.memory_output"), FALSE);
    wrapperData->memoryOutputInterval = getIntProperty(properties, TEXT("wrapper.memory_output.interval"), 1);

    /* Get the cpu output status. */
    wrapperData->isCPUOutputEnabled = getBooleanProperty(properties, TEXT("wrapper.cpu_output"), FALSE);
    wrapperData->cpuOutputInterval = getIntProperty(properties, TEXT("wrapper.cpu_output.interval"), 1);

    /* Get the pageFault output status. */
    if (!wrapperData->configured) {
        wrapperData->isPageFaultOutputEnabled = getBooleanProperty(properties, TEXT("wrapper.pagefault_output"), FALSE);
        wrapperData->pageFaultOutputInterval = getIntProperty(properties, TEXT("wrapper.pagefault_output.interval"), 1);
    }
    
    /* Get the disable tests flag. */
    wrapperData->isTestsDisabled = getBooleanProperty(properties, TEXT("wrapper.disable_tests"), FALSE);

    /* Get the shutdown hook status */
    wrapperData->isShutdownHookDisabled = getBooleanProperty(properties, TEXT("wrapper.disable_shutdown_hook"), FALSE);
    
    /* Get the forced shutdown flag status. */
    wrapperData->isForcedShutdownDisabled = getBooleanProperty(properties, TEXT("wrapper.disable_forced_shutdown"), FALSE);

    /* Get the startup delay. */
    startupDelay = getIntProperty(properties, TEXT("wrapper.startup.delay"), 0);
    wrapperData->startupDelayConsole = getIntProperty(properties, TEXT("wrapper.startup.delay.console"), startupDelay);
    if (wrapperData->startupDelayConsole < 0) {
        wrapperData->startupDelayConsole = 0;
    }
    wrapperData->startupDelayService = getIntProperty(properties, TEXT("wrapper.startup.delay.service"), startupDelay);
    if (wrapperData->startupDelayService < 0) {
        wrapperData->startupDelayService = 0;
    }

    /* Get the restart delay. */
    wrapperData->restartDelay = getIntProperty(properties, TEXT("wrapper.restart.delay"), 5);
    if (wrapperData->restartDelay < 0) {
        wrapperData->restartDelay = 0;
    }

    /* Get the flag which decides whether or not configuration should be reloaded on JVM restart. */
    wrapperData->restartReloadConf = getBooleanProperty(properties, TEXT("wrapper.restart.reload_configuration"), FALSE);

    /* Get the disable restart flag */
    wrapperData->isRestartDisabled = getBooleanProperty(properties, TEXT("wrapper.disable_restarts"), FALSE);
    wrapperData->isAutoRestartDisabled = getBooleanProperty(properties, TEXT("wrapper.disable_restarts.automatic"), wrapperData->isRestartDisabled);

    /* Get the timeout settings */
    wrapperData->cpuTimeout = getIntProperty(properties, TEXT("wrapper.cpu.timeout"), 10);
    wrapperData->startupTimeout = getIntProperty(properties, TEXT("wrapper.startup.timeout"), 30);
    wrapperData->pingTimeout = getIntProperty(properties, TEXT("wrapper.ping.timeout"), 30);
    if (wrapperData->pingActionList) {
        free(wrapperData->pingActionList);
    }
    wrapperData->pingActionList = wrapperGetActionListForNames(getStringProperty(properties, TEXT("wrapper.ping.timeout.action"), TEXT("RESTART")), TEXT("wrapper.ping.timeout.action"));
    wrapperData->pingAlertThreshold = getIntProperty(properties, TEXT("wrapper.ping.alert.threshold"), __max(1, wrapperData->pingTimeout / 4));
    wrapperData->pingAlertLogLevel = getLogLevelForName(getStringProperty(properties, TEXT("wrapper.ping.alert.loglevel"), TEXT("STATUS")));
    wrapperData->pingInterval = getIntProperty(properties, TEXT("wrapper.ping.interval"), 5);
    wrapperData->pingIntervalLogged = getIntProperty(properties, TEXT("wrapper.ping.interval.logged"), 1);
    wrapperData->shutdownTimeout = getIntProperty(properties, TEXT("wrapper.shutdown.timeout"), 30);
    wrapperData->jvmExitTimeout = getIntProperty(properties, TEXT("wrapper.jvm_exit.timeout"), 15);
    wrapperData->jvmCleanupTimeout = getIntProperty(properties, TEXT("wrapper.jvm_cleanup.timeout"), 10);
    wrapperData->jvmTerminateTimeout = getIntProperty(properties, TEXT("wrapper.jvm_terminate.timeout"), 10);

    wrapperData->cpuTimeout = validateTimeout(TEXT("wrapper.cpu.timeout"), wrapperData->cpuTimeout);
    wrapperData->startupTimeout = validateTimeout(TEXT("wrapper.startup.timeout"), wrapperData->startupTimeout);
    wrapperData->pingTimeout = validateTimeout(TEXT("wrapper.ping.timeout"), wrapperData->pingTimeout);
    wrapperData->shutdownTimeout = validateTimeout(TEXT("wrapper.shutdown.timeout"), wrapperData->shutdownTimeout);
    wrapperData->jvmExitTimeout = validateTimeout(TEXT("wrapper.jvm_exit.timeout"), wrapperData->jvmExitTimeout);
    wrapperData->jvmTerminateTimeout = validateTimeout(TEXT("wrapper.jvm_terminate.timeout"), wrapperData->jvmTerminateTimeout);
    wrapperData->jvmCleanupTimeout = validateTimeout(TEXT("wrapper.jvm_cleanup.timeout"), wrapperData->jvmCleanupTimeout);

    if (wrapperData->pingInterval < 1) {
        wrapperData->pingInterval = 1;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            TEXT("The value of %s must be at least %d second(s).  Changing to %d."), TEXT("wrapper.ping.interval"), 1, wrapperData->pingInterval);
    } else if (wrapperData->pingInterval > 3600) {
        wrapperData->pingInterval = 3600;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            TEXT("wrapper.ping.interval must be less than or equal to 1 hour (3600 seconds).  Changing to 3600."));
    }
    if (wrapperData->pingIntervalLogged < 1) {
        wrapperData->pingIntervalLogged = 1;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            TEXT("The value of %s must be at least %d second(s).  Changing to %d."), TEXT("wrapper.ping.interval.logged"), 1, wrapperData->pingIntervalLogged);
    } else if (wrapperData->pingIntervalLogged > 86400) {
        wrapperData->pingIntervalLogged = 86400;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            TEXT("wrapper.ping.interval.logged must be less than or equal to 1 day (86400 seconds).  Changing to 86400."));
    }

    if ((wrapperData->pingTimeout > 0) && (wrapperData->pingTimeout < wrapperData->pingInterval + 5)) {
        wrapperData->pingTimeout = wrapperData->pingInterval + 5;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            TEXT("wrapper.ping.timeout must be at least 5 seconds longer than wrapper.ping.interval.  Changing to %d."), wrapperData->pingTimeout);
    }
    if (wrapperData->pingAlertThreshold <= 0) {
        /* Ping Alerts disabled. */
        wrapperData->pingAlertThreshold = 0;
    } else if ((wrapperData->pingTimeout > 0) && (wrapperData->pingAlertThreshold > wrapperData->pingTimeout)) {
        wrapperData->pingAlertThreshold = wrapperData->pingTimeout;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            TEXT("wrapper.ping.alert.threshold must be less than or equal to the value of wrapper.ping.timeout (%d seconds).  Changing to %d."),
            wrapperData->pingInterval, wrapperData->pingTimeout);
    }
    if (wrapperData->cpuTimeout > 0) {
        /* Make sure that the timeouts are all longer than the cpu timeout. */
        if ((wrapperData->startupTimeout > 0) && (wrapperData->startupTimeout < wrapperData->cpuTimeout)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                TEXT("CPU timeout detection may not operate correctly during startup because wrapper.cpu.timeout is not smaller than wrapper.startup.timeout."));
        }
        if ((wrapperData->pingTimeout > 0) && (wrapperData->pingTimeout < wrapperData->cpuTimeout)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                TEXT("CPU timeout detection may not operate correctly because wrapper.cpu.timeout is not smaller than wrapper.ping.timeout."));
        }
        if ((wrapperData->shutdownTimeout > 0) && (wrapperData->shutdownTimeout < wrapperData->cpuTimeout)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                TEXT("CPU timeout detection may not operate correctly during shutdown because wrapper.cpu.timeout is not smaller than wrapper.shutdown.timeout."));
        }
        /* jvmExit timeout can be shorter than the cpu timeout. */
    }

    /* Load properties controlling the number times the JVM can be restarted. */
    wrapperData->maxFailedInvocations = getIntProperty(properties, TEXT("wrapper.max_failed_invocations"), 5);
    wrapperData->successfulInvocationTime = getIntProperty(properties, TEXT("wrapper.successful_invocation_time"), 300);
    if (wrapperData->maxFailedInvocations < 1) {
        wrapperData->maxFailedInvocations = 1;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
            TEXT("The value of %s must be at least %d second(s).  Changing to %d."), TEXT("wrapper.max_failed_invocations"), 1, wrapperData->maxFailedInvocations);
    }

    /* TRUE if the JVM should be asked to dump its state when it fails to halt on request. */
    wrapperData->requestThreadDumpOnFailedJVMExit = getBooleanProperty(properties, TEXT("wrapper.request_thread_dump_on_failed_jvm_exit"), FALSE);
    wrapperData->requestThreadDumpOnFailedJVMExitDelay = getIntProperty(properties, TEXT("wrapper.request_thread_dump_on_failed_jvm_exit.delay"), 5);
    if (wrapperData->requestThreadDumpOnFailedJVMExitDelay < 1) {
        wrapperData->requestThreadDumpOnFailedJVMExitDelay = 1;
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
            TEXT("The value of %s must be at least %d second(s).  Changing to %d."), TEXT("wrapper.request_thread_dump_on_failed_jvm_exit.delay"), 1, wrapperData->requestThreadDumpOnFailedJVMExitDelay);
    }

    /* Load the output filters. */
    if (loadConfigurationTriggers()) {
        return TRUE;
    }

    /** Get the pid files if any.  May be NULL */
    if (!wrapperData->configured) {
        updateStringValue(&wrapperData->pidFilename, getFileSafeStringProperty(properties, TEXT("wrapper.pidfile"), NULL));
        wrapperCorrectWindowsPath(wrapperData->pidFilename);
    }
    wrapperData->pidFileStrict = getBooleanProperty(properties, TEXT("wrapper.pidfile.strict"), FALSE);
    
    updateStringValue(&wrapperData->javaPidFilename, getFileSafeStringProperty(properties, TEXT("wrapper.java.pidfile"), NULL));
    wrapperCorrectWindowsPath(wrapperData->javaPidFilename);

    /** Get the lock file if any.  May be NULL */
    if (!wrapperData->configured) {
        updateStringValue(&wrapperData->lockFilename, getFileSafeStringProperty(properties, TEXT("wrapper.lockfile"), NULL));
        wrapperCorrectWindowsPath(wrapperData->lockFilename);
    }

    /** Get the java id file.  May be NULL */
    updateStringValue(&wrapperData->javaIdFilename, getFileSafeStringProperty(properties, TEXT("wrapper.java.idfile"), NULL));
    wrapperCorrectWindowsPath(wrapperData->javaIdFilename);

    /** Get the status files if any.  May be NULL */
    if (!wrapperData->configured) {
        updateStringValue(&wrapperData->statusFilename, getFileSafeStringProperty(properties, TEXT("wrapper.statusfile"), NULL));
        wrapperCorrectWindowsPath(wrapperData->statusFilename);
    }
    updateStringValue(&wrapperData->javaStatusFilename, getFileSafeStringProperty(properties, TEXT("wrapper.java.statusfile"), NULL));
    wrapperCorrectWindowsPath(wrapperData->javaStatusFilename);

    /** Get the command file if any. May be NULL */
    updateStringValue(&wrapperData->commandFilename, getFileSafeStringProperty(properties, TEXT("wrapper.commandfile"), NULL));
    wrapperCorrectWindowsPath(wrapperData->commandFilename);
    wrapperData->commandFileTests = getBooleanProperty(properties, TEXT("wrapper.commandfile.enable_tests"), FALSE);

    /** Get the interval at which the command file will be polled. */
    wrapperData->commandPollInterval = propIntMin(propIntMax(getIntProperty(properties, TEXT("wrapper.commandfile.poll_interval"), getIntProperty(properties, TEXT("wrapper.command.poll_interval"), 5)), 1), 3600);

    /** Get the anchor file if any.  May be NULL */
    if (!wrapperData->configured) {
        updateStringValue(&wrapperData->anchorFilename, getFileSafeStringProperty(properties, TEXT("wrapper.anchorfile"), NULL));
        wrapperCorrectWindowsPath(wrapperData->anchorFilename);
    }

    /** Get the interval at which the anchor file will be polled. */
    wrapperData->anchorPollInterval = propIntMin(propIntMax(getIntProperty(properties, TEXT("wrapper.anchorfile.poll_interval"), getIntProperty(properties, TEXT("wrapper.anchor.poll_interval"), 5)), 1), 3600);

    /** Flag controlling whether or not system signals should be ignored. */
    val = getStringProperty(properties, TEXT("wrapper.ignore_signals"), TEXT("FALSE"));
    if ( ( strcmpIgnoreCase( val, TEXT("TRUE") ) == 0 ) || ( strcmpIgnoreCase( val, TEXT("BOTH") ) == 0 ) ) {
        wrapperData->ignoreSignals = WRAPPER_IGNORE_SIGNALS_WRAPPER + WRAPPER_IGNORE_SIGNALS_JAVA;
    } else if ( strcmpIgnoreCase( val, TEXT("WRAPPER") ) == 0 ) {
        wrapperData->ignoreSignals = WRAPPER_IGNORE_SIGNALS_WRAPPER;
    } else if ( strcmpIgnoreCase( val, TEXT("JAVA") ) == 0 ) {
        wrapperData->ignoreSignals = WRAPPER_IGNORE_SIGNALS_JAVA;
    } else {
        wrapperData->ignoreSignals = 0;
    }

    /* Obtain the Console Title. */
    _sntprintf(propName, 256, TEXT("wrapper.console.title.%s"), wrapperOS);
    updateStringValue(&wrapperData->consoleTitle, getStringProperty(properties, propName, getStringProperty(properties, TEXT("wrapper.console.title"), NULL)));

    /* Load the service name (Used to be windows specific so use those properties if set.) */
    updateStringValue(&wrapperData->serviceName, getStringProperty(properties, TEXT("wrapper.name"), getStringProperty(properties, TEXT("wrapper.ntservice.name"), TEXT("wrapper"))));

    /* Load the service display name (Used to be windows specific so use those properties if set.) */
    updateStringValue(&wrapperData->serviceDisplayName, getStringProperty(properties, TEXT("wrapper.displayname"), getStringProperty(properties, TEXT("wrapper.ntservice.displayname"), wrapperData->serviceName)));

    /* Load the service description, default to display name (Used to be windows specific so use those properties if set.) */
    updateStringValue(&wrapperData->serviceDescription, getStringProperty(properties, TEXT("wrapper.description"), getStringProperty(properties, TEXT("wrapper.ntservice.description"), wrapperData->serviceDisplayName)));

    /* Pausable */
    if (!wrapperData->configured) {
        wrapperData->pausable = getBooleanProperty(properties, TEXT("wrapper.pausable"), getBooleanProperty(properties, TEXT("wrapper.ntservice.pausable"), FALSE));
        wrapperData->pausableStopJVM = getBooleanProperty(properties, TEXT("wrapper.pausable.stop_jvm"), getBooleanProperty(properties, TEXT("wrapper.ntservice.pausable.stop_jvm"), TRUE));
        wrapperData->initiallyPaused = getBooleanProperty(properties, TEXT("wrapper.pause_on_startup"), FALSE);
    }

#ifdef WIN32
    wrapperData->ignoreUserLogoffs = getBooleanProperty(properties, TEXT("wrapper.ignore_user_logoffs"), FALSE);

    /* Configure the NT service information */
    if (wrapperBuildNTServiceInfo()) {
        return TRUE;
    }

    if (wrapperData->generateConsole) {
        if (!wrapperData->ntAllocConsole) {
            /* We need to allocate a console in order for the thread dumps to work
             *  when running as a service.  But the user did not request that a
             *  console be visible so we want to hide it. */
            wrapperData->ntAllocConsole = TRUE;
            wrapperData->ntHideWrapperConsole = TRUE;
        }
    }
 #ifdef WIN32
    if ((!strcmpIgnoreCase(wrapperData->argCommand, TEXT("s")) || !strcmpIgnoreCase(wrapperData->argCommand, TEXT("-service")))
        && (!wrapperData->ntAllocConsole || wrapperData->ntHideWrapperConsole)) {
 #else
    if (!wrapperData->isConsole) {
 #endif
        /* The console is not visible, so we shouldn't waste time logging to it. */
        setConsoleLogLevelInt(LEVEL_NONE);
    }

#else /* UNIX */
    /* Configure the Unix daemon information */
    if (wrapperBuildUnixDaemonInfo()) {
        return TRUE;
    }

    if (loadResourcesLimitsConfiguration()) {
        return TRUE;
    }
#endif

    if (_tcscmp(wrapperVersionRoot, getStringProperty(properties, TEXT("wrapper.script.version"), wrapperVersionRoot)) != 0) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
            TEXT("The version of the script (%s) doesn't match the version of this Wrapper (%s). This might cause some problems"), 
            getStringProperty(properties, TEXT("wrapper.script.version"), wrapperVersionRoot), wrapperVersionRoot);
    }

    wrapperData->configured = TRUE;

    return FALSE;
}

/**
 * Requests a lock on the tick mutex.
 *
 * @return TRUE if there were any problems, FALSE if successful.
 */
int wrapperLockTickMutex() {
#ifdef WIN32
    switch (WaitForSingleObject(tickMutexHandle, INFINITE)) {
    case WAIT_ABANDONED:
        _tprintf(TEXT("Tick was abandoned.\n"));
        return TRUE;
    case WAIT_FAILED:
        _tprintf(TEXT("Tick wait failed.\n"));
        return TRUE;
    case WAIT_TIMEOUT:
        _tprintf(TEXT("Tick wait timed out.\n"));
        return TRUE;
    default:
        /* Ok */
        break;
    }
#else
    if (pthread_mutex_lock(&tickMutex)) {
        _tprintf(TEXT("Failed to lock the Tick mutex. %s\n"), getLastErrorText());
        return TRUE;
    }
#endif

    return FALSE;
}

/**
 * Releases a lock on the tick mutex.
 *
 * @return TRUE if there were any problems, FALSE if successful.
 */
int wrapperReleaseTickMutex() {
#ifdef WIN32
    if (!ReleaseMutex(tickMutexHandle)) {
        _tprintf(TEXT("Failed to release tick mutex. %s\n"), getLastErrorText());
        return TRUE;
    }
#else
    if (pthread_mutex_unlock(&tickMutex)) {
        _tprintf(TEXT("Failed to unlock the tick mutex. %s\n"), getLastErrorText());
        return TRUE;
    }
#endif
    return FALSE;
}

/**
 * Calculates a tick count using the system time.
 *
 * We normally need 64 bits to do this calculation.  Play some games to get
 *  the correct values with 32 bit variables.
 */
TICKS wrapperGetSystemTicks() {
    static int firstCall = TRUE;
    static TICKS initialTicks = 0;
    struct timeb timeBuffer;
    DWORD high, low;
    TICKS sum;
#ifdef _DEBUG
    TICKS assertSum;
#endif

    wrapperGetCurrentTime(&timeBuffer);

    /* Break in half. */
    high = (DWORD)(timeBuffer.time >> 16) & 0xffff;
    low = (DWORD)(timeBuffer.time & 0xffff);

    /* Work on each half. */
    high = high * 1000 / WRAPPER_TICK_MS;
    low = (low * 1000 + timeBuffer.millitm) / WRAPPER_TICK_MS;

    /* Now combine them in such a way that the correct bits are truncated. */
    high = high + ((low >> 16) & 0xffff);
    sum = (TICKS)(((high & 0xffff) << 16) + (low & 0xffff));

    /* Check the result. */
#ifdef _DEBUG
#ifdef WIN32
    assertSum = (TICKS)((timeBuffer.time * 1000UI64 + timeBuffer.millitm) / WRAPPER_TICK_MS);
#else
    /* This will produce the following warning on some compilers:
     *  warning: ANSI C forbids long long integer constants
     * Is there another way to do this? */
    assertSum = (TICKS)((timeBuffer.time * 1000ULL + timeBuffer.millitm) / WRAPPER_TICK_MS);
#endif
    if (assertSum != sum) {
        _tprintf(TEXT("wrapperGetSystemTicks() resulted in %08x rather than %08x\n"), sum, assertSum);
    }
#endif

    if (firstCall) {
        initialTicks = sum - WRAPPER_TICK_INITIAL;
        firstCall = FALSE;
    }

    return (sum - initialTicks);
}

/**
 * Returns difference in seconds between the start and end ticks.  This function
 *  handles cases where the tick counter has wrapped between when the start
 *  and end tick counts were taken.  See the wrapperGetTicks() function.
 *
 * This can be done safely in 32 bits
 */
int wrapperGetTickAgeSeconds(TICKS start, TICKS end) {
    /*
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("      wrapperGetTickAgeSeconds(%08x, %08x) -> %08x"), start, end, (int)((end - start) * WRAPPER_TICK_MS) / 1000);
    */

    /* Simply subtracting the values will always work even if end has wrapped due to overflow.
     *  This is only true if end has wrapped by less than half of the range of TICKS/WRAPPER_TICK_MS!
     *  0x00000001 - 0xffffffff = 0x00000002 = 2
     *  0xffffffff - 0x00000001 = 0xfffffffe = -2
     */
    return (int)((end - start) * WRAPPER_TICK_MS) / 1000;
}

/**
 * Returns difference in ticks between the start and end ticks.  This function
 *  handles cases where the tick counter has wrapped between when the start
 *  and end tick counts were taken.  See the wrapperGetTicks() function.
 *
 * This can be done safely in 32 bits
 */
int wrapperGetTickAgeTicks(TICKS start, TICKS end) {
    /*
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("      wrapperGetTickAgeSeconds(%08x, %08x) -> %08x"), start, end, (int)(end - start));
    */

    /* Simply subtracting the values will always work even if end has wrapped due to overflow.
     *  This is only true if end has wrapped by less than half of the range of TICKS/WRAPPER_TICK_MS!
     *  0x00000001 - 0xffffffff = 0x00000002 = 2
     *  0xffffffff - 0x00000001 = 0xfffffffe = -2
     */
    return (int)(end - start);
}

/**
 * Returns TRUE if the specified tick timeout has expired relative to the
 *  specified tick count.
 */
int wrapperTickExpired(TICKS nowTicks, TICKS timeoutTicks) {
    /* Convert to a signed value. */
    int age = nowTicks - timeoutTicks;

    if (age >= 0) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/**
 * Returns a tick count that is the specified number of seconds later than
 *  the base tick count.
 *
 * This calculation will work as long as the number of seconds is not large
 *  enough to require more than 32 bits when multiplied by 1000.
 */
TICKS wrapperAddToTicks(TICKS start, int seconds) {
    /*
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("      wrapperAddToTicks(%08x, %08x) -> %08x"), start, seconds, start + (seconds * 1000 / WRAPPER_TICK_MS));
    */
    return start + (seconds * 1000 / WRAPPER_TICK_MS);
}

/**
 * Do some sanity checks on the tick timer math.
 */
int wrapperTickAssertions() {
    int result = FALSE;
    TICKS ticks1, ticks2, ticksR, ticksE;
    int value1, valueR, valueE;

    /** wrapperGetTickAgeTicks test. */
    ticks1 = 0xfffffffe;
    ticks2 = 0xffffffff;
    valueE = 1;
    valueR = wrapperGetTickAgeTicks(ticks1, ticks2);
    if (valueR != valueE) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Assert Failed: wrapperGetTickAgeTicks(%08x, %08x) == %0d != %0d"), ticks1, ticks2, valueR, valueE);
        result = TRUE;
    }

    ticks1 = 0xffffffff;
    ticks2 = 0xfffffffe;
    valueE = -1;
    valueR = wrapperGetTickAgeTicks(ticks1, ticks2);
    if (valueR != valueE) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Assert Failed: wrapperGetTickAgeTicks(%08x, %08x) == %0d != %0d"), ticks1, ticks2, valueR, valueE);
        result = TRUE;
    }

    ticks1 = 0xffffffff;
    ticks2 = 0x00000000;
    valueE = 1;
    valueR = wrapperGetTickAgeTicks(ticks1, ticks2);
    if (valueR != valueE) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Assert Failed: wrapperGetTickAgeTicks(%08x, %08x) == %0d != %0d"), ticks1, ticks2, valueR, valueE);
        result = TRUE;
    }

    ticks1 = 0x00000000;
    ticks2 = 0xffffffff;
    valueE = -1;
    valueR = wrapperGetTickAgeTicks(ticks1, ticks2);
    if (valueR != valueE) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Assert Failed: wrapperGetTickAgeTicks(%08x, %08x) == %0d != %0d"), ticks1, ticks2, valueR, valueE);
        result = TRUE;
    }

    /** wrapperGetTickAgeSeconds test. */
    ticks1 = 0xfffffff0;
    ticks2 = 0xffffffff;
    valueE = 1;
    valueR = wrapperGetTickAgeSeconds(ticks1, ticks2);
    if (valueR != valueE) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Assert Failed: wrapperGetTickAgeSeconds(%08x, %08x) == %0d != %0d"), ticks1, ticks2, valueR, valueE);
        result = TRUE;
    }

    ticks1 = 0xffffffff;
    ticks2 = 0x0000000f;
    valueE = 1;
    valueR = wrapperGetTickAgeSeconds(ticks1, ticks2);
    if (valueR != valueE) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Assert Failed: wrapperGetTickAgeSeconds(%08x, %08x) == %0d != %0d"), ticks1, ticks2, valueR, valueE);
        result = TRUE;
    }

    ticks1 = 0x0000000f;
    ticks2 = 0xffffffff;
    valueE = -1;
    valueR = wrapperGetTickAgeSeconds(ticks1, ticks2);
    if (valueR != valueE) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Assert Failed: wrapperGetTickAgeSeconds(%08x, %08x) == %0d != %0d"), ticks1, ticks2, valueR, valueE);
        result = TRUE;
    }


    /** wrapperTickExpired test. */
    ticks1 = 0xfffffffe;
    ticks2 = 0xffffffff;
    valueE = FALSE;
    valueR = wrapperTickExpired(ticks1, ticks2);
    if (valueR != valueE) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Assert Failed: wrapperTickExpired(%08x, %08x) == %0d != %0d"), ticks1, ticks2, valueR, valueE);
        result = TRUE;
    }

    ticks1 = 0xffffffff;
    ticks2 = 0xffffffff;
    valueE = TRUE;
    valueR = wrapperTickExpired(ticks1, ticks2);
    if (valueR != valueE) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Assert Failed: wrapperTickExpired(%08x, %08x) == %0d != %0d"), ticks1, ticks2, valueR, valueE);
        result = TRUE;
    }

    ticks1 = 0xffffffff;
    ticks2 = 0x00000001;
    valueE = FALSE;
    valueR = wrapperTickExpired(ticks1, ticks2);
    if (valueR != valueE) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Assert Failed: wrapperTickExpired(%08x, %08x) == %0d != %0d"), ticks1, ticks2, valueR, valueE);
        result = TRUE;
    }

    ticks1 = 0x00000001;
    ticks2 = 0xffffffff;
    valueE = TRUE;
    valueR = wrapperTickExpired(ticks1, ticks2);
    if (valueR != valueE) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Assert Failed: wrapperTickExpired(%08x, %08x) == %0d != %0d"), ticks1, ticks2, valueR, valueE);
        result = TRUE;
    }

    /** wrapperAddToTicks test. */
    ticks1 = 0xffffffff;
    value1 = 1;
    ticksE = 0x00000009;
    ticksR = wrapperAddToTicks(ticks1, value1);
    if (ticksR != ticksE) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Assert Failed: wrapperAddToTicks(%08x, %d) == %08x != %08x"), ticks1, value1, ticksR, ticksE);
        result = TRUE;
    }

    return result;
}

/**
 * Sets the working directory of the Wrapper to the specified directory.
 *  The directory can be relative or absolute.
 * If there are any problems then a non-zero value will be returned.
 *
 * @param dir Directory to change to.
 * @param logErrors TRUE if errors should be logged.
 */
int wrapperSetWorkingDir(const TCHAR* dir, int logErrors) {
    int showOutput = wrapperData->configured;

    if (_tchdir(dir)) {
        if ( logErrors )
        {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                TEXT("Unable to set working directory to: %s (%s)"), dir, getLastErrorText());
        }
        return TRUE;
    }

    /* This function is sometimes called before the configuration is loaded. */
#ifdef _DEBUG
    showOutput = TRUE;
#endif

    if (showOutput) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Working directory set to: %s"), dir);
    }

    /* Set a variable to the location of the binary. */
    setEnv(TEXT("WRAPPER_WORKING_DIR"), dir, ENV_SOURCE_APPLICATION);

    return FALSE;
}

/******************************************************************************
 * Protocol callback functions
 *****************************************************************************/
void wrapperLogSignaled(int logLevel, TCHAR *msg) {
    /* */
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Got a log message from JVM: %s"), msg);
    }
    /* */

    log_printf(wrapperData->jvmRestarts, logLevel, msg);
}

void wrapperKeyRegistered(TCHAR *key) {
    /* Allow for a large integer + \0 */
    TCHAR buffer[11];

    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Got key from JVM: %s"), key);
    }
    switch (wrapperData->jState) {
    case WRAPPER_JSTATE_LAUNCHING:
        /* We now know that the Java side wrapper code has started and
         *  registered with a key.  We still need to verify that it is
         *  the correct key however. */
        if (_tcscmp(key, wrapperData->key) == 0) {
            /* This is the correct key. */
            /* We now know that the Java side wrapper code has started. */
            wrapperSetJavaState(WRAPPER_JSTATE_LAUNCHED, 0, -1);

            /* Send the low log level to the JVM so that it can control output via the log method. */
            _sntprintf(buffer, 11, TEXT("%d"), getLowLogLevel());
            wrapperProtocolFunction(WRAPPER_MSG_LOW_LOG_LEVEL, buffer);

            /* Send the log file name. */
            sendLogFileName();

            /* Send the properties. */
            sendProperties();
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("Received a connection request with an incorrect key.  Waiting for another connection."));

            /* This was the wrong key.  Send a response. */
            wrapperProtocolFunction(WRAPPER_MSG_BADKEY, TEXT("Incorrect key.  Connection rejected."));

            /* Close the current connection.  Assume that the real JVM
             *  is still out there trying to connect.  So don't change
             *  the state.  If for some reason, this was the correct
             *  JVM, but the key was wrong.  then this state will time
             *  out and recover. */
            wrapperProtocolClose();
        }
        break;

    case WRAPPER_JSTATE_STOP:
        /* We got a key registration.  This means that the JVM thinks it was
         *  being launched but the Wrapper is trying to stop.  This state
         *  will clean up correctly. */
        break;

    case WRAPPER_JSTATE_STOPPING:
        /* We got a key registration.  This means that the JVM thinks it was
         *  being launched but the Wrapper is trying to stop.  Now that the
         *  connection to the JVM has been opened, tell it to stop cleanly. */
        wrapperSetJavaState(WRAPPER_JSTATE_STOP, 0, -1);
        break;

    default:
        /* We got a key registration that we were not expecting.  Ignore it. */
        break;
    }
}

/**
 * Called when a ping is first determined to be slower than the wrapper.ping.alert.threshold.
 *  This will happen before it has actually been responded to.
 */
void wrapperPingSlow() {
}

/**
 * Called when a ping is responded to, but was slower than the wrapper.ping.alert.threshold.
 *
 * @param tickAge The number of seconds it took to respond.
 */
void wrapperPingRespondedSlow(int tickAge) {
    log_printf(WRAPPER_SOURCE_WRAPPER, wrapperData->pingAlertLogLevel, TEXT("Pinging the JVM took %d seconds to respond."), tickAge);
}

/**
 * Called when a ping response is received.
 *
 * @param pingSendTicks Time in ticks when the ping was originally sent.
 * @param queueWarnings TRUE if warnings about the queue should be logged, FALSE if the ping response did not contain a time.
 */
void wrapperPingResponded(TICKS pingSendTicks, int queueWarnings) {
    TICKS nowTicks;
    int tickAge;
    PPendingPing pendingPing;
    int pingSearchDone;
    
#ifdef DEBUG_PING_QUEUE
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("    PING QUEUE Ping Response (tick %08x)"), pingSendTicks);
#endif
    /* We want to purge the ping from the PendingPing list. */
    do {
        pendingPing = wrapperData->firstPendingPing;
        if (pendingPing != NULL) {
            tickAge = wrapperGetTickAgeTicks(pingSendTicks, pendingPing->sentTicks);
#ifdef DEBUG_PING_QUEUE
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("    PING QUEUE First Queued Ping (tick %08x, age %d)"), pendingPing->sentTicks, tickAge);
#endif
            if (tickAge > 0) { /* pendingPing->sentTicks > pingSendTicks */
                /* We received a ping response that is earlier than the one we were expecting.
                 *  If the pendingPingQueue has overflown then we will stop writing to it.  Don't long warning messages when in this state as they would be confusing and are expected.
                 *  Leave this one in the queue for later. */
                if (queueWarnings) {
                    if ((!wrapperData->pendingPingQueueOverflow) && (!wrapperData->pendingPingQueueOverflowEmptied)) {
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("Received an unexpected ping response, sent at tick %08x.  First expected ping was sent at tick %08x."), pingSendTicks, pendingPing->sentTicks);
                    } else {
#ifdef DEBUG_PING_QUEUE
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("    PING QUEUE Silently skipping unexpected ping response. (tick %08x) (First tick %08x)"), pingSendTicks, pendingPing->sentTicks);
#endif
                    }
                }
                pendingPing = NULL;
                pingSearchDone = TRUE;
            } else {
                if (tickAge < 0) {
                    /* This PendingPing object was sent before the PING that we received.  This means that we somehow lost a ping. */
                    if (queueWarnings) {
                        if ((!wrapperData->pendingPingQueueOverflow) && (!wrapperData->pendingPingQueueOverflowEmptied)) {
                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("Lost a ping response, sent at tick %08x."), pendingPing->sentTicks);
                        } else {
#ifdef DEBUG_PING_QUEUE
                            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("    PING QUEUE Silently skipping lost ping response. (tick %08x)"), pendingPing->sentTicks);
#endif
                        }
                    }
                    pingSearchDone = FALSE;
                } else {
                    /* This PendingPing object is for this PING event. */
                    pingSearchDone = TRUE;
#ifdef DEBUG_PING_QUEUE
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("    PING QUEUE Expected Ping Response. (tick %08x)"), pendingPing->sentTicks);
#endif
                    
                    /* When the emptied flag is set, we know that we are recovering from an overflow.
                     *  That flag is reset on the first expected PendingPing found in the queue. */
                    if (wrapperData->pendingPingQueueOverflowEmptied) {
                        wrapperData->pendingPingQueueOverflowEmptied = FALSE;
#ifdef DEBUG_PING_QUEUE
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("    PING QUEUE Emptied Set flag reset."));
#endif
                    }
                }
                
                /* Detach the PendingPing from the queue. */
                if (pendingPing->nextPendingPing != NULL) {
                    /* This was the first PendingPing of several in the queue. */
                    wrapperData->pendingPingCount--;
                    if (wrapperData->firstUnwarnedPendingPing == wrapperData->firstPendingPing) {
                        wrapperData->firstUnwarnedPendingPing = pendingPing->nextPendingPing;
                    }
                    wrapperData->firstPendingPing = pendingPing->nextPendingPing;
                    pendingPing->nextPendingPing = NULL;
#ifdef DEBUG_PING_QUEUE
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("--- PING QUEUE Size: %d"), wrapperData->pendingPingCount);
#endif
                } else {
                    /* This was the only PendingPing in the queue. */
                    wrapperData->pendingPingCount = 0;
                    wrapperData->firstUnwarnedPendingPing = NULL;
                    wrapperData->firstPendingPing = NULL;
                    wrapperData->lastPendingPing = NULL;
#ifdef DEBUG_PING_QUEUE
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("--- PING QUEUE Empty.") );
#endif
                    if (wrapperData->pendingPingQueueOverflow) {
                        wrapperData->pendingPingQueueOverflowEmptied = TRUE;
                        wrapperData->pendingPingQueueOverflow = FALSE;
#ifdef DEBUG_PING_QUEUE
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("    PING QUEUE Reset Overflow, Emptied Set.") );
#endif
                    }
                }
                
                /* Free up the pendingPing object. */
                if (pendingPing != NULL) {
                    free(pendingPing);
                    pendingPing = NULL;
                }
            }
        } else {
            /* Got a ping response when the queue was empty. */
            if (queueWarnings) {
                if ((!wrapperData->pendingPingQueueOverflow) && (!wrapperData->pendingPingQueueOverflowEmptied)) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("Received an unexpected ping response, sent at tick %08x."), pingSendTicks);
                } else {
#ifdef DEBUG_PING_QUEUE
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("    PING QUEUE Silently skipping unexpected ping response. (tick %08x) (Empty)"), pingSendTicks);
#endif
                }
            }
            pingSearchDone = TRUE;
        }
    } while (!pingSearchDone);
    
    /* Depending on the current JVM state, do something. */
    switch (wrapperData->jState) {
    case WRAPPER_JSTATE_STARTED:
        /* We got a response to a ping. */
        nowTicks = wrapperGetTicks();
        
        /* Figure out how long it took for us to get this ping response in seconds. */
        tickAge = wrapperGetTickAgeSeconds(pingSendTicks, nowTicks);
        
        /* If we took longer than the threshold then we want to log a message. */
        if ((wrapperData->pingAlertThreshold > 0) && (tickAge >= wrapperData->pingAlertThreshold)) {
            wrapperPingRespondedSlow(tickAge);
        }
        
        /* Allow 5 + <pingTimeout> more seconds before the JVM is considered to be dead. */
        if (wrapperData->pingTimeout > 0) {
            wrapperUpdateJavaStateTimeout(nowTicks, 5 + wrapperData->pingTimeout);
        } else {
            wrapperUpdateJavaStateTimeout(nowTicks, -1);
        }

        break;

    default:
        /* We got a ping response that we were not expecting.  Ignore it. */
        break;
    }
}

void wrapperPingTimeoutResponded() {
    wrapperProcessActionList(wrapperData->pingActionList, TEXT("JVM appears hung: Timed out waiting for signal from JVM."),
                             WRAPPER_ACTION_SOURCE_CODE_PING_TIMEOUT, 0, TRUE, wrapperData->errorExitCode);
}

void wrapperStopRequested(int exitCode) {
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
            TEXT("JVM requested a shutdown. (%d)"), exitCode);
    }

    /* Get things stopping on this end.  Ask the JVM to stop again in case the
     *  user code on the Java side is not written correctly. */
    wrapperStopProcess(exitCode, FALSE);
}

void wrapperRestartRequested() {
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("JVM requested a restart."));
    
    /* Make a note of the fact that we received this restart packet. */
    wrapperData->restartPacketReceived = TRUE;
    
    wrapperRestartProcess();
}

/**
 * If the current state of the JVM is STOPPING then this message is used to
 *  extend the time that the wrapper will wait for a STOPPED message before
 *  giving up on the JVM and killing it.
 */
void wrapperStopPendingSignaled(int waitHint) {
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("JVM signaled a stop pending with waitHint of %d millis."), waitHint);
    }

    if (wrapperData->jState == WRAPPER_JSTATE_STARTED) {
        /* Change the state to STOPPING */
        wrapperSetJavaState(WRAPPER_JSTATE_STOPPING, 0, -1);
        /* Don't need to set the timeout here because it will be set below. */
    }

    if (wrapperData->jState == WRAPPER_JSTATE_STOPPING) {
        if (waitHint < 0) {
            waitHint = 0;
        }

        wrapperUpdateJavaStateTimeout(wrapperGetTicks(), (int)ceil(waitHint / 1000.0));
    }
}

/**
 * The wrapper received a signal from the JVM that it has completed the stop
 *  process.  If the state of the JVM is STOPPING, then change the state to
 *  STOPPED.  It is possible to get this request after the Wrapper has given up
 *  waiting for the JVM.  In this case, the message is ignored.
 */
void wrapperStoppedSignaled() {
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("JVM signaled that it was stopped."));
    }
    
    /* If the restart mode is already set and it is WRAPPER_RESTART_REQUESTED_AUTOMATIC but we
     *  have not yet received a RESTART packet, this means that state engine got confused because
     *  of an unexpected delay.  The fact that the STOPPED packet arived but not the RESTART packet
     *  means that the application did not intend for the restart to take place.
     * Reset the restart and let the Wrapper exit. */
    if ((wrapperData->restartRequested == WRAPPER_RESTART_REQUESTED_AUTOMATIC) && (!wrapperData->restartPacketReceived)) {
        /* If we get here it is because the Wrapper previously decided to do a restart to recover.
         *  That means that another message was already shown to the user.  We want to show another
         *  message here so there is a record of why we don't restart. */
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("Received Stopped packet late.  Cancel automatic restart."));
        
        wrapperData->restartRequested = WRAPPER_RESTART_REQUESTED_NO;
    }
    
    /* Make a note of the fact that we received this stopped packet. */
    wrapperData->stoppedPacketReceived = TRUE;

    if ((wrapperData->jState == WRAPPER_JSTATE_STARTING) ||
        (wrapperData->jState == WRAPPER_JSTATE_STARTED) ||
        (wrapperData->jState == WRAPPER_JSTATE_STOPPING)) {
        /* The Java side of the wrapper signaled that it stopped
         *  allow 5 + jvmExitTimeout seconds for the JVM to exit. */
        if (wrapperData->jvmExitTimeout > 0) {
            wrapperSetJavaState(WRAPPER_JSTATE_STOPPED, wrapperGetTicks(), 5 + wrapperData->jvmExitTimeout);
        } else {
            wrapperSetJavaState(WRAPPER_JSTATE_STOPPED, 0, -1);
        }
    }
}

/**
 * If the current state of the JVM is STARTING then this message is used to
 *  extend the time that the wrapper will wait for a STARTED message before
 *  giving up on the JVM and killing it.
 */
void wrapperStartPendingSignaled(int waitHint) {
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("JVM signaled a start pending with waitHint of %d millis."), waitHint);
    }

    /* Only process the start pending signal if the JVM state is starting or
     *  stopping.  Stopping are included because if the user hits CTRL-C while
     *  the application is starting, then the stop request will not be noticed
     *  until the application has completed its startup. */
    if ((wrapperData->jState == WRAPPER_JSTATE_STARTING) ||
        (wrapperData->jState == WRAPPER_JSTATE_STOPPING)) {
        if (waitHint < 0) {
            waitHint = 0;
        }

        wrapperUpdateJavaStateTimeout(wrapperGetTicks(), (int)ceil(waitHint / 1000.0));
    }
}

/**
 * The wrapper received a signal from the JVM that it has completed the startup
 *  process.  If the state of the JVM is STARTING, then change the state to
 *  STARTED.  It is possible to get this request after the Wrapper has given up
 *  waiting for the JVM.  In this case, the message is ignored.
 */
void wrapperStartedSignaled() {
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("JVM signaled that it was started."));
    }


    if (wrapperData->jState == WRAPPER_JSTATE_STARTING) {
        /* We got the expected started packed.  Now start pinging.  Allow 5 + <pingTimeout> more seconds before the JVM
         *  is considered to be dead. */
        if (wrapperData->pingTimeout > 0) {
            wrapperSetJavaState(WRAPPER_JSTATE_STARTED, wrapperGetTicks(), 5 + wrapperData->pingTimeout);
        } else {
            wrapperSetJavaState(WRAPPER_JSTATE_STARTED, 0, -1);
        }
        /* Is the wrapper state STARTING? */
        if (wrapperData->wState == WRAPPER_WSTATE_STARTING) {
            wrapperSetWrapperState(WRAPPER_WSTATE_STARTED);

            if (!wrapperData->isConsole) {
                /* Tell the service manager that we started */
                wrapperReportStatus(FALSE, WRAPPER_WSTATE_STARTED, 0, 0);
            }
        }
        
        /* If we are configured to detach and shutdown when the JVM is started then start doing so. */
        if (wrapperData->detachStarted) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("JVM launched and detached.  Wrapper Shutting down..."));
            wrapperProtocolClose();
            wrapperDetachJava();
            wrapperStopProcess(0, FALSE);
        }
        
    } else if (wrapperData->jState == WRAPPER_JSTATE_STOP) {
        /* This will happen if the Wrapper was asked to stop as the JVM is being launched. */
    } else if (wrapperData->jState == WRAPPER_JSTATE_STOPPING) {
        /* This will happen if the Wrapper was asked to stop as the JVM is being launched. */
        wrapperSetJavaState(WRAPPER_JSTATE_STOP, 0, -1);
    }
}

#ifdef CUNIT
static void tsJAP_subTestJavaAdditionalParamSuite(int stripQuote, TCHAR *config, TCHAR **strings, int strings_len, int isJVMParam) {
    LoadParameterFileCallbackParam param;
    int ret;
    /*int i;*/

    param.stripQuote = stripQuote;
    param.strings = NULL;
    param.index = 0;
    param.isJVMParam = isJVMParam;
    ret = loadParameterFileCallback((void *)(&param), NULL, 0, config, FALSE, LEVEL_NONE);
    CU_ASSERT_TRUE(ret);
    if (!ret) {
        return;
    }
    CU_ASSERT(strings_len == param.index);

    param.stripQuote = stripQuote;
    param.strings = (TCHAR **)malloc(sizeof(TCHAR *) * strings_len);
    if (!param.strings) {
        return;
    }
    
    param.index = 0;
    param.isJVMParam = isJVMParam;

    ret = loadParameterFileCallback((void *)(&param), NULL, 0, config, FALSE, LEVEL_NONE);
    CU_ASSERT_TRUE(ret);
    if (!ret) {
        return;
    }
    CU_ASSERT(strings_len == param.index);

    if (!param.strings) {
        return;
    }
    
    /*for (i = 0; i < strings_len; i++) {
        CU_ASSERT(_tcscmp(strings[i], param.strings[i]) == 0);
    }*/

    /*for (i = 0; i < strings_len; i++) {
        free(param.strings[i]);
    }*/
    free(param.strings);
}

#define TSJAP_ARRAY_LENGTH(a) (sizeof(a) / sizeof(a[0]))

void tsJAP_testJavaAdditionalParamSuite(void) {
    int stripQuote;
    int i = 0;
    int isJVM = TRUE;
    for (i = 0; i < 2; i++) {
        _tprintf(TEXT("%d round\n"), i);
        if (i > 0) {
            isJVM = FALSE;
        }
        /* Test set #1 */
        {
            /* Single parameter in 1 line. */
            TCHAR *config = TEXT("-Dsomething=something");
            TCHAR *strings[1];
            strings[0] = TEXT("-Dsomething=something");
            tsJAP_subTestJavaAdditionalParamSuite(FALSE, config, strings, TSJAP_ARRAY_LENGTH(strings), isJVM);
        }
        {
            /* Multiple parameters in 1 line. */
            TCHAR *config = TEXT("-Dsomething=something -Dxxx=xxx");
            TCHAR *strings[2];
            strings[0] = TEXT("-Dsomething=something");
            strings[1] = TEXT("-Dxxx=xxx");
            tsJAP_subTestJavaAdditionalParamSuite(FALSE, config, strings, TSJAP_ARRAY_LENGTH(strings), isJVM);
        }
        {
            /* Horizontal Tab is not a delimiter. */
            TCHAR *config = TEXT("-Dsomething1=something1\t-Dsomething2=something2 -Dxxx=xxx");
            TCHAR *strings[2];
            strings[0] = TEXT("-Dsomething1=something1\t-Dsomething2=something2");
            strings[1] = TEXT("-Dxxx=xxx");
            tsJAP_subTestJavaAdditionalParamSuite(FALSE, config, strings, TSJAP_ARRAY_LENGTH(strings), isJVM);
        }
        {
            /* Horizontal Tab is not a delimiter. */
            TCHAR *config = TEXT("-Dsomething1=something1\t-Dsomething2=something2 -Dxxx=xxx");
            TCHAR *strings[2];
            strings[0] = TEXT("-Dsomething1=something1\t-Dsomething2=something2");
            strings[1] = TEXT("-Dxxx=xxx");
            tsJAP_subTestJavaAdditionalParamSuite(FALSE, config, strings, TSJAP_ARRAY_LENGTH(strings), isJVM);
        }
        if (isJVM == TRUE) {
            {
                /* A parameter without heading '-' will be skipped. */
                TCHAR *config = TEXT("something=something -Dxxx=xxx");
                TCHAR *strings[1];
                strings[0] = TEXT("-Dxxx=xxx");
                tsJAP_subTestJavaAdditionalParamSuite(FALSE, config, strings, TSJAP_ARRAY_LENGTH(strings), isJVM);
            }
        } else {
            {
            /* A parameter without heading '-' will not be skipped. */
            TCHAR *config = TEXT("something=something -Dxxx=xxx");
            TCHAR *strings[2];
            strings[0] = TEXT("something=something");
            strings[1] = TEXT("-Dxxx=xxx");
            tsJAP_subTestJavaAdditionalParamSuite(FALSE, config, strings, TSJAP_ARRAY_LENGTH(strings), isJVM);
            }
        }

        /* Test set #2 : without stripping double quotations */
        stripQuote = FALSE;    
        {
            /* Quotations #1 */
            TCHAR *config = TEXT("-DmyApp.x1=\"Hello World.\" -DmyApp.x2=x2");
            TCHAR *strings[2];
            strings[0] = TEXT("-DmyApp.x1=\"Hello World.\"");
            strings[1] = TEXT("-DmyApp.x2=x2");
            tsJAP_subTestJavaAdditionalParamSuite(stripQuote, config, strings, TSJAP_ARRAY_LENGTH(strings), isJVM);
        }
        {
            /* Quotations #2 */
            TCHAR *config = TEXT("\"-DmyApp.x1=Hello World.\" -DmyApp.x2=x2");
            TCHAR *strings[2];
            strings[0] = TEXT("\"-DmyApp.x1=Hello World.\"");
            strings[1] = TEXT("-DmyApp.x2=x2");
            tsJAP_subTestJavaAdditionalParamSuite(stripQuote, config, strings, TSJAP_ARRAY_LENGTH(strings), isJVM);
        }
        {
            /* Escaped quotation */
            TCHAR *config = TEXT("-DmyApp.x1=\"Hello \\\"World.\" -DmyApp.x2=x2");
            TCHAR *strings[2];
            strings[0] = TEXT("-DmyApp.x1=\"Hello \\\"World.\"");
            strings[1] = TEXT("-DmyApp.x2=x2");
            tsJAP_subTestJavaAdditionalParamSuite(stripQuote, config, strings, TSJAP_ARRAY_LENGTH(strings), isJVM);
        }
        {
            /* Escaped backslash */
            TCHAR *config = TEXT("-DmyApp.x1=\"Hello World.\\\\\" -DmyApp.x2=x2");
            TCHAR *strings[2];
            strings[0] = TEXT("-DmyApp.x1=\"Hello World.\\\\\"");
            strings[1] = TEXT("-DmyApp.x2=x2");
            tsJAP_subTestJavaAdditionalParamSuite(stripQuote, config, strings, TSJAP_ARRAY_LENGTH(strings), isJVM);
        }
        /* Test set #3 : with stripping double quotations */
        stripQuote = TRUE;
        {
            /* Quotations #1 */
            TCHAR *config = TEXT("-DmyApp.x1=\"Hello World.\" -DmyApp.x2=x2");
            TCHAR *strings[2];
            strings[0] = TEXT("-DmyApp.x1=Hello World.");
            strings[1] = TEXT("-DmyApp.x2=x2");
            tsJAP_subTestJavaAdditionalParamSuite(stripQuote, config, strings, TSJAP_ARRAY_LENGTH(strings), isJVM);
        }
        {
            /* Quotations #2 */
            TCHAR *config = TEXT("\"-DmyApp.x1=Hello World.\" -DmyApp.x2=x2");
            TCHAR *strings[2];
            strings[0] = TEXT("-DmyApp.x1=Hello World.");
            strings[1] = TEXT("-DmyApp.x2=x2");
            tsJAP_subTestJavaAdditionalParamSuite(stripQuote, config, strings, TSJAP_ARRAY_LENGTH(strings), isJVM);
        }
        {
            /* Escaped quotation */
            TCHAR *config = TEXT("-DmyApp.x1=\"Hello \\\"World.\" -DmyApp.x2=x2");
            TCHAR *strings[2];
            strings[0] = TEXT("-DmyApp.x1=Hello \"World.");
            strings[1] = TEXT("-DmyApp.x2=x2");  
            tsJAP_subTestJavaAdditionalParamSuite(stripQuote, config, strings, TSJAP_ARRAY_LENGTH(strings), isJVM);
        }
        {
            /* Escaped backslash */
            TCHAR *config = TEXT("-DmyApp.x1=\"Hello World.\\\\\" -DmyApp.x2=x2");
            TCHAR *strings[2];
            strings[0] = TEXT("-DmyApp.x1=Hello World.\\");
            strings[1] = TEXT("-DmyApp.x2=x2");
            tsJAP_subTestJavaAdditionalParamSuite(stripQuote, config, strings, TSJAP_ARRAY_LENGTH(strings), isJVM);
        }
    }
}
#endif /* CUNIT */
