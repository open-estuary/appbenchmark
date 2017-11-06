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
#include <stdlib.h>
#include <string.h>
#include "Basic.h"
#include "logger.h"
#include "property.h"
#include "wrapper.h"

/********************************************************************
 * Filter Tests
 *******************************************************************/
#define TSFLTR_WORK_BUFFER_LEN 4096
TCHAR tsFLTR_workBuffer[TSFLTR_WORK_BUFFER_LEN];

void tsFLTR_dummyLogFileChanged(const TCHAR *logFile) {
}

int tsFLTR_init_wrapper(void) {
    initLogging(tsFLTR_dummyLogFileChanged);
    logRegisterThread(WRAPPER_THREAD_MAIN);
    setLogfileLevelInt(LEVEL_NONE);
    setConsoleLogFormat(TEXT("LPM"));
    setConsoleLogLevelInt(LEVEL_DEBUG);
    setConsoleFlush(TRUE);
    setSyslogLevelInt(LEVEL_NONE);
    return 0;
}

int tsFLTR_clean_wrapper(void) {
    disposeLogging();
    return 0;
}

void tsFLTR_subTestWrapperWildcardMatch(const TCHAR *pattern, const TCHAR *text, size_t expectedMinLen, int expectedMatch) {
    size_t minLen;
    int matched;

    minLen = wrapperGetMinimumTextLengthForPattern(pattern);
    if (minLen != expectedMinLen) {
        _sntprintf(tsFLTR_workBuffer, TSFLTR_WORK_BUFFER_LEN, TEXT("wrapperGetMinimumTextLengthForPattern(\"%s\") returned %d rather than expected %d."), pattern, minLen, expectedMinLen);
        _tprintf(TEXT("%s\n"), tsFLTR_workBuffer);
        CU_FAIL(tsFLTR_workBuffer);
    } else {
        _sntprintf(tsFLTR_workBuffer, TSFLTR_WORK_BUFFER_LEN, TEXT("wrapperGetMinimumTextLengthForPattern(\"%s\") returned %d."), pattern, minLen);
        CU_PASS(tsFLTR_workBuffer);
    }

    matched = wrapperWildcardMatch(text, pattern, expectedMinLen);
    if (matched != expectedMatch) {
        _sntprintf(tsFLTR_workBuffer, TSFLTR_WORK_BUFFER_LEN, TEXT("wrapperWildcardMatch(\"%s\", \"%s\", %d) returned %s rather than expected %s."),
            text, pattern, expectedMinLen, (matched ? TEXT("TRUE") : TEXT("FALSE")), (expectedMatch ? TEXT("TRUE") : TEXT("FALSE")));
        _tprintf(TEXT("%s\n"), tsFLTR_workBuffer);
        CU_FAIL(tsFLTR_workBuffer);
    } else {
        _sntprintf(tsFLTR_workBuffer, TSFLTR_WORK_BUFFER_LEN, TEXT("wrapperWildcardMatch(\"%s\", \"%s\", %d) returned %s."),
            text, pattern, expectedMinLen, (matched ? TEXT("TRUE") : TEXT("FALSE")));
        CU_PASS(tsFLTR_workBuffer);
    }
}

void tsFLTR_testWrapperWildcardMatch() {
    tsFLTR_subTestWrapperWildcardMatch(TEXT("a"), TEXT("a"), 1, TRUE);
    tsFLTR_subTestWrapperWildcardMatch(TEXT("a"), TEXT("b"), 1, FALSE);
    tsFLTR_subTestWrapperWildcardMatch(TEXT("a"), TEXT(""), 1, FALSE);

    tsFLTR_subTestWrapperWildcardMatch(TEXT("a"), TEXT("abc"), 1, TRUE);
    tsFLTR_subTestWrapperWildcardMatch(TEXT("b"), TEXT("abc"), 1, TRUE);
    tsFLTR_subTestWrapperWildcardMatch(TEXT("c"), TEXT("abc"), 1, TRUE);
    tsFLTR_subTestWrapperWildcardMatch(TEXT("d"), TEXT("abc"), 1, FALSE);

    tsFLTR_subTestWrapperWildcardMatch(TEXT("?"), TEXT("a"), 1, TRUE);
    tsFLTR_subTestWrapperWildcardMatch(TEXT("?"), TEXT(""), 1, FALSE);

    tsFLTR_subTestWrapperWildcardMatch(TEXT("*"), TEXT(""), 0, TRUE);
    tsFLTR_subTestWrapperWildcardMatch(TEXT("*"), TEXT("a"), 0, TRUE);
    tsFLTR_subTestWrapperWildcardMatch(TEXT("*"), TEXT("abc"), 0, TRUE);

    tsFLTR_subTestWrapperWildcardMatch(TEXT("*a"), TEXT("a"), 1, TRUE);
    tsFLTR_subTestWrapperWildcardMatch(TEXT("*a"), TEXT("abc"), 1, TRUE);
    tsFLTR_subTestWrapperWildcardMatch(TEXT("*b"), TEXT("abc"), 1, TRUE);
    tsFLTR_subTestWrapperWildcardMatch(TEXT("a*"), TEXT("a"), 1, TRUE);
    tsFLTR_subTestWrapperWildcardMatch(TEXT("a*"), TEXT("abc"), 1, TRUE);
    tsFLTR_subTestWrapperWildcardMatch(TEXT("b*"), TEXT("abc"), 1, TRUE);
    tsFLTR_subTestWrapperWildcardMatch(TEXT("*a*"), TEXT("a"), 1, TRUE);
    tsFLTR_subTestWrapperWildcardMatch(TEXT("*a*"), TEXT("abc"), 1, TRUE);
    tsFLTR_subTestWrapperWildcardMatch(TEXT("*b*"), TEXT("abc"), 1, TRUE);

    tsFLTR_subTestWrapperWildcardMatch(TEXT("HEAD*TAIL"), TEXT("This is the HEAD and this is the TAIL....."), 8, TRUE);
    tsFLTR_subTestWrapperWildcardMatch(TEXT("HEAD**TAIL"), TEXT("This is the HEAD and this is the TAIL....."), 8, TRUE);
    tsFLTR_subTestWrapperWildcardMatch(TEXT("*HEAD*TAIL*"), TEXT("This is the HEAD and this is the TAIL....."), 8, TRUE);
    tsFLTR_subTestWrapperWildcardMatch(TEXT("HEAD*TAIL"), TEXT("This is the HEAD and this is the TaIL....."), 8, FALSE);
    tsFLTR_subTestWrapperWildcardMatch(TEXT("HEAD**TAIL"), TEXT("This is the HEAD and this is the TaIL....."), 8, FALSE);
    tsFLTR_subTestWrapperWildcardMatch(TEXT("*HEAD*TAIL*"), TEXT("This is the HEAD and this is the TaIL....."), 8, FALSE);
    tsFLTR_subTestWrapperWildcardMatch(TEXT("HEAD*TA?L"), TEXT("This is the HEAD and this is the TAIL....."), 8, TRUE);
    tsFLTR_subTestWrapperWildcardMatch(TEXT("HEAD**TA?L"), TEXT("This is the HEAD and this is the TAIL....."), 8, TRUE);
    tsFLTR_subTestWrapperWildcardMatch(TEXT("*HEAD*TA?L*"), TEXT("This is the HEAD and this is the TAIL....."), 8, TRUE);
}

int tsFLTR_suiteFilter() {
    CU_pSuite filterSuite;

    filterSuite = CU_add_suite("Filter Suite", tsFLTR_init_wrapper, tsFLTR_clean_wrapper);
    if (NULL == filterSuite) {
        return CU_get_error();
    }

    CU_add_test(filterSuite, "wrapperWildcardMatch", tsFLTR_testWrapperWildcardMatch);

    return FALSE;
}

