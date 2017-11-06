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

/* NOTES:
 *
 * See for ASSERTS:
 *  http://cunit.sourceforge.net/doc/writing_tests.html#tests
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Automated.h"
#include "Basic.h"
#include "Console.h"
#include "testsuite.h"
#include "logger.h"

/********************************************************************
 * Main
 *******************************************************************/
static void showHelp(TCHAR *app) {
    _tprintf(TEXT("Wrapper testsuite help.\n"));
    _tprintf(TEXT("%s <COMMAND>\n"), app);
    _tprintf(TEXT("\n"));
    _tprintf(TEXT("Commands:\n"));
    _tprintf(TEXT("  --basic   : Runs all tests in basic mode.  Only summaries visible.\n"));
    _tprintf(TEXT("  --auto    : Runs all tests in automatic mode.  Output visible but results output to file.\n"));
    _tprintf(TEXT("              See CUnitAutomated-Results.xml for results.\n"));
    _tprintf(TEXT("  --console : Interactive mode.\n"));
    _tprintf(TEXT("  --help    : This help.\n"));
    _tprintf(TEXT("\n"));
}

/* The main() function for setting up and running the tests.
 * Returns a CUE_SUCCESS on successful running, another
 * CUnit error code on failure.
 */
int main(int argc, char **cargv) {
    TCHAR **argv;
    int i;
    size_t req;
    int errorCode;

    argv = malloc(argc * sizeof * argv);
    if (!argv) {
        _tprintf(TEXT("Out of Memory in Main\n"));
        return 1;
    }
    for (i = 0; i < argc; i++) {
        req = mbstowcs(NULL, cargv[i], MBSTOWCS_QUERY_LENGTH);
        if (req == (size_t)-1) {
            _tprintf(TEXT("Encoding problem with arguments in Main\n"));
            while (--i > 0) {
                free(argv[i]);
            }
            free(argv);
            return 1;
        }
        argv[i] = malloc(sizeof(TCHAR) * (req + 1));
        if (!argv[i]) {
            _tprintf(TEXT("Out of Memory in Main\n"));
            while (--i > 0) {
                free(argv[i]);
            }
            free(argv);
            return 1;
        }
        mbstowcs(argv[i], cargv[i], req + 1);
        argv[i][req] = TEXT('\0'); /* Avoid bufferflows caused by badly encoded characters. */
    }

    /* Initialize the random seed. */
    srand((unsigned)time(NULL));

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
    {
        errorCode = CU_get_error();
        goto error;
    }

    /* add a suite to the registry */
    if (tsEX_suiteExample()) {
        CU_cleanup_registry();
        errorCode = CU_get_error();
        goto error;
    }

    if (tsFLTR_suiteFilter()) {
        CU_cleanup_registry();
        errorCode = CU_get_error();
        goto error;
    }

    if (tsJAP_suiteJavaAdditionalParam()) {
        CU_cleanup_registry();
        errorCode = CU_get_error();
        goto error;
    }

    if (tsHASH_suiteHashMap()) {
        CU_cleanup_registry();
        errorCode = CU_get_error();
        goto error;
    }

    if (argc < 2) {
        showHelp(argv[0]);
        errorCode = 1;
    } else if (strcmpIgnoreCase(argv[1], TEXT("--basic")) == 0) {
        /* Run all tests using the CUnit Basic interface */
        CU_set_output_filename("testsuite");
        CU_basic_set_mode(CU_BRM_VERBOSE);
        CU_basic_run_tests();
        CU_cleanup_registry();
        errorCode = CU_get_error();
    } else if (strcmpIgnoreCase(argv[1], TEXT("--auto")) == 0) {
        /* Run all tests using the CUnit Automated interface */
        CU_list_tests_to_file();
        CU_automated_run_tests();
        CU_cleanup_registry();
        errorCode = CU_get_error();
    } else if (strcmpIgnoreCase(argv[1], TEXT("--console")) == 0) {
        /* Run all tests using the CUnit Console interface */
        CU_console_run_tests();
        CU_cleanup_registry();
        errorCode = CU_get_error();
    } else {
        showHelp(argv[0]);
        errorCode = 1;
    }

error:
    for (i = 0; i < argc; i++) {
        free(argv[i]);
    }
    free(argv);

    return errorCode;
}

