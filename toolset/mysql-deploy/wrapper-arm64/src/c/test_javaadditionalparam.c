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
 * JavaAdditionalParam Utilities
 *******************************************************************/

/* TODO: Currently the source of tsJAP_testJavaAdditionalParamSuite is in wrapper.c  Should we move it in here? */

void tsJAP_dummyLogFileChanged(const TCHAR *logFile) {
}

int tsJAP_init_properties(void) {
    initLogging(tsJAP_dummyLogFileChanged);
    logRegisterThread(WRAPPER_THREAD_MAIN);
    setLogfileLevelInt(LEVEL_NONE);
    setConsoleLogFormat(TEXT("LPM"));
    setConsoleLogLevelInt(LEVEL_DEBUG);
    setConsoleFlush(TRUE);
    setSyslogLevelInt(LEVEL_NONE);

    properties = createProperties(FALSE, LEVEL_NONE, FALSE);
    return properties ? 0 : 1;
}

int tsJAP_clean_properties(void) {
    disposeLogging();

    disposeProperties(properties);
    return 0;
}

int tsJAP_suiteJavaAdditionalParam() {
    CU_pSuite javaAdditionalParamSuite;

    javaAdditionalParamSuite = CU_add_suite("Java Additional Parameter Suite", tsJAP_init_properties, tsJAP_clean_properties);
    if (NULL == javaAdditionalParamSuite) {
        return CU_get_error();
    }

    CU_add_test(javaAdditionalParamSuite, "loadJavaAdditionalCallback()", tsJAP_testJavaAdditionalParamSuite);

    return FALSE;
}

