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

#ifndef WIN32

#ifdef LINUX
 #include <features.h>
#endif
#include <limits.h>
#include <wchar.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <pthread.h>
#include <pwd.h>
#include <sys/resource.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "wrapper_i18n.h"
#include "wrapper.h"
#include "wrapperinfo.h"
#include "property.h"
#include "logger.h"
#include "wrapper_file.h"

#include <sys/resource.h>
#include <sys/time.h>

#if defined(IRIX)
 #define PATH_MAX FILENAME_MAX
#endif

#ifndef USE_USLEEP
 #include <time.h>
#endif

#ifndef getsid
/* getpid links ok on Linux, but is not defined correctly. */
pid_t getsid(pid_t pid);
#endif

#define max(x,y) (((x) > (y)) ? (x) : (y))
#define min(x,y) (((x) < (y)) ? (x) : (y))


/* Define a global pipe descriptor so that we don't have to keep allocating
 *  a new pipe each time a JVM is launched. */
int pipedes[2] = {-1, -1};
#define PIPE_READ_END 0
#define PIPE_WRITE_END 1

/**
 * maximum length for a user name should be 8, 
 * but according to 'man useradd' it may be 32
 */
#define MAX_USER_NAME_LENGTH 32

TCHAR wrapperClasspathSeparator = TEXT(':');

int javaIOThreadSet = FALSE;
pthread_t javaIOThreadId;
int javaIOThreadStarted = FALSE;
int stopJavaIOThread = FALSE;
int javaIOThreadStopped = FALSE;

int timerThreadSet = FALSE;
pthread_t timerThreadId;
int timerThreadStarted = FALSE;
int stopTimerThread = FALSE;
int timerThreadStopped = FALSE;

TICKS timerTicks = WRAPPER_TICK_INITIAL;

/******************************************************************************
 * Platform specific methods
 *****************************************************************************/

/**
 * exits the application after running shutdown code.
 */
void appExit(int exitCode, int argc, TCHAR** argv) {
    int i;
    /* Remove pid file.  It may no longer exist. */
    if (wrapperData->pidFilename) {
        _tunlink(wrapperData->pidFilename);
    }

    /* Remove lock file.  It may no longer exist. */
    if (wrapperData->lockFilename) {
        _tunlink(wrapperData->lockFilename);
    }

    /* Remove status file.  It may no longer exist. */
    if (wrapperData->statusFilename) {
        _tunlink(wrapperData->statusFilename);
    }

    /* Remove java status file if it was registered and created by this process. */
    if (wrapperData->javaStatusFilename) {
        _tunlink(wrapperData->javaStatusFilename);
    }

    /* Remove java id file if it was registered and created by this process. */
    if (wrapperData->javaIdFilename) {
        _tunlink(wrapperData->javaIdFilename);
    }

    /* Remove anchor file.  It may no longer exist. */
    if (wrapperData->anchorFilename) {
        _tunlink(wrapperData->anchorFilename);
    }

    /* Common wrapper cleanup code. */
    wrapperDispose();
#if defined(UNICODE)
    for (i = 0; i < argc; i++) {
        if (argv[i]) {
            free(argv[i]);
        }
    }
    if (argv) {
        free(argv);
    }
#endif
    exit(exitCode);
}

/**
 * Writes a PID to disk.
 *
 * filename: File to write to.
 * pid: pid to write in the file.
 * strict: If true then an error will be reported and the call will fail if the
 *         file already exists.
 *
 * return 1 if there was an error, 0 if Ok.
 */
int writePidFile(const TCHAR *filename, DWORD pid, int newUmask, int strict) {
    FILE *pid_fp = NULL;
    int old_umask;

    if (strict && wrapperFileExists(filename)) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("%d pid file, %s, already exists."), pid, filename);
        return 1;
    }

    old_umask = umask(newUmask);
    pid_fp = _tfopen(filename, TEXT("w"));
    umask(old_umask);

    if (pid_fp != NULL) {
        _ftprintf(pid_fp, TEXT("%d\n"), (int)pid);
        fclose(pid_fp);
    } else {
        return 1;
    }
    return 0;
}

/**
 * Send a signal to the JVM process asking it to dump its JVM state.
 */
void wrapperRequestDumpJVMState() {
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
        TEXT("Dumping JVM state."));
    if (kill(wrapperData->javaPID, SIGQUIT) < 0) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                   TEXT("Could not dump JVM state: %s"), getLastErrorText());
    }
}

/**
 * Called when a signal is processed.  This is actually called from within the main event loop
 *  and NOT the signal handler.  So it is safe to use the normal logging functions.
 *
 * @param sigNum Signal that was fired.
 * @param sigName Name of the signal for logging.
 * @param mode Action that should be taken.
 */
void takeSignalAction(int sigNum, const TCHAR *sigName, int mode) {
    if (wrapperData->ignoreSignals & WRAPPER_IGNORE_SIGNALS_WRAPPER) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
            TEXT("%s trapped, but ignored."), sigName);
    } else {
        switch (mode) {
        case WRAPPER_SIGNAL_MODE_RESTART:
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                TEXT("%s trapped.  %s"), sigName, wrapperGetRestartProcessMessage());
            wrapperRestartProcess();
            break;

        case WRAPPER_SIGNAL_MODE_SHUTDOWN:
            if (wrapperData->exitRequested || wrapperData->restartRequested ||
                (wrapperData->jState == WRAPPER_JSTATE_DOWN_CLEAN) ||
                (wrapperData->jState == WRAPPER_JSTATE_STOP) ||
                (wrapperData->jState == WRAPPER_JSTATE_STOPPING) ||
                (wrapperData->jState == WRAPPER_JSTATE_STOPPED) ||
                (wrapperData->jState == WRAPPER_JSTATE_KILLING) ||
                (wrapperData->jState == WRAPPER_JSTATE_KILL) ||
                (wrapperData->jState == WRAPPER_JSTATE_DOWN_CHECK) ||
                (wrapperData->jState == WRAPPER_JSTATE_DOWN_FLUSH)) {

                /* Signaled while we were already shutting down. */
                if (wrapperData->isForcedShutdownDisabled) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                        TEXT("%s trapped.  Already shutting down."), sigName);
                } else {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                        TEXT("%s trapped.  Forcing immediate shutdown."), sigName);
    
                    /* Disable the thread dump on exit feature if it is set because it
                     *  should not be displayed when the user requested the immediate exit. */
                    wrapperData->requestThreadDumpOnFailedJVMExit = FALSE;
                    wrapperKillProcess(FALSE);
                }
            } else {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                    TEXT("%s trapped.  Shutting down."), sigName);
                /* Always force the shutdown as this is an external event. */
                wrapperStopProcess(0, TRUE);
            }
            /* Don't actually kill the process here.  Let the application shut itself down */

            /* To make sure that the JVM will not be restarted for any reason,
             *  start the Wrapper shutdown process as well. */
            if ((wrapperData->wState == WRAPPER_WSTATE_STOPPING) ||
                (wrapperData->wState == WRAPPER_WSTATE_STOPPED)) {
                /* Already stopping. */
            } else {
                wrapperSetWrapperState(WRAPPER_WSTATE_STOPPING);
            }
            break;

        case WRAPPER_SIGNAL_MODE_FORWARD:
            if (wrapperData->javaPID > 0) {
                if (wrapperData->isDebugging) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                        TEXT("%s (%d) trapped.  Forwarding to JVM process."), sigName, sigNum);
                }
                if (kill(wrapperData->javaPID, sigNum)) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                        TEXT("Unable to forward %s signal to JVM process.  %s"), sigName, getLastErrorText());
                }
            } else {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                    TEXT("%s trapped.  Unable to forward signal to JVM because it is not running."), sigName);
            }
            break;

        case WRAPPER_SIGNAL_MODE_PAUSE:
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                TEXT("%s trapped.  %s"), sigName, wrapperGetPauseProcessMessage());
            wrapperPauseProcess(WRAPPER_ACTION_SOURCE_CODE_SIGNAL);
            break;

        case WRAPPER_SIGNAL_MODE_RESUME:
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                TEXT("%s trapped.  %s"), sigName, wrapperGetResumeProcessMessage());
            wrapperResumeProcess(WRAPPER_ACTION_SOURCE_CODE_SIGNAL);
            break;

        case WRAPPER_SIGNAL_MODE_CLOSE_LOGFILE:
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                TEXT("%s trapped.  Closing the log file."), sigName);
            flushLogfile();
            closeLogfile();
            break;

        default: /* WRAPPER_SIGNAL_MODE_IGNORE */
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                TEXT("%s trapped, but ignored."), sigName);
            break;
        }
    }
}

/**
 * This function goes through and checks flags for each of several signals to see if they
 *  have been fired since the last time this function was called.  This is the only thread
 *  which will ever clear these flags, but they can be set by other threads within the
 *  signal handlers at ANY time.  So only check the value of each flag once and reset them
 *  immediately to decrease the chance of missing duplicate signals.
 */
void wrapperMaintainSignals() {
    /* SIGINT */
    if (wrapperData->signalInterruptTrapped) {
        wrapperData->signalInterruptTrapped = FALSE;
        
        takeSignalAction(SIGINT, TEXT("INT"), WRAPPER_SIGNAL_MODE_SHUTDOWN);
    }
    
    /* SIGQUIT */
    if (wrapperData->signalQuitTrapped) {
        wrapperData->signalQuitTrapped = FALSE;
        
        if (wrapperData->signalQuitKernel) {
            /* When CTRL+'\' is captured by the terminal driver (in the kernel), SIGQUIT
             *  is sent to the foreground process group of the current session.
             *  Since the JVM and Wrapper processes belong to the same process group,
             *  the JVM would receive the signal twice if we forward it. Instead, just log
             *  a message and let the JVM handle the signal on its own. */
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("Dumping JVM state."));
            wrapperData->signalQuitKernel = FALSE;
        } else {
            wrapperRequestDumpJVMState();
        }
    }
    
    /* SIGCHLD */
    if (wrapperData->signalChildTrapped) {
        wrapperData->signalChildTrapped = FALSE;
        
        wrapperGetProcessStatus(wrapperGetTicks(), TRUE);
    }
    
    /* SIGTERM */
    if (wrapperData->signalTermTrapped) {
        wrapperData->signalTermTrapped = FALSE;
        
        takeSignalAction(SIGTERM, TEXT("TERM"), WRAPPER_SIGNAL_MODE_SHUTDOWN);
    }
    
    /* SIGHUP */
    if (wrapperData->signalHUPTrapped) {
        wrapperData->signalHUPTrapped = FALSE;
        
        takeSignalAction(SIGHUP, TEXT("HUP"), wrapperData->signalHUPMode);
    }
    
    /* SIGUSR1 */
    if (wrapperData->signalUSR1Trapped) {
        wrapperData->signalUSR1Trapped = FALSE;
        
        takeSignalAction(SIGUSR1, TEXT("USR1"), wrapperData->signalUSR1Mode);
    }
    
#ifndef VALGRIND
    /* SIGUSR2 */
    if (wrapperData->signalUSR2Trapped) {
        wrapperData->signalUSR2Trapped = FALSE;
        
        takeSignalAction(SIGUSR2, TEXT("USR2"), wrapperData->signalUSR2Mode);
    }
#endif
}

/**
 * This is called from within signal handlers so NO MALLOCs are allowed here.
 */
const TCHAR* getSignalName(int signo) {
    switch (signo) {
    case SIGALRM:
        return TEXT("SIGALRM");
    case SIGINT:
        return TEXT("SIGINT");
    case SIGKILL:
        return TEXT("SIGKILL");
    case SIGQUIT:
        return TEXT("SIGQUIT");
    case SIGCHLD:
        return TEXT("SIGCHLD");
    case SIGTERM:
        return TEXT("SIGTERM");
    case SIGHUP:
        return TEXT("SIGHUP");
    case SIGUSR1:
        return TEXT("SIGUSR1");
    case SIGUSR2:
        return TEXT("SIGUSR2");
    case SIGSEGV:
        return TEXT("SIGSEGV");
    default:
        return TEXT("UNKNOWN");
    }
}

/**
 * This is called from within signal handlers so NO MALLOCs are allowed here.
 */
const TCHAR* getSignalCodeDesc(int code) {
    switch (code) {
#ifdef SI_USER
    case SI_USER:
        return TEXT("kill, sigsend or raise");
#endif

#ifdef SI_KERNEL
    case SI_KERNEL:
        return TEXT("the kernel");
#endif

    case SI_QUEUE:
        return TEXT("sigqueue");

#ifdef SI_TIMER
    case SI_TIMER:
        return TEXT("timer expired");
#endif

#ifdef SI_MESGQ
    case SI_MESGQ:
        return TEXT("mesq state changed");
#endif

    case SI_ASYNCIO:
        return TEXT("AIO completed");

#ifdef SI_SIGIO
    case SI_SIGIO:
        return TEXT("queued SIGIO");
#endif

    default:
        return TEXT("unknown");
    }
}

/**
 * Describe a signal.  This is called from within signal handlers so NO MALLOCs are allowed here.
 */
void descSignal(siginfo_t *sigInfo) {
#ifdef SI_USER
    struct passwd *pw;
 #ifdef UNICODE
    size_t req;
 #endif
    TCHAR uName[MAX_USER_NAME_LENGTH + 1];
#endif

    /* Not supported on all platforms */
    if (sigInfo == NULL) {
        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
            TEXT("Signal trapped.  No details available."));
        return;
    }

    if (wrapperData->isDebugging) {
        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
            TEXT("Signal trapped.  Details:"));

        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
#if defined(UNICODE)
            TEXT("  signal number=%d (%S), source=\"%S\""),
#else
            TEXT("  signal number=%d (%s), source=\"%s\""),
#endif
            sigInfo->si_signo,
            getSignalName(sigInfo->si_signo),
            getSignalCodeDesc(sigInfo->si_code));

        if (sigInfo->si_errno != 0) {
            log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
#if defined(UNICODE)
                TEXT("  signal err=%d, \"%S\""),
#else
                TEXT("  signal err=%d, \"%s\""),
#endif
                sigInfo->si_errno,
                strerror(sigInfo->si_errno));
        }
            
#ifdef SI_USER
        if (sigInfo->si_code == SI_USER) {
            pw = getpwuid(sigInfo->si_uid);
            if (pw == NULL) {
                _sntprintf(uName, MAX_USER_NAME_LENGTH + 1, TEXT("<unknown>"));
            } else {
 #ifndef UNICODE
                _sntprintf(uName, MAX_USER_NAME_LENGTH + 1, TEXT("%s"), pw->pw_name);
 #else
                req = mbstowcs(NULL, pw->pw_name, MBSTOWCS_QUERY_LENGTH);
                if (req == (size_t)-1) {
                    return;
                }
                if (req > MAX_USER_NAME_LENGTH) {
                    req = MAX_USER_NAME_LENGTH;
                }
                mbstowcs(uName, pw->pw_name, req + 1);
                uName[req] = TEXT('\0'); /* Avoid bufferflows caused by badly encoded characters. */
 #endif
            }

            /* It appears that the getsid function was added in version 1.3.44 of the linux kernel. */
            log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
 #ifdef UNICODE
                TEXT("  signal generated by PID: %d (Session PID: %d), UID: %d (%S)"),
 #else
                TEXT("  signal generated by PID: %d (Session PID: %d), UID: %d (%s)"),
 #endif
                sigInfo->si_pid, getsid(sigInfo->si_pid), sigInfo->si_uid, uName);
        }
#endif
    }
}

/**
 * Handle alarm signals.  We are getting them on solaris when running with
 *  the tick timer.  Not yet sure where they are coming from.
 */
void sigActionAlarm(int sigNum, siginfo_t *sigInfo, void *na) {
    pthread_t threadId;

    /* On UNIX the calling thread is the actual thread being interrupted
     *  so it has already been registered with logRegisterThread. */

    descSignal(sigInfo);

    threadId = pthread_self();

    if (wrapperData->isDebugging) {
        if (timerThreadSet && pthread_equal(threadId, timerThreadId)) {
            log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                TEXT("Timer thread received an Alarm signal.  Ignoring."));
        } else {
            log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                TEXT("Received an Alarm signal.  Ignoring."));
        }
    }
}

/**
 * Handle interrupt signals (i.e. Crtl-C).
 */
void sigActionInterrupt(int sigNum, siginfo_t *sigInfo, void *na) {
    /* On UNIX the calling thread is the actual thread being interrupted
     *  so it has already been registered with logRegisterThread. */

    descSignal(sigInfo);

    wrapperData->signalInterruptTrapped = TRUE;
}

/**
 * Handle quit signals (i.e. Crtl-\).
 */
void sigActionQuit(int sigNum, siginfo_t *sigInfo, void *na) {
    pthread_t threadId;

    /* On UNIX the calling thread is the actual thread being interrupted
     *  so it has already been registered with logRegisterThread. */

    descSignal(sigInfo);

    threadId = pthread_self();

    if (timerThreadSet && pthread_equal(threadId, timerThreadId)) {
        if (wrapperData->isDebugging) {
            log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                TEXT("Timer thread received an Quit signal.  Ignoring."));
        }
    } else {
        wrapperData->signalQuitTrapped = TRUE;
#ifdef SI_KERNEL
        wrapperData->signalQuitKernel = (sigInfo->si_code == SI_KERNEL);
#else
        /* On some platforms we can't know the source of a signal. */
        wrapperData->signalQuitKernel = FALSE;
#endif
    }
}

/**
 * Handle termination signals (i.e. machine is shutting down).
 */
void sigActionChildDeath(int sigNum, siginfo_t *sigInfo, void *na) {
    pthread_t threadId;

    /* On UNIX, when a Child process changes state, a SIGCHLD signal is sent to the parent.
     *  The parent should do a wait to make sure the child is cleaned up and doesn't become
     *  a zombie process. */
                
    threadId = pthread_self();
    if (timerThreadSet && pthread_equal(threadId, timerThreadId)) {
        if (wrapperData->isDebugging) {
            log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                TEXT("Timer thread received a SigChild signal.  Ignoring."));
        }
    } else {
        descSignal(sigInfo);

        if (wrapperData->isDebugging) {
            log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                TEXT("Received SIGCHLD, checking JVM process status."));
        }
        
        /* This is set whenever any child signals that it has exited.
         *  Inside the code we go on to check to make sure that we only test for the JVM */
        wrapperData->signalChildTrapped = TRUE;
    }
}

/**
 * Handle termination signals (i.e. machine is shutting down).
 */
void sigActionTermination(int sigNum, siginfo_t *sigInfo, void *na) {
    /* On UNIX the calling thread is the actual thread being interrupted
     *  so it has already been registered with logRegisterThread. */

    descSignal(sigInfo);
    
    wrapperData->signalTermTrapped = TRUE;
}

/**
 * Handle hangup signals.
 */
void sigActionHangup(int sigNum, siginfo_t *sigInfo, void *na) {
    /* On UNIX the calling thread is the actual thread being interrupted
     *  so it has already been registered with logRegisterThread. */

    descSignal(sigInfo);
    
    wrapperData->signalHUPTrapped = TRUE;
}

/**
 * Handle USR1 signals.
 */
void sigActionUSR1(int sigNum, siginfo_t *sigInfo, void *na) {
    /* On UNIX the calling thread is the actual thread being interrupted
     *  so it has already been registered with logRegisterThread. */

    descSignal(sigInfo);
    
    wrapperData->signalUSR1Trapped = TRUE;
}

/**
 * Handle USR2 signals.
 */
void sigActionUSR2(int sigNum, siginfo_t *sigInfo, void *na) {
    /* On UNIX the calling thread is the actual thread being interrupted
     *  so it has already been registered with logRegisterThread. */

    descSignal(sigInfo);
    
    wrapperData->signalUSR2Trapped = TRUE;
}

/**
 * Registers a single signal handler.
 */
int registerSigAction(int sigNum, void (*sigAction)(int, siginfo_t *, void *)) {
    struct sigaction newAct;

    newAct.sa_sigaction = sigAction;
    sigemptyset(&newAct.sa_mask);
    newAct.sa_flags = SA_SIGINFO;

    if (sigaction(sigNum, &newAct, NULL)) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
            TEXT("Unable to register signal handler for signal %d.  %s"), sigNum, getLastErrorText());
        return 1;
    }
    return 0;
}

/**
 * The main entry point for the javaio thread which is started by
 *  initializeJavaIO().  Once started, this thread will run for the
 *  life of the process.
 *
 * This thread will only be started if we are configured to use a
 *  dedicated thread to read JVM output.
 */
void *javaIORunner(void *arg) {
    sigset_t signal_mask;
    int nextSleep;
#ifndef VALGRIND
    int rc;
#endif

    javaIOThreadStarted = TRUE;
    
    /* Immediately register this thread with the logger. */
    logRegisterThread(WRAPPER_THREAD_JAVAIO);

    /* mask signals so the javaIO doesn't get any of these. */
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGTERM);
    sigaddset(&signal_mask, SIGINT);
    sigaddset(&signal_mask, SIGQUIT);
    sigaddset(&signal_mask, SIGALRM);
    sigaddset(&signal_mask, SIGHUP);
    sigaddset(&signal_mask, SIGUSR1);
#ifndef VALGRIND
    sigaddset(&signal_mask, SIGUSR2);
    rc = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
    if (rc != 0) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("Could not mask signals for javaIO thread."));
    }
#endif

    if (wrapperData->isJavaIOOutputEnabled) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("JavaIO thread started."));
    }

    nextSleep = TRUE;
    /* Loop until we are shutting down, but continue as long as there is more output from the JVM. */
    while ((!stopJavaIOThread) || (!nextSleep)) {
        if (nextSleep) {
            /* Sleep as little as possible. */
            wrapperSleep(1);
        }
        nextSleep = TRUE;
        
        if (wrapperData->pauseThreadJavaIO) {
            wrapperPauseThread(wrapperData->pauseThreadJavaIO, TEXT("javaio"));
            wrapperData->pauseThreadJavaIO = 0;
        }
        
        if (wrapperReadChildOutput(0)) {
            if (wrapperData->isDebugging) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                    TEXT("Pause reading child process output to share cycles."));
            }
            nextSleep = FALSE;
        }
    }

    javaIOThreadStopped = TRUE;
    if (wrapperData->isJavaIOOutputEnabled) {
        log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("JavaIO thread stopped."));
    }
    return NULL;
}

/**
 * Creates a process whose job is to loop and simply increment a ticks
 *  counter.  The tick counter can then be used as a clock as an alternative
 *  to using the system clock.
 */
int initializeJavaIO() {
    int res;

    if (wrapperData->isJavaIOOutputEnabled) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("Launching JavaIO thread."));
    }

    res = pthread_create(&javaIOThreadId,
        NULL, /* No attributes. */
        javaIORunner,
        NULL); /* No parameters need to be passed to the thread. */
    if (res) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
            TEXT("Unable to create a javaIO thread: %d, %s"), res, getLastErrorText());
        javaIOThreadSet = TRUE;
        return 1;
    } else {
        if (pthread_detach(javaIOThreadId)) {
            javaIOThreadSet = TRUE;
            return 1;
        }
        javaIOThreadSet = FALSE;
        return 0;
    }
}

void disposeJavaIO() {
    stopJavaIOThread = TRUE;
    /* Wait until the javaIO thread is actually stopped to avoid timing problems. */
    if (javaIOThreadStarted) {
        while (!javaIOThreadStopped) {
#ifdef _DEBUG
            wprintf(TEXT("Waiting for javaIO thread to stop.\n"));
#endif
            wrapperSleep(100);
        }
        pthread_cancel(javaIOThreadId);
    }
}

/**
 * The main entry point for the timer thread which is started by
 *  initializeTimer().  Once started, this thread will run for the
 *  life of the process.
 *
 * This thread will only be started if we are configured NOT to
 *  use the system time as a base for the tick counter.
 */
void *timerRunner(void *arg) {
    TICKS sysTicks;
    TICKS lastTickOffset = 0;
    TICKS tickOffset;
    TICKS nowTicks;
    int offsetDiff;
    int first = TRUE;
    sigset_t signal_mask;
#ifndef VALGRIND
    int rc;
#endif

    timerThreadStarted = TRUE;
    
    /* Immediately register this thread with the logger. */
    logRegisterThread(WRAPPER_THREAD_TIMER);

    /* mask signals so the timer doesn't get any of these. */
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGTERM);
    sigaddset(&signal_mask, SIGINT);
    sigaddset(&signal_mask, SIGQUIT);
    sigaddset(&signal_mask, SIGALRM);
    sigaddset(&signal_mask, SIGHUP);
    sigaddset(&signal_mask, SIGUSR1);
#ifndef VALGRIND
    sigaddset(&signal_mask, SIGUSR2);
    rc = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
    if (rc != 0) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("Could not mask signals for timer thread."));
    }
#endif

    if (wrapperData->isTickOutputEnabled) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("Timer thread started."));
    }

    wrapperGetSystemTicks();

    while (!stopTimerThread) {
        wrapperSleep(WRAPPER_TICK_MS);
        
        if (wrapperData->pauseThreadTimer) {
            wrapperPauseThread(wrapperData->pauseThreadTimer, TEXT("timer"));
            wrapperData->pauseThreadTimer = 0;
        }

        /* Get the tick count based on the system time. */
        sysTicks = wrapperGetSystemTicks();

        /* Lock the tick mutex whenever the "timerTicks" variable is accessed. */
        if (wrapperData->useTickMutex && wrapperLockTickMutex()) {
            timerThreadStopped = TRUE;
            return NULL;
        }
        
        /* Advance the timer tick count. */
        nowTicks = timerTicks++;
        
        if (wrapperData->useTickMutex && wrapperReleaseTickMutex()) {
            timerThreadStopped = TRUE;
            return NULL;
        }

        /* Calculate the offset between the two tick counts. This will always work due to overflow. */
        tickOffset = sysTicks - nowTicks;

        /* The number we really want is the difference between this tickOffset and the previous one. */
        offsetDiff = wrapperGetTickAgeTicks(lastTickOffset, tickOffset);

        if (first) {
            first = FALSE;
        } else {
            if (offsetDiff > wrapperData->timerSlowThreshold) {
                log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_INFO,
                    TEXT("The timer fell behind the system clock by %ldms."), offsetDiff * WRAPPER_TICK_MS);
            } else if (offsetDiff < -1 * wrapperData->timerFastThreshold) {
                log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_INFO,
                    TEXT("The system clock fell behind the timer by %ldms."), -1 * offsetDiff * WRAPPER_TICK_MS);
            }

            if (wrapperData->isTickOutputEnabled) {
                log_printf_queue(TRUE, WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT(
                    "    Timer: ticks=0x%08x, system ticks=0x%08x, offset=0x%08x, offsetDiff=0x%08x"),
                    nowTicks, sysTicks, tickOffset, offsetDiff);
            }
        }

        /* Store this tick offset for the next time through the loop. */
        lastTickOffset = tickOffset;
    }

    timerThreadStopped = TRUE;
    if (wrapperData->isTickOutputEnabled) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("Timer thread stopped."));
    }
    return NULL;
}

/**
 * Creates a process whose job is to loop and simply increment a ticks
 *  counter.  The tick counter can then be used as a clock as an alternative
 *  to using the system clock.
 */
int initializeTimer() {
    int res;

    if (wrapperData->isTickOutputEnabled) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("Launching Timer thread."));
    }

    res = pthread_create(&timerThreadId,
        NULL, /* No attributes. */
        timerRunner,
        NULL); /* No parameters need to be passed to the thread. */
    if (res) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
            TEXT("Unable to create a timer thread: %d, %s"), res, getLastErrorText());
        timerThreadSet = TRUE;
        return 1;
    } else {
        if (pthread_detach(timerThreadId)) {
            timerThreadSet = TRUE;
            return 1;
        }
        timerThreadSet = FALSE;
        return 0;
    }
}

void disposeTimer() {
    stopTimerThread = TRUE;
    /* Wait until the timer thread is actually stopped to avoid timing problems. */
    if (timerThreadStarted) {
        while (!timerThreadStopped) {
#ifdef _DEBUG
            wprintf(TEXT("Waiting for timer thread to stop.\n"));
#endif
            wrapperSleep(100);
        }
        pthread_cancel(timerThreadId);
    }
}

/**
 * Execute initialization code to get the wrapper set up.
 */
int wrapperInitializeRun() {
    int retval = 0;
    int res;

    /* Register any signal actions we are concerned with. */
    if (registerSigAction(SIGALRM, sigActionAlarm) ||
        registerSigAction(SIGINT,  sigActionInterrupt) ||
        registerSigAction(SIGQUIT, sigActionQuit) ||
        registerSigAction(SIGCHLD, sigActionChildDeath) ||
        registerSigAction(SIGTERM, sigActionTermination) ||
        registerSigAction(SIGHUP,  sigActionHangup) ||
        registerSigAction(SIGUSR1, sigActionUSR1)
#ifndef VALGRIND
        ||
        registerSigAction(SIGUSR2, sigActionUSR2)
#endif
        ) {
        retval = -1;
    }

    /* Attempt to set the console title if it exists and is accessable.
     *  This works on all UNIX versions, but only Linux resets it
     *  correctly when the wrapper process terminates. */
#if defined(LINUX)
    if (wrapperData->consoleTitle) {
        if (wrapperData->isConsole) {
            /* The console should be visible. */
            _tprintf(TEXT("%c]0;%s%c"), TEXT('\033'), wrapperData->consoleTitle, TEXT('\007'));
        }
    }
#endif

    if (wrapperData->useSystemTime) {
        /* We are going to be using system time so there is no reason to start up a timer thread. */
        timerThreadSet = FALSE;
        /* Unable to set the timerThreadId to a null value on all platforms
         * timerThreadId = 0;*/
    } else {
        /* Create and initialize a timer thread. */
        if ((res = initializeTimer()) != 0) {
            return res;
        }
    }
    
    if (wrapperData->useJavaIOThread) {
        /* Create and initialize a javaIO thread. */
        if ((res = initializeJavaIO()) != 0) {
            return res;
        }
    } else {
        javaIOThreadSet = FALSE;
        /* Unable to set the javaIOThreadId to a null value on all platforms
         * javaIOThreadId = 0;*/
    }

    return retval;
}

/**
 * Cause the current thread to sleep for the specified number of milliseconds.
 *  Sleeps over one second are not allowed.
 *
 * @param ms Number of milliseconds to wait for.
 *
 * @return TRUE if the was interrupted, FALSE otherwise.  Neither is an error.
 */
int wrapperSleep(int ms) {
    /* We want to use nanosleep if it is available, but make it possible for the
       user to build a version that uses usleep if they want.
       usleep does not behave nicely with signals thrown while sleeping.  This
       was the believed cause of a hang experienced on one Solaris system. */
#ifdef USE_USLEEP
    if (wrapperData->isSleepOutputEnabled) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
            TEXT("    Sleep: usleep %dms"), ms);
    }
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

    if (wrapperData->isSleepOutputEnabled) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("    Sleep: nanosleep %dms"), ms);
    }
    if (nanosleep(&ts, NULL)) {
        if (errno == EINTR) {
            if (wrapperData->isSleepOutputEnabled) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                    TEXT("    Sleep: nanosleep interrupted"));
            }
            return TRUE;
        } else if (errno == EAGAIN) {
            /* On 64-bit AIX this happens once on shutdown. */
            if (wrapperData->isSleepOutputEnabled) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                    TEXT("    Sleep: nanosleep unavailable"));
            }
            return TRUE;
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                TEXT("nanosleep(%dms) failed. %s"), ms, getLastErrorText());
        }
    }
#endif

    if (wrapperData->isSleepOutputEnabled) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("    Sleep: awake"));
    }
    
    return FALSE;
}

/**
 * Detaches the Java process so the Wrapper will if effect forget about it.
 */
void wrapperDetachJava() {
    wrapperSetJavaState(WRAPPER_JSTATE_DOWN_CLEAN, 0, -1);
    
    /* Leave the stdout/stderr pipe in pipedes alone so we grab any remaining output.
     *  They should have been redirected on the JVM side anyway. */
}


/**
 * Build the java command line.
 *
 * @return TRUE if there were any problems.
 */
int wrapperBuildJavaCommand() {
    TCHAR **strings;
    int length, i;

    /* If this is not the first time through, then dispose the old command array */
    if (wrapperData->jvmVersionCommand) {
        i = 0;
        while(wrapperData->jvmVersionCommand[i] != NULL) {
            free(wrapperData->jvmVersionCommand[i]);
            wrapperData->jvmVersionCommand[i] = NULL;
            i++;
        }

        free(wrapperData->jvmVersionCommand);
        wrapperData->jvmVersionCommand = NULL;
    }
    if (wrapperData->jvmCommand) {
        i = 0;
        while(wrapperData->jvmCommand[i] != NULL) {
            free(wrapperData->jvmCommand[i]);
            wrapperData->jvmCommand[i] = NULL;
            i++;
        }

        free(wrapperData->jvmCommand);
        wrapperData->jvmCommand = NULL;
    }

    /* First generate the classpath. */
    if (wrapperData->classpath) {
        free(wrapperData->classpath);
        wrapperData->classpath = NULL;
    }
    if (wrapperBuildJavaClasspath(&wrapperData->classpath) < 0) {
        return TRUE;
    }

    /* Build the Java Command Strings */
    strings = NULL;
    length = 0;
    if (wrapperBuildJavaCommandArray(&strings, &length, FALSE, wrapperData->classpath)) {
        return TRUE;
    }

    /* Allocate memory to hold array of version command strings.  The array is itself NULL terminated */
    wrapperData->jvmVersionCommand = malloc(sizeof(TCHAR *) * (2 + 1));
    if (!wrapperData->jvmVersionCommand) {
        outOfMemory(TEXT("WBJC"), 1);
        return TRUE;
    }
    memset(wrapperData->jvmVersionCommand, 0, sizeof(TCHAR *) * (2 + 1));
    /* Java Command */
    wrapperData->jvmVersionCommand[0] = malloc(sizeof(TCHAR) * (_tcslen(strings[0]) + 1));
    if (!wrapperData->jvmVersionCommand[0]) {
        outOfMemory(TEXT("WBJC"), 2);
        return TRUE;
    }
    _tcsncpy(wrapperData->jvmVersionCommand[0], strings[0], _tcslen(strings[0]) + 1);
    /* -version */
    wrapperData->jvmVersionCommand[1] = malloc(sizeof(TCHAR) * (8 + 1));
    if (!wrapperData->jvmVersionCommand[1]) {
        outOfMemory(TEXT("WBJC"), 3);
        return TRUE;
    }
    _tcsncpy(wrapperData->jvmVersionCommand[1], TEXT("-version"), 8 + 1);
    /* NULL */
    wrapperData->jvmVersionCommand[2] = NULL;
    
    /* Allocate memory to hold array of command strings.  The array is itself NULL terminated */
    wrapperData->jvmCommand = malloc(sizeof(TCHAR *) * (length + 1));
    if (!wrapperData->jvmCommand) {
        outOfMemory(TEXT("WBJC"), 1);
        return TRUE;
    }
    memset(wrapperData->jvmCommand, 0, sizeof(TCHAR *) * (length + 1));
    /* number of arguments + 1 for a NULL pointer at the end */
    for (i = 0; i <= length; i++) {
        if (i < length) {
            wrapperData->jvmCommand[i] = malloc(sizeof(TCHAR) * (_tcslen(strings[i]) + 1));
            if (!wrapperData->jvmCommand[i]) {
                outOfMemory(TEXT("WBJC"), 2);
                return TRUE;
            }
            _tcsncpy(wrapperData->jvmCommand[i], strings[i], _tcslen(strings[i]) + 1);
            
            wrapperData->jvmCommand[i] = wrapperPostProcessCommandElement(wrapperData->jvmCommand[i]);
        } else {
            wrapperData->jvmCommand[i] = NULL;
        }
    }

    /* Free up the temporary command array */
    wrapperFreeJavaCommandArray(strings, length);

    return FALSE;
}

/**
 * Calculate the total length of the environment assuming that they are separated by spaces.
 */
size_t wrapperCalculateEnvironmentLength() {
    /* The compiler won't let us reference environ directly in the for loop on OSX because it is actually a function. */
    char **environment = environ;
    size_t i;
    size_t len;
    size_t lenTotal;
    
    i = 0;
    lenTotal = 0;
    while (environment[i]) {
        /* All we need is the length so we don't actually have to convert them. */
        len = mbstowcs(NULL, environment[i], MBSTOWCS_QUERY_LENGTH);
        if (len == (size_t)-1) {
            /* Invalid string.  Skip. */
        } else {
            /* Add length of variable + null + pointer to next element */
            lenTotal += len + 1 + sizeof(char *);
        }
        i++;
    }
    /* Null termination of the list. */
    lenTotal += sizeof(char *) + sizeof(char *);
    
    return lenTotal;
}

/**
 * Create a child process to print the Java version running the command:
 *    /path/to/java -version
 *  After printing the java version, the process is terminated.
 * 
 * In case the JVM is slow to start, it will time out after
 * the number of seconds set in "wrapper.java.version.timeout".
 * 
 * Note: before the timeout is reached, the user can ctrl+c to stop the Wrapper.
 */
void launchChildProcessPrintJavaVersion() {
    int blockTimeout;      /* max time (in ms) to wait for the child process to terminate */
    int result;            /* result of waitpid */
    int status;            /* status of child process */
    pid_t procJavaVersion; /* pid of the child process */
    
    /* Create the child process! */
    procJavaVersion = fork();
                
    if (procJavaVersion >= 0) {
        /* fork successful */
        
        if (procJavaVersion == 0) {
            /* in the child process */
            TCHAR *javaVersionArgv[3];
            javaVersionArgv[0] = wrapperData->jvmCommand[0];
            javaVersionArgv[1] = TEXT("-version");
            javaVersionArgv[2] = 0;
            _texecvp(wrapperData->jvmCommand[0], javaVersionArgv);
        } else {
            /* in the parent process */
            /* Note: on CentOS, in case we don't call waitpid(), then the child process becomes a zombie */
            /* Note2: use _tprintf to log messages instead of log_printf. With log_printf, the columns are printed twice */
            
            /* If the user set the value to 0, then we will wait indefinitely. */
            blockTimeout = getIntProperty(properties, TEXT("wrapper.java.version.timeout"), DEFAULT_JAVA_VERSION_TIMEOUT) * 1000;
            
            if (blockTimeout > 0) {
                while (((result = waitpid(procJavaVersion, &status, WNOHANG)) == 0) && (blockTimeout > 0)) {
#ifdef _DEBUG
                    _tprintf(TEXT("Child process: Java version: waiting... result=%d waitpidStatLoc=%d blockTimeout=%d\n"), result, status, blockTimeout);
#endif
                    wrapperSleep(100);
                    blockTimeout -= 100;
                }
            } else {
                /* Wait indefinately. */
                result = waitpid(procJavaVersion, &status, 0);
            }
            
            if (result == 0) {
                /* Timed out. */
                _tprintf(TEXT("Child process: Java version: timed out\n"));
                kill(procJavaVersion, SIGKILL);
            } else if (result > 0) {
                /* Process completed. */
#ifdef _DEBUG
                _tprintf(TEXT("Child process: Java version: successful\n"));
#endif
            } else {
                /* Wait failed. */
                _tprintf(TEXT("Child process: Java version: wait failed\n"));
                kill(procJavaVersion, SIGKILL);
            }
        }
    } else {
        /* Fork failed. */
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("Unable to spawn process to output Java version: %s"), getLastErrorText());
    }
}

/**
 * Launches a JVM process and stores it internally.
 *
 * @return TRUE if there were any problems.  When this happens the Wrapper will not try to restart.
 */
int wrapperExecute() {
    int i;
    pid_t proc;
    int execErrno;
    size_t lenCmd;
    size_t lenEnv;

    /* Create the pipe. */
    if (pipe(pipedes) < 0) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                   TEXT("Could not init pipe: %s"), getLastErrorText());
        return TRUE;
    }
    
    /* Log the Java commands. */
    
    /* If the JVM version printout is requested then log its command line first. */
    if (wrapperData->printJVMVersion) {
        if (wrapperData->isDebugging) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Java Command Line (Query Java Version):"));
            for (i = 0; wrapperData->jvmVersionCommand[i] != NULL; i++) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                    TEXT("  Command[%d] : %s"), i, wrapperData->jvmVersionCommand[i]);
            }
        }
    }
    
    /* Log ghe application java command line */
    if (wrapperData->commandLogLevel != LEVEL_NONE) {
        log_printf(WRAPPER_SOURCE_WRAPPER, wrapperData->commandLogLevel, TEXT("Java Command Line:"));
        for (i = 0; wrapperData->jvmCommand[i] != NULL; i++) {
            log_printf(WRAPPER_SOURCE_WRAPPER, wrapperData->commandLogLevel,
                TEXT("  Command[%d] : %s"), i, wrapperData->jvmCommand[i]);
        }

        if (wrapperData->environmentClasspath) {
            log_printf(WRAPPER_SOURCE_WRAPPER, wrapperData->commandLogLevel,
                TEXT("  Classpath in Environment : %s"), wrapperData->classpath);
        }
    }

    /* Update the CLASSPATH in the environment if requested so the JVM can access it. */ 
    if (wrapperData->environmentClasspath) {
        setEnv(TEXT("CLASSPATH"), wrapperData->classpath, ENV_SOURCE_APPLICATION);
    }

    /* Make sure the log file is closed before the Java process is created.  Failure to do
     *  so will give the Java process a copy of the open file.  This means that this process
     *  will not be able to rename the file even after closing it because it will still be
     *  open in the Java process.  Also set the auto close flag to make sure that other
     *  threads do not reopen the log file as the new process is being created. */
    setLogfileAutoClose(TRUE);
    closeLogfile();
        
    /* Reset the log duration so we get new counts from the time the JVM is launched. */
    resetDuration();
    
    /* Fork off the child. */
    proc = fork();

    if (proc == -1) {
        /* Fork failed. */

        /* Restore the auto close flag. */
        setLogfileAutoClose(wrapperData->logfileCloseTimeout == 0);

        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                   TEXT("Could not spawn JVM process: %s"), getLastErrorText());

        /* The pipedes array is global so do not close the pipes. */
        return TRUE;
    } else {
        /* Reset the exit code when we launch a new JVM. */
        wrapperData->exitCode = 0;

        /* Reset the stopped flag. */
        wrapperData->jvmStopped = FALSE;

        if (proc == 0) {
            /* We are the child side. */

            /* Set the umask of the JVM */
            umask(wrapperData->javaUmask);

            /* The logging code causes some log corruption if logging is called from the
             *  child of a fork.  Not sure exactly why but most likely because the forked
             *  child receives a copy of the mutex and thus synchronization is not working.
             * It is ok to log errors in here, but avoid output otherwise.
             * TODO: Figure out a way to fix this.  Maybe using shared memory? */

            /* Send output to the pipe by dupicating the pipe fd and setting the copy as the stdout fd. */
            if (dup2(pipedes[PIPE_WRITE_END], STDOUT_FILENO) < 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                    TEXT("%sUnable to set JVM's stdout: %s"), LOG_FORK_MARKER, getLastErrorText());
                /* This process needs to end. */
                exit(wrapperData->errorExitCode);
                return TRUE; /* Will not get here. */
            }

            /* Send errors to the pipe by duplicating the pipe fd and setting the copy as the stderr fd. */
            if (dup2(pipedes[PIPE_WRITE_END], STDERR_FILENO) < 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                    TEXT("%sUnable to set JVM's stderr: %s"), LOG_FORK_MARKER, getLastErrorText());
                /* This process needs to end. */
                exit(wrapperData->errorExitCode);
                return TRUE; /* Will not get here. */
            }
            
            /* Close both ends of the pipe as we have already duplicated the Write end for our purposes. */
            close(pipedes[PIPE_READ_END]);
            pipedes[PIPE_READ_END] = -1;
            close(pipedes[PIPE_WRITE_END]);
            pipedes[PIPE_WRITE_END] = -1;

            /* forking at this point, the child process has set all pipes already, so no
               further assignments needed */
            if (wrapperData->printJVMVersion) {
                launchChildProcessPrintJavaVersion();
            }
            
            /* The pipedes array is global so do not close the pipes. */
            /* Child process: execute the JVM. */
            _texecvp(wrapperData->jvmCommand[0], wrapperData->jvmCommand);
            execErrno = errno;

            /* We reached this point...meaning we were unable to start. */
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                TEXT("%sUnable to start JVM: %s (%d)"), LOG_FORK_MARKER, getLastErrorText(), execErrno);
            if (execErrno == E2BIG) {
                /* Command line too long. */
                /* Calculate the total length of the command line. */
                lenCmd = 0;
                for (i = 0; wrapperData->jvmCommand[i] != NULL; i++) {
                    lenCmd += _tcslen(wrapperData->jvmCommand[i]) + 1;
                }
                lenEnv = wrapperCalculateEnvironmentLength();
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("%s  The generated command line plus the environment was larger than the maximum allowed."), LOG_FORK_MARKER);
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("%s  The current length is %d bytes of which %d is the command line, and %d is the environment."), LOG_FORK_MARKER, lenCmd + lenEnv + 1, lenCmd, lenEnv); 
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("%s  It is not possible to calculate an exact maximum length as it depends on a number of factors for each system."), LOG_FORK_MARKER);

                /* TODO: Figure out a way to inform the Wrapper not to restart and try again as repeatedly doing this is meaningless. */
            }


            if (wrapperData->isAdviserEnabled) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE, TEXT("%s"), LOG_FORK_MARKER );
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                    TEXT("%s------------------------------------------------------------------------"), LOG_FORK_MARKER );
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                    TEXT("%sAdvice:"), LOG_FORK_MARKER );
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                    TEXT("%sUsually when the Wrapper fails to start the JVM process, it is because"), LOG_FORK_MARKER );
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                    TEXT("%sof a problem with the value of the configured Java command.  Currently:"), LOG_FORK_MARKER );
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                    TEXT("%swrapper.java.command=%s"), LOG_FORK_MARKER, getStringProperty(properties, TEXT("wrapper.java.command"), TEXT("java")));
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                    TEXT("%sPlease make sure that the PATH or any other referenced environment"), LOG_FORK_MARKER );
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                    TEXT("%svariables are correctly defined for the current environment."), LOG_FORK_MARKER );
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE,
                    TEXT("%s------------------------------------------------------------------------"), LOG_FORK_MARKER );
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE, TEXT("%s"), LOG_FORK_MARKER );
            }

            /* This process needs to end. */
            exit(wrapperData->errorExitCode);
            return TRUE; /* Will not get here. */
        } else {
            /* We are the parent side and need to assume that at this point the JVM is up. */
            wrapperData->javaPID = proc;
            
            /* Close the write end as it is not used. */
            close(pipedes[PIPE_WRITE_END]);
            pipedes[PIPE_WRITE_END] = -1;

            /* Restore the auto close flag. */
            setLogfileAutoClose(wrapperData->logfileCloseTimeout == 0);			

            /* The pipedes array is global so do not close the pipes. */

            /* Mark our side of the pipe so that it won't block
             * and will close on exec, so new children won't see it. */
            if (fcntl(pipedes[PIPE_READ_END], F_SETFL, O_NONBLOCK) < 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                    TEXT("Failed to set JVM output handle to non blocking mode: %s (%d)"),
                    getLastErrorText(), errno);
            }
            if (fcntl(pipedes[PIPE_READ_END], F_SETFD, FD_CLOEXEC) < 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
                    TEXT("Failed to set JVM output handle to close on JVM exit: %s (%d)"),
                    getLastErrorText(), errno);
            }

            /* If a java pid filename is specified then write the pid of the java process. */
            if (wrapperData->javaPidFilename) {
                if (writePidFile(wrapperData->javaPidFilename, wrapperData->javaPID, wrapperData->javaPidFileUmask, FALSE)) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                        TEXT("Unable to write the Java PID file: %s"), wrapperData->javaPidFilename);
                }
            }

            /* If a java id filename is specified then write the Id of the java process. */
            if (wrapperData->javaIdFilename) {
                if (writePidFile(wrapperData->javaIdFilename, wrapperData->jvmRestarts, wrapperData->javaIdFileUmask, FALSE)) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                        TEXT("Unable to write the Java Id file: %s"), wrapperData->javaIdFilename);
                }
            }
            return FALSE;
        }
    }
}

/**
 * Returns a tick count that can be used in combination with the
 *  wrapperGetTickAgeSeconds() function to perform time keeping.
 */
TICKS wrapperGetTicks() {
    TICKS ticks;
    
    if (wrapperData->useSystemTime) {
        /* We want to return a tick count that is based on the current system time. */
        ticks = wrapperGetSystemTicks();

    } else {
        /* Lock the tick mutex whenever the "timerTicks" variable is accessed. */
        if (wrapperData->useTickMutex && wrapperLockTickMutex()) {
            return 0;
        }
        
        /* Return a snapshot of the current tick count. */
        ticks = timerTicks;
        
        if (wrapperData->useTickMutex && wrapperReleaseTickMutex()) {
            return 0;
        }
    }
    
    return ticks;
}


/**
 * Outputs a a log entry describing what the memory dump columns are.
 */
void wrapperDumpMemoryBanner() {
    /* Not yet implemented on UNIX platforms. */
}

/**
 * Outputs a log entry at regular intervals to track the memory usage of the
 *  Wrapper and its JVM.
 */
void wrapperDumpMemory() {
    struct rusage wUsage;
    struct rusage jUsage;

    if (getrusage(RUSAGE_SELF, &wUsage)) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
            TEXT("Call to getrusage failed for Wrapper process: %s"), getLastErrorText());
        return;
    }
    /* The Children is only going to show the value for terminated children. */
    if (getrusage(RUSAGE_CHILDREN, &jUsage)) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
            TEXT("Call to getrusage failed for Java process: %s"), getLastErrorText());
        return;
    }

    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
        TEXT("Wrapper Memory: maxrss=%ld, ixrss=%ld, idrss=%ld, isrss=%ld, minflt=%ld, majflt=%ld, nswap=%ld, inblock=%ld, oublock=%ld, msgsnd=%ld, msgrcv=%ld, nsignals=%ld, nvcsw=%ld, nvcsw=%ld"),
        wUsage.ru_maxrss,
        wUsage.ru_ixrss,
        wUsage.ru_idrss,
        wUsage.ru_isrss,
        wUsage.ru_minflt,
        wUsage.ru_majflt,
        wUsage.ru_nswap,
        wUsage.ru_inblock,
        wUsage.ru_oublock,
        wUsage.ru_msgsnd,
        wUsage.ru_msgrcv,
        wUsage.ru_nsignals,
        wUsage.ru_nvcsw,
        wUsage.ru_nvcsw);
}

/**
 * Outputs a log entry at regular intervals to track the CPU usage over each
 *  interval for the Wrapper and its JVM.
 */
void wrapperDumpCPUUsage() {
    struct rusage wUsage;
    struct rusage jUsage;

    if (getrusage(RUSAGE_SELF, &wUsage)) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
            TEXT("Call to getrusage failed for Wrapper process: %s"), getLastErrorText());
        return;
    }
    /* The Children is only going to show the value for terminated children. */
    if (getrusage(RUSAGE_CHILDREN, &jUsage)) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR,
            TEXT("Call to getrusage failed for Java process: %s"), getLastErrorText());
        return;
    }

    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
        TEXT("Wrapper CPU: system %ld.%03ld, user %ld.%03ld  Java CPU: system %ld.%03ld, user %ld.%03ld"),
        wUsage.ru_stime.tv_sec, wUsage.ru_stime.tv_usec / 1000,
        wUsage.ru_utime.tv_sec, wUsage.ru_utime.tv_usec / 1000,
        jUsage.ru_stime.tv_sec, jUsage.ru_stime.tv_usec / 1000,
        jUsage.ru_utime.tv_sec, jUsage.ru_utime.tv_usec / 1000);
}

/**
 * Checks on the status of the JVM Process.
 * Returns WRAPPER_PROCESS_UP or WRAPPER_PROCESS_DOWN
 */
int wrapperGetProcessStatus(TICKS nowTicks, int sigChild) {
    int retval;
    int status;
    int exitCode;
    int res;
    
    if (wrapperData->javaPID <= 0) {
        /* We do not think that a JVM is currently running so return that it is down.
         * If we call waitpid with 0, it will wait for any child and cause problems with the event commands. */
        return WRAPPER_PROCESS_DOWN;
    }

    retval = waitpid(wrapperData->javaPID, &status, WNOHANG | WUNTRACED);
    if (retval == 0) {
        /* Up and running. */
        if (sigChild && wrapperData->jvmStopped) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("JVM process was continued."));
            wrapperData->jvmStopped = FALSE;
        }
        res = WRAPPER_PROCESS_UP;
    } else if (retval < 0) {
        if (errno == ECHILD) {
            if ((wrapperData->jState == WRAPPER_JSTATE_DOWN_CHECK) ||
                (wrapperData->jState == WRAPPER_JSTATE_DOWN_FLUSH) ||
                (wrapperData->jState == WRAPPER_JSTATE_DOWN_CLEAN) ||
                (wrapperData->jState == WRAPPER_JSTATE_STOPPED)) {
                res = WRAPPER_PROCESS_DOWN;
                wrapperJVMProcessExited(nowTicks, 0);
                return res;
            } else {
                /* Process is gone.  Happens after a SIGCHLD is handled. Normal. */
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("JVM process is gone."));
            }
        } else {
            /* Error requesting the status. */
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT("Unable to request JVM process status: %s"), getLastErrorText());
        }
        exitCode = wrapperData->errorExitCode;
        res = WRAPPER_PROCESS_DOWN;
        wrapperJVMProcessExited(nowTicks, exitCode);
    } else {
#ifdef _DEBUG
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  WIFEXITED=%d"), WIFEXITED(status));
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  WIFSTOPPED=%d"), WIFSTOPPED(status));
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  WIFSIGNALED=%d"), WIFSIGNALED(status));
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  WTERMSIG=%d"), WTERMSIG(status));
#endif

        /* Get the exit code of the process. */
        if (WIFEXITED(status)) {
            /* JVM has exited. */
            exitCode = WEXITSTATUS(status);
            res = WRAPPER_PROCESS_DOWN;

            wrapperJVMProcessExited(nowTicks, exitCode);
        } else if (WIFSIGNALED(status)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                TEXT("JVM received a signal %s (%d)."), getSignalName(WTERMSIG(status)), WTERMSIG(status));
            res = WRAPPER_PROCESS_UP;
        } else if (WIFSTOPPED(status)) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN,
                TEXT("JVM process was stopped.  It will be killed if the ping timeout expires."));
            wrapperData->jvmStopped = TRUE;
            res = WRAPPER_PROCESS_UP;
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG,
                TEXT("JVM process signaled the Wrapper unexpectedly."));
            res = WRAPPER_PROCESS_UP;
        }
    }

    return res;
}

/**
 * This function does nothing on Unix machines.
 */
void wrapperReportStatus(int useLoggerQueue, int status, int errorCode, int waitHint) {
    return;
}

/**
 * Reads a single block of data from the child pipe.
 *
 * @param blockBuffer Pointer to the buffer where the block will be read.
 * @param blockSize Maximum number of bytes to read.
 * @param readCount Pointer to an int which will hold the number of bytes
 *                  actually read by the call.
 *
 * Returns TRUE if there were any problems, FALSE otherwise.
 */
int wrapperReadChildOutputBlock(char *blockBuffer, int blockSize, int *readCount) {
    if (pipedes[PIPE_READ_END] == -1) {
        /* The child is not up. */
        *readCount = 0;
        return FALSE;
    }

#if defined OPENBSD || defined FREEBSD
    /* Work around FreeBSD Bug #kern/64313
     *  http://www.freebsd.org/cgi/query-pr.cgi?pr=kern/64313
     *
     * When linked with the pthreads library the O_NONBLOCK flag is being reset
     *  on the pipedes[PIPE_READ_END] handle.  Not sure yet of the exact event that is causing
     *  this, but once it happens reads will start to block even though calls
     *  to fcntl(pipedes[PIPE_READ_END], F_GETFL) say that the O_NONBLOCK flag is set.
     * Calling fcntl(pipedes[PIPE_READ_END], F_SETFL, O_NONBLOCK) again will set the flag back
     *  again and cause it to start working correctly.  This may only need to
     *  be done once, however, because F_GETFL does not return the accurate
     *  state there is no reliable way to check.  Be safe and always set the
     *  flag. */
    if (fcntl(pipedes[PIPE_READ_END], F_SETFL, O_NONBLOCK) < 0) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT(
            "Failed to set JVM output handle to non blocking mode to read child process output: %s (%d)"),
            getLastErrorText(), errno);
        return TRUE;
    }
#endif

    /* Fill read buffer. */
    *readCount = read(pipedes[PIPE_READ_END], blockBuffer, blockSize);
    if (*readCount < 0) {
        /* No more bytes available, return for now.  But make sure that this was not an error. */
        if (errno == EAGAIN) {
            /* Normal, the call would have blocked as there is no data available. */
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT(
                "Failed to read console output from the JVM: %s (%d)"),
                getLastErrorText(), errno);
            return TRUE;
        }
    } else if (*readCount == 0) {
        /* We reached the EOF.  This means that the other end of the pipe was closed. */
        close(pipedes[PIPE_READ_END]);
        pipedes[PIPE_READ_END] = -1;
    }

    return FALSE;
}

/**
 * Transform a program into a daemon.
 *
 * The idea is to first fork, then make the child a session leader,
 * and then fork again, so that it, (the session group leader), can
 * exit. This means that we, the grandchild, as a non-session group
 * leader, can never regain a controlling terminal.
 */
void daemonize(int argc, TCHAR** argv) {
    pid_t pid;
    int fd;

    /* Set the auto close flag and close the logfile before doing any forking to avoid
     *  duplicate open files. */
    setLogfileAutoClose(TRUE);
    closeLogfile();

    /* first fork */
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Spawning intermediate process..."));
    }
    if ((pid = fork()) < 0) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("Could not spawn daemon process: %s"),
            getLastErrorText());
        appExit(wrapperData->errorExitCode, argc, argv);
    } else if (pid != 0) {
        /* Intermediate process is now running.  This is the original process, so exit. */

        /* If the main process was not launched in the background, then we want to make
         * the console output look nice by making sure that all output from the
         * intermediate and daemon threads are complete before this thread exits.
         * Sleep for 0.5 seconds. */
        wrapperSleep(500);

        /* Call exit rather than appExit as we are only exiting this process. */
        exit(0);
    }

    /* become session leader */
    if (setsid() == -1) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("setsid() failed: %s"),
           getLastErrorText());
        appExit(wrapperData->errorExitCode, argc, argv);
    }

    signal(SIGHUP, SIG_IGN); /* don't let future opens allocate controlling terminals */

    /* Redirect stdin, stdout and stderr before closing to prevent the shell which launched
     *  the Wrapper from hanging when it exits. */
    fd = _topen(TEXT("/dev/null"), O_RDWR, 0);
    if (fd != -1) {
        close(STDIN_FILENO);
        dup2(fd, STDIN_FILENO);
        close(STDOUT_FILENO);
        dup2(fd, STDOUT_FILENO);
        close(STDERR_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd != STDIN_FILENO &&
            fd != STDOUT_FILENO &&
            fd != STDERR_FILENO) {
            close(fd);
        }
    }
    /* Console output was disabled above, so make sure the console log output is disabled
     *  so we don't waste any CPU formatting and sending output to '/dev/null'/ */
    setConsoleLogLevelInt(LEVEL_NONE);

    /* second fork */
    if (wrapperData->isDebugging) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Spawning daemon process..."));
    }
    if ((pid = fork()) < 0) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("Could not spawn daemon process: %s"),
            getLastErrorText());
        appExit(wrapperData->errorExitCode, argc, argv);
    } else if (pid != 0) {
        /* Daemon process is now running.  This is the intermediate process, so exit. */
        /* Call exit rather than appExit as we are only exiting this process. */
        exit(0);
    }

    /* Restore the auto close flag in the daemonized process. */
    setLogfileAutoClose(wrapperData->logfileCloseTimeout == 0);
}


/**
 * Sets the working directory to that of the current executable
 */
int setWorkingDir(TCHAR *app) {
    TCHAR *szPath;
    TCHAR* pos;

    /* Get the full path and filename of this program */
    if ((szPath = findPathOf(app, TEXT("Wrapper binary"))) == NULL) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Unable to get the path for '%s'-%s"),
            app, getLastErrorText());
        return 1;
    }

    /* The wrapperData->isDebugging flag will never be set here, so we can't really use it. */
#ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Executable Name: %s"), szPath);
#endif

    /* To get the path, strip everything off after the last '\' */
    pos = _tcsrchr(szPath, TEXT('/'));
    if (pos == NULL) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Unable to extract path from: %s"), szPath);
        free(szPath);
        return 1;
    } else {
        /* Clip the path at the position of the last backslash */
        pos[0] = (TCHAR)0;
    }

    /* Set a variable to the location of the binary. */
    setEnv(TEXT("WRAPPER_BIN_DIR"), szPath, ENV_SOURCE_APPLICATION);

    if (wrapperSetWorkingDir(szPath, TRUE)) {
        free(szPath);
        return 1;
    }
    free(szPath);
    return 0;
}

/*******************************************************************************
 * Main function                                                               *
 *******************************************************************************/
#ifndef CUNIT
#ifdef UNICODE
int main(int argc, char **cargv) {
    size_t req;
    TCHAR **argv;
#else
int main(int argc, char **argv) {
#endif
#if defined(_DEBUG) || defined(UNICODE)
    int i;
#endif
    TCHAR *retLocale;
    int exitCode;
    int localeSet;
    TCHAR *envLang;

#ifdef FREEBSD
    /* In the case of FreeBSD, we need to dynamically load and initialize the iconv library to work with all versions of FreeBSD. */
    if (loadIconvLibrary()) {
        /* Already reported. */
        /* Don't call appExit here as we are not far enough along. */
        return 1;
    }
#endif  
  
    /* Set the default locale here so any startup error messages will have a chance of working.
     *  This should be done before converting cargv to argv, because there might be accentued letters in cargv. */
    envLang = _tgetenv(TEXT("LANG"));
    retLocale = _tsetlocale(LC_ALL, TEXT(""));
    if (!retLocale) {
        /* On some platforms (i.e. Linux ARM), the locale can't be set if LC_ALL is empty.
         *  In such case, set LC_ALL to the value of LANG and try again. */
        setEnv(TEXT("LC_ALL"), envLang, ENV_SOURCE_APPLICATION);
        retLocale = _tsetlocale(LC_ALL, TEXT(""));
    }
    if (retLocale) {
#if defined(UNICODE)
        free(retLocale);
        if (envLang) {
            free(envLang);
        }
#endif
        localeSet = TRUE;
    } else {
        /* Do not free envLang yet. We will use it below to print a warning. */
        localeSet = FALSE;
    }
    
#ifdef UNICODE
    /* Create UNICODE versions of the argv array for internal use. */
    argv = malloc(argc * sizeof(TCHAR *));
    if (!argv) {
        _tprintf(TEXT("Out of Memory in Main\n"));
        appExit(1, 0, NULL);
        return 1;
    }
    for (i = 0; i < argc; i++) {
        req = mbstowcs(NULL, cargv[i], MBSTOWCS_QUERY_LENGTH);
        if (req == (size_t)-1) {
            _tprintf(TEXT("Encoding problem with arguments in Main\n"));
            free(argv);
            appExit(1, 0, NULL);
            return 1;
        }
        argv[i] = malloc(sizeof(TCHAR) * (req + 1));
        if (!argv[i]) {
            _tprintf(TEXT("Out of Memory in Main\n"));
            while (--i > 0) {
                free(argv[i]);
            }
            free(argv);
            appExit(1, 0, NULL);
            return 1;
        }
        mbstowcs(argv[i], cargv[i], req + 1);
        argv[i][req] = TEXT('\0'); /* Avoid bufferflows caused by badly encoded characters. */
    }
#endif

    if (wrapperInitialize()) {
        appExit(1, argc, argv);
        return 1; /* For compiler. */
    }

    /* Main thread initialized in wrapperInitialize. */

#ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("Wrapper DEBUG build!"));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("Logging initialized."));
#endif
    /* Get the current process. */
    wrapperData->wrapperPID = getpid();

    if (setWorkingDir(argv[0])) {
        appExit(1, argc, argv);
        return 1; /* For compiler. */
    }
#ifdef _DEBUG
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("Working directory set."));
    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("Arguments:"));
    for (i = 0; i < argc; i++) {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("  argv[%d]=%s"), i, argv[i]);
    }
#endif
    /* Parse the command and configuration file from the command line. */
    if (!wrapperParseArguments(argc, argv)) {
        appExit(1, argc, argv);
        return 1; /* For compiler. */
    }
    if (!localeSet) {
        if (strcmpIgnoreCase(wrapperData->argCommand, TEXT("-translate")) != 0) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS, TEXT("Unable to set the locale to '%s'.  Please make sure $LC_* and $LANG are correct."), (envLang ? envLang : TEXT("<NULL>")));
        }
#if defined(UNICODE)
        if (envLang) {
            free(envLang);
        }
#endif
    }
    wrapperLoadHostName();
    if (!strcmpIgnoreCase(wrapperData->argCommand, TEXT("-translate"))) {
        /* We want to disable all log output when a translation request is made. */
        setSilentLogLevels();
    }
    /* At this point, we have a command, confFile, and possibly additional arguments. */
    if (!strcmpIgnoreCase(wrapperData->argCommand, TEXT("?")) || !strcmpIgnoreCase(wrapperData->argCommand, TEXT("-help"))) {
        /* User asked for the usage. */
        setSimpleLogLevels();
        wrapperUsage(argv[0]);
        appExit(0, argc, argv);
        return 0; /* For compiler. */
    } else if (!strcmpIgnoreCase(wrapperData->argCommand, TEXT("v")) || !strcmpIgnoreCase(wrapperData->argCommand, TEXT("-version"))) {
        /* User asked for version. */
        setSimpleLogLevels();
        wrapperVersionBanner();
        appExit(0, argc, argv);
        return 0; /* For compiler. */
    } else if (!strcmpIgnoreCase(wrapperData->argCommand, TEXT("h")) || !strcmpIgnoreCase(wrapperData->argCommand, TEXT("-hostid"))) {
        /* User asked for version. */
        setSimpleLogLevels();
        wrapperVersionBanner();
        showHostIds(LEVEL_STATUS);
        appExit(0, argc, argv);
        return 0; /* For compiler. */
    }

    /* Load the properties. */
    /* To make the WRAPPER_LANG references in the configuration work correctly,
     *  it is necessary to load the configuration twice.
     * The first time, we want to ignore the return value.  Any errors will be
     *  suppressed and will get reported again the second time through. */
    /* From version 3.5.27, the community edition will also preload the configuration properties. */ 
    wrapperLoadConfigurationProperties(TRUE);
    if (wrapperLoadConfigurationProperties(FALSE)) {
        /* Unable to load the configuration.  Any errors will have already
         *  been reported. */
        if (wrapperData->argConfFileDefault && !wrapperData->argConfFileFound) {
            /* The config file that was being looked for was default and
             *  it did not exist.  Show the usage. */
            wrapperUsage(argv[0]);
        }
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("  The Wrapper will stop."));
        appExit(wrapperData->errorExitCode, argc, argv);
        return 1; /* For compiler. */
    }

    /* Set the default umask of the Wrapper process. */
    umask(wrapperData->umask);
    if (!strcmpIgnoreCase(wrapperData->argCommand, TEXT("-translate"))) {
        setSimpleLogLevels();
        /* Print out the string so the caller sees it as its translated output. */
        _tprintf(TEXT("%s"), argv[2]);
        appExit(0, argc, argv);
        return 0; /* For compiler. */
    } else if (!strcmpIgnoreCase(wrapperData->argCommand, TEXT("c")) || !strcmpIgnoreCase(wrapperData->argCommand, TEXT("-console"))) {
        /* Run as a console application */

        /* fork to a Daemonized process if configured to do so. */
        if (wrapperData->daemonize) {
            daemonize(argc, argv);
            
            /* We are now daemonized, so mark this as being a service. */
            wrapperData->isConsole = FALSE;

            /* When we daemonize the Wrapper, its PID changes. Because of the
             *  WRAPPER_PID environment variable, we need to set it again here
             *  and then reload the configuration in case the PID is referenced
             *  in the configuration. */

            /* Get the current process. */
            wrapperData->wrapperPID = getpid();

            if (wrapperData->isDebugging) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_DEBUG, TEXT("Reloading configuration."));
            }
    
            /* If the working dir has been changed then we need to restore it before
             *  the configuration can be reloaded.  This is needed to support relative
             *  references to include files. */
            if (wrapperData->workingDir && wrapperData->originalWorkingDir) {
                if (wrapperSetWorkingDir(wrapperData->originalWorkingDir, TRUE)) {
                    /* Failed to restore the working dir.  Shutdown the Wrapper */
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("  The Wrapper will stop."));
                    appExit(wrapperData->errorExitCode, argc, argv);
                    return 1; /* For compiler. */
                }
            }
    
            /* Load the properties. */
            if (wrapperLoadConfigurationProperties(FALSE)) {
                /* Unable to load the configuration.  Any errors will have already
                 *  been reported. */
                if (wrapperData->argConfFileDefault && !wrapperData->argConfFileFound) {
                    /* The config file that was being looked for was default and
                     *  it did not exist.  Show the usage. */
                    wrapperUsage(argv[0]);
                }
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("  The Wrapper will stop."));
                appExit(wrapperData->errorExitCode, argc, argv);
                return 1; /* For compiler. */
            }
        }


        /* See if the logs should be rolled on Wrapper startup. */
        if ((getLogfileRollMode() & ROLL_MODE_WRAPPER) ||
            (getLogfileRollMode() & ROLL_MODE_JVM)) {
            rollLogs();
        }

        if (wrapperData->pidFilename) {
            if (writePidFile(wrapperData->pidFilename, wrapperData->wrapperPID, wrapperData->pidFileUmask, wrapperData->pidFileStrict)) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                     TEXT("ERROR: Could not write pid file %s: %s"),
                wrapperData->pidFilename, getLastErrorText());
                exitCode = wrapperData->errorExitCode;
                /* Common wrapper cleanup code. */
                wrapperDispose();
#if defined(UNICODE)
                for (i = 0; i < argc; i++) {
                    if (argv[i]) {
                        free(argv[i]);
                    }
                }
                if (argv) {
                    free(argv);
                }
#endif
                exit(exitCode);
                return 1; /* For compiler. */
            }
        }

        /* Write pid and anchor files as requested.  If they are the same file the file is
         *  simply overwritten. */
        if (wrapperData->anchorFilename) {
            if (writePidFile(wrapperData->anchorFilename, wrapperData->wrapperPID, wrapperData->anchorFileUmask, FALSE)) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL,
                     TEXT("ERROR: Could not write anchor file %s: %s"),
                     wrapperData->anchorFilename, getLastErrorText());
                appExit(wrapperData->errorExitCode, argc, argv);
                return 1; /* For compiler. */
            }
        }

        if (wrapperData->lockFilename) {
            if (writePidFile(wrapperData->lockFilename, wrapperData->wrapperPID, wrapperData->lockFileUmask, FALSE)) {
                /* This will fail if the user is running as a user without full privileges.
                 *  To make things easier for user configuration, this is ignored if sufficient
                 *  privileges do not exist. */
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_STATUS,
                     TEXT("WARNING: Could not write lock file %s: %s"),
                     wrapperData->lockFilename, getLastErrorText());
                wrapperData->lockFilename = NULL;
            }
        }

        if (wrapperData->isConsole) {
            appExit(wrapperRunConsole(), argc, argv);
        } else {
            appExit(wrapperRunService(), argc, argv);
        }
        return 0; /* For compiler. */
    } else {
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT(""));
        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_WARN, TEXT("Unrecognized option: -%s"), wrapperData->argCommand);
        wrapperUsage(argv[0]);
        appExit(wrapperData->errorExitCode, argc, argv);
        return 1; /* For compiler. */
    }
}
#endif

#endif /* ifndef WIN32 */
