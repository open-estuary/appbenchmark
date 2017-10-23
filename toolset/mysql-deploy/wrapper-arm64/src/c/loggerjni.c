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

#include <stdio.h>
#include <string.h>
#ifdef WIN32
 #include <tchar.h>
 #include <windows.h>
#else
 #include <pthread.h>
#endif
#include <errno.h>
#include "loggerjni.h"

void outOfMemory(const TCHAR *context, int id) {
    _tprintf(TEXT("WrapperJNI Error: Out of memory (%s%02d). %s\n"), context, id, getLastErrorText());fflush(NULL);
}

void invalidMultiByteSequence(const TCHAR *context, int id) {
    _tprintf(TEXT("WrapperJNI Error: Invalid multibyte Sequence found in (%s%02d). %s"), context, id, getLastErrorText());fflush(NULL);
}

#define LAST_ERROR_TEXT_BUFFER_SIZE 1024
/** Buffer holding the last error message.
 *  TODO: This needs to be made thread safe, meaning that we need a buffer for each thread. */
TCHAR lastErrorTextBufferW[LAST_ERROR_TEXT_BUFFER_SIZE];

/**
 * Returns a textual error message of the last error encountered.
 *
 * @return The last error message.
 */
const TCHAR* getLastErrorText() {
    int errorNum;
#ifdef WIN32
    DWORD dwRet;
    TCHAR* lpszTemp = NULL;
#else
    char* lastErrorTextMB;
    size_t req;
#endif

#ifdef WIN32
    errorNum = GetLastError();
    dwRet = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                          NULL,
                          GetLastError(),
                          LANG_NEUTRAL,
                          (TCHAR*)&lpszTemp,
                          0,
                          NULL);

    /* supplied buffer is not long enough */
    if (!dwRet) {
        /* There was an error calling FormatMessage. */
        _sntprintf(lastErrorTextBufferW, LAST_ERROR_TEXT_BUFFER_SIZE, TEXT("Failed to format system error message (Error: %d) (Original Error: 0x%x)"), GetLastError(), errorNum);
    } else if ((long)LAST_ERROR_TEXT_BUFFER_SIZE - 1 < (long)dwRet + 14) {
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
    errorNum = errno;
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

