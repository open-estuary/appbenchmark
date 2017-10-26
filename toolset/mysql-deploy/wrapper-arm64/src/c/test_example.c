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
#include "property.h"

/********************************************************************
 * Example Tests
 *******************************************************************/

/**
 * The suite initialization function.
 * Opens the temporary file used by the tests.
 * Returns zero on success, non-zero otherwise.
 */
int tsEX_init_suite1(void) {
    return 0;
}

/**
 * The suite cleanup function.
 * Closes the temporary file used by the tests.
 * Returns zero on success, non-zero otherwise.
 */
int tsEX_clean_suite1(void) {
    return 0;
}

/**
 * Simple test that passes.
 */
void tsEX_testPass(void) {
    CU_ASSERT_EQUAL(0, 0);
}

/**
 * Simple test that passes.
 */
void tsEX_testFail(void) {
    CU_ASSERT_NOT_EQUAL(0, 1);
}

int tsEX_suiteExample() {
    CU_pSuite exampleSuite;

    exampleSuite = CU_add_suite("Example Suite", tsEX_init_suite1, tsEX_clean_suite1);
    if (NULL == exampleSuite) {
        return CU_get_error();
    }

    CU_add_test(exampleSuite, "Pass", tsEX_testPass);
    CU_add_test(exampleSuite, "Fail", tsEX_testFail);

    return FALSE;
}

