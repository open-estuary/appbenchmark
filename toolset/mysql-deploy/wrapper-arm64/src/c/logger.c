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
 *   Johan Sorlin   <Johan.Sorlin@Paregos.se>
 *   Leif Mortenson <leif@tanukisoftware.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "logger_file.h"

#ifdef WIN32
 #include <io.h>
 #include <Fcntl.h>
 #include <windows.h>
 #include <tchar.h>
 #include <conio.h>
 #include <sys/timeb.h>
 #include "messages.h"

/* MS Visual Studio 8 went and deprecated the POXIX names for functions.
 *  Fixing them all would be a big headache for UNIX versions. */
 #pragma warning(disable : 4996)

/* Defines for MS Visual Studio 6 */
 #ifndef _INTPTR_T_DEFINED
typedef long intptr_t;
  #define _INTPTR_T_DEFINED
 #endif

#else
 #include <syslog.h>
 #include <strings.h>
 #include <pthread.h>
 #include <sys/time.h>
 #include <limits.h>

 #if defined(SOLARIS)
  #include <sys/errno.h>
  #include <sys/fcntl.h>
 #elif defined(AIX) || defined(HPUX) || defined(MACOSX) || defined(OSF1)
 #elif defined(IRIX)
 #elif defined(FREEBSD)
  #include <sys/param.h>
  #include <errno.h>
 #else /* LINUX */
  #include <asm/errno.h>
  #include <gnu/libc-version.h>
 #endif

#endif

#include "wrapper_i18n.h"
#include "logger.h"

#ifndef TRUE
 #define TRUE -1
#endif

#ifndef FALSE
 #define FALSE 0
#endif

TCHAR* defaultLogFile;

#ifdef WIN32
const TCHAR* syslogName = TEXT("Event Log");
#else
const TCHAR* syslogName = TEXT("syslog");
#endif

/* Global data for logger */

/* Maximum number of milliseconds that a log write can take before we show a warning. */
int logPrintfWarnThreshold = 0;

/* Number of millisecoonds which the previous log message took to process. */
time_t previousLogLag;

/* Keep track of when the last log entry was made so we can show the information in the log. */
time_t previousNow;
int    previousNowMillis;

/* Initialize all log levels to unknown until they are set */
int currentConsoleLevel = LEVEL_UNKNOWN;
int currentLogfileLevel = LEVEL_UNKNOWN;
int currentLoginfoLevel = LEVEL_UNKNOWN;
int currentLogSplitMessages = FALSE;
int currentLogRegister = TRUE;

/* Default syslog facility is LOG_USER */
int currentLogfacilityLevel = LOG_USER;

/* Callback notified whenever the active logfile changes. */
void (*logFileChangedCallback)(const TCHAR *logFile);

/* Callback to support additional logging format. */
int (*logFormatCountCallback)(const TCHAR format, size_t *reqSize);
int (*logFormatPrintCallback)(const TCHAR format, size_t printSize, TCHAR** pBuffer);

/* Stores a carefully malloced filename of the most recent log file change.   This value is only set in log_printf(), and only cleared in maintainLogger(). */
TCHAR *pendingLogFileChange = NULL;

int logPauseTime = -1;
int logBufferGrowth = FALSE;

TCHAR *logFilePath;

/* Keep track if the log file path has changed since we last opened the log file. */
int logFilePathChanged;

/* Keep track if the configured log file path has changed since we last opened the log file. */
int confLogFilePathChanged;

/* Size of the currentLogFileName and workLogFileName buffers. */
size_t currentLogFileNameSize;
TCHAR *currentLogFileName;
TCHAR *workLogFileName;
size_t confLogFileNameSize;
TCHAR *confLogFileName;
TCHAR *workConfLogFileName;
int    confLogFileLevelInt = LEVEL_UNKNOWN;
int    whichLogFile;

#define LOG_FILE_UNSET       0
#define LOG_FILE_CONFIGURED  1
#define LOG_FILE_DEFAULT     2
#define LOG_FILE_DISABLED    3

int logFileRollMode = ROLL_MODE_SIZE;
int confLogFileRollMode = ROLL_MODE_SIZE;
int logFileUmask = 0022;
TCHAR *logLevelNames[] = { TEXT("NONE  "), TEXT("DEBUG "), TEXT("INFO  "), TEXT("STATUS"), TEXT("WARN  "), TEXT("ERROR "), TEXT("FATAL "), TEXT("ADVICE"), TEXT("NOTICE") };
#ifdef WIN32
TCHAR *defaultLoginfoSourceName = TEXT("wrapper");
TCHAR *loginfoSourceName = NULL;
#else
char *defaultLoginfoSourceName = "wrapper";
char *loginfoSourceName = NULL;
#endif
int  logFileMaxSize = -1;
int  confLogFileMaxSize = -1;
int  logFileMaxLogFiles = -1;
int  confLogFileMaxLogFiles = -1;
TCHAR *logFilePurgePattern = NULL;
int  logFilePurgeSortMode = LOGGER_FILE_SORT_MODE_TIMES;

TCHAR logFileLastNowDate[9];
/* Defualt formats (Must be 4 chars) */
TCHAR consoleFormat[32];
TCHAR logfileFormat[32];
/* Flag to keep track of whether the console output should be flushed or not. */
int consoleFlush = FALSE;

#ifdef WIN32
/* Flag to keep track of whether we should write directly to the console or not. */
int consoleDirect = TRUE;
#endif

/* Flags to contol where error log level output goes to the console. */
int consoleFatalToStdErr = TRUE;
int consoleErrorToStdErr = TRUE;
int consoleWarnToStdErr = FALSE;

/* Number of seconds since the Wrapper was launched. */
int uptimeSeconds = 0;
/* TRUE once the uptime is so large that it is meaningless. */
int uptimeFlipped = FALSE;

int isPreload = FALSE;

/* Internal function declaration */
#ifdef WIN32
void sendEventlogMessage( int source_id, int level, const TCHAR *szBuff );
#else
void sendLoginfoMessage( int source_id, int level, const TCHAR *szBuff );
#endif
#ifdef WIN32
int writeToConsole( HANDLE hdl, TCHAR *lpszFmt, ...);
#endif
int doesFtellCauseMemoryLeak();
void checkAndRollLogs(const TCHAR *nowDate, size_t printBufferSize);
int lockLoggingMutex();
int releaseLoggingMutex();

#if defined(UNICODE) && !defined(WIN32)
TCHAR formatMessages[WRAPPER_THREAD_COUNT][QUEUED_BUFFER_SIZE];
#endif
int queueWrapped[WRAPPER_THREAD_COUNT];
int queueWriteIndex[WRAPPER_THREAD_COUNT];
int queueReadIndex[WRAPPER_THREAD_COUNT];
TCHAR queueMessages[WRAPPER_THREAD_COUNT][QUEUE_SIZE][QUEUED_BUFFER_SIZE];
int queueSourceIds[WRAPPER_THREAD_COUNT][QUEUE_SIZE];
int queueLevels[WRAPPER_THREAD_COUNT][QUEUE_SIZE];

/* Thread specific work buffers. */
int threadSets[WRAPPER_THREAD_COUNT];
#ifdef WIN32
DWORD threadIds[WRAPPER_THREAD_COUNT];
#else
pthread_t threadIds[WRAPPER_THREAD_COUNT];
#endif
TCHAR *threadMessageBuffer = NULL;
size_t threadMessageBufferSize = 0;
size_t threadMessageBufferInitialSize = 100;
TCHAR *threadPrintBuffer = NULL;
size_t threadPrintBufferSize = 0;

#ifdef WIN32
int launcherSource = FALSE;
#endif

/* Flag which gets set when a log entry is written to the log file. */
int logFileAccessed = FALSE;

/* Logger file pointer.  It is kept open under high log loads but closed whenever it has been idle. */
FILE *logfileFP = NULL;

/** Flag which controls whether or not the logfile is auto flushed after each line. */
int autoFlushLogfile = 0;

/** Flag which controls whether or not the logfile is auto closed after each line. */
int autoCloseLogfile = 0;

/* The number of lines sent to the log file since the getLogfileActivity method was last called. */
DWORD logfileActivityCount = 0;


/* Mutex for synchronization of the log_printf function. */
#ifdef WIN32
HANDLE log_printfMutexHandle = NULL;
#else
pthread_mutex_t log_printfMutex = PTHREAD_MUTEX_INITIALIZER;
#endif

void outOfMemory(const TCHAR *context, int id) {
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Out of memory (%s%02d). %s"),
        context, id, getLastErrorText());
}

/* This can be called from within logging code that would otherwise get stuck in recursion.
 *  Log to the console exactly when it happens and then also try to get it into the log
 *  file at the next oportunity. */
void outOfMemoryQueued(const TCHAR *context, int id) {
    _tprintf(TEXT("Out of memory (%s%02d). %s\n"), context, id, getLastErrorText());
    log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Out of memory (%s%02d). %s"),
        context, id, getLastErrorText());
}

#ifdef _DEBUG
/**
 * Used to dump memory directly to the log file in both HEX and readable format.
 *  Useful in debugging applications to track down memory overflows etc.
 *
 * @param label A label that will be prepended on all lines of output.
 * @param memory The memory to be dumped.
 * @param len The length of the memory to be dumped.
 */
void log_dumpHex(TCHAR *label, TCHAR *memory, size_t len) {
    TCHAR *buffer;
    TCHAR *pos;
    size_t i;
    int c;
    
    buffer = malloc(sizeof(TCHAR) * (len * 3 + 1));
    if (!buffer) {
        outOfMemory(TEXT("DH"), 1);
    }
    
    pos = buffer;
    for (i = 0; i < len; i++) {
        c = memory[i] & 0xff;
        _sntprintf(pos, 4, TEXT("%02x "), c);
        pos += 3;
    }
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("%s (HEX)  = %s"), label, buffer);
    
    pos = buffer;
    for (i = 0; i < len; i++) {
        c = memory[i] & 0xff;
        if (c == 0) {
            _sntprintf(pos, 4, TEXT("\\0 "));
        } else if (c <= 26) {
            _sntprintf(pos, 4, TEXT("\\%c "), TEXT('a') + c - 1);
        } else if (c < 127) {
            _sntprintf(pos, 4, TEXT("%c  "), c);
        } else {
            _sntprintf(pos, 4, TEXT(".  "));
        }
        pos += 3;
    }
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_INFO, TEXT("%s (CHAR) = %s"), label, buffer);
    
    free(buffer);
}
#endif

void invalidMultiByteSequence(const TCHAR *context, int id) {
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("Invalid multibyte Sequence found in (%s%02d). %s"),
        context, id, getLastErrorText());
}

/**
 * Replaces one token with another.  The length of the new token must be equal
 *  to or less than that of the old token.
 *
 * newToken may be null, implying "".
 */
TCHAR *replaceStringLongWithShort(TCHAR *string, const TCHAR *oldToken, const TCHAR *newToken) {
    size_t oldLen = _tcslen(oldToken);
    size_t newLen;
    TCHAR *in = string;
    TCHAR *out = string;

    if (newToken) {
        newLen = _tcslen(newToken);
    } else {
        newLen = 0;
    }

    /* Assertion check. */
    if (newLen > oldLen) {
        return string;
    }

    while (in[0] != L'\0') {
        if (_tcsncmp(in, oldToken, oldLen) == 0) {
            /* Found the oldToken.  Replace it with the new. */
            if (newLen > 0) {
                _tcsncpy(out, newToken, newLen);
            }
            in += oldLen;
            out += newLen;
        }
        else
        {
            out[0] = in[0];
            in++;
            out++;
        }
    }
    out[0] = L'\0';

    return string;
}

/**
 * Initializes the logger.  Returns 0 if the operation was successful.
 */
int initLogging(void (*logFileChanged)(const TCHAR *logFile)) {
    int threadId, i;

    logFileChangedCallback = logFileChanged;

#ifdef WIN32
    if (!(log_printfMutexHandle = CreateMutex(NULL, FALSE, NULL))) {
        _tprintf(TEXT("Failed to create logging mutex. %s\n"), getLastErrorText());
        return 1;
    }
#endif

    defaultLogFile = malloc(sizeof(TCHAR) * 12);
    if (!defaultLogFile) {
        _tprintf(TEXT("Out of memory in logging code (%s)\n"), TEXT("IL1"));
        return 1;
    }
    _tcsncpy(defaultLogFile, TEXT("wrapper.log"), 12);
    defaultLogFile[11] = TEXT('\0');
    
    logPauseTime = -1;

    loginfoSourceName = defaultLoginfoSourceName;

    logFileAccessed = FALSE;
    logFileLastNowDate[0] = L'\0';

    for ( threadId = 0; threadId < WRAPPER_THREAD_COUNT; threadId++ ) {
        threadSets[threadId] = FALSE;
        /* threadIds[threadId] = 0; */

#if defined(UNICODE) && !defined(WIN32)
        formatMessages[threadId][0] = TEXT('\0');
#endif
        for ( i = 0; i < QUEUE_SIZE; i++ )
        {
            queueWrapped[threadId] = 0;
            queueWriteIndex[threadId] = 0;
            queueReadIndex[threadId] = 0;
            queueMessages[threadId][i][0] = TEXT('\0');
            queueSourceIds[threadId][i] = 0;
            queueLevels[threadId][i] = 0;
        }
    }
    return 0;
}

/**
 * Disposes of any logging resouces prior to shutdown.
 */
int disposeLogging() {
#ifdef WIN32
    
    /* Always call maintain logger once to make sure that all queued messages are logged before we exit. */
    maintainLogger();
    
    if (log_printfMutexHandle) {
        if (!CloseHandle(log_printfMutexHandle)) {
            _tprintf(TEXT("Unable to close Logging Mutex handle. %s\n"), getLastErrorText());
            return 1;
        }
    }
#endif
    if (threadPrintBuffer && threadPrintBufferSize > 0) {
        free(threadPrintBuffer);
        threadPrintBuffer = NULL;
        threadPrintBufferSize = 0;
    }
    if (threadMessageBuffer && threadMessageBufferSize > 0) {
        free(threadMessageBuffer);
        threadMessageBuffer = NULL;
        threadMessageBufferSize = 0;
    }


    if (defaultLogFile) {
        free(defaultLogFile);
        defaultLogFile = NULL;
    }
    
    if (logFilePath) {
        free(logFilePath);
        logFilePath = NULL;
    }
    if (currentLogFileName) {
        free(currentLogFileName);
        currentLogFileName = NULL;
    }
    if (confLogFileName) {
        free(confLogFileName);
        confLogFileName = NULL;
    }
    if (workLogFileName) {
        free(workLogFileName);
        workLogFileName = NULL;
    }
    if (workConfLogFileName) {
        free(workConfLogFileName);
        workConfLogFileName = NULL;
    }
    if (pendingLogFileChange) {
        free(pendingLogFileChange);
        pendingLogFileChange = NULL;
    }
    if ((loginfoSourceName != defaultLoginfoSourceName) && (loginfoSourceName != NULL)) {
        free(loginfoSourceName);
        loginfoSourceName = NULL;
    }
    if (logfileFP) {
        fclose(logfileFP);
        logfileFP = NULL;
    }
    return 0;
}

void logRegisterFormatCallbacks(int (*countCallback)(const TCHAR format, size_t *reqSize), 
                                int (*printCallback)(const TCHAR format, size_t printSize, TCHAR** pBuffer)) {
    logFormatCountCallback = countCallback;
    logFormatPrintCallback = printCallback;
}

/** Registers the calling thread so it can be recognized when it calls
 *  again later. */
void logRegisterThread( int thread_id ) {
#ifdef WIN32
    DWORD threadId;
    threadId = GetCurrentThreadId();
#else
    pthread_t threadId;
    threadId = pthread_self();
#endif

#ifdef _DEBUG
    _tprintf(TEXT("logRegisterThread(%d)\n"), thread_id);
#endif
    if ( thread_id >= 0 && thread_id < WRAPPER_THREAD_COUNT )
    {
        threadSets[thread_id] = TRUE;
        threadIds[thread_id] = threadId;
#ifdef _DEBUG
        _tprintf(TEXT("logRegisterThread(%d) found\n"), thread_id);
#endif
    }
}

int getThreadId() {
    int i;
#ifdef WIN32
    DWORD threadId;
    threadId = GetCurrentThreadId();
#else
    pthread_t threadId;
    threadId = pthread_self();
#endif
    /*_tprintf(TEXT("threadId=%lu\n"), threadId );*/

    for ( i = 0; i < WRAPPER_THREAD_COUNT; i++ ) {
#ifdef WIN32
        if (threadSets[i] && (threadIds[i] == threadId)) {
#else
        if (threadSets[i] && pthread_equal(threadIds[i], threadId)) {
#endif
            return i;
        }
    }

    _tprintf( TEXT("WARNING - Encountered an unknown thread %ld in getThreadId().\n"),
        (long int)threadId
        );
    return 0; /* WRAPPER_THREAD_SIGNAL */
}

int getLogfileRollModeForName( const TCHAR *logfileRollName ) {
    if (strcmpIgnoreCase(logfileRollName, TEXT("NONE")) == 0) {
        return ROLL_MODE_NONE;
    } else if (strcmpIgnoreCase(logfileRollName, TEXT("SIZE")) == 0) {
        return ROLL_MODE_SIZE;
    } else if (strcmpIgnoreCase(logfileRollName, TEXT("WRAPPER")) == 0) {
        return ROLL_MODE_WRAPPER;
    } else if (strcmpIgnoreCase(logfileRollName, TEXT("JVM")) == 0) {
        return ROLL_MODE_JVM;
    } else if (strcmpIgnoreCase(logfileRollName, TEXT("SIZE_OR_WRAPPER")) == 0) {
        return ROLL_MODE_SIZE_OR_WRAPPER;
    } else if (strcmpIgnoreCase(logfileRollName, TEXT("SIZE_OR_JVM")) == 0) {
        return ROLL_MODE_SIZE_OR_JVM;
    } else if (strcmpIgnoreCase(logfileRollName, TEXT("DATE")) == 0) {
        return ROLL_MODE_DATE;
    } else {
        return ROLL_MODE_UNKNOWN;
    }
}

int getLogLevelForName( const TCHAR *logLevelName ) {
    if (strcmpIgnoreCase(logLevelName, TEXT("NONE")) == 0) {
        return LEVEL_NONE;
    } else if (strcmpIgnoreCase(logLevelName, TEXT("NOTICE")) == 0) {
        return LEVEL_NOTICE;
    } else if (strcmpIgnoreCase(logLevelName, TEXT("ADVICE")) == 0) {
        return LEVEL_ADVICE;
    } else if (strcmpIgnoreCase(logLevelName, TEXT("FATAL")) == 0) {
        return LEVEL_FATAL;
    } else if (strcmpIgnoreCase(logLevelName, TEXT("ERROR")) == 0) {
        return LEVEL_ERROR;
    } else if (strcmpIgnoreCase(logLevelName, TEXT("WARN")) == 0) {
        return LEVEL_WARN;
    } else if (strcmpIgnoreCase(logLevelName, TEXT("STATUS")) == 0) {
        return LEVEL_STATUS;
    } else if (strcmpIgnoreCase(logLevelName, TEXT("INFO")) == 0) {
        return LEVEL_INFO;
    } else if (strcmpIgnoreCase(logLevelName, TEXT("DEBUG")) == 0) {
        return LEVEL_DEBUG;
    } else {
        return LEVEL_UNKNOWN;
    }
}

#ifndef WIN32
int getLogFacilityForName( const TCHAR *logFacilityName ) {
    if (strcmpIgnoreCase(logFacilityName, TEXT("USER")) == 0) {
      return LOG_USER;
    } else if (strcmpIgnoreCase(logFacilityName, TEXT("LOCAL0")) == 0) {
      return LOG_LOCAL0;
    } else if (strcmpIgnoreCase(logFacilityName, TEXT("LOCAL1")) == 0) {
      return LOG_LOCAL1;
    } else if (strcmpIgnoreCase(logFacilityName, TEXT("LOCAL2")) == 0) {
      return LOG_LOCAL2;
    } else if (strcmpIgnoreCase(logFacilityName, TEXT("LOCAL3")) == 0) {
      return LOG_LOCAL3;
    } else if (strcmpIgnoreCase(logFacilityName, TEXT("LOCAL4")) == 0) {
      return LOG_LOCAL4;
    } else if (strcmpIgnoreCase(logFacilityName, TEXT("LOCAL5")) == 0) {
      return LOG_LOCAL5;
    } else if (strcmpIgnoreCase(logFacilityName, TEXT("LOCAL6")) == 0) {
      return LOG_LOCAL6;
    } else if (strcmpIgnoreCase(logFacilityName, TEXT("LOCAL7")) == 0) {
      return LOG_LOCAL7;
    } else {
      return LOG_USER;
    }
}
#endif

/**
 * Sets the number of milliseconds to allow logging to take before a warning is logged.
 *  Defaults to 0 for no limit.  Possible values 0 to 3600000.
 *
 * @param threshold Warning threashold.
 */
void setLogWarningThreshold(int threshold) {
    logPrintfWarnThreshold = __max(__min(threshold, 3600000), 0);
}

/**
 * Sets the log levels to a silence so we never output anything.
 */
void setSilentLogLevels() {
    setConsoleLogLevelInt(LEVEL_NONE);
    setLogfileLevelInt(LEVEL_NONE);
    setSyslogLevelInt(LEVEL_NONE);
}

/**
 * Sets the console log levels to a simple format for help and usage messages.
 */
void setSimpleLogLevels() {
    /* Force the log levels to control output. */
    setConsoleLogFormat(TEXT("M"));
    setConsoleLogLevelInt(LEVEL_INFO);

    setLogfileLevelInt(LEVEL_NONE);
    setSyslogLevelInt(LEVEL_NONE);
}

#ifdef WIN32
/**
 * This sets a flag which tells the logger that alternate source labels should be used to indicate that the current process is a launcher.
 */
void setLauncherSource() {
    launcherSource = TRUE;
}
#endif

int getLoggingIsPreload() {
    return isPreload;
}

void setLoggingIsPreload(int value) {
    isPreload = value;
}

/* Logfile functions */
int isLogfileAccessed() {
    return logFileAccessed;
}

/**
 * Set the default log file name to an absolute path using the working directory.
 *  This function must be called after setting the working directory.
 */
int resolveDefaultLogFilePath() {
    TCHAR* resolved_log_file_path;
    
    if (defaultLogFile) {
        resolved_log_file_path = getAbsolutePathOfFile(defaultLogFile, TEXT("default log file path"), LEVEL_WARN, TRUE);
#ifdef _DEBUG
        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Absolute path to the default log file resolved to %s."), resolved_log_file_path);
#endif
        if (resolved_log_file_path) {
            free(defaultLogFile);
            /* defaultLogFile now points to resolved_log_file_path which was malloced. */
            defaultLogFile = resolved_log_file_path;
            return FALSE;
        }
    }
    return TRUE;
}

/**
 * Sets the log file to be used.  If the specified file is not absolute then
 *  it will be resolved into an absolute path.  If there are any problems with
 *  the path, like a directory not existing then the call will fail and the
 *  cause will be written to the existing log.
 *
 * @param log_file_path Path to the logfile.
 * @param isConfigured  The value comes from the configuration file.
 *
 * @return TRUE if there were any problems.
 */
int setLogfilePath(const TCHAR *log_file_path, int isConfigured) {
    size_t len;
    TCHAR* prevLogFilePath = NULL;
    TCHAR* fixed_log_file_path;

    if (!log_file_path) {
        return TRUE;
    }
    
    /* Save a copy of logFilePath and free it up. */
    if (logFilePath) {
        len = _tcslen(logFilePath);
        prevLogFilePath = malloc(sizeof(TCHAR) * (len + 1));
        if (!prevLogFilePath) {
            outOfMemoryQueued(TEXT("SLP"), 1);
            return TRUE;
        }
        _tcsncpy(prevLogFilePath, logFilePath, len + 1);
        free(logFilePath);
        logFilePath = NULL;
    }
    
    /* Create a copy of log_file_path and fix '/' and '\' depending on whether it is UNIX or Windows. */
    len = _tcslen(log_file_path);
    fixed_log_file_path = malloc(sizeof(TCHAR) * (len + 1));
    if (!fixed_log_file_path) {
        outOfMemoryQueued(TEXT("SLP"), 2);
        return TRUE;
    }
    _tcsncpy(fixed_log_file_path, log_file_path, len + 1);
#ifdef WIN32
    wrapperCorrectWindowsPath(fixed_log_file_path);
#else
    wrapperCorrectNixPath(fixed_log_file_path);
#endif

    /* Convert the path to an absolute path.
     * Log in DEBUG here. We will later show a warning with checkLogfileDir() if the directory does not exist. */
    logFilePath = getAbsolutePathOfFile(fixed_log_file_path, TEXT("log file path"), getLoggingIsPreload() ? LEVEL_NONE : LEVEL_DEBUG, FALSE);
#ifdef _DEBUG
    log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Absolute path to the configured log file resolved to %s."), logFilePath);
#endif

    if (!logFilePath) {
        /* Continue with the relative path. */
        logFilePath = fixed_log_file_path;
    } else {
        len = _tcslen(logFilePath);
        free(fixed_log_file_path);
    }

    if (prevLogFilePath) {
        if (_tcscmp(prevLogFilePath, logFilePath) == 0) {
            /* The path is unchanged. We don't want to reset currentLogFileName and other variables, so return here. */
            free(prevLogFilePath);
            return FALSE;
        }
        free(prevLogFilePath);
    }

    /* The currentLogFileNameSize is the size of logFilePath + 10 ("." + a roll number) + 1 (NULL). */
    currentLogFileNameSize = len + 10 + 1;

    if (currentLogFileName) {
        free(currentLogFileName);
    }
    currentLogFileName = malloc(sizeof(TCHAR) * currentLogFileNameSize);
    if (!currentLogFileName) {
        outOfMemoryQueued(TEXT("SLP"), 3);
        free(logFilePath);
        logFilePath = NULL;
        return TRUE;
    }
    currentLogFileName[0] = TEXT('\0');

    if (workLogFileName) {
        free(workLogFileName);
    }
    workLogFileName = malloc(sizeof(TCHAR) * currentLogFileNameSize);
    if (!workLogFileName) {
        outOfMemoryQueued(TEXT("SLP"), 4);
        free(logFilePath);
        logFilePath = NULL;
        free(currentLogFileName);
        currentLogFileNameSize = 0;
        currentLogFileName = NULL;
        return TRUE;
    }
    workLogFileName[0] = TEXT('\0');

    if (isConfigured) {
        if ((confLogFileName == NULL) || (strcmpIgnoreCase(logFilePath, confLogFileName) != 0)) {
            confLogFileNameSize = currentLogFileNameSize;
            
            if (confLogFileName) {
                /* This message will be printed in the new configured log file because it is queued. */
                log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("Configured log file changed from '%s' to '%s'."), confLogFileName, logFilePath);
                free(confLogFileName);
            } else {
                log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Configured log file set to '%s'."), logFilePath);
            }
            confLogFileName = malloc(sizeof(TCHAR) * confLogFileNameSize);
            if (!confLogFileName) {
                outOfMemoryQueued(TEXT("SLP"), 5);
                free(logFilePath);
                logFilePath = NULL;
                free(currentLogFileName);
                confLogFileNameSize = 0;
                currentLogFileName = NULL;
                free(workLogFileName);
                workLogFileName = NULL;
                return TRUE;
            }
            _tcsncpy(confLogFileName, logFilePath, confLogFileNameSize);
            
            if (workConfLogFileName) {
                free(workConfLogFileName);
            }
            workConfLogFileName = malloc(sizeof(TCHAR) * confLogFileNameSize);
            if (!confLogFileName) {
                outOfMemoryQueued(TEXT("SLP"), 6);
                free(logFilePath);
                logFilePath = NULL;
                free(currentLogFileName);
                confLogFileNameSize = 0;
                currentLogFileName = NULL;
                free(workLogFileName);
                workLogFileName = NULL;
                free(confLogFileName);
                confLogFileName = NULL;
                return TRUE;
            } 
            workConfLogFileName[0] = TEXT('\0');
            
            confLogFilePathChanged = TRUE;
        }
    }
    logFilePathChanged = TRUE;
    
    return FALSE;
}

/**
 * Returns the default logfile.
 */
const TCHAR *getDefaultLogfilePath() {
    return defaultLogFile;
}

/**
 * Returns a reference to the currect log file path.
 *  This return value may be changed at any time if the log file is rolled.
 */
const TCHAR *getLogfilePath()
{
    return logFilePath;
}
    
/**
 * Returns a snapshot of the current log file path.  This call safely gets the current path
 *  and returns a copy.  It is the responsibility of the caller to free up the memory on
 *  return.  Could return null if there was an error.
 */
TCHAR *getCurrentLogfilePath() {
    TCHAR *logFileCopy;
    
    /* Lock the logging mutex. */
    if (lockLoggingMutex()) {
        return NULL;
    }
    
    /* We should always have a current log file name here because there will be at least one line of log output before this is called.
     *  If that is false then we will return an empty length, but valid, string. */
    logFileCopy = malloc(sizeof(TCHAR) * (_tcslen(currentLogFileName) + 1));
    if (!logFileCopy) {
        _tprintf(TEXT("Out of memory in logging code (%s)\n"), TEXT("CLFP1"));
    } else {
        _tcsncpy(logFileCopy, currentLogFileName, _tcslen(currentLogFileName) + 1);
    }

    /* Release the lock we have on the logging mutex so that other threads can get in. */
    if (releaseLoggingMutex()) {
        if (logFileCopy) {
            free(logFileCopy);
        }
        return NULL;
    }
    
    return logFileCopy;
}


/**
 * Check the directory of the current logfile path to make sure it is writable.
 *  If there are any problems, log a warning.
 *
 * @return TRUE if there were any problems.
 */
int checkLogfileDir() {
    size_t len;
    TCHAR *c;
    TCHAR *logFileDir;
    TCHAR *testfile;
    int fd;
    
    len = _tcslen(logFilePath) + 1;
    logFileDir = malloc(len * sizeof(TCHAR));
    if (!logFileDir) {
        outOfMemory(TEXT("CLD"), 1);
        return TRUE;
    }
    _tcsncpy(logFileDir, logFilePath, len);
    
#ifdef WIN32
    c = _tcsrchr(logFileDir, TEXT('\\'));
#else
    c = _tcsrchr(logFileDir, TEXT('/'));
#endif
    if (c) {
        c[0] = TEXT('\0');
        
        /* We want to try writing a test file to the configured log directory to make sure it is writable. */
        len = _tcslen(logFileDir) + 23 + 1 + 1000;
        testfile = malloc(len * sizeof(TCHAR));
        if (!testfile) {
            outOfMemory(TEXT("CLD"), 2);
            free(logFileDir);
            return TRUE;
        }
        
        _sntprintf(testfile, len, TEXT("%s%c.wrapper_test-%.4d%.4d"),
            logFileDir,
#ifdef WIN32
            TEXT('\\'),
#else
            TEXT('/'),
#endif
            rand() % 9999, rand() % 9999);
        
        if ((fd = _topen(testfile, O_WRONLY | O_CREAT | O_EXCL
#ifdef WIN32
                , _S_IWRITE
#else
                , S_IRUSR | S_IWUSR
#endif 
                )) == -1) {
            if (errno == EACCES) {
                log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                    TEXT("Unable to write to the configured log directory: %s (%s)\n  The Wrapper may also have problems writing or rolling the log file.\n  Please make sure that the current user has read/write access."),
                    logFileDir, getLastErrorText());
            } else if (errno == ENOENT) {
                log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                    TEXT("Unable to write to the configured log directory: %s (%s)\n  The directory does not exist."),
                    logFileDir, getLastErrorText());
            }
        } else {
            /* Successfully wrote the temp file. */
#ifdef WIN32
            _close(fd);
#else
            close(fd);
#endif
            if (_tremove(testfile)) {
                log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                    TEXT("Unable to remove temporary file: %s (%s)\n  The Wrapper may also have problems writing or rolling the log file.\n  Please make sure that the current user has read/write access."),
                    testfile, getLastErrorText());
            }
        }
        
        free(testfile);
    }
    
    free(logFileDir);
    
    return FALSE;
}

/**
 * This method will wiat for the specified number of seconds.  It is only meant for special
 *  uses within the logging code as it does not itself log any output in a correct way.
 */
void logSleep(int ms) {
#ifdef WIN32
    Sleep(ms);
#else
    /* We want to use nanosleep if it is available, but make it possible for the
       user to build a version that uses usleep if they want.
       usleep does not behave nicely with signals thrown while sleeping.  This
       was the believed cause of a hang experienced on one Solaris system. */
 #ifdef USE_USLEEP
    usleep(ms * 1000); /* microseconds */
 #else
    struct timespec ts;

    if (ms >= 1000) {
        ts.tv_sec = (ms * 1000000) / 1000000000;
        ts.tv_nsec = (ms * 1000000) % 1000000000; /* nanoseconds */
    } else {
        ts.tv_sec = 0;
        ts.tv_nsec = ms * 1000000; /* nanoseconds */
    }

    if (nanosleep(&ts, NULL)) {
        if (errno == EINTR) {
            return;
        } else if (errno == EAGAIN) {
            /* On 64-bit AIX this happens once on shutdown. */
            return;
        } else {
            _tprintf(TEXT("nanosleep(%dms) failed. \n"), ms, getLastErrorText());
        }
    }
 #endif
#endif
}
    
/**
 * Used for testing to set a pause into the next log entry made.
 *
 * @param pauseTime Number of seconds to pause, 0 pauses indefinitely.
 */
void setPauseTime(int pauseTime)
{
    logPauseTime = pauseTime;
}
    
/**
 * Set to true to cause changes in internal buffer sizes to be logged.  Useful for debugging.
 *
 * @param log TRUE if changes should be logged.
 */
void setLogBufferGrowth(int log) {
    logBufferGrowth = log;
}

void setLogfileRollMode( int log_file_roll_mode ) {
    logFileRollMode = log_file_roll_mode;
    confLogFileRollMode = log_file_roll_mode;
}

int getLogfileRollMode() {
    return logFileRollMode;
}

void setLogfileUmask( int log_file_umask ) {
    logFileUmask = log_file_umask;
}

void setLogfileFormat( const TCHAR *log_file_format ) {
    if ( log_file_format != NULL ) {
        _tcsncpy( logfileFormat, log_file_format, 32 );
        
        /* We only want to time logging if it is needed. */
        if ((logPrintfWarnThreshold <= 0) && (_tcschr(log_file_format, TEXT('G')))) {
            logPrintfWarnThreshold = 99999999;
        }
    }
}

void setLogfileLevelInt( int log_file_level ) {
    /* Update the configured log level in case the log file was disabled while reloading the configuration. */
    confLogFileLevelInt = log_file_level;
    currentLogfileLevel = log_file_level;
}

int getLogfileLevelInt() {
    return currentLogfileLevel;
}

void setLogfileLevel( const TCHAR *log_file_level ) {
    setLogfileLevelInt(getLogLevelForName(log_file_level));
}

void setLogfileMaxFileSize( const TCHAR *max_file_size ) {
    int multiple, i, newLength;
    TCHAR *tmpFileSizeBuff;
    TCHAR chr;

    if ( max_file_size != NULL ) {
        /* Allocate buffer */
        tmpFileSizeBuff = malloc(sizeof(TCHAR) * (_tcslen( max_file_size ) + 1));
        if (!tmpFileSizeBuff) {
            outOfMemoryQueued(TEXT("SLMFS"), 1);
            return;
        }

        /* Generate multiple and remove unwanted chars */
        multiple = 1;
        newLength = 0;
        for( i = 0; i < (int)_tcslen(max_file_size); i++ ) {
            chr = max_file_size[i];

            switch( chr ) {
                case TEXT('k'): /* Kilobytes */
                case TEXT('K'):
                    multiple = 1024;
                break;

                case TEXT('M'): /* Megabytes */
                case TEXT('m'):
                    multiple = 1048576;
                break;
            }

            if( (chr >= TEXT('0') && chr <= TEXT('9')) || (chr == TEXT('-')) )
                tmpFileSizeBuff[newLength++] = max_file_size[i];
        }
        tmpFileSizeBuff[newLength] = TEXT('\0');/* Crop string */

        logFileMaxSize = _ttoi( tmpFileSizeBuff );
        if( logFileMaxSize > 0 )
            logFileMaxSize *= multiple;

        /* Free memory */
        free( tmpFileSizeBuff );
        tmpFileSizeBuff = NULL;

        if ((logFileMaxSize > 0) && (logFileMaxSize < 1024)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT(
                "wrapper.logfile.maxsize must be 0 or at least 1024.  Changing to %d."), logFileMaxSize);
            logFileMaxSize = 1024;
        }
        confLogFileMaxSize = logFileMaxSize;
    }
}

void setLogfileMaxLogFiles( int max_log_files ) {
    logFileMaxLogFiles = max_log_files;
    confLogFileMaxLogFiles = max_log_files;
}

void setLogfilePurgePattern(const TCHAR *pattern) {
    size_t len;

    if (logFilePurgePattern) {
        free(logFilePurgePattern);
        logFilePurgePattern = NULL;
    }

    len = _tcslen(pattern);
    if (len > 0) {
        logFilePurgePattern = malloc(sizeof(TCHAR) * (len + 1));
        if (!logFilePurgePattern) {
            outOfMemoryQueued(TEXT("SLPP"), 1);
            return;
        }
        _tcsncpy(logFilePurgePattern, pattern, len + 1);
    }
}

void setLogfilePurgeSortMode(int sortMode) {
    logFilePurgeSortMode = sortMode;
}

/** 
 * Disable the logfile.
 */
void disableLogFile() {
    /* Don't use setLogfileLevelInt() as it would overwrite confLogFileLevelInt. */
    currentLogfileLevel = LEVEL_NONE;
    whichLogFile = LOG_FILE_DISABLED;
}

/** 
 * Restore the log level of the logfile. It is the responsibility
 *  of the caller to specify which log file will be used after that.
 */
void enableLogFile() {
    /* Restore the log level. */
    currentLogfileLevel = confLogFileLevelInt;
}

/** Returns the number of lines of log file activity since the last call. */
DWORD getLogfileActivity() {
    DWORD logfileLines;

    /* Don't worry about synchronization here.  Any errors are not critical the way this is used. */
    logfileLines = logfileActivityCount;
    logfileActivityCount = 0;

    return logfileLines;
}

/** Obtains a lock on the logging mutex. */
int lockLoggingMutex() {
#ifdef WIN32
    switch (WaitForSingleObject(log_printfMutexHandle, INFINITE)) {
    case WAIT_ABANDONED:
        _tprintf(TEXT("Logging mutex was abandoned.\n"));
        return -1;
    case WAIT_FAILED:
        _tprintf(TEXT("Logging mutex wait failed.\n"));
        return -1;
    case WAIT_TIMEOUT:
        _tprintf(TEXT("Logging mutex wait timed out.\n"));
        return -1;
    default:
        /* Ok */
        break;
    }
#else
    if (pthread_mutex_lock(&log_printfMutex)) {
        _tprintf(TEXT("Failed to lock the Logging mutex. %s\n"), getLastErrorText());
        return -1;
    }
#endif

    return 0;
}

/** Releases a lock on the logging mutex. */
int releaseLoggingMutex() {
#ifdef WIN32
    if (!ReleaseMutex(log_printfMutexHandle)) {
        _tprintf( TEXT("Failed to release logging mutex. %s\n"), getLastErrorText());
        return -1;
    }
#else
    if (pthread_mutex_unlock(&log_printfMutex)) {
        _tprintf(TEXT("Failed to unlock the Logging mutex. %s\n"), getLastErrorText());
        return -1;
    }
#endif
    return 0;
}

/** Sets the auto flush log file flag. */
void setLogfileAutoFlush(int autoFlush) {
    autoFlushLogfile = autoFlush;
}

/** Sets the auto close log file flag. */
void setLogfileAutoClose(int autoClose) {
    autoCloseLogfile = autoClose;
}

/** Closes the logfile if it is open. */
void closeLogfile() {
    /* We need to be very careful that only one thread is allowed in here
     *  at a time.  On Windows this is done using a Mutex object that is
     *  initialized in the initLogging function. */
    if (lockLoggingMutex()) {
        return;
    }

    if (logfileFP != NULL) {
#ifdef _DEBUG
        _tprintf(TEXT("Closing logfile by request...\n"));
#endif

        fclose(logfileFP);
        logfileFP = NULL;
        /* Do not clean the currentLogFileName here as the name is not actually changing. */
    }

    /* Release the lock we have on this function so that other threads can get in. */
    if (releaseLoggingMutex()) {
        return;
    }
}

/** Flushes any buffered logfile output to the disk. */
void flushLogfile() {
    /* We need to be very careful that only one thread is allowed in here
     *  at a time.  On Windows this is done using a Mutex object that is
     *  initialized in the initLogging function. */
    if (lockLoggingMutex()) {
        return;
    }

    if (logfileFP != NULL) {
#ifdef _DEBUG
        _tprintf(TEXT("Flushing logfile by request...\n"));
#endif

        fflush(logfileFP);
    }

    /* Release the lock we have on this function so that other threads can get in. */
    if (releaseLoggingMutex()) {
        return;
    }
}

/* Console functions */
void setConsoleLogFormat( const TCHAR *console_log_format ) {
    if ( console_log_format != NULL ) {
        _tcsncpy( consoleFormat, console_log_format, 32 );
        
        /* We only want to time logging if it is needed. */
        if ((logPrintfWarnThreshold <= 0) && (_tcschr(console_log_format, TEXT('G')))) {
            logPrintfWarnThreshold = 99999999;
        }
    }
}

void setConsoleLogLevelInt( int console_log_level ) {
    currentConsoleLevel = console_log_level;
}

int getConsoleLogLevelInt() {
    return currentConsoleLevel;
}

void setConsoleLogLevel( const TCHAR *console_log_level ) {
    setConsoleLogLevelInt(getLogLevelForName(console_log_level));
}

void setConsoleFlush( int flush ) {
    consoleFlush = flush;
}
    
#ifdef WIN32
void setConsoleDirect( int direct ) {
    consoleDirect = direct;
}
#endif

void setConsoleFatalToStdErr(int toStdErr) {
    consoleFatalToStdErr = toStdErr;
}

void setConsoleErrorToStdErr(int toStdErr) {
    consoleErrorToStdErr = toStdErr;
}

void setConsoleWarnToStdErr(int toStdErr) {
    consoleWarnToStdErr = toStdErr;
}

/* Syslog/eventlog functions */
void setSyslogLevelInt( int loginfo_level ) {
    currentLoginfoLevel = loginfo_level;
}

int getSyslogLevelInt() {
    return currentLoginfoLevel;
}

void setSyslogLevel( const TCHAR *loginfo_level ) {
    setSyslogLevelInt(getLogLevelForName(loginfo_level));
}

void setSyslogSplitMessages(int splitMessages) {
    currentLogSplitMessages = splitMessages;
}

#ifdef WIN32
void setSyslogRegister(int sysRegister) {
    currentLogRegister = sysRegister;
}

int getSyslogRegister() {
    return currentLogRegister;
}

TCHAR* getSyslogEventSourceName() {
    return loginfoSourceName;
}
#endif

#ifndef WIN32
void setSyslogFacilityInt( int logfacility_level ) {
    currentLogfacilityLevel = logfacility_level;
}

void setSyslogFacility( const TCHAR *loginfo_level ) {
    setSyslogFacilityInt(getLogFacilityForName(loginfo_level));
}
#endif

void setSyslogEventSourceName( const TCHAR *event_source_name ) {
    size_t size;
    if (event_source_name != NULL) {
        if (loginfoSourceName != defaultLoginfoSourceName) {
            if (loginfoSourceName != NULL) {
                free(loginfoSourceName);
            }
        }
#ifdef WIN32
        size = sizeof(TCHAR) * (_tcslen(event_source_name) + 1);
#else
        size = wcstombs(NULL, event_source_name, 0);
        if (size == (size_t)-1) {
            return;
        }
        size++;
#endif
        loginfoSourceName = malloc(size);
        if (!loginfoSourceName) {
            _tprintf(TEXT("Out of memory in logging code (%s)\n"), TEXT("SSESN"));
            loginfoSourceName = defaultLoginfoSourceName;
            return;
        }
#ifdef WIN32
        _tcsncpy(loginfoSourceName, event_source_name, _tcslen(event_source_name) + 1);
        if (_tcslen(loginfoSourceName) > 32) {
            loginfoSourceName[32] = TEXT('\0');
        }
#else
        wcstombs(loginfoSourceName, event_source_name, size);
        if (strlen(loginfoSourceName) > 32) {
            loginfoSourceName[32] = '\0';
        }
#endif

    }
}

void resetDuration() {
#ifdef WIN32
    struct _timeb timebNow;
#else
    struct timeval timevalNow;
#endif
    time_t      now;
    int         nowMillis;

#ifdef WIN32
    _ftime(&timebNow);
    now = (time_t)timebNow.time;
    nowMillis = timebNow.millitm;
#else
    gettimeofday(&timevalNow, NULL);
    now = (time_t)timevalNow.tv_sec;
    nowMillis = timevalNow.tv_usec / 1000;
#endif
    
    previousNow = now;
    previousNowMillis = nowMillis;
}
    
int getLowLogLevel() {
    int lowLogLevel = (currentLogfileLevel < currentConsoleLevel ? currentLogfileLevel : currentConsoleLevel);
    lowLogLevel =  (currentLoginfoLevel < lowLogLevel ? currentLoginfoLevel : lowLogLevel);
    return lowLogLevel;
}

void setThreadMessageBufferInitialSize(int initialValue) {
    /* do not allow too big and too small values */
    threadMessageBufferInitialSize = __min(__max(initialValue, 100), 32768);
}

TCHAR* preparePrintBuffer(size_t reqSize) {
    if (threadPrintBuffer == NULL) {
        threadPrintBuffer = malloc(sizeof(TCHAR) * reqSize);
        if (!threadPrintBuffer) {
            _tprintf(TEXT("Out of memory in logging code (%s)\n"), TEXT("PPB1"));
            threadPrintBufferSize = 0;
            return NULL;
        }
        threadPrintBufferSize = reqSize;
    } else if (threadPrintBufferSize < reqSize) {
        free(threadPrintBuffer);
        threadPrintBuffer = malloc(sizeof(TCHAR) * reqSize);
        if (!threadPrintBuffer) {
            _tprintf(TEXT("Out of memory in logging code (%s)\n"), TEXT("PPB2"));
            threadPrintBufferSize = 0;
            return NULL;
        }
        threadPrintBufferSize = reqSize;
    }

    return threadPrintBuffer;
}

/* Returns the number of columns and come up with a required length for the printBuffer. */
int GetColumnsAndReqSizeForPrintBuffer(const TCHAR *format, const TCHAR *message, size_t *reqSize) {
    int i;
    int numColumns;

    *reqSize = 0;
    for( i = 0, numColumns = 0; i < (int)_tcslen( format ); i++ ) {
        switch( format[i] ) {
        case TEXT('P'):
        case TEXT('p'):
            *reqSize += 8 + 3;
            numColumns++;
            break;

        case TEXT('L'):
        case TEXT('l'):
            *reqSize += 6 + 3;
            numColumns++;
            break;

        case TEXT('D'):
        case TEXT('d'):
            *reqSize += 7 + 3;
            numColumns++;
            break;

        case TEXT('Q'):
        case TEXT('q'):
            *reqSize += 1 + 3;
            numColumns++;
            break;

        case TEXT('T'):
        case TEXT('t'):
            *reqSize += 19 + 3;
            numColumns++;
            break;

        case TEXT('Z'):
        case TEXT('z'):
            *reqSize += 23 + 3;
            numColumns++;
            break;

        case TEXT('U'):
        case TEXT('u'):
            *reqSize += 8 + 3;
            numColumns++;
            break;

        case TEXT('R'):
        case TEXT('r'):
            *reqSize += 8 + 3;
            numColumns++;
            break;

        case TEXT('G'):
        case TEXT('g'):
            *reqSize += 10 + 3;
            numColumns++;
            break;

        case TEXT('M'):
        case TEXT('m'):
            *reqSize += _tcslen( message ) + 3;
            numColumns++;
            break;
            
        default:
            if (logFormatCountCallback && logFormatCountCallback(format[i], reqSize)) {
                numColumns++;
            }
        }
    }
    return numColumns;
}

/* Writes to and then returns a buffer that is reused by the current thread.
 *  It should not be released. */
TCHAR* buildPrintBuffer( int source_id, int level, int threadId, int queued, struct tm *nowTM, int nowMillis, time_t durationMillis, const TCHAR *format, const TCHAR *defaultFormat, const TCHAR *message) {
    int       i;
    size_t    reqSize;
    int       numColumns;
    TCHAR     *pos;
    int       currentColumn;
    int       handledFormat;
    int       temp;
    int       len;
    
    numColumns = GetColumnsAndReqSizeForPrintBuffer(format, message, &reqSize);
    
    if ((reqSize == 0) && (defaultFormat != NULL)) {
        /* This means that the specified format was completely invalid.
         *  Recurse using the defaultFormat instead.
         *  The alternative would be to log an empty line, which is useless to everyone. */
        return buildPrintBuffer( source_id, level, threadId, queued, nowTM, nowMillis, durationMillis, defaultFormat, NULL /* No default. Prevent further recursion. */, message );
    }

    /* Always add room for the null. */
    reqSize += 1;

    if ( !preparePrintBuffer(reqSize)) {
        return NULL;
    }

    /* Always start with a null terminated string in case there are no formats specified. */
    threadPrintBuffer[0] = TEXT('\0');

    /* Create a pointer to the beginning of the print buffer, it will be advanced
     *  as the formatted message is build up. */
    pos = threadPrintBuffer;
    
    /* We now have a buffer large enough to store the entire formatted message. */
    for( i = 0, currentColumn = 0, len = 0, temp = 0; i < (int)_tcslen( format ); i++ ) {
        handledFormat = TRUE;

        switch( format[i] ) {
        case TEXT('P'):
        case TEXT('p'):
            switch ( source_id ) {
            case WRAPPER_SOURCE_WRAPPER:
#ifdef WIN32
                if (launcherSource) {
                    temp = _sntprintf( pos, reqSize - len, TEXT("wrapperm") );
                } else {
                    temp = _sntprintf( pos, reqSize - len, TEXT("wrapper ") );
                }
#else
                temp = _sntprintf( pos, reqSize - len, TEXT("wrapper ") );
#endif
                break;

            case WRAPPER_SOURCE_PROTOCOL:
                temp = _sntprintf( pos, reqSize - len, TEXT("wrapperp") );
                break;

            default:
                temp = _sntprintf( pos, reqSize - len, TEXT("jvm %-4d"), source_id );
                break;
            }
            currentColumn++;
            break;

        case TEXT('L'):
        case TEXT('l'):
            temp = _sntprintf( pos, reqSize - len, TEXT("%s"), logLevelNames[ level ] );
            currentColumn++;
            break;

        case TEXT('D'):
        case TEXT('d'):
            switch ( threadId )
            {
            case WRAPPER_THREAD_SIGNAL:
                temp = _sntprintf( pos, reqSize - len, TEXT("signal ") );
                break;

            case WRAPPER_THREAD_MAIN:
                temp = _sntprintf( pos, reqSize - len, TEXT("main   ") );
                break;

            case WRAPPER_THREAD_SRVMAIN:
                temp = _sntprintf( pos, reqSize - len, TEXT("srvmain") );
                break;

            case WRAPPER_THREAD_TIMER:
                temp = _sntprintf( pos, reqSize - len, TEXT("timer  ") );
                break;

            case WRAPPER_THREAD_JAVAIO:
                temp = _sntprintf( pos, reqSize - len, TEXT("javaio ") );
                break;

            case WRAPPER_THREAD_STARTUP:
                temp = _sntprintf( pos, reqSize - len, TEXT("startup") );
                break;

            default:
                temp = _sntprintf( pos, reqSize - len, TEXT("unknown") );
                break;
            }
            currentColumn++;
            break;

        case TEXT('Q'):
        case TEXT('q'):
            temp = _sntprintf( pos, reqSize - len, TEXT("%c"), ( queued ? TEXT('Q') : TEXT(' ')));
            currentColumn++;
            break;

        case TEXT('T'):
        case TEXT('t'):
            temp = _sntprintf( pos, reqSize - len, TEXT("%04d/%02d/%02d %02d:%02d:%02d"),
                nowTM->tm_year + 1900, nowTM->tm_mon + 1, nowTM->tm_mday,
                nowTM->tm_hour, nowTM->tm_min, nowTM->tm_sec );
            currentColumn++;
            break;

        case TEXT('Z'):
        case TEXT('z'):
            temp = _sntprintf( pos, reqSize - len, TEXT("%04d/%02d/%02d %02d:%02d:%02d.%03d"),
                nowTM->tm_year + 1900, nowTM->tm_mon + 1, nowTM->tm_mday,
                nowTM->tm_hour, nowTM->tm_min, nowTM->tm_sec, nowMillis );
            currentColumn++;
            break;
            
        case TEXT('U'):
        case TEXT('u'):
            if (uptimeFlipped) {
                temp = _sntprintf( pos, reqSize - len, TEXT("--------") );
            } else {
                temp = _sntprintf( pos, reqSize - len, TEXT("%8d"), uptimeSeconds);
            }
            currentColumn++;
            break;
            
        case TEXT('R'):
        case TEXT('r'):
            if (durationMillis == (time_t)-1) {
                temp = _sntprintf( pos, reqSize - len, TEXT("        ") );
            } else if (durationMillis > 99999999) {
                temp = _sntprintf( pos, reqSize - len, TEXT("99999999") );
            } else {
                temp = _sntprintf( pos, reqSize - len, TEXT("%8d"), durationMillis);
            }
            currentColumn++;
            break;
            
        case TEXT('G'):
        case TEXT('g'):
            temp = _sntprintf( pos, reqSize - len, TEXT("%8d"), __min(previousLogLag, 99999999));
            currentColumn++;
            break;

        case TEXT('M'):
        case TEXT('m'):
            temp = _sntprintf( pos, reqSize - len, TEXT("%s"), message );
            currentColumn++;
            break;

        default:
            if (logFormatPrintCallback && (temp = logFormatPrintCallback(format[i], reqSize - len, &pos))) {
                currentColumn++;
            } else {
                handledFormat = FALSE;
            }
        }
        
        if (handledFormat) {
            pos += temp;
            len += temp;
            
            /* Add separator chars */
            if (currentColumn != numColumns) {
                temp = _sntprintf(pos, reqSize - len, TEXT(" | "));
                pos += temp;
                len += temp;
            }
        }
    }

    /* Return the print buffer to the caller. */
    return threadPrintBuffer;
}

/**
 * Generates a log file name given.
 *
 * buffer - Buffer into which to _sntprintf the generated name.
 * bufferSize - Size of the buffer.
 * template - Template from which the name is generated.
 * nowDate - Optional date used to replace any YYYYMMDD tokens.
 * rollNum - Optional roll number used to replace any ROLLNUM tokens.
 */
void generateLogFileName(TCHAR *buffer, size_t bufferSize, const TCHAR *template, const TCHAR *nowDate, const TCHAR *rollNum ) {
    size_t bufferLen;
    
    /* Copy the template to the buffer to get started. */
    _tcsncpy(buffer, template, _tcslen(template) + 1);

    /* Handle the date token. */
    if (_tcsstr(buffer, TEXT("YYYYMMDD"))) {
        if (nowDate == NULL) {
            /* The token needs to be removed. */
            replaceStringLongWithShort(buffer, TEXT("-YYYYMMDD"), NULL);
            replaceStringLongWithShort(buffer, TEXT("_YYYYMMDD"), NULL);
            replaceStringLongWithShort(buffer, TEXT(".YYYYMMDD"), NULL);
            replaceStringLongWithShort(buffer, TEXT("YYYYMMDD"), NULL);
        } else {
            /* The token needs to be replaced. */
            replaceStringLongWithShort(buffer, TEXT("YYYYMMDD"), nowDate);
        }
    }

    /* Handle the roll number token. */
    if (_tcsstr(buffer, TEXT("ROLLNUM"))) {
        if (rollNum == NULL ) {
            /* The token needs to be removed. */
            replaceStringLongWithShort(buffer, TEXT("-ROLLNUM"), NULL);
            replaceStringLongWithShort(buffer, TEXT("_ROLLNUM"), NULL);
            replaceStringLongWithShort(buffer, TEXT(".ROLLNUM"), NULL);
            replaceStringLongWithShort(buffer, TEXT("ROLLNUM"), NULL);
        } else {
            /* The token needs to be replaced. */
            replaceStringLongWithShort(buffer, TEXT("ROLLNUM"), rollNum);
        }
    } else {
        /* The name did not contain a ROLLNUM token. */
        if (rollNum != NULL ) {
            /* Generate the name as if ".ROLLNUM" was appended to the template. */
            bufferLen = _tcslen(buffer);
            _sntprintf(buffer + bufferLen, bufferSize - bufferLen, TEXT(".%s"), rollNum);
            buffer[bufferSize - 1] = TEXT('\0');
        }
    }
}

/**
 * Prints the contents of a buffer to the sysLog target.  The log level is
 *  tested prior to this function being called.
 *
 * Must be called while locked.
 */
void log_printf_message_sysLogInner(int source_id, int level, TCHAR *message, struct tm *nowTM) {
#ifdef WIN32
        sendEventlogMessage(source_id, level, message);
#else
        sendLoginfoMessage(source_id, level, message);
#endif
}
/**
 * @param invertLogLevelCheck There is a special case where we want to log to
 *                            the syslog IF and only if the normal log output
 *                            would not go to the syslog.  This flag makes it
 *                            possible to log it if we normally would not.
 */
void log_printf_message_sysLog(int source_id, int level, TCHAR *message, struct tm *nowTM, int invertLogLevelCheck) {
    switch (level) {
    case LEVEL_NOTICE:
    case LEVEL_ADVICE:
        /* Advice and Notice level messages are special in that they never get logged to the
         *  EventLog / SysLog. */
        break;
        
    default:
        if (invertLogLevelCheck) {
            /* Log if we normally should not log. */
            if (level < currentLoginfoLevel) {
                log_printf_message_sysLogInner(source_id, level, message, nowTM);
            }
        } else {
            if (level >= currentLoginfoLevel) {
                log_printf_message_sysLogInner(source_id, level, message, nowTM);
            }
        }
    }
}

static void setDefaultRolling() {
    logFileRollMode = ROLL_MODE_SIZE;
    logFileMaxSize = 5 * 1048576;
    logFileMaxLogFiles = 1;
}

static void restoreConfiguredRolling() {
    logFileRollMode = confLogFileRollMode;
    logFileMaxSize = confLogFileMaxSize;
    logFileMaxLogFiles = confLogFileMaxLogFiles;
}

static void printFailoverFileHeader(TCHAR* confFileName) {
    _ftprintf(logfileFP, TEXT("********************************************************************************\n"));
    _ftprintf(logfileFP, TEXT("* This is a Java Service Wrapper failover log file.\n*  Was unable to write to %s.\n"), confFileName);
    _ftprintf(logfileFP, TEXT("********************************************************************************\n\n"));
}

/**
 * Open log file in the following order of priority: configured log file, default log file, no log file.
 *  This function will also try to resume logging in a file with a higher priority as soon as it becomes
 *  available.
 *
 * Must be called while locked.
 *
 * @return True if the logfile name changed.
 */
int openLogFile(struct tm *nowTM, TCHAR *message) {
    static TCHAR confLogFileStopDateStr[20];
    int logFileChanged = FALSE;
    TCHAR nowDate[9];
    int old_umask;
    const TCHAR *tempBufferFormat;
    TCHAR tempBufferLastErrorText1[1024];
    TCHAR tempBufferLastErrorText2[1024];
    size_t tempBufferLen;
    TCHAR *tempBuffer;
    TCHAR tempConfLogFileResumeDateStr[20];
    size_t reqSize = 0;
    char *messageMB;
    size_t messageMBMaxLen = 0;
    FILE *tempLogfileFP = NULL;
    int dummyReset = FALSE;
    int reset = FALSE;
#if defined(WIN32) && !defined(WIN64)
    struct _stat64i32 fileStat;
#else
    struct stat fileStat;
#endif
    
    /* If the log file was set to a blank value then it will not be used. */
    if (logFilePath && (_tcslen(logFilePath) > 0)) {
        /* If this the roll mode is date then we need a nowDate for this log entry. */
        if ((logFileRollMode & ROLL_MODE_DATE) || (confLogFileRollMode & ROLL_MODE_DATE)) {
            _sntprintf(nowDate, 9, TEXT("%04d%02d%02d"), nowTM->tm_year + 1900, nowTM->tm_mon + 1, nowTM->tm_mday );
        } else {
            nowDate[0] = TEXT('\0');
        }

        /* If ftell() can't be used, we need the size of the logging message in order to calculate the size of the buffered data that is not flushed.  */
        if (doesFtellCauseMemoryLeak()) {
            /* We will not use ftell(), so we have to take into account the size of the buffered data that have not been flushed yet. */
            GetColumnsAndReqSizeForPrintBuffer(logfileFormat, message, &reqSize);
            /* The previous function will process the length of message in number of characters. We want it in bytes. */
            messageMBMaxLen = _tcslen(message) * sizeof(TCHAR);
            if (messageMBMaxLen > 0) {
                messageMB = malloc(messageMBMaxLen);
                if (!messageMB) {
                    outOfMemoryQueued(TEXT("OLF"), 1);
                } else {
                    wcstombs(messageMB, message, messageMBMaxLen);
                    
                    reqSize -= _tcslen(message);
                    reqSize += strlen(messageMB);
                    /* Actually GetColumnsAndReqSizeForPrintBuffer() return 3 characters more than needed, but we need to add 2 more caracters for carriage return. */
                    reqSize -= 1;
                }
                free(messageMB);
            }
        }
        /* Make sure that the log file does not need to be rolled. */
        checkAndRollLogs(nowDate, reqSize);
        
        if (confLogFileName) {
            /* The logging configuration is now loaded and the configured log file is known. */
            workConfLogFileName[0] = TEXT('\0');

            /* Check if the log file path has changed since the last call. */
            if (logFilePathChanged && (whichLogFile != LOG_FILE_UNSET)) {
                if (confLogFilePathChanged) {
                    if (whichLogFile == LOG_FILE_CONFIGURED) {
                        /* We are moving from one configured log file to another. We need to reinit. */
                        reset = TRUE;
                    } else {
                        if (confLogFileRollMode & ROLL_MODE_DATE) {
                            generateLogFileName(workConfLogFileName, confLogFileNameSize, confLogFileName, nowDate, NULL);
                        } else {
                            generateLogFileName(workConfLogFileName, confLogFileNameSize, confLogFileName, NULL, NULL);
                        }
                        if ((tempLogfileFP = _tfopen(workConfLogFileName, TEXT("a"))) == NULL) {
                        /* The configured log file has changed but is not accessible. Reset the file opening
                         *  system in its original state to clearly show that the new file is not accessible. */
                            dummyReset = TRUE;
                        }
                    }
                    
                    if (reset || dummyReset) {
                        /* Now actually reset the file opening system */
                        if (logfileFP != NULL) {
                            /* Close the previous log file. We can do this safely because we are already locked. */
                            fclose(logfileFP);
                            logfileFP = NULL;
                        }
                        if (whichLogFile == LOG_FILE_DISABLED) {
                            /* We previously disabled file logging. Reactivate it. */
                            enableLogFile();
                        }
                        whichLogFile = LOG_FILE_UNSET;
                    }
                }
                
                if (whichLogFile == LOG_FILE_DEFAULT) {
                    /* Continue with the default file. If the configured log file changed, we will resume logging in it just below. */
                    setLogfilePath(defaultLogFile, FALSE);
                    _sntprintf(currentLogFileName, currentLogFileNameSize, defaultLogFile);
                    setDefaultRolling();
                }
            }
            
            /* If we previously fell back to the default log file or disabled log file, try to return to the configured log file */
            if ((whichLogFile != LOG_FILE_UNSET) && (whichLogFile != LOG_FILE_CONFIGURED) && (confLogFileName != NULL)) {
                /* Check if the configured log file is available */
                old_umask = umask( logFileUmask );
                /* Generate the log file name if it is not already set. */
                if (workConfLogFileName[0] == TEXT('\0')) {
                    if (confLogFileRollMode & ROLL_MODE_DATE) {
                        generateLogFileName(workConfLogFileName, confLogFileNameSize, confLogFileName, nowDate, NULL);
                    } else {
                        generateLogFileName(workConfLogFileName, confLogFileNameSize, confLogFileName, NULL, NULL);
                    }
                }
                tempLogfileFP = _tfopen(workConfLogFileName, TEXT("a"));
                if (!tempLogfileFP) {
                    _tcsncpy(tempBufferLastErrorText1, getLastErrorText(), 1023);
                    tempBufferLastErrorText1[1023] = 0;
                } else {
                    if (logfileFP != NULL) {
                        /* Make sure to close the default log file (we have not set logfileFP yet). */
                        /* We are already locked. */
                        fclose(logfileFP);
                        logfileFP = NULL;
                    }
                    if (whichLogFile == LOG_FILE_DISABLED) {
                        /* We previously disabled file logging. Reactivate it. */
                        enableLogFile();
                    } 
                    
                    /* We need to write our message into a buffer manually so we can use it
                     *  both for the log_printf_queue and log_printf_message_sysLog calls below. */
                    tempBufferFormat = TEXT("The messages could not be logged in the configured log file (%s) between %s and %s.\n  The missing log entries may be found in the default log file (%s) of the current working directory or in the %s.");
                    _sntprintf(tempConfLogFileResumeDateStr, 20, TEXT("%04d/%02d/%02d %02d:%02d:%02d"),
                                    nowTM->tm_year + 1900, nowTM->tm_mon + 1, nowTM->tm_mday,
                                    nowTM->tm_hour, nowTM->tm_min, nowTM->tm_sec );
                    tempConfLogFileResumeDateStr[19] = 0;
                    tempBufferLen = _tcslen(tempBufferFormat) - 2 - 2 - 2 - 2 - 2 + _tcslen(workConfLogFileName) + _tcslen(confLogFileStopDateStr) + _tcslen(tempConfLogFileResumeDateStr) + _tcslen(defaultLogFile) + _tcslen(syslogName) + 1;
                    tempBuffer = malloc(sizeof(TCHAR) * tempBufferLen);
                    if (!tempBuffer) {
                        outOfMemoryQueued(TEXT("OLF"), 2);
                    } else {
                        _sntprintf(tempBuffer, tempBufferLen, tempBufferFormat, workConfLogFileName, confLogFileStopDateStr, tempConfLogFileResumeDateStr, defaultLogFile, syslogName);
                        
                        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT("%s"), tempBuffer);
                        
                        /* This is critical for debugging problems.  If the above message would not have shown
                         *  up in the syslog then send it there manually.  We are already locked here so this
                         *  can be done safely. */
                        log_printf_message_sysLog(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, tempBuffer, nowTM, TRUE);
                        
                        free(tempBuffer);
                    }
                    
                    /* Set the log file path */
                    setLogfilePath(confLogFileName, FALSE);
                    _sntprintf(currentLogFileName, currentLogFileNameSize, workConfLogFileName);
                    restoreConfiguredRolling();
                    logFileChanged = TRUE;
                    whichLogFile = LOG_FILE_CONFIGURED;
                    logfileFP = tempLogfileFP;
                }
                umask(old_umask);
            }
        }

        /* If the file needs to be opened then do so. */
        if (logfileFP == NULL) {
            old_umask = umask( logFileUmask );
            /* Generate the log file name if it is not already set. */
            if (whichLogFile != LOG_FILE_DISABLED) {
                if (currentLogFileName[0] == TEXT('\0')) {
                    if (logFileRollMode & ROLL_MODE_DATE) {
                        generateLogFileName(currentLogFileName, currentLogFileNameSize, logFilePath, nowDate, NULL);
                    } else {
                        generateLogFileName(currentLogFileName, currentLogFileNameSize, logFilePath, NULL, NULL);
                    }
                    logFileChanged = TRUE;
                }

                logfileFP = _tfopen(currentLogFileName, TEXT("a"));
                if (!logfileFP) {
                    if (whichLogFile == LOG_FILE_DEFAULT) {
                        _tcsncpy(tempBufferLastErrorText2, getLastErrorText(), 1023);
                        tempBufferLastErrorText2[1023] = 0;
                    } else {
                        _tcsncpy(tempBufferLastErrorText1, getLastErrorText(), 1023);
                        tempBufferLastErrorText1[1023] = 0;
                    }
                }
            }
            if (logfileFP != NULL) {
                if (whichLogFile == LOG_FILE_UNSET) {
                    whichLogFile = LOG_FILE_CONFIGURED;
                } else if ((whichLogFile == LOG_FILE_DEFAULT) && (_tstat(logFilePath, &fileStat) == 0) && (fileStat.st_size == 0)) {
                    /* Generate the log file name if it is not already set. */
                    if (workConfLogFileName[0] == TEXT('\0')) {
                        if (confLogFileRollMode & ROLL_MODE_DATE) {
                            generateLogFileName(workConfLogFileName, confLogFileNameSize, confLogFileName, nowDate, NULL);
                        } else {
                            generateLogFileName(workConfLogFileName, confLogFileNameSize, confLogFileName, NULL, NULL);
                        }
                    }
                    /* This is a new default log file, so add a header. */
                    printFailoverFileHeader(workConfLogFileName);
                }
            } else {
                if (whichLogFile != LOG_FILE_DEFAULT) {
                    logfileFP = _tfopen(defaultLogFile, TEXT("a"));
                    if (!logfileFP) {
                        _tcsncpy(tempBufferLastErrorText2, getLastErrorText(), 1023);
                        tempBufferLastErrorText2[1023] = 0;
                    }
                    if (whichLogFile == LOG_FILE_DISABLED) {
                        if (logfileFP != NULL) {
                            enableLogFile();
                        } else {
                            /* The logging was disabled and both the default file and the configured file could not be opened. */
                            confLogFilePathChanged = FALSE;
                            logFilePathChanged = FALSE;
                            return FALSE;
                        }
                    } else if (!dummyReset) {
                        /* Save the date in string format so that we don't need to worry about time changes. */
                        _sntprintf(confLogFileStopDateStr, 20, TEXT("%04d/%02d/%02d %02d:%02d:%02d"),
                                        nowTM->tm_year + 1900, nowTM->tm_mon + 1, nowTM->tm_mday,
                                        nowTM->tm_hour, nowTM->tm_min, nowTM->tm_sec );
                        confLogFileStopDateStr[19] = 0;
                    }
                    
                    /* We need to write our error message into a buffer manually so we can use it
                     *  both for the log_printf_queue and log_printf_message_sysLog calls below. */
                    tempBufferFormat = TEXT("Unable to write to the configured log file: %s (%s)\n  Falling back to the default file in the current working directory: %s");
                    tempBufferLen = _tcslen(tempBufferFormat) - 2 - 2 - 2 + _tcslen(currentLogFileName) + _tcslen(tempBufferLastErrorText1) + _tcslen(defaultLogFile) + 1;
                    tempBuffer = malloc(sizeof(TCHAR) * tempBufferLen);
                    if (!tempBuffer) {
                        outOfMemoryQueued(TEXT("OLF"), 3);
                    } else {
                        _sntprintf(tempBuffer, tempBufferLen, tempBufferFormat, currentLogFileName, tempBufferLastErrorText1, defaultLogFile);
                        
                        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT("%s"), tempBuffer);
                        
                        /* This is critical for debugging problems.  If the above message would not have shown
                         *  up in the syslog then send it there manually.  We are already locked here so this
                         *  can be done safely. */
                        log_printf_message_sysLog(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, tempBuffer, nowTM, TRUE);
                        
                        free(tempBuffer);
                    }
                    
                    setLogfilePath(defaultLogFile, FALSE);
                    _sntprintf(currentLogFileName, currentLogFileNameSize, defaultLogFile);
                    setDefaultRolling();
                    logFileChanged = TRUE;
                }
                if (logfileFP != NULL) {
                    whichLogFile = LOG_FILE_DEFAULT;
                    if ((_tstat(logFilePath, &fileStat) == 0) && (fileStat.st_size == 0)) {
                        /* Generate the log file name if it is not already set. */
                        if (workConfLogFileName[0] == TEXT('\0')) {
                            if (confLogFileRollMode & ROLL_MODE_DATE) {
                                generateLogFileName(workConfLogFileName, confLogFileNameSize, confLogFileName, nowDate, NULL);
                            } else {
                                generateLogFileName(workConfLogFileName, confLogFileNameSize, confLogFileName, NULL, NULL);
                            }
                        }
                        /* This is a new default log file, so add a header. */
                        printFailoverFileHeader(workConfLogFileName);
                    }
                } else {
                    /* Still unable to write. */
                    
                    /* We need to write our error message into a buffer manually so we can use it
                     *  both for the log_printf_queue and log_printf_message_sysLog calls below. */
                    tempBufferFormat = TEXT("Unable to write to the default log file: %s (%s)\n  Disabling log file.");
                    tempBufferLen = _tcslen(tempBufferFormat) - 2 - 2 + _tcslen(currentLogFileName) + _tcslen(tempBufferLastErrorText2) + 1;
                    tempBuffer = malloc(sizeof(TCHAR) * tempBufferLen);
                    if (!tempBuffer) {
                        outOfMemoryQueued(TEXT("OLF"), 4);
                    } else {
                        _sntprintf(tempBuffer, tempBufferLen, tempBufferFormat, currentLogFileName, tempBufferLastErrorText2);
                        
                        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT("%s"), tempBuffer);
                        
                        /* This is critical for debugging problems.  If the above message would not have shown
                         *  up in the syslog then send it there manually.  We are already locked here so this
                         *  can be done safely. */
                        log_printf_message_sysLog(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, tempBuffer, nowTM, TRUE);
                        
                        free(tempBuffer);
                    }
                    disableLogFile();
                    logFileChanged = FALSE;
                }
            }
            umask(old_umask);

#ifdef _DEBUG
            if (logfileFP != NULL) {
                _tprintf(TEXT("Opened logfile\n"));
            }
#endif
        }

        if (logfileFP == NULL) {
            currentLogFileName[0] = TEXT('\0');
            /* Failure to write to logfile already reported. */
        } else if ((whichLogFile == LOG_FILE_CONFIGURED) && (logFileRollMode & ROLL_MODE_DATE)) {
            /* We need to store the date the file was opened for (only the configured log file is rolled by date). */
            _tcsncpy(logFileLastNowDate, nowDate, 9);
        }
    }
    
    confLogFilePathChanged = FALSE;
    logFilePathChanged = FALSE;
    return logFileChanged;
}

/**
 * Prints the contents of a buffer to the logfile target.  The log level is
 *  tested prior to this function being called.
 *
 * Must be called while locked.
 */
void log_printf_message_logFileInner(int source_id, int level, int threadId, int queued, TCHAR *message, struct tm *nowTM, int nowMillis, time_t durationMillis) {
    TCHAR *printBuffer;

    if (logfileFP != NULL) {
        /* Build up the printBuffer. */
        printBuffer = buildPrintBuffer(source_id, level, threadId, queued, nowTM, nowMillis, durationMillis, logfileFormat, LOG_FORMAT_LOGFILE_DEFAULT, message);
        if (printBuffer) {
            _ftprintf(logfileFP, TEXT("%s\n"), printBuffer);
            logFileAccessed = TRUE;

            /* Increment the activity counter. */
            logfileActivityCount++;

            /* Decide whether we want to close or flush the log file immediately after each line.
             *  If not then flushing and closing will be handled externally by calling flushLogfile() or closeLogfile(). */
            if (autoCloseLogfile) {
                /* Close the log file immediately. */
#ifdef _DEBUG
                _tprintf(TEXT("Closing logfile immediately...\n"));
#endif

                fclose(logfileFP);
                logfileFP = NULL;
                /* Do not clear the currentLogFileName here as we are not changing its name. */
            } else if (autoFlushLogfile) {
                /* Flush the log file immediately. */
#ifdef _DEBUG
                _tprintf(TEXT("Flushing logfile immediately...\n"));
#endif
                
                fflush(logfileFP);
            }

            /* Leave the file open.  It will be closed later after a period of inactivity. */
        }
    }
}
int log_printf_message_logFile(int source_id, int level, int threadId, int queued, TCHAR *message, struct tm *nowTM, int nowMillis, time_t durationMillis) {
    int logFileChanged = FALSE;

    if ((level >= currentLogfileLevel) || (whichLogFile == LOG_FILE_DISABLED)) {
        logFileChanged = openLogFile(nowTM, message);
        if (level >= currentLogfileLevel) {
            log_printf_message_logFileInner(source_id, level, threadId, queued, message, nowTM, nowMillis, durationMillis);
        }
    }
    
    return logFileChanged;
}

/**
 * Prints the contents of a buffer to the console target.  The log level is
 *  tested prior to this function being called.
 *
 * Must be called while locked.
 */
void log_printf_message_consoleInner(int source_id, int level, int threadId, int queued, TCHAR *message, struct tm *nowTM, int nowMillis, time_t durationMillis) {
    TCHAR *printBuffer;
    FILE *target;
#ifdef WIN32
    HANDLE targetH;
    int complete = FALSE;
#endif
    
    /* Build up the printBuffer. */
    printBuffer = buildPrintBuffer(source_id, level, threadId, queued, nowTM, nowMillis, durationMillis, consoleFormat, LOG_FORMAT_CONSOLE_DEFAULT, message);
    if (printBuffer) {
        /* Decide where to send the output. */
        switch (level) {
            case LEVEL_FATAL:
                if (consoleFatalToStdErr) {
                    target = stderr;
                } else {
                    target = stdout;
                }
                break;
                
            case LEVEL_ERROR:
                if (consoleErrorToStdErr) {
                    target = stderr;
                } else {
                    target = stdout;
                }
                break;
                
            case LEVEL_WARN:
                if (consoleWarnToStdErr) {
                    target = stderr;
                } else {
                    target = stdout;
                }
                break;
                
            default:
                target = stdout;
                break;
        }

        /* Write the print buffer to the console. */
#ifdef WIN32
        /* Using the WinAPI function WriteConsole would make it impossible to pipe the console output.
         *  We never want to allow direct console writing if this is a launcher instance.*/
        if (consoleDirect) {
            if (target == stderr) {
                targetH = GetStdHandle(STD_ERROR_HANDLE);
            } else {
                targetH = GetStdHandle(STD_OUTPUT_HANDLE);
            }
            if (targetH != NULL) {
                complete = writeToConsole(targetH, TEXT("%s\n"), printBuffer);
            } else {
                /* Should not happen.  But just in case. */
                _tprintf(TEXT("Failed to find standard handle.  Disabled direct console output.\n"));
                consoleDirect = FALSE;
            }
        }
        
        if (!complete) {
#endif
            _ftprintf(target, TEXT("%s\n"), printBuffer);
            if (consoleFlush) {
                fflush(target);
            }
#ifdef WIN32
        }
#endif
    }
}
void log_printf_message_console(int source_id, int level, int threadId, int queued, TCHAR *message, struct tm *nowTM, int nowMillis, time_t durationMillis) {
    if (level >= currentConsoleLevel) {
        log_printf_message_consoleInner(source_id, level, threadId, queued, message, nowTM, nowMillis, durationMillis);
    }
}


/**
 * Prints the contents of a buffer to all configured targets.
 *
 * Must be called while locked.
 *
 * @param sysLogEnabled A flag that is used to help with recursion to control
 *                      whether or not the syslog should be considered as a log
 *                      target for this call.  It is always disabled when we
 *                      recurse.
 *
 * @return True if the logfile name changed.
 */
int log_printf_message(int source_id, int level, int threadId, int queued, TCHAR *message, int sysLogEnabled) {
#ifndef WIN32
    TCHAR       *printBuffer;
    FILE        *target;
#endif
    int         logFileChanged = FALSE;
    TCHAR       *subMessage;
    TCHAR       *nextLF;
#ifdef WIN32
    struct _timeb timebNow;
#else
    size_t      reqSize;
    struct timeval timevalNow;
    TCHAR       intBuffer[3];
    TCHAR*      pos;
#endif
    time_t      now;
    int         nowMillis;
    struct tm   *nowTM;
    time_t      durationMillis;
    
#ifndef WIN32
    if ((_tcsstr(message, LOG_SPECIAL_MARKER) == message) && (_tcslen(message) >= _tcslen(LOG_SPECIAL_MARKER) + 10)) {
        /* Got a special encoded log message from the child Wrapper process.
         *  Parse it and continue as if the log message came from this process.
         * These should never be multi-line messages as the forked child
         *  process will have already broken them up. */
        pos = (TCHAR *)(message + _tcslen(LOG_SPECIAL_MARKER) + 1);

        /* source_id */
        _tcsncpy(intBuffer, pos, 2);
        intBuffer[2] = TEXT('\0');
        source_id = _ttoi(intBuffer);
        pos += 3;

        /* level */
        _tcsncpy(intBuffer, pos, 2);
        intBuffer[2] = TEXT('\0');
        level = _ttoi(intBuffer);
        pos += 3;

        /* threadId */
        _tcsncpy(intBuffer, pos, 2);
        intBuffer[2] = TEXT('\0');
        threadId = _ttoi(intBuffer);
        pos += 3;

        /* message */
        message = pos;
    }
#endif
    
    /* Build a timestamp */
#ifdef WIN32
    _ftime( &timebNow );
    now = (time_t)timebNow.time;
    nowMillis = timebNow.millitm;
#else
    gettimeofday( &timevalNow, NULL );
    now = (time_t)timevalNow.tv_sec;
    nowMillis = timevalNow.tv_usec / 1000;
#endif
    nowTM = localtime( &now );
    
    /* Calculate the number of milliseconds which have passed since the previous log entry.
     * We only need to display up to 8 digits, so if the result is going to be larger than
     *  that, set it to 100000000.
     * We only want to do this for output coming from the JVM.  Any other log output should
     *  be set to (time_t)-1. */
    switch(source_id) {
    case WRAPPER_SOURCE_WRAPPER:
    case WRAPPER_SOURCE_PROTOCOL:
        durationMillis = (time_t)-1;
        break;
        
    default:
        if (now - previousNow > 100000) {
            /* Without looking at the millis, we know it is already too long. */
            durationMillis = 100000000;
        } else {
            durationMillis = (now - previousNow) * 1000 + nowMillis - previousNowMillis;
        }
        previousNow = now;
        previousNowMillis = nowMillis;
        break;
    }
    
    if (!currentLogSplitMessages) {
        /* Syslog messages are printed first so we can print them including line feeds as is.
         *  This must be done before we break up multi-line messages into individual lines. */
#ifdef WIN32
        if (sysLogEnabled) {
#else
        /* On UNIX we never want to log to the syslog here if this is in a forked thread.
         *  In this case, any lines will be broken up into individual lines and then logged
         *  as usual by the main process.  But this can't be helped and is very rare anyway. */
        if (sysLogEnabled && (_tcsstr(message, LOG_FORK_MARKER) != message)) {
#endif
            /* syslog/Eventlog. */
            log_printf_message_sysLog(source_id, level, message, nowTM, FALSE);
        }
    }

    /* If the message contains line feeds then break up the line into substrings and recurse. */
    subMessage = message;
    nextLF = _tcschr(subMessage, TEXT('\n'));
    if (nextLF) {
        /* This string contains more than one line.   Loop over the strings.  It is Ok to corrupt this string because it is only used once. */
        while (nextLF) {
            nextLF[0] = TEXT('\0');
            logFileChanged |= log_printf_message(source_id, level, threadId, queued, subMessage, FALSE);
            
            /* Locate the next one. */
            subMessage = &(nextLF[1]);
            nextLF = _tcschr(subMessage, TEXT('\n'));
        }
        
        /* The rest of the buffer will be the final line. */
        logFileChanged |= log_printf_message(source_id, level, threadId, queued, subMessage, FALSE);
        
        return logFileChanged;
    }
    
#ifndef WIN32
    /* See if this is a special case log entry from the forked child. */
    if (_tcsstr(message, LOG_FORK_MARKER) == message) {
        /* Found the marker.  We only want to log the message as is to the console with a special prefix.
         *  This is used to pass the log output through the pipe to the parent Wrapper process where it
         *  will be decoded below and displayed appropriately. */
        reqSize = _tcslen(LOG_SPECIAL_MARKER) + 1 + 2 + 1 + 2 + 1 + 2 + 1 + _tcslen(message) - _tcslen(LOG_FORK_MARKER) + 1;
        if (!(printBuffer = preparePrintBuffer(reqSize))) {
            return FALSE;
        }
        _sntprintf(printBuffer, reqSize, TEXT("%s|%02d|%02d|%02d|%s"), LOG_SPECIAL_MARKER, source_id, level, threadId, message + _tcslen(LOG_FORK_MARKER));
        
        /* Decide where to send the output. */
        switch (level) {
        case LEVEL_FATAL:
            if (consoleFatalToStdErr) {
                target = stderr;
            } else {
                target = stdout;
            }
            break;
            
        case LEVEL_ERROR:
            if (consoleErrorToStdErr) {
                target = stderr;
            } else {
                target = stdout;
            }
            break;
            
        case LEVEL_WARN:
            if (consoleWarnToStdErr) {
                target = stderr;
            } else {
                target = stdout;
            }
            break;
            
        default:
            target = stdout;
            break;
        }
        
        _ftprintf(target, TEXT("%s\n"), printBuffer);
        if (consoleFlush) {
            fflush(target);
        }
        return FALSE;
    }
#endif

    /* Get the current threadId. */
    if ( threadId < 0 )
    {
        /* The current thread was specified.  Resolve what thread this actually is. */
        threadId = getThreadId();
    }
    
    /* Syslog outbut by format (If messages splitting is enabled.  Otherwise done above.) */
    if (currentLogSplitMessages) {
        log_printf_message_sysLog(source_id, level, message, nowTM, FALSE);
    }

    /* Console output by format */
    log_printf_message_console(source_id, level, threadId, queued, message, nowTM, nowMillis, durationMillis);

    /* Logfile output by format */
    logFileChanged = log_printf_message_logFile(source_id, level, threadId, queued, message, nowTM, nowMillis, durationMillis);

    return logFileChanged;
}

/**
 * Used for testing to pause the current thread for the specified number of seconds.
 *  This can only be called when logging is locked.
 *
 * @param pauseTime Number of seconds to pause, 0 pauses indefinitely.
 */
void pauseThread(int pauseTime) {
    int i;
    
    if (pauseTime > 0) {
        for (i = 0; i < pauseTime; i++) {
            logSleep(1000);
        }
    } else {
        while (TRUE) {
            logSleep(1000);
        }
    }
}

/* General log functions */
void log_printf( int source_id, int level, const TCHAR *lpszFmt, ... ) {
    va_list     vargs;
    int         count;
    int         threadId;
    int         logFileChanged;
    TCHAR       *logFileCopy;
#if defined(UNICODE) && !defined(WIN32)
    TCHAR       *msg = NULL;
    int         i;
    int         flag;
#endif
#ifdef WIN32
    struct _timeb timebNow;
#else
    struct timeval timevalNow;
#endif
    time_t      startNow;
    int         startNowMillis;
    time_t      endNow;
    int         endNowMillis;
    
    if (level == LEVEL_NONE) {
        /* Some APIs allow the user to potentially configure the NONE log level. Skip it as it means no logging in this case. */
        return;
    }
    
    /* If we are checking on the log time then store the start time. */
    if (logPrintfWarnThreshold > 0) {
#ifdef WIN32
        _ftime(&timebNow);
        startNow = (time_t)timebNow.time;
        startNowMillis = timebNow.millitm;
#else
        gettimeofday(&timevalNow, NULL);
        startNow = (time_t)timevalNow.tv_sec;
        startNowMillis = timevalNow.tv_usec / 1000;
#endif
    } else {
        startNow = 0;
        startNowMillis = 0;
    }

    /* We need to be very careful that only one thread is allowed in here
     *  at a time.  On Windows this is done using a Mutex object that is
     *  initialized in the initLogging function. */
    if (lockLoggingMutex()) {
        return;
    }
    
    /* If there is a queued pause then do so. */
    if ((logPauseTime >= 0) && (level > LEVEL_DEBUG) && (source_id < 0)) {
        pauseThread(logPauseTime);
        logPauseTime = -1;
    }
    
#if defined(UNICODE) && !defined(WIN32)
    if (source_id < 1 && wcsstr(lpszFmt, TEXT("%s")) != NULL) {
        msg = malloc(sizeof(wchar_t) * (wcslen(lpszFmt) + 1));
        if (msg) {
            /* Loop over the format and convert all '%s' patterns to %S' so the UNICODE displays correctly. */
            if (wcslen(lpszFmt) > 0) {
                for (i = 0; i < _tcslen(lpszFmt); i++){
                    msg[i] = lpszFmt[i];
                    if ((lpszFmt[i] == TEXT('%')) && (i  < _tcslen(lpszFmt)) && (lpszFmt[i + 1] == TEXT('s')) && ((i == 0) || (lpszFmt[i - 1] != TEXT('%')))){
                        msg[i+1] = TEXT('S'); i++;
                    }
                }
            }
            msg[wcslen(lpszFmt)] = TEXT('\0');
        } else {
            _tprintf(TEXT("Out of memory in logging code (%s)\n"), TEXT("P1"));
            return;
        }
        flag = TRUE;
    } else {
        msg = (TCHAR*) lpszFmt;
        flag = FALSE;
    }
#endif
    threadId = getThreadId();
    if (source_id <= 0) {
        /* Loop until the buffer is large enough that we are able to successfully
         *  print into it. Once the buffer has grown to the largest message size,
         *  smaller messages will pass through this code without looping. */
        do {
            if ( threadMessageBufferSize == 0 )
            {
                /* No buffer yet. Allocate one to get started. */
                threadMessageBufferSize = threadMessageBufferInitialSize;
#if defined(HPUX)
                /* Due to a bug in the HPUX libc (version < 1403), the buffer passed to _vsntprintf must have a length of 1 + N, where N is a multiple of 8.  Adjust it as necessary. */
                threadMessageBufferSize = threadMessageBufferSize + (((threadMessageBufferSize - 1) % 8) == 0 ? 0 : 8 - ((threadMessageBufferSize - 1) % 8));
#endif
                threadMessageBuffer = malloc(sizeof(TCHAR) * threadMessageBufferSize);
                if (!threadMessageBuffer) {
                    _tprintf(TEXT("Out of memory in logging code (%s)\n"), TEXT("P2"));
                    threadMessageBufferSize = 0;
#if defined(UNICODE) && !defined(WIN32)
                    if (flag == TRUE) {
                        free(msg);
                    }
#endif
                    return;
                }
            }
            /* Try writing to the buffer. */
            va_start( vargs, lpszFmt );
#if defined(UNICODE) && !defined(WIN32)
            count = _vsntprintf( threadMessageBuffer, threadMessageBufferSize, msg, vargs );
#else
            count = _vsntprintf( threadMessageBuffer, threadMessageBufferSize, lpszFmt, vargs );
#endif
            va_end( vargs );
            /*
            _tprintf(TEXT(" vsnprintf->%d, size=%d\n"), count, threadMessageBufferSize );
            */
            if ( ( count < 0 ) || ( count >= (int)threadMessageBufferSize ) ) {
                /* If the count is exactly equal to the buffer size then a null TCHAR was not written.
                 *  It must be larger.
                 * Windows will return -1 if the buffer is too small. If the number is
                 *  exact however, we still need to expand it to have room for the null.
                 * UNIX will return the required size. */

                /* Free the old buffer for starters. */
                free( threadMessageBuffer );

                /* Decide on a new buffer size.
                 * We can't tell how long the resulting string will be without expanding because the
                 *  results are stored in the vargs.
                 * Most messages will be short, but there is a possibility that some will be very
                 *  long.  To minimize the number of times that we need to loop, while at the same
                 *  time trying to avoid using too much memory, increase the size by the maximum of
                 *  1024 or 10% of the current length.
                 * Some platforms will return the required size as count.  Use that if available. */
                threadMessageBufferSize = __max(threadMessageBufferSize + 1024, __max(threadMessageBufferSize + threadMessageBufferSize / 10, (size_t)count + 1));
#if defined(HPUX)
                /* Due to a bug in the HPUX libc (version < 1403), the buffer passed to _vsntprintf must have a length of 1 + N, where N is a multiple of 8.  Adjust it as necessary. */
                threadMessageBufferSize = threadMessageBufferSize + (((threadMessageBufferSize - 1) % 8) == 0 ? 0 : 8 - ((threadMessageBufferSize - 1) % 8));
#endif

                threadMessageBuffer = malloc(sizeof(TCHAR) * threadMessageBufferSize);
                if (!threadMessageBuffer) {
                    _tprintf(TEXT("Out of memory in logging code (%s)\n"), TEXT("P3"));
                    threadMessageBufferSize = 0;
#if defined(UNICODE) && !defined(WIN32)
                    if (flag == TRUE) {
                        free(msg);
                    }
#endif
                    return;
                }

                /* Always set the count to -1 so we will loop again. */
                count = -1;
            }
        } while ( count < 0 );
    }
#if defined(UNICODE) && !defined(WIN32)
    if (flag == TRUE) {
        free(msg);
    }
#endif
    logFileCopy = NULL;
    if (source_id > 0) {
#if defined(UNICODE) && !defined(WIN32)
        logFileChanged = log_printf_message(source_id, level, threadId, FALSE, msg, TRUE);
#else
        logFileChanged = log_printf_message(source_id, level, threadId, FALSE, (TCHAR*) lpszFmt, TRUE);
#endif
    } else {
        logFileChanged = log_printf_message(source_id, level, threadId, FALSE, threadMessageBuffer, TRUE);
    }
    if (logFileChanged) {
        /* We need to enqueue a notification that the log file name was changed.
         *  We can NOT directly send the notification here as that could cause a deadlock,
         *  depending on where exactly this function was called from. (See Wrapper protocol mutex.) */
        logFileCopy = malloc(sizeof(TCHAR) * (_tcslen(currentLogFileName) + 1));
        if (!logFileCopy) {
            _tprintf(TEXT("Out of memory in logging code (%s)\n"), TEXT("P4"));
        } else {
            _tcsncpy(logFileCopy, currentLogFileName, _tcslen(currentLogFileName) + 1);
            /* Now after we have 100% prepared the log file name.  Put into the queue variable
             *  so the maintainLogging() function can safely grab it at any time.
             * The reading code is also in a semaphore so we can do a quick test here safely as well. */
            if (pendingLogFileChange) {
                /* The previous file was still in the queue.  Free it up to avoid a memory leak.
                 *  This can happen if the log file size is 1k or something like that.  We will always
                 *  keep the most recent file however, so this should not be that big a problem. */
#ifdef _DEBUG
                _tprintf(TEXT("Log file name change was overwritten in queue: %s\n"), pendingLogFileChange);
#endif
                free(pendingLogFileChange);
            }
            pendingLogFileChange = logFileCopy;
        }
    }

    /* Release the lock we have on this function so that other threads can get in. */
    if (releaseLoggingMutex()) {
        return;
    }

    /* If we are checking on the log time then store the stop time.
     *  It is Ok that some of the error paths don't make it this far. */
    if (logPrintfWarnThreshold > 0) {
#ifdef WIN32
        _ftime(&timebNow);
        endNow = (time_t)timebNow.time;
        endNowMillis = timebNow.millitm;
#else
        gettimeofday(&timevalNow, NULL);
        endNow = (time_t)timevalNow.tv_sec;
        endNowMillis = timevalNow.tv_usec / 1000;
#endif
        previousLogLag = __min(endNow - startNow, 3600) * 1000 + endNowMillis - startNowMillis;
        if (previousLogLag >= logPrintfWarnThreshold) {
            log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT("Write to log took %d milliseconds."), previousLogLag);
        }
    }
}

/* Internal functions */
#ifdef WIN32
static int sysLangId = LANG_NEUTRAL;

/**
 * Sets the language id to use to format system messages.
 *  This function should be called when resolving the locale.
 *
 * @param id Language Identifier (16-bit value that consists of a 
 *           primary language identifier and a sublanguage identifier)
 */
void setLogSysLangId(int id) {
    sysLangId = id;
}
#endif

#define LAST_ERROR_TEXT_BUFFER_SIZE 1024
/** Buffer holding the last error message.
 *  TODO: This needs to be made thread safe, meaning that we need a buffer for each thread. */
TCHAR lastErrorTextBufferW[LAST_ERROR_TEXT_BUFFER_SIZE];

/**
 * Returns a textual error message of a given error number.
 *
 * @param errorNum Error code.
 * @param handle (for Windows only) A module handle containing the message-table resource(s) to search. If NULL, the current process's application image file will be searched.
 * 
 * @return The error message.
 */
const TCHAR* getErrorText(int errorNum, void* handle) {
#ifdef WIN32
    DWORD dwRet;
    TCHAR* lpszTemp = NULL;
    DWORD   dwFlags;
    int formatError;
#else
    char* lastErrorTextMB;
    size_t req;
#endif

#ifdef WIN32
    if (handle) {
        dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_FROM_HMODULE;
    } else {
        dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY;
    }

    dwRet = FormatMessage(dwFlags,
                          handle,
                          errorNum,
                          sysLangId,
                          (TCHAR*)&lpszTemp,
                          0,
                          NULL);

    if (!dwRet) {
        /* There was an error calling FormatMessage. */
        
        formatError = getLastError();
        if ((formatError == ERROR_MUI_FILE_NOT_FOUND) ||
            (formatError == ERROR_MUI_INVALID_FILE) ||
            (formatError == ERROR_MUI_INVALID_RC_CONFIG) ||
            (formatError == ERROR_MUI_INVALID_LOCALE_NAME) ||
            (formatError == ERROR_MUI_INVALID_ULTIMATEFALLBACK_NAME) ||
            (formatError == ERROR_MUI_FILE_NOT_LOADED) ||
            (formatError == ERROR_RESOURCE_LANG_NOT_FOUND)) {
            /* fall back to the system language. Do it once cause probably other messages will not be found either. */
            setLogSysLangId(LANG_NEUTRAL);
            return getErrorText(errorNum, handle);
        }
        
        _sntprintf(lastErrorTextBufferW, LAST_ERROR_TEXT_BUFFER_SIZE, TEXT("Failed to format system error message (Error: %d) (Original Error: 0x%x)"), formatError, errorNum);
    } else if ((long)LAST_ERROR_TEXT_BUFFER_SIZE - 1 < (long)dwRet + 14) {
        /* supplied buffer is not long enough (14 is for the length of the error code in hexadecimal notation (12)+ space + null termination character) */
        _sntprintf(lastErrorTextBufferW, LAST_ERROR_TEXT_BUFFER_SIZE, TEXT("System error message is too large to convert (Required size: %d) (Original Error: 0x%x)"), dwRet, errorNum);
    } else {
        lpszTemp[lstrlen(lpszTemp)-2] = TEXT('\0');  /*remove cr and newline character */
        _sntprintf(lastErrorTextBufferW, LAST_ERROR_TEXT_BUFFER_SIZE, TEXT("%s (0x%x)"), lpszTemp, errorNum);
    }

    /* following the documentation of FormatMessage, LocalFree should be called to free the output buffer. */
    if (lpszTemp) {
        LocalFree(lpszTemp);
    }

#else
    lastErrorTextMB = strerror(errorNum);
    req = mbstowcs(NULL, lastErrorTextMB, MBSTOWCS_QUERY_LENGTH);
    if (req == (size_t)-1) {
        invalidMultiByteSequence(TEXT("GLET"), 1);
        _sntprintf(lastErrorTextBufferW, LAST_ERROR_TEXT_BUFFER_SIZE, TEXT("System error message could not be decoded (Error 0x%x)"), errorNum);
    } else if (req >= LAST_ERROR_TEXT_BUFFER_SIZE) {
        _sntprintf(lastErrorTextBufferW, LAST_ERROR_TEXT_BUFFER_SIZE, TEXT("System error message too large to convert (Require size: %d) (Original Error: 0x%x)"), req, errorNum);
    } else {
        mbstowcs(lastErrorTextBufferW, lastErrorTextMB, LAST_ERROR_TEXT_BUFFER_SIZE);
    }
#endif
    /* Always reterminate the buffer just to be sure it is safe because badly encoded characters can cause issues. */
    lastErrorTextBufferW[LAST_ERROR_TEXT_BUFFER_SIZE - 1] = TEXT('\0');

    return lastErrorTextBufferW;
}

/**
 * Returns a textual error message of the last error encountered.
 *
 * @return The last error message.
 */
const TCHAR* getLastErrorText() {
    return getErrorText(getLastError(), NULL);
}

/**
 * Returns the last error number.
 *
 * @return The last error number.
 */
int getLastError() {
#ifdef WIN32
    return GetLastError();
#else
    return errno;
#endif
}

#ifdef WIN32
static int eventLogSourceInstalled = FALSE;

/**
 * Disable Event Log.
 *  This function has to be called if the registration could not be completed.
 */
void disableSysLog(int silent) {
    if (getSyslogLevelInt() != LEVEL_NONE) {
        setSyslogLevelInt(LEVEL_NONE);
        if (!silent) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, 
                TEXT("Disabling Event Log because the application is not registered.\n  Run the wrapper with the '--setup' option to register."));
        }
    }
}

/**
 * Check if the Event Log source was registered successfully.
 *  There are 2 ways to register: 
 *  CASE 1: If we need elevated privileges, it should be done through the setup process (see --setup argument).
 *  CASE 2: On older versions of windows where running elevated was not needed, registerSyslogMessageFile() can be called when the wrapper runs.
 * 
 * Returns TRUE if registered, FALSE if not registered.
 */
int syslogMessageFileRegistered(int silent) {
    static int checked = FALSE;
    TCHAR bufferPath[_MAX_PATH];
    TCHAR bufferKVal[_MAX_PATH];
    TCHAR regPath[1024];
    DWORD cbData = _MAX_PATH;
    DWORD usedLen;
    DWORD error;
    HKEY hKey;
    
    /* first check if the function was called before */
    if (checked || eventLogSourceInstalled) {  /* || eventLogSourceInstalled: just in case we would have run registerSyslogMessageFile(TRUE) before this function (should not happen) */
        return eventLogSourceInstalled;
    }

    /* if the registry key exist, and the path to the wrapper is ok, lets assume the registration was done successfully. No need to check the other values of the key */
    
    /* Get absolute path to service manager */
    /* Important : For win XP getLastError() is unchanged if the buffer is too small, so if we don't reset the last error first, we may actually test an old pending error. */
    SetLastError(ERROR_SUCCESS);
    usedLen = GetModuleFileName(NULL, bufferPath, _MAX_PATH);
    if (usedLen == 0) {
        if (!silent) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, 
                TEXT("Unable to obtain the full path to the Wrapper. %s"), getLastErrorText());
        }
    } else if ((usedLen == _MAX_PATH) || (getLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        if (!silent) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, 
                TEXT("Unable to obtain the full path to the Wrapper. Path to Wrapper binary too long."));
        }
    } else {
        _sntprintf( regPath, 1024, TEXT("SYSTEM\\CurrentControlSet\\Services\\Eventlog\\Application\\%s"), loginfoSourceName );
        
        /* check that the registry key exists. */
        if ((error = RegOpenKeyEx( HKEY_LOCAL_MACHINE, regPath, 0, KEY_READ, (PHKEY) &hKey )) == ERROR_SUCCESS ) {
            /* check that the path to the wrapper is correct. */
            if ((error = RegQueryValueEx( hKey, TEXT("EventMessageFile"), NULL, NULL, (LPBYTE) bufferKVal, &cbData)) == ERROR_SUCCESS ) {
                if (strcmpIgnoreCase(bufferPath, bufferKVal) == 0) {
                    RegCloseKey( hKey );
                    eventLogSourceInstalled = TRUE;
                } else {
                    if (!silent) {
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, 
                            TEXT("The path registered for the Event Log (%s) did not match the location of the Wrapper binary (%s)."), bufferKVal, bufferPath);
                    }
                }
            } else {
                if (!silent) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, 
                        TEXT("The path registered for the Event Log could not be read (0x%x)."), error);
                }
            }
            RegCloseKey( hKey );
        } else if (error != ERROR_FILE_NOT_FOUND) {
            if (!silent) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, 
                    TEXT("The Event Log source could not be found (0x%x)."), error);
            }
        }
    }
    
    checked = TRUE;
    
    return eventLogSourceInstalled;
}
#endif

/**
 * Register to the Log Event System
 *  There are 2 ways to register: 
 *  CASE 1: If we need elevated privileges, it should be done through the setup process (see --setup argument).
 *  CASE 2: On older versions of windows where running elevated was not needed, registerSyslogMessageFile() can called when the wrapper runs.
 */
int registerSyslogMessageFile(int forceInstall, int silent) {
#ifdef WIN32
    TCHAR buffer[_MAX_PATH];
    DWORD usedLen;
    TCHAR regPath[1024];
    TCHAR regValueName[32];
    HKEY hKey;
    DWORD categoryCount, typesSupported;
    DWORD error;
    
    if (!forceInstall && syslogMessageFileRegistered(silent)) {
        return 0;
    }

    /* Get absolute path to service manager */
    /* Important : For win XP getLastError() is unchanged if the buffer is too small, so if we don't reset the last error first, we may actually test an old pending error. */
    SetLastError(ERROR_SUCCESS);
    usedLen = GetModuleFileName(NULL, buffer, _MAX_PATH);
    if (usedLen == 0) {
        if (!silent) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, 
                TEXT("Unable to obtain the full path to the Wrapper. %s"), getLastErrorText());
        }
    } else if ((usedLen == _MAX_PATH) || (getLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        if (!silent) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, 
                TEXT("Unable to obtain the full path to the Wrapper. Path to Wrapper binary too long."));
        }
    } else {
        _sntprintf( regPath, 1024, TEXT("SYSTEM\\CurrentControlSet\\Services\\Eventlog\\Application\\%s"), loginfoSourceName );

        if( RegCreateKey( HKEY_LOCAL_MACHINE, regPath, (PHKEY) &hKey ) == ERROR_SUCCESS ) {
            RegCloseKey( hKey );

            if( RegOpenKeyEx( HKEY_LOCAL_MACHINE, regPath, 0, KEY_WRITE, (PHKEY) &hKey ) == ERROR_SUCCESS ) {
                
                /* Set EventMessageFile */
                _tcsncpy(regValueName, TEXT("EventMessageFile"), 32);
                if ((error = RegSetValueEx(hKey, regValueName, 0, REG_SZ, (LPBYTE) buffer, (DWORD)(sizeof(TCHAR) * (_tcslen(buffer) + 1)))) == ERROR_SUCCESS) {
                    
                    /* Set CategoryMessageFile */
                    _tcsncpy(regValueName, TEXT("CategoryMessageFile"), 32);
                    if ((error = RegSetValueEx(hKey, regValueName, 0, REG_SZ, (LPBYTE) buffer, (DWORD)(sizeof(TCHAR) * (_tcslen(buffer) + 1)))) == ERROR_SUCCESS) {
                        
                        /* Set CategoryCount */
                        _tcsncpy(regValueName, TEXT("CategoryCount"), 32);
                        categoryCount = 12;
                        if ((error = RegSetValueEx(hKey, regValueName, 0, REG_DWORD, (LPBYTE) &categoryCount, sizeof(DWORD))) == ERROR_SUCCESS) {
                            
                            /* Set TypesSupported */
                            _tcsncpy(regValueName, TEXT("TypesSupported"), 32);
                            typesSupported = 7;
                            if ((error = RegSetValueEx(hKey, regValueName, 0, REG_DWORD, (LPBYTE) &typesSupported, sizeof(DWORD))) == ERROR_SUCCESS) {
                                eventLogSourceInstalled = TRUE;
                            }
                        }
                    }
                }
                
                RegCloseKey( hKey );
                
                if (error != ERROR_SUCCESS) {
                    if (!silent) {
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, 
                            TEXT("Failed to set '%s' when registering to the Event Log (0x%x)."), regValueName, error);
                    }
                }
            }
        }
    }

    if (!eventLogSourceInstalled) {
        /* not registered or failed to register correctly */
        disableSysLog(silent);
        return -1;
    }

    return 0;
#else
    return 0;
#endif
}

/**
 * Unregister from the Log Event System
 */
int unregisterSyslogMessageFile(int silent) {
#ifdef WIN32
    DWORD error;
    /* If we deregister this application, then the event viewer will not work when the program is not running. */
    /* Don't want to clutter up the Registry, but is there another way?  */
    /* From 3.5.28 we want to allow the user to never register. */
    TCHAR regPath[ 1024 ];

    /* Get absolute path to service manager */
    _sntprintf( regPath, 1024, TEXT("SYSTEM\\CurrentControlSet\\Services\\Eventlog\\Application\\%s"), loginfoSourceName );

    error = RegDeleteKey(HKEY_LOCAL_MACHINE, regPath);
    if((error == ERROR_SUCCESS) || (error == ERROR_FILE_NOT_FOUND)) {
        eventLogSourceInstalled = FALSE;
        disableSysLog(silent);
        return 0;
    }

    return -1; /* Failure */
#else
    return 0;
#endif
}

#ifdef WIN32
void sendEventlogMessage( int source_id, int level, const TCHAR *szBuff ) {
    TCHAR   header[16];
    const TCHAR   **strings;
    WORD   eventType;
    HANDLE handle;
    WORD   eventID, categoryID;
    int    result;
    
    strings = malloc(sizeof(TCHAR *) * 3);
    if (!strings) {
        _tprintf(TEXT("Out of memory in logging code (%s)\n"), TEXT("SEM1"));
        return;
    }

    /* Build the source header */
    switch(source_id) {
    case WRAPPER_SOURCE_WRAPPER:
#ifdef WIN32
        if (launcherSource) {
            _sntprintf( header, 16, TEXT("wrapperm") );
        } else {
            _sntprintf( header, 16, TEXT("wrapper") );
        }
#else
        _sntprintf( header, 16, TEXT("wrapper") );
#endif
        break;

    case WRAPPER_SOURCE_PROTOCOL:
        _sntprintf( header, 16, TEXT("wrapperp") );
        break;

    default:
        _sntprintf( header, 16, TEXT("jvm %d"), source_id );
        header[15] = TEXT('\0'); /* Just in case we get lots of restarts. */
        break;
    }

    /* Build event type by level */
    switch(level) {
    case LEVEL_NOTICE: /* Will not get in here. */
    case LEVEL_ADVICE: /* Will not get in here. */
    case LEVEL_FATAL:
        eventType = EVENTLOG_ERROR_TYPE;
        break;

    case LEVEL_ERROR:
    case LEVEL_WARN:
        eventType = EVENTLOG_WARNING_TYPE;
        break;

    case LEVEL_STATUS:
    case LEVEL_INFO:
    case LEVEL_DEBUG:
        eventType = EVENTLOG_INFORMATION_TYPE;
        break;
    }

    /* Set the category id to the appropriate resource id. */
    if ( source_id == WRAPPER_SOURCE_WRAPPER ) {
        categoryID = MSG_EVENT_LOG_CATEGORY_WRAPPER;
    } else if ( source_id == WRAPPER_SOURCE_PROTOCOL ) {
        categoryID = MSG_EVENT_LOG_CATEGORY_PROTOCOL;
    } else {
        /* Source is a JVM. */
        switch ( source_id ) {
        case 1:
            categoryID = MSG_EVENT_LOG_CATEGORY_JVM1;
            break;

        case 2:
            categoryID = MSG_EVENT_LOG_CATEGORY_JVM2;
            break;

        case 3:
            categoryID = MSG_EVENT_LOG_CATEGORY_JVM3;
            break;

        case 4:
            categoryID = MSG_EVENT_LOG_CATEGORY_JVM4;
            break;

        case 5:
            categoryID = MSG_EVENT_LOG_CATEGORY_JVM5;
            break;

        case 6:
            categoryID = MSG_EVENT_LOG_CATEGORY_JVM6;
            break;

        case 7:
            categoryID = MSG_EVENT_LOG_CATEGORY_JVM7;
            break;

        case 8:
            categoryID = MSG_EVENT_LOG_CATEGORY_JVM8;
            break;

        case 9:
            categoryID = MSG_EVENT_LOG_CATEGORY_JVM9;
            break;

        default:
            categoryID = MSG_EVENT_LOG_CATEGORY_JVMXX;
            break;
        }
    }

    /* Place event in eventlog */
    strings[0] = header;
    strings[1] = szBuff;
    strings[2] = 0;
    eventID = level;

    handle = RegisterEventSource( NULL, loginfoSourceName );
    if( !handle )
        return;

    result = ReportEvent(
        handle,                   /* handle to event log */
        eventType,                /* event type */
        categoryID,               /* event category */
        MSG_EVENT_LOG_MESSAGE,    /* event identifier */
        NULL,                     /* user security identifier */
        2,                        /* number of strings to merge */
        0,                        /* size of binary data */
        (const TCHAR**) strings,  /* array of strings to merge */
        NULL                      /* binary data buffer */
    );
    if (result == 0) {
        /* If there are any errors accessing the event log, like it is full, then disable its output. */
        setSyslogLevelInt(LEVEL_NONE);

        /* Recurse so this error gets set in the log file and console.  The syslog
         *  output has been disabled so we will not get back here. */
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("Unable to write to the EventLog due to: %s"), getLastErrorText());
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("Internally setting wrapper.syslog.loglevel=NONE to prevent further messages."));
    }

    DeregisterEventSource( handle );

    free( (void *) strings );
    strings = NULL;
}
#else
void sendLoginfoMessage( int source_id, int level, const TCHAR *szBuff ) {
    int eventType;

    /* Build event type by level */
    switch( level ) {
        case LEVEL_FATAL:
            eventType = LOG_CRIT;
        break;

        case LEVEL_ERROR:
            eventType = LOG_ERR;
        break;

        case LEVEL_WARN:
        case LEVEL_STATUS:
            eventType = LOG_NOTICE;
        break;

        case LEVEL_INFO:
            eventType = LOG_INFO;
        break;

        case LEVEL_DEBUG:
            eventType = LOG_DEBUG;
        break;

        default:
            eventType = LOG_DEBUG;
    }
    
    /* openlog, closelog, and syslog all return void. */
    openlog( loginfoSourceName, LOG_PID | LOG_NDELAY, currentLogfacilityLevel );
    _tsyslog( eventType, szBuff );
    closelog( );
}
#endif

#ifdef WIN32
 #define CONSOLE_BLOCK_SIZE 1024
/* The following is an initial (max) size for the number of characters to try writing to WriteConsole at once.
 *  See notes on the WriteConsole function below for details. */
size_t vWriteToConsoleMaxHeapBufferSize = 30000;
size_t vWriteToConsoleBufferSize = 0;
TCHAR *vWriteToConsoleBuffer = NULL;
/**
 * Write a line of output to the console.
 *
 * @param hdl The handle to write to.  Must be a valid handle.
 *
 * @return TRUE if successful, FALSE if the line was not written.
 */
int writeToConsole(HANDLE hdl, TCHAR *lpszFmt, ...) {
    va_list        vargs;
    int cnt;
    size_t fullLen;
    size_t remainLen;
    size_t offset;
    size_t thisLen;
    DWORD wrote;

 #ifdef DEBUG_CONSOLE_OUTPUT
        _tprintf(TEXT("writeToConsole BEGIN\n"));
 #endif
    
    if (vWriteToConsoleBuffer == NULL) {
        vWriteToConsoleBufferSize = CONSOLE_BLOCK_SIZE * 2;
        vWriteToConsoleBuffer = malloc(sizeof(TCHAR) * vWriteToConsoleBufferSize);
        if (!vWriteToConsoleBuffer) {
            _tprintf(TEXT("Out of memory in logging code (%s)\n"), TEXT("WTC1"));
            return FALSE;
        }
 #ifdef DEBUG_CONSOLE_OUTPUT
        _tprintf(TEXT("Console Buffer Size = %d (Initial Size)\n"), vWriteToConsoleBufferSize);
 #endif
        if (logBufferGrowth) {
            /* This is queued as we can't use direct logging here. */
            log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("Console Buffer Size initially set to %d characters."), vWriteToConsoleBufferSize);
        }
    }

    va_start(vargs, lpszFmt);
    
    /* The only way I could figure out how to write to the console
     *  returned by AllocConsole when running as a service was to
     *  do all of this special casing and use the handle to the new
     *  console's stdout and the WriteConsole function.  If anyone
     *  puzzling over this code knows a better way of doing this
     *  let me know.
     * WriteConsole takes a fixed buffer and does not do any expansions
     *  We need to prepare the string to be displayed ahead of time.
     *  This means storing the message into a temporary buffer.  The
     *  following loop will expand the global buffer to hold the current
     *  message.  It will grow as needed to handle any arbitrarily large
     *  user message.  The buffer needs to be able to hold all available
     *  characters + a null TCHAR.
     * The _vsntprintf function will fill all available space and only
     *  terminate the string if there is room.  Because of this we need
     *  to make sure and reserve room for the null terminator and add it
     *  if needed below. */
    while ((cnt = _vsntprintf(vWriteToConsoleBuffer, vWriteToConsoleBufferSize - 1, lpszFmt, vargs)) < 0) {
 #ifdef DEBUG_CONSOLE_OUTPUT
        _tprintf(TEXT("writeToConsole ProcessCount=%d\n"), cnt);
 #endif
        /* Expand the size of the buffer */
        free(vWriteToConsoleBuffer);
        
        /* Increase the buffer by the CONSOLE_BLOCK_SIZE or an additional 10%, which ever is larger.
         *  The goal here is to grow the buffer size quickly, but not waste too much memory. */
        vWriteToConsoleBufferSize = __max(vWriteToConsoleBufferSize + CONSOLE_BLOCK_SIZE, vWriteToConsoleBufferSize + vWriteToConsoleBufferSize / 10);
        vWriteToConsoleBuffer = malloc(sizeof(TCHAR) * vWriteToConsoleBufferSize);
        if (!vWriteToConsoleBuffer) {
            _tprintf(TEXT("Out of memory in logging code (%s)\n"), TEXT("WTC2"));
            va_end( vargs );
            return FALSE;
        }
 #ifdef DEBUG_CONSOLE_OUTPUT
        _tprintf(TEXT("Console Buffer Size = %d (Increased Size) ****************************************\n"), vWriteToConsoleBufferSize);
 #endif
        if (logBufferGrowth) {
            /* This is queued as we can't use direct logging here. */
            log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("Console Buffer Size increased to %d characters."), vWriteToConsoleBufferSize);
        }
    }
 #ifdef DEBUG_CONSOLE_OUTPUT
    _tprintf(TEXT("writeToConsole ProcessCount=%d\n"), cnt);
 #endif
    if (cnt == (vWriteToConsoleBufferSize - 1)) {
        /* The maximum number of characters were read.  All of the characters fit in the available space, but because of the way the API works, the string was not null terminated. */
        vWriteToConsoleBuffer[vWriteToConsoleBufferSize - 1] = '\0';
    }
    
    va_end(vargs);
    
 #ifdef DEBUG_CONSOLE_OUTPUT
    _tprintf(TEXT("writeToConsole BufferSize=%d, MessageLen=%d, Message=[%s]\n"), vWriteToConsoleBufferSize, _tcslen(vWriteToConsoleBuffer), vWriteToConsoleBuffer);
 #endif
    
    /* The WriteConsole API is a nasty little beast.
     *  It can accept a buffer that is up to 64KB in size, but they can't tell us exactly how much before hand.
     *  The size on tests on a 64-bit XP system appear to be around 25000 characters.
     *  Windows 7 returns success, but starts writing garbled characters after around 31397 characters.  (Not sure if this number is system specific however.)
     *  The problem is that this is highly dependent on the current system state.
     *  We used to start with 32000, but now use 30000 to avoid problems on Windows 7.  (Not sure if this is small enough to avoid the issue on all systems.)
     *  Start with a large size for efficiency, but then reduce it automatically in a sticky way in 5% increments to get to a size that works. */
    fullLen = _tcslen(vWriteToConsoleBuffer);
    remainLen = fullLen;
    offset = 0;
    while (remainLen > 0) {
        thisLen = __min(remainLen, vWriteToConsoleMaxHeapBufferSize);
 #ifdef DEBUG_CONSOLE_OUTPUT
        _tprintf(TEXT("writeToConsole write %d of %d characters\n"), thisLen, fullLen);
 #endif
        if (WriteConsole(hdl, &(vWriteToConsoleBuffer[offset]), (DWORD)thisLen, &wrote, NULL)) {
 #ifdef DEBUG_CONSOLE_OUTPUT
            _tprintf(TEXT("\nwriteToConsole (WriteConsole wrote %d of requested %d characters)\n"), wrote, thisLen);
 #endif
            /* Success. */
            offset += thisLen;
            remainLen -= thisLen;
 #ifdef DEBUG_CONSOLE_OUTPUT
            if (remainLen > 0) {
                /* We have not written out the whole line which means there was no line feed.  Add one or the debug output will be hard to read. */
                _tprintf(TEXT("\nwriteToConsole (Previous line was incomplete)\n"));
            }
 #endif
        } else {
            /* Failed. */
 #ifdef DEBUG_CONSOLE_OUTPUT
            _tprintf(TEXT("\nwriteToConsole (Fail WriteConsole wrote %d of requested %d characters)\n"), wrote, thisLen);
 #endif
            switch (getLastError()) {
            case ERROR_NOT_ENOUGH_MEMORY:
                /* This means that the max heap buffer size is too large and needs to be reduced. */
                if (vWriteToConsoleMaxHeapBufferSize < 100) {
                    _tprintf(TEXT("Not enough available HEAP to write to console.\n"));
                    return FALSE;
                }
                vWriteToConsoleMaxHeapBufferSize = vWriteToConsoleMaxHeapBufferSize - vWriteToConsoleMaxHeapBufferSize / 20;
 #ifdef DEBUG_CONSOLE_OUTPUT
                _tprintf(TEXT("Usable Console HEAP Buffer Size reduced to = %d ****************************************\n"), vWriteToConsoleMaxHeapBufferSize);
 #endif
                if (logBufferGrowth) {
                    /* This is queued as we can't use direct logging here. */
                    log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("Usable Console HEAP Buffer Size decreased to %d characters."), vWriteToConsoleMaxHeapBufferSize);
                }
                break;
                
            case ERROR_INVALID_FUNCTION:
            case ERROR_INVALID_HANDLE:
                /* This is a fairly normal thing to happen if the Wrapper is run without an actual console.
                 * ERROR_INVALID_FUNCTION happens when we launch a forked elevated Wrapper.
                 * ERROR_INVALID_HANDLE happens when the Wrapper is launched without its own console.
                 *  Log to debug so there is a note, but it is fine if this does not show up in commands where debug output can't be enabled. */
                if (currentConsoleLevel <= LEVEL_DEBUG) {
                    _tprintf(TEXT("A console does not exist.  Disabling direct console output and falling back to using pipes.\n"));
                }
                consoleDirect = FALSE;
                return FALSE;
                
            default:
                _tprintf(TEXT("Failed to write to console: %s\n"), getLastErrorText());
                return FALSE;
            }
        }
    }

 #ifdef DEBUG_CONSOLE_OUTPUT
    _tprintf(TEXT("writeToConsole END\n"));
 #endif
    return TRUE;
}
#endif

/**
 * Does a search for all files matching the specified pattern and deletes all
 *  but the most recent 'count' files.  The files are sorted by their names.
 */
void limitLogFileCount(const TCHAR *current, const TCHAR *pattern, int sortMode, int count) {
    TCHAR **files;
    int index;
    int foundCurrent;

#ifdef _DEBUG
    _tprintf(TEXT("limitLogFileCount(%s, %s, %d, %d)\n"), current, pattern, sortMode, count);
#endif

    files = loggerFileGetFiles(pattern, sortMode);
    if (!files) {
        /* Failed */
        return;
    }

    /* When this loop runs we keep the first COUNT files in the list and everything thereafter is deleted. */
    foundCurrent = FALSE;
    index = 0;
    while (files[index]) {
        if (index < count) {
#ifdef _DEBUG
            _tprintf(TEXT("Keep files[%d] %s\n"), index, files[index]);
#endif
            if (_tcscmp(current, files[index]) == 0) {
                /* This is the current file, as expected. */
#ifdef _DEBUG
                _tprintf(TEXT("  Current\n"));
#endif
                foundCurrent = TRUE;
            }
        } else {
#ifdef _DEBUG
            _tprintf(TEXT("Delete files[%d] %s\n"), index, files[index]);
#endif
            if (_tcscmp(current, files[index]) == 0) {
                /* This is the current file, we don't want to delete it. */
                _tprintf(TEXT("Log file sort order would result in current log file being deleted: %s\n"), current);
                foundCurrent = TRUE;
            } else if (_tremove(files[index])) {
                _tprintf(TEXT("Unable to delete old log file: %s (%s)\n"), files[index], getLastErrorText());
            }
        }

        index++;
    }

    /* Now if we did not find the current file, and there are <count> files
       still in the directory, then we want to also delete the oldest one.
       Otherwise, the addition of the current file would result in too many
       files. */
    if (!foundCurrent) {
        if (index >= count) {
#ifdef _DEBUG
            _tprintf(TEXT("Delete files[%d] %s\n"), count - 1, files[count - 1]);
#endif
            if (_tremove(files[count - 1])) {
                _tprintf(TEXT("Unable to delete old log file: %s (%s)\n"), files[count - 1], getLastErrorText());
            }
        }
    }

    loggerFileFreeFiles(files);
}

/**
 * Sets the current uptime in seconds.
 *
 * @param uptime Uptime in seconds.
 * @param flipped TRUE when the uptime is no longer meaningful.
 */
void setUptime(int uptime, int flipped) {
    uptimeSeconds = uptime;
    uptimeFlipped = flipped;
}

int rollFailure = FALSE;
/**
 * Rolls log files using the ROLLNUM system.
 */
void rollLogs() {
    int i;
    TCHAR rollNum[11];
#if defined(WIN32) && !defined(WIN64)
    struct _stat64i32 fileStat;
#else
    struct stat fileStat;
#endif
    int result;

#ifdef _DEBUG
    _tprintf(TEXT("rollLogs()\n"));
#endif
    if (!logFilePath) {
        return;
    }

    /* If the log file is currently open, it needs to be closed. */
    if (logfileFP != NULL) {
#ifdef _DEBUG
        _tprintf(TEXT("Closing logfile so it can be rolled...\n"));
#endif

        fclose(logfileFP);
        logfileFP = NULL;
        currentLogFileName[0] = TEXT('\0');
    }

#ifdef _DEBUG
    _tprintf(TEXT("Rolling log files... (rollFailure=%d)\n"), rollFailure);
#endif

    /* We don't know how many log files need to be rotated yet, so look. */
    i = 0;
    do {
        i++;
        _sntprintf(rollNum, 11, TEXT("%d"), i);
        generateLogFileName(workLogFileName, currentLogFileNameSize, logFilePath, NULL, rollNum);
        result = _tstat(workLogFileName, &fileStat);
#ifdef _DEBUG
        if (result == 0) {
            _tprintf(TEXT("Rolled log file %s exists.\n"), workLogFileName);
        }
#endif
    } while (result == 0);

    /* Now, starting at the highest file rename them up by one index. */
    for (; i > 1; i--) {
        _tcsncpy(currentLogFileName, workLogFileName, _tcslen(logFilePath) + 11);
        _sntprintf(rollNum, 11, TEXT("%d"), i - 1);
        generateLogFileName(workLogFileName, currentLogFileNameSize, logFilePath, NULL, rollNum);

        if ((logFileMaxLogFiles > 0) && (i > logFileMaxLogFiles) && (!logFilePurgePattern)) {
            /* The file needs to be deleted rather than rolled.   If a purge pattern was not specified,
             *  then the files will be deleted here.  Otherwise they will be deleted below. */

#ifdef _DEBUG
            _tprintf(TEXT("Remove old log file %s\n"), workLogFileName);
#endif
            if (_tremove(workLogFileName)) {
#ifdef _DEBUG
                _tprintf(TEXT("Failed to remove old log file %s. err=%d\n"), workLogFileName, getLastError());
#endif
                if (getLastError() == 2) {
                    /* The file did not exist. */
                } else if (getLastError() == 3) {
                    /* The path did not exist. */
                } else {
                    if (rollFailure == FALSE) {
                        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT("Unable to delete old log file: %s (%s)"), workLogFileName, getLastErrorText());
                    }
                    rollFailure = TRUE;
                    generateLogFileName(currentLogFileName, currentLogFileNameSize, logFilePath, NULL, NULL); /* Set the name back so we don't cause a logfile name changed event. */
                    return;
                }
            } else {
                /* On Windows, in some cases if the file can't be deleted, we still get here without an error. Double check. */
                if (_tstat(workLogFileName, &fileStat) == 0) {
                    /* The file still existed. */
#ifdef _DEBUG
                        _tprintf(TEXT("Failed to remove old log file %s\n"), workLogFileName);
#endif
                    if (rollFailure == FALSE) {
                        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT("Unable to delete old log file: %s"), workLogFileName);
                    }
                    rollFailure = TRUE;
                    generateLogFileName(currentLogFileName, currentLogFileNameSize, logFilePath, NULL, NULL); /* Set the name back so we don't cause a logfile name changed event. */
                    return;
                }
#ifdef _DEBUG
                else {
                    _tprintf(TEXT("Deleted %s\n"), workLogFileName);
                }
#endif
            }
        } else {
            if (_trename(workLogFileName, currentLogFileName) != 0) {
                if (rollFailure == FALSE) {
#ifdef WIN32
                    if (errno == EACCES) {
                        /* This access denied message is treated as a special case, but the use by other applications issue only happens on Windows. */
                        /* Don't log this as with other errors as that would cause recursion. */
                        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT("Unable to rename log file %s to %s.  File is in use by another application."),
                            workLogFileName, currentLogFileName);
                    } else {
#endif
                        /* Don't log this as with other errors as that would cause recursion. */
                        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT("Unable to rename log file %s to %s. (%s)"),
                            workLogFileName, currentLogFileName, getLastErrorText());
#ifdef WIN32
                    }
#endif
                } 
                rollFailure = TRUE;
                generateLogFileName(currentLogFileName, currentLogFileNameSize, logFilePath, NULL, NULL); /* Set the name back so we don't cause a logfile name changed event. */
                return;
            }
#ifdef _DEBUG
            else {
                _tprintf(TEXT("Renamed %s to %s\n"), workLogFileName, currentLogFileName);
            }
#endif
        }
    }

    /* Rename the current file to the #1 index position */
    generateLogFileName(currentLogFileName, currentLogFileNameSize, logFilePath, NULL, NULL);
    if (_trename(currentLogFileName, workLogFileName) != 0) {
        if (rollFailure == FALSE) {
            if (getLastError() == 2) {
                 /* File does not yet exist. */
            } else if (getLastError() == 3) {
                /* Path does not yet exist. */
            } else if (errno == 13) {
                /* Don't log this as with other errors as that would cause recursion. */
                    log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, 
                        TEXT("Unable to rename log file %s to %s.  File is in use by another application."),
                        currentLogFileName, workLogFileName);
            } else {
                /* Don't log this as with other errors as that would cause recursion. */
                log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT("Unable to rename log file %s to %s. (%s)"),
                    currentLogFileName, workLogFileName, getLastErrorText());
            } 
        }
        rollFailure = TRUE;
        generateLogFileName(currentLogFileName, currentLogFileNameSize, logFilePath, NULL, NULL); /* Set the name back so we don't cause a logfile name changed event. */
        return;
    }
#ifdef _DEBUG
    else {
        _tprintf(TEXT("Renamed %s to %s\n"), currentLogFileName, workLogFileName);
    }
#endif

    /* Now limit the number of files using the standard method. */
    if (logFileMaxLogFiles > 0) {
        if (logFilePurgePattern) {
            limitLogFileCount(currentLogFileName, logFilePurgePattern, logFilePurgeSortMode, logFileMaxLogFiles + 1);
        }
    }
    if (rollFailure == TRUE) {
        /* We made it here, but the rollFailure flag had been previously set.  Make a note that we are back and then continue. */
        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
            TEXT("Logfile rolling is working again."));
    }
    rollFailure = FALSE;
    
    /* Reset the current log file name as it is not being used yet. */
    currentLogFileName[0] = TEXT('\0'); /* Log file was rolled, so we want to cause a logfile change event. */
}

#ifdef LINUX
/**
 * Get description found in a release file.
 *  This function will only check for the first line because there is only one line in these files.
 *
 * @return TRUE if the description could be found.
 */
int getReleaseDescription(TCHAR **description, const TCHAR *releaseFile) {
#if defined(WIN32) && !defined(WIN64)
    struct _stat64i32 fileStat;
#else
    struct stat fileStat;
#endif
    FILE *file = NULL;
    int result = FALSE;
    
    /* check if the file exists */
    if (_tstat(releaseFile, &fileStat) == 0) {
        file = _tfopen(releaseFile, TEXT("r"));
        if (file != NULL) {
            *description = malloc(100 * sizeof(TCHAR));
            if (*description == NULL) {
                outOfMemoryQueued(TEXT("GRD"), 1);
            } else if (_fgetts(*description, 100, file) != NULL) {
                /* got the release description inside the file */
                result = TRUE;
            } else {
                /* _fgetts failed but *description remains unchanged. Free it up. */
                free(*description);
                *description = NULL;
            }
            fclose(file);
        }
    }
    return result;
}

static TCHAR distroDescription[100];
const TCHAR *centosPattern = TEXT("CentOS Linux");
const TCHAR *amiPattern = TEXT("Amazon Linux AMI");
const TCHAR *rhelPattern = TEXT("Red Hat Enterprise Linux");
const TCHAR *fedoraPattern = TEXT("Fedora release");
const TCHAR *linuxPattern = TEXT("Linux");

/**
 * Get a description of the linux distribution.
 */
const TCHAR *getDistro() {
    static int firstCall = TRUE;
    TCHAR *sysDescription = NULL;
    TCHAR *rhelDescription = NULL;
    int foundSysDescription;
    int foundRhDescription;
    
    if (firstCall) {
        firstCall = FALSE;

        foundSysDescription = getReleaseDescription(&sysDescription, TEXT("/etc/system-release"));
        foundRhDescription = getReleaseDescription(&rhelDescription, TEXT("/etc/redhat-release"));
        
        if ((foundRhDescription && _tcsstr(rhelDescription, centosPattern) != NULL) || 
            (foundSysDescription  && _tcsstr(sysDescription, centosPattern) != NULL)) {
            _tcsncpy(distroDescription, centosPattern, 100);
        } else if ((foundRhDescription && _tcsstr(rhelDescription, rhelPattern) != NULL) || 
            (foundSysDescription  && _tcsstr(sysDescription, rhelPattern) != NULL)) {
            _tcsncpy(distroDescription, rhelPattern, 100);
        } else if ((foundRhDescription && _tcsstr(rhelDescription, fedoraPattern) != NULL) || 
            (foundSysDescription  && _tcsstr(sysDescription, fedoraPattern) != NULL)) {
            _tcsncpy(distroDescription, fedoraPattern, 100);
        } else if (foundSysDescription && _tcsstr(sysDescription, amiPattern) != NULL) {
            _tcsncpy(distroDescription, amiPattern, 100);
        } else {
            _tcsncpy(distroDescription, linuxPattern, 100);
        }
        if (sysDescription) {
            free(sysDescription);
        }
        if (rhelDescription) {
            free(rhelDescription);
        }
    }
    
    return distroDescription;
}

int isCentos() {
    static int result = -1;
    return result != -1 ? result : (result = (_tcsicmp(getDistro(), centosPattern) == 0));
}

int isAMI() {
    static int result = -1;
    return result != -1 ? result : (result = (_tcsicmp(getDistro(), amiPattern) == 0));
}

int isRHEL() {
    static int result = -1;
    return result != -1 ? result : (result = (_tcsicmp(getDistro(), rhelPattern) == 0));
}

int isFedora() {
    static int result = -1;
    return result != -1 ? result : (result = (_tcsicmp(getDistro(), fedoraPattern) == 0));
}

/**
 * Check if the glibc version of the user is upper to given numbers.
 */
int wrapperAssertGlibcUserBis(unsigned int maj, unsigned int min, unsigned int rev) {
    unsigned int vmaj=0;
    unsigned int vmin=0;
    unsigned int vrev=0;

    TCHAR versionW[10];
    
    mbstowcs(versionW, gnu_get_libc_version(), 10);
    _stscanf(versionW, TEXT("%d.%d.%d"), &vmaj, &vmin, &vrev);
    return ((vmaj == maj && vmin == min &&  vrev >= rev) || (vmaj == maj && vmin > min) || vmaj > maj);
}
#endif

/**
 * Check if the system is Centos with glibc < 2.21 as there is a memory leak issue under these conditions.
 */
int doesFtellCauseMemoryLeak() {
    static int result = -1;
#ifdef LINUX
    if (result == -1) {
        if ((isCentos() || isAMI() || isRHEL() || isFedora()) && !wrapperAssertGlibcUserBis(2, 21, 0)){
            result = 1;
        } else {
            result = 0;
        }
    }
#endif
    return (result > 0);
}

/**
 * Check to see whether or not the log file needs to be rolled.
 *  This is only called when synchronized.
 */
void checkAndRollLogs(const TCHAR *nowDate, size_t printBufferSize) {
    size_t position;
#if defined(WIN32) && !defined(WIN64)
    struct _stat64i32 fileStat;
#else
    struct stat fileStat;
#endif
    static size_t unflushedBufferSize = 0;
    static size_t previousFileSize = 0;

    /* Depending on the roll mode, decide how to roll the log file. */
    if (logFileRollMode & ROLL_MODE_SIZE) {
        /* Roll based on the size of the file. */
        if (logFileMaxSize <= 0) {
            return;
        }

        /* Find out the current size of the file.  If the file is currently open then we need to
         *  use ftell to make sure that the buffered data is also included. */
        if (logfileFP != NULL && !doesFtellCauseMemoryLeak()) {
            /* File is open */
            if ((position = ftell(logfileFP)) < 0) {
                _tprintf(TEXT("Unable to get the current logfile size with ftell: %s\n"), getLastErrorText());
                return;
            }
        } else {
            /* File is not open or we can't use ftell because of memory leak issue */
            if (_tstat(logFilePath, &fileStat) != 0) {
                if (getLastError() == 2) {
                    /* File does not yet exist. */
                    position = 0;
                } else if (getLastError() == 3) {
                    /* Path does not exist. */
                    position = 0;
                } else {
                    _tprintf(TEXT("Unable to get the current logfile size with stat: %s\n"), getLastErrorText());
                    return;
                }
            } else {
                position = fileStat.st_size;
                if (doesFtellCauseMemoryLeak()) {
                    if (previousFileSize != position) {
                        /* the file has been flushed */
                        previousFileSize = position;
                        unflushedBufferSize = 0;
                    }
                    unflushedBufferSize += printBufferSize;
                    position += unflushedBufferSize;
                }
            }
        }

        /* Does the log file need to rotated? */
        if ((int)position - 2 >= logFileMaxSize) { /* -2: no carriage return for the last message being logged. */
            rollLogs();
        }
    } else if (logFileRollMode & ROLL_MODE_DATE) {
        /* Roll based on the date of the log entry. */
        if (_tcscmp(nowDate, logFileLastNowDate) != 0) {
            /* The date has changed.  Close the file. */
            if (logfileFP != NULL) {
#ifdef _DEBUG
                _tprintf(TEXT("Closing logfile because the date changed...\n"));
#endif

                fclose(logfileFP);
                logfileFP = NULL;
            }
            /* Always reset the name so the the log file name will be regenerated correctly. */
            currentLogFileName[0] = TEXT('\0');

            /* This will happen just before a new log file is created.
             *  Check the maximum file count. */
            if (logFileMaxLogFiles > 0) {
                /* We will check for too many files here and then clear the current log file name so it will be set later. */
                generateLogFileName(currentLogFileName, currentLogFileNameSize, logFilePath, nowDate, NULL);

                if (logFilePurgePattern) {
                    limitLogFileCount(currentLogFileName, logFilePurgePattern, logFilePurgeSortMode, logFileMaxLogFiles + 1);
                } else {
                    generateLogFileName(workLogFileName, currentLogFileNameSize, logFilePath, TEXT("????????"), NULL);
                    limitLogFileCount(currentLogFileName, workLogFileName, LOGGER_FILE_SORT_MODE_NAMES_DEC, logFileMaxLogFiles + 1);
                }

                currentLogFileName[0] = TEXT('\0');
                workLogFileName[0] = TEXT('\0');
            }
        }
    }
}

void log_printf_queue( int useQueue, int source_id, int level, const TCHAR *lpszFmt, ... ) {
    int threadId;
    int localWriteIndex;
    int localReadIndex;
    va_list     vargs;
    int         count;
#if defined(UNICODE) && !defined(WIN32)
    TCHAR       *format;
    size_t      i;
    size_t      len;
#endif
    TCHAR       *buffer;

    /* Start by processing any arguments so that we can store a simple string. */
#ifdef _DEBUG_QUEUE
    _tprintf(TEXT("log_printf_queue(%d, %d, %d, %S)\n"), useQueue, source_id, level, lpszFmt);
#endif

#if defined(UNICODE) && !defined(WIN32)
    if (wcsstr(lpszFmt, TEXT("%s")) != NULL) {
        /* On UNIX platforms string tokens must always use "%S" variables and not "%s".  We can
         *  not safely use malloc here as the call may have originated from a signal handler.
         *  Copy the template into the formatMessages string reserved for this thread, replace
         *  the tokens and then continue using that.  This is a bit of overhead, but these async
         *  messages are fairly rare and this greatly simplifies the code throughout the rest of
         *  the application by making it possible to always use the "%s" syntax. */
        threadId = getThreadId();
        _tcsncpy(formatMessages[threadId], lpszFmt, QUEUED_BUFFER_SIZE);
        /* Terminate just in case the format was too long. */
        formatMessages[threadId][QUEUED_BUFFER_SIZE - 1] = TEXT('\0');
        
        format = formatMessages[threadId];
        
        /* Replace the tokens. */
 #ifdef _DEBUG_QUEUE
        _tprintf(TEXT("Replacing string tokens.\n"));
        _tprintf(TEXT("  From: %S\n"), format);
 #endif
        len = wcslen(format);
        if (len > 0) {
            for (i = 0; i < len; i++) {
                if ((i > 0) && (format[i - 1] == TEXT('%')) && (format[i] == TEXT('s'))) {
                    /* Make sure the '%' was not escaped. */
                    if ((i > 1) && (format[i - 2] == TEXT('%'))) {
                        /* Escaped. Do nothing. */
                    } else {
                        /* 's' needs to be changed to 'S' */
                        format[i] = TEXT('S');
                    }
                }
            }
        }
 #ifdef _DEBUG_QUEUE
        _tprintf(TEXT("  To:   %S\n"), format);
 #endif
        lpszFmt = format;
    }
#endif

    
    /** For queued logging, we have a fixed length buffer to work with.  Just to make it easy to catch
     *   problems, always use the same sized fixed buffer even if we will be using the non-queued logging. */
    if (useQueue) {
        /* Care needs to be taken both with this code and the code below to get done as quick as possible.
         *  It is generally safe because each thread has its own queue.  The only danger is if a message is
         *  being queued while that thread is interrupted by a signal.  If things are setup correctly however
         *  then non-signal threads should not be here in the first place. */
        threadId = getThreadId();
        
        localWriteIndex = queueWriteIndex[threadId];
        localReadIndex = queueReadIndex[threadId];
        
        if ((localWriteIndex == localReadIndex - 1) || ((localWriteIndex == QUEUE_SIZE - 1) && (localReadIndex == 0))) {
            _tprintf(TEXT("WARNING log queue overflow for thread[%d]:%d:%d dropping entry: %s\n"), threadId, localWriteIndex, localReadIndex, lpszFmt);
            return;
        }
        
        /* Get a reference to the message buffer we will use. */
        buffer = queueMessages[threadId][queueWriteIndex[threadId]];
    } else {
        /* This will not be queued so we can use malloc to create a new buffer. */
        buffer = malloc(sizeof(TCHAR) * QUEUED_BUFFER_SIZE);
        if (!buffer) {
            _tprintf(TEXT("Out of memory in logging code (%s)\n"), TEXT("PQ1"));
            return;
        }
        
        /* For compiler */
        threadId = -1;
        localWriteIndex = -1;
    }
    
    /* Now actually generate our buffer. */
    va_start(vargs, lpszFmt);
    count = _vsntprintf(buffer, QUEUED_BUFFER_SIZE_USABLE, lpszFmt, vargs);
    va_end(vargs);
    
    /* vswprintf returns -1 on overflow. */
    if ((count < 0) || (count >= QUEUED_BUFFER_SIZE_USABLE - 1)) {
        /* The expanded message was too big to fit into the buffer.
         *  On Windows, it writes as much as it can so we can make it look pretty.
         *  But on other platforms, nothing is written so we need a message.
         *  It is illegal to do any mallocs in here, so there is nothing we can really do on UNIX. */
#if defined(WIN32)
        /* To be safe, make sure we are null terminated. */
        buffer[QUEUED_BUFFER_SIZE_USABLE - 1] = 0;
        _tcsncat(buffer, TEXT("..."), QUEUED_BUFFER_SIZE);
#else
        /* Write an error string that we know will fit.  This doesn't need to be localized as it should be caught in testing. */
        _sntprintf(buffer, QUEUED_BUFFER_SIZE, TEXT("(Message too long to be logged as a queued message.  Please report this.)"));
#endif
    }
    
    if (useQueue) {
#ifdef _DEBUG_QUEUE
        _tprintf(TEXT("LOG ENQUEUE[%d] Thread[%d]: %s\n"), localWriteIndex, threadId, buffer);
#endif
        /* Store additional information about the call. */
        queueSourceIds[threadId][localWriteIndex] = source_id;
        queueLevels[threadId][localWriteIndex] = level;
    
        /* Lastly increment and wrap the write index. */
        queueWriteIndex[threadId]++;
        if (queueWriteIndex[threadId] >= QUEUE_SIZE) {
            queueWriteIndex[threadId] = 0;
            queueWrapped[threadId] = 1;
        }
    } else {
        /* Make a normal logging call with our new buffer.  Parameters are already expanded. */
        log_printf(source_id, level,
#if defined(UNICODE) && !defined(WIN32)
            TEXT("%S"),
#else
            TEXT("%s"),
#endif
            buffer);
        
        free(buffer);
    }
}

/**
 * Perform any required logger maintenance at regular intervals.
 *
 * One operation is to log any queued messages.  This must be done very
 *  carefully as it is possible that a signal handler could be thrown at
 *  any time as this function is being executed.
 */
void maintainLogger() {
    int localWriteIndex;
    int source_id;
    int level;
    int threadId;
    TCHAR *buffer;
    int logFileChanged;
    TCHAR *logFileCopy;
        
    /* Check to see if there is a pending log file change notification. Do this first as we could
     *  generate our own here as well.  It is important that we do our best to keep them in order.
     *  Grab it and clear the reference quick in case another is set.  This order is thread safe. */
    if (pendingLogFileChange) {
        /* Lock the logging mutex. */
        if (lockLoggingMutex()) {
            return;
        }
        
        logFileCopy = pendingLogFileChange;
        pendingLogFileChange = NULL;
        
        /* Release the lock we have on the logging mutex so that other threads can get in. */
        if (releaseLoggingMutex()) {
            return;
        }
        
        /* Now see if a log file name was queued, using our local copy. */
        if (logFileCopy) {
#ifdef _DEBUG
            _tprintf(TEXT("Sending notification of queued log file name change: %s"), logFileCopy);
#endif
            logFileChangedCallback(logFileCopy);
            free(logFileCopy);
            logFileCopy = NULL;
        }
    }
    
    for (threadId = 0; threadId < WRAPPER_THREAD_COUNT; threadId++) {
        /* NOTE - The queue variables are not synchronized so we need to access them
         *        carefully and assume that data could possibly be corrupted. */
        localWriteIndex = queueWriteIndex[threadId]; /* Snapshot the value to maintain a constant reference. */
        if ( queueReadIndex[threadId] != localWriteIndex ) {
            logFileCopy = NULL;

            /* Lock the logging mutex. */
            if (lockLoggingMutex()) {
                return;
            }
        
            /* Empty the queue of any logged messages. */
            localWriteIndex = queueWriteIndex[threadId]; /* Snapshot the value to maintain a constant reference. */
            while (queueReadIndex[threadId] != localWriteIndex) {
                /* Snapshot the values in the queue at that index. */
                source_id = queueSourceIds[threadId][queueReadIndex[threadId]];
                level = queueLevels[threadId][queueReadIndex[threadId]];
                buffer = queueMessages[threadId][queueReadIndex[threadId]];

                /* The buffer is static in the queue and will be reused. */
#ifdef _DEBUG_QUEUE
                _tprintf(TEXT("LOG QUEUED[%d]: %s\n"), queueReadIndex[threadId], buffer );
#endif

                logFileChanged = log_printf_message(source_id, level, threadId, TRUE, buffer, TRUE);
                if (logFileChanged) {
                    if (logFileCopy) {
                        /* This can happen if there are multiple changes while printing the queued messages
                         *  (for example if the files are rolled with a very low size limit).
                         *  To keep it simple, we will reuse logFileCopy and report only the last change. */
                        free(logFileCopy);
                    }
                    /* We need to make a copy of currentLogFileName because we will call logFileChangedCallback() outside of the semaphore. */
                    logFileCopy = malloc(sizeof(TCHAR) * (_tcslen(currentLogFileName) + 1));
                    if (!logFileCopy) {
                        _tprintf(TEXT("Out of memory in logging code (%s)\n"), TEXT("ML1"));
                    } else {
                        _tcsncpy(logFileCopy, currentLogFileName, _tcslen(currentLogFileName) + 1);
                    }
                }
#ifdef _DEBUG_QUEUE
                _tprintf(TEXT("  Queue lw=%d, qw=%d, qr=%d\n"), localWriteIndex, queueWriteIndex[threadId], queueReadIndex[threadId]);
#endif
                /* Clear the string we just wrote. */
                buffer[0] = TEXT('\0');
                
                queueReadIndex[threadId]++;
                if ( queueReadIndex[threadId] >= QUEUE_SIZE ) {
                    queueReadIndex[threadId] = 0;
                }
            }

            /* Release the lock we have on the logging mutex so that other threads can get in. */
            if (releaseLoggingMutex()) {
                if (logFileCopy) {
                    free(logFileCopy);
                }
                return;
            }

            /* Register the change of the logfile. This can be a long operation so do it when we are no longer in the semaphore. */
            if (logFileCopy) {
                logFileChangedCallback(logFileCopy);
                free(logFileCopy);
            }
        }
    }
}

