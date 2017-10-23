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

#ifndef _LOGGER_H
#define _LOGGER_H

/* If defined, output debug information about console output. */
/*#define DEBUG_CONSOLE_OUTPUT*/

#ifdef _DEBUG
 #define _DEBUG_QUEUE
#endif

#ifdef WIN32
 #include <windows.h>
 #define LOG_USER    (1<<3)
#endif
#ifndef DWORD
 #define DWORD unsigned long
#endif
/* Define constants that may not exist in WinError.h (e.g. on Windows IA). */
#ifndef ERROR_MUI_FILE_NOT_FOUND
 #define ERROR_MUI_FILE_NOT_FOUND                   0x3AFC
#endif
#ifndef ERROR_MUI_INVALID_FILE
 #define ERROR_MUI_INVALID_FILE                     0x3AFD
#endif
#ifndef ERROR_MUI_INVALID_RC_CONFIG
 #define ERROR_MUI_INVALID_RC_CONFIG                0x3AFE
#endif
#ifndef ERROR_MUI_INVALID_LOCALE_NAME
 #define ERROR_MUI_INVALID_LOCALE_NAME              0x3AFF
#endif
#ifndef ERROR_MUI_INVALID_ULTIMATEFALLBACK_NAME
 #define ERROR_MUI_INVALID_ULTIMATEFALLBACK_NAME    0x3B00
#endif
#ifndef ERROR_MUI_FILE_NOT_LOADED
 #define ERROR_MUI_FILE_NOT_LOADED                  0x3B01
#endif
#include "logger_base.h"

/* * * Log source constants * * */

#define WRAPPER_SOURCE_WRAPPER -1
#define WRAPPER_SOURCE_PROTOCOL -2

/* * * Log thread constants * * */
/* These are indexes in an array so they must be sequential, start
 *  with zero and be one less than the final WRAPPER_THREAD_COUNT */
#define WRAPPER_THREAD_CURRENT  -1
#define WRAPPER_THREAD_SIGNAL   0
#define WRAPPER_THREAD_MAIN     1
#define WRAPPER_THREAD_SRVMAIN  2
#define WRAPPER_THREAD_TIMER    3
#define WRAPPER_THREAD_JAVAIO   4
#define WRAPPER_THREAD_STARTUP  5
#define WRAPPER_THREAD_COUNT    6

#define MAX_LOG_SIZE 4096

#ifdef WIN32
#else
/* A special prefix on log messages that can be bassed through from a forked process
   so the parent will handle the log message correctly. */
#define LOG_FORK_MARKER TEXT("#!#WrApPeR#!#")
#define LOG_SPECIAL_MARKER TEXT("#!#WrApPeRsPeCiAl#!#")
#endif

/* Default log formats */
#define LOG_FORMAT_LOGFILE_DEFAULT TEXT("LPTM")
#define LOG_FORMAT_CONSOLE_DEFAULT TEXT("PM")

/* * * Log file roll mode constants * * */
#define ROLL_MODE_UNKNOWN         0
#define ROLL_MODE_NONE            1
#define ROLL_MODE_SIZE            2
#define ROLL_MODE_WRAPPER         4
#define ROLL_MODE_JVM             8
#define ROLL_MODE_SIZE_OR_WRAPPER ROLL_MODE_SIZE + ROLL_MODE_WRAPPER
#define ROLL_MODE_SIZE_OR_JVM     ROLL_MODE_SIZE + ROLL_MODE_JVM
#define ROLL_MODE_DATE            16

#define ROLL_MODE_DATE_TOKEN      TEXT("YYYYMMDD")


/* Any log messages generated within signal handlers must be stored until we
 *  have left the signal handler to avoid deadlocks in the logging code.
 *  Messages are stored in a round robin buffer of log messages until
 *  maintainLogger is next called.
 * When we are inside of a signal, and thus when calling log_printf_queue,
 *  we know that it is safe to modify the queue as needed.  But it is possible
 *  that a signal could be fired while we are in maintainLogger, so case is
 *  taken to make sure that volatile changes are only made in log_printf_queue.
 */
#define QUEUE_SIZE 20
/* The size of QUEUED_BUFFER_SIZE_USABLE is arbitrary as the largest size which can be logged in full,
 *  but to avoid crashes due to a bug in the HPUX libc (version < 1403), the length of the buffer passed to _vsntprintf must have a length of 1 + N, where N is a multiple of 8. */
#define QUEUED_BUFFER_SIZE_USABLE (512 + 1)
#define QUEUED_BUFFER_SIZE (QUEUED_BUFFER_SIZE_USABLE + 4)

/* This can be called from within logging code that would otherwise get stuck in recursion.
 *  Log to the console exactly when it happens and then also try to get it into the log
 *  file at the next oportunity. */
extern void outOfMemoryQueued(const TCHAR *context, int id);

extern void outOfMemory(const TCHAR *context, int id);

#ifdef _DEBUG
/**
 * Used to dump memory directly to the log file in both HEX and readable format.
 *  Useful in debugging applications to track down memory overflows etc.
 *
 * @param label A label that will be prepended on all lines of output.
 * @param memory The memory to be dumped.
 * @param len The length of the memory to be dumped.
 */
extern void log_dumpHex(TCHAR *label, TCHAR *memory, size_t len);
#endif

/**
 * Sets the number of milliseconds to allow logging to take before a warning is logged.
 *  Defaults to 0 for no limit.  Possible values 0 to 3600000.
 *
 * @param threshold Warning threashold.
 */
extern void setLogWarningThreshold(int threshold);

/**
 * Sets the log levels to a silence so we never output anything.
 */
extern void setSilentLogLevels();

/**
 * Sets the console log levels to a simple format for help and usage messages.
 */
extern void setSimpleLogLevels();

#ifdef WIN32
/**
 * This sets a flag which tells the logger that alternate source labels should be used to indicate that the current process is a launcher.
 */
extern void setLauncherSource();
#endif

/**
 * Used for testing to set a pause into the next log entry made.
 *
 * @param pauseTime Number of seconds to pause, 0 pauses indefinitely.
 */
extern void setPauseTime(int pauseTime);

/**
 * Set to true to cause changes in internal buffer sizes to be logged.  Useful for debugging.
 *
 * @param log TRUE if changes should be logged.
 */
void setLogBufferGrowth(int log);

extern int getLoggingIsPreload();

extern void setLoggingIsPreload(int value);

/* * Logfile functions * */
extern int isLogfileAccessed();

extern int resolveDefaultLogFilePath();

/**
 * Sets the log file to be used.  If the specified file is not absolute then
 *  it will be resolved into an absolute path.  If there are any problems with
 *  the path, like a directory not existing then the call will fail and the
 *  cause will be written to the existing log.
 *
 * @param log_file_path Log file to start using.
 * @param isConfigured  The value comes from the configuration file.
 *
 * @return TRUE if there were any problems.
 */
extern int setLogfilePath( const TCHAR *log_file_path, int isConfigured);

/**
 * Returns the default logfile.
 */
extern const TCHAR *getDefaultLogfilePath();

/**
 * Returns a reference to the currect log file path.
 *  This return value may be changed at any time if the log file is rolled.
 */
extern const TCHAR *getLogfilePath();

/**
 * Returns a snapshot of the current log file path.  This call safely gets the current path
 *  and returns a copy.  It is the responsibility of the caller to free up the memory on
 *  return.  Could return null if there was an error.
 */
extern TCHAR *getCurrentLogfilePath();

/**
 * Check the directory of the current logfile path to make sure it is writable.
 *  If there are any problems, log a warning.
 *
 * @return TRUE if there were any problems.
 */
extern int checkLogfileDir();

extern int getLogfileRollModeForName( const TCHAR *logfileRollName );
extern void setLogfileRollMode(int log_file_roll_mode);
extern int getLogfileRollMode();
extern void setLogfileUmask(int log_file_umask);
extern void setLogfileFormat( const TCHAR *log_file_format );
extern void setLogfileLevelInt(int log_file_level);
extern int getLogfileLevelInt();
extern void setLogfileLevel( const TCHAR *log_file_level );
extern void setLogfileMaxFileSize( const TCHAR *max_file_size );
extern void setLogfileMaxLogFiles(int max_log_files);
extern void setLogfilePurgePattern(const TCHAR *pattern);
extern void setLogfilePurgeSortMode(int sortMode);
extern DWORD getLogfileActivity();

/** Sets the auto flush log file flag. */
extern void setLogfileAutoFlush(int autoFlush);

/** Sets the auto close log file flag. */
extern void setLogfileAutoClose(int autoClose);

/** Closes the logfile if it is open. */
extern void closeLogfile();

/** Flushes any buffered logfile output to the disk. */
extern void flushLogfile();

/* * Console functions * */
extern void setConsoleLogFormat( const TCHAR *console_log_format );
extern void setConsoleLogLevelInt(int console_log_level);
extern int getConsoleLogLevelInt();
extern void setConsoleLogLevel( const TCHAR *console_log_level );
extern void setConsoleFlush(int flush);
#ifdef WIN32
extern void setConsoleDirect(int direct);
#endif
extern void setConsoleFatalToStdErr(int toStdErr);
extern void setConsoleErrorToStdErr(int toStdErr);
extern void setConsoleWarnToStdErr(int toStdErr);

/* * Syslog/eventlog functions * */
extern void setSyslogLevelInt(int loginfo_level);
extern int getSyslogLevelInt();
extern void setSyslogLevel( const TCHAR *loginfo_level );
extern void setSyslogSplitMessages(int splitMessages);
#ifdef WIN32
extern void setSyslogRegister(int sysRegister);
extern int getSyslogRegister();
extern TCHAR* getSyslogEventSourceName();
#endif
#ifndef WIN32
extern void setSyslogFacility( const TCHAR *loginfo_level );
#endif
extern void setSyslogEventSourceName( const TCHAR *event_source_name );
extern void setThreadMessageBufferInitialSize(int initialValue);
extern void disableSysLog(int silent);
extern int syslogMessageFileRegistered(int silent);
extern int registerSyslogMessageFile(int forceInstall, int silent);
extern int unregisterSyslogMessageFile(int silent);


extern void resetDuration();

extern int getLowLogLevel();

/* * General log functions * */
extern int initLogging(void (*logFileChanged)(const TCHAR *logFile));
extern int disposeLogging();
extern void setUptime(int uptime, int flipped);
extern void rollLogs();
extern int getLogLevelForName( const TCHAR *logLevelName );
#ifndef WIN32
extern int getLogFacilityForName( const TCHAR *logFacilityName );
#endif
extern void logRegisterThread(int thread_id);
extern void logRegisterFormatCallbacks(int (*countCallback)(const TCHAR format, size_t *reqSize), 
                                       int (*printCallback)(const TCHAR format, size_t printSize, TCHAR** pBuffer));

/**
 * The log_printf function logs a message to the configured log targets.
 *
 * This method can be used safely in most cases.  See the log_printf_queue
 *  funtion for the exceptions.
 */
extern void log_printf( int source_id, int level, const TCHAR *lpszFmt, ... );

/**
 * The log_printf_queue function is less efficient than the log_printf
 *  function and will cause logged messages to be logged out of order from
 *  those logged with log_printf because the messages are queued and then
 *  logged from another thread.
 *
 * Use of this function is required in cases where the thread may possibly
 *  be a signal callback.  In these cases, it is possible for the original
 *  thread to have been suspended within a log_printf call.  If the signal
 *  thread then attempted to call log_printf, it would result in a deadlock.
 */
extern void log_printf_queue( int useQueue, int source_id, int level, const TCHAR *lpszFmt, ... );

extern void maintainLogger();
extern void invalidMultiByteSequence(const TCHAR *context, int id);

#ifdef WIN32
extern void setLogSysLangId(int id);
#endif
#endif
