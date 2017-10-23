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

#ifndef WIN32
#ifndef _WRAPPER_ULIMIT_H
#define _WRAPPER_ULIMIT_H
#include <sys/resource.h>
#include "wrapper_i18n.h"

typedef struct ResourceLimit ResourceLimit, *PResourceLimit;
struct ResourceLimit {
    rlim_t value;
    int    useCurrent;
    int    useHard;
    int    isValid;
};

typedef struct ResourceLimits ResourceLimits, *PResourceLimits;
struct ResourceLimits {
    ResourceLimit *rlim_max;
    ResourceLimit *rlim_cur;
};

int loadResourcesLimitsConfiguration();

void showResourceslimits();

#endif
#endif
