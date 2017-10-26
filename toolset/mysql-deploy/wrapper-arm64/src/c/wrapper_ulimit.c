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
#include <errno.h>
#include "logger.h"
#include "property.h"
#include "wrapper.h"
#include "wrapper_ulimit.h"

#if defined(LINUX) && defined(__USE_FILE_OFFSET64) && !defined(JSW64)
 #define WRAPPER_RLIM_INFINITY  ((unsigned long int)(~0UL))
#else
 #define WRAPPER_RLIM_INFINITY  RLIM_INFINITY
#endif

PResourceLimit getResourceProperty(Properties *properties, const TCHAR *propertyName, const int multiplier) {
    const TCHAR* value;
    PResourceLimit result;

    result = malloc(sizeof(ResourceLimit));
    if (!result) {
        outOfMemoryQueued(TEXT("GRSP"), 1);
    } else {
        result->useCurrent = FALSE;
        result->useHard = FALSE;
        result->isValid = FALSE;
        
        value = getStringProperty(properties, propertyName, TEXT("current"));
        
        if ((strcmpIgnoreCase(value, TEXT("current")) == 0) || (value[0] == 0)) {
            result->value = 0;
            result->useCurrent = TRUE;
            result->isValid = TRUE;
        } else if (strcmpIgnoreCase(value, TEXT("hard")) == 0) {
            result->value = 0;
            if (_tcsstr(propertyName, TEXT("soft"))) {
                result->useHard = TRUE;
            } else {
                result->useCurrent = TRUE;
            }
            result->isValid = TRUE;
        } else if (strcmpIgnoreCase(value, TEXT("unlimited")) == 0) {
            result->value = WRAPPER_RLIM_INFINITY;
            result->isValid = TRUE;
        } else if (value[0] == TEXT('-')) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Encountered an invalid value '%s' in the %s property."), value, propertyName);
        } else {
            result->value = (rlim_t)(_tcstoul(value, NULL, 10) * multiplier);
            if (((result->value == 0) && (errno != 0))) {
                /* Failed to convert to an integer. */
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Encountered an invalid value '%s' in the %s property."), value, propertyName);
            } else {
                result->isValid = TRUE;
            }
        }
    }
    return result;
}

PResourceLimits getResourcePropertyPair(Properties *properties, const TCHAR *propertyBaseName, const int multiplier) {
    TCHAR propSoft[MAX_PROPERTY_NAME_LENGTH];
    TCHAR propHard[MAX_PROPERTY_NAME_LENGTH];
    PResourceLimit softLimit;
    PResourceLimit hardLimit;
    PResourceLimits result = NULL;
    
    _sntprintf(propSoft, MAX_PROPERTY_NAME_LENGTH, TEXT("%s.soft"), propertyBaseName);
    propSoft[MAX_PROPERTY_NAME_LENGTH-1] = 0;
    
    _sntprintf(propHard, MAX_PROPERTY_NAME_LENGTH, TEXT("%s.hard"), propertyBaseName);
    propHard[MAX_PROPERTY_NAME_LENGTH-1] = 0;
    
    softLimit = getResourceProperty(properties, propSoft, multiplier);
    hardLimit = getResourceProperty(properties, propHard, multiplier);
    
    if (softLimit && softLimit->isValid && hardLimit && hardLimit->isValid) {
        result = malloc(sizeof(ResourceLimits));
        if (!result) {
            outOfMemoryQueued(TEXT("GRSPP"), 1);
        } else {
            result->rlim_cur = softLimit;
            result->rlim_max = hardLimit;
        }
    } else {
        if (softLimit) {
            free(softLimit);
        }
        if (hardLimit) {
            free(hardLimit);
        }
    } 
    
    return result;
}

void disposeResourceLimits(PResourceLimits limits) {
    if (limits) {
        if (limits->rlim_cur) {
            free(limits->rlim_cur);
        }
        if (limits->rlim_max) {
            free(limits->rlim_max);
        }
        free(limits);
    }
}

TCHAR* printRlim(rlim_t value, TCHAR* buffer, const int divisor) {
    /* On Linux 32-bit, the value can be greater than (unsigned long int)(~0UL)
     *  when the compilation option __USE_FILE_OFFSET64 is used. */
    if (value >= WRAPPER_RLIM_INFINITY) {
        _sntprintf(buffer, 32, TEXT("unlimited"));
    } else {
        _sntprintf(buffer, 32, TEXT("%lu"), (unsigned long)(value/divisor));
    }
    buffer[31] = 0;
    return buffer;
}

int setResourceLimits(int resourceId, const TCHAR* resourceName, const TCHAR* propertyBaseName, PResourceLimits confLimits, int strict, const int divisor) {
    struct rlimit oldLimits, newLimits, checkLimits;
    TCHAR limBuf1[32];
    TCHAR limBuf2[32];
    TCHAR limBuf3[32];
    TCHAR limBuf4[32];
    int logLevel;
    int errorNum = 0;
    int setResult;
    
    if (!confLimits->rlim_cur->useCurrent || !confLimits->rlim_max->useCurrent) {
        /* The user has specified limits for the number of open file descriptors. */
        if (!confLimits->rlim_cur->useCurrent && !confLimits->rlim_cur->useHard && !confLimits->rlim_max->useCurrent && (confLimits->rlim_max->value < confLimits->rlim_cur->value)) {
            /* This is a configuration error, return 1 no matter we are strict or not. */
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("The soft limit (%s) for %s is set higher than the hard limit (%s)."), printRlim(confLimits->rlim_cur->value, limBuf1, divisor), resourceName, printRlim(confLimits->rlim_max->value, limBuf2, divisor));
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE, TEXT("  Make sure to correctly set the values of the %s.soft and %s.hard properties."), propertyBaseName, propertyBaseName);
            return 1;
        }
        
        /* Get the limits for the resource. */
        if (getrlimit(resourceId, &oldLimits) != 0) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Unable to get the limits for %s: (0x%x)"), resourceName, errno);
            return 1;
        }
        
        /* Unless we fail to set the limits for some unknown reason, any error below will return 1 if we are strict, 0 otherwise. */
        logLevel = strict ? LEVEL_FATAL : properties->logWarningLogLevel;
        
        /* Resolve the hard limit. */
        if (confLimits->rlim_max->useCurrent) {
            /* Use the current value */
            newLimits.rlim_max = oldLimits.rlim_max;
        } else {
            /* Use the configured value */
            newLimits.rlim_max = confLimits->rlim_max->value;
        }
        
        /* Resolve the soft limit. */
        if (confLimits->rlim_cur->useCurrent) {
            /* Use the current value */
            newLimits.rlim_cur = oldLimits.rlim_cur;
        } else if (confLimits->rlim_cur->useHard) {
            /* Use the hard value */
            newLimits.rlim_cur = newLimits.rlim_max;
        } else {
            /* Use the configured value */
            newLimits.rlim_cur = confLimits->rlim_cur->value;
        }
        
        /* Resolve cases where the soft limit is greater than the hard limit. */
        if (newLimits.rlim_max < newLimits.rlim_cur) {
            if (confLimits->rlim_max->useCurrent) {
                /* The user has only set the SOFT limit. */
                log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, TEXT("The soft limit (%s) for %s is set higher than the current hard limit (%s)."), printRlim(confLimits->rlim_cur->value, limBuf1, divisor), resourceName, printRlim(oldLimits.rlim_max, limBuf2, divisor));
                if (strict) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE, TEXT("  Make sure to correctly set the value of the %s.soft property."), propertyBaseName);
                    return 1;
                }
                newLimits.rlim_cur = oldLimits.rlim_max;
            } else {
                /* The user has only set the HARD limit. */
                log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, TEXT("The hard limit (%s) for %s is set lower than the current soft limit (%s)."), printRlim(confLimits->rlim_max->value, limBuf1, divisor), resourceName, printRlim(oldLimits.rlim_cur, limBuf2, divisor));
                if (strict) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE, TEXT("  Make sure to correctly set the value of the %s.hard property."), propertyBaseName);
                    return 1;
                }
                newLimits.rlim_cur = newLimits.rlim_max;
            }
            log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, TEXT("  Decreasing the soft limit to the value of the hard limit."));
        }
        
        /* Try to set the limits */
        setResult = setrlimit(resourceId, &newLimits);
        errorNum = errno;
        if (setResult == 0) {
            /* setrlimit() did not return an error but on some platforms, this doesn't mean that the configured values were set correctly. 
             *  For example on freeBSD, the limits for the number of open file descriptors can't be raised to a value greater than 1677,
             *  but the function will silently increase the limits up to that maximum value. */
            if (getrlimit(resourceId, &checkLimits) != 0) {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Unable to get the limits for %s: (0x%x)"), resourceName, errno);
                return 1;
            }
            if (checkLimits.rlim_max < confLimits->rlim_max->value) {
                /* Mark as an EINVAL error and continue. */
                errorNum = EINVAL;
                setResult = -1;
            } else if (((checkLimits.rlim_max != newLimits.rlim_max) || (checkLimits.rlim_cur != newLimits.rlim_cur))
#if defined(LINUX) && defined(__USE_FILE_OFFSET64) && !defined(JSW64)
                        /* On Linux 32-bit, we defined WRAPPER_RLIM_INFINITY to use the max value of unsigned long
                         *  (see comment near the definition of WRAPPER_RLIM_INFINITY). However, since we compile
                         *  with __USE_FILE_OFFSET64, the real unlimited value is the max of unsigned long long.
                         *  setrlimit() will set unlimited limits even with WRAPPER_RLIM_INFINITY being unsigned long,
                         *  but getrlimit() will collect greater values (max of unsigned long long). The limits are
                         *  set correctly, so just ignore this case. We will no longer need this when using C99. */
                        && !(newLimits.rlim_max == WRAPPER_RLIM_INFINITY && (checkLimits.rlim_max > newLimits.rlim_max))
                        && !(newLimits.rlim_cur == WRAPPER_RLIM_INFINITY && (checkLimits.rlim_cur > newLimits.rlim_cur))
#endif
            ) {
                /* This should never happen... */
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Unable to set the limits for %s (HARD: expected %s, got %s; SOFT: expected %s, got %s)."),
                    resourceName, 
                    printRlim(newLimits.rlim_max  , limBuf1, divisor),
                    printRlim(checkLimits.rlim_max, limBuf2, divisor),
                    printRlim(newLimits.rlim_cur  , limBuf3, divisor),
                    printRlim(checkLimits.rlim_cur, limBuf4, divisor));
                return 1;
            }
        }
        
        if (setResult != 0) {
            /* Resolve cases where the configured hard limit is greater than the current hard limit. */
            if ((oldLimits.rlim_max < confLimits->rlim_max->value) && ((errorNum == EPERM) || (errorNum == EINVAL))) {
                if (errorNum == EPERM) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, TEXT("The process doesn't have sufficient privileges to raise the hard limit (from %s to %s) for %s."), printRlim(oldLimits.rlim_max, limBuf1, divisor), printRlim(confLimits->rlim_max->value, limBuf2, divisor), resourceName);
                } else {
                    log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, TEXT("Could not raise the hard limit (from %s to %s) for %s."), printRlim(oldLimits.rlim_max, limBuf1, divisor), printRlim(confLimits->rlim_max->value, limBuf2, divisor), resourceName);
                }
                if (strict) {
                    if (errorNum == EPERM) {
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE, TEXT("  Please run the Wrapper with sufficient privileges or adjust the value of the %s.hard property."), propertyBaseName);
                    } else {
                        log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ADVICE, TEXT("  Please adjust the value of the %s.hard property."), propertyBaseName);
                    }
                    return 1;
                }
                newLimits.rlim_max = oldLimits.rlim_max;
                if (newLimits.rlim_max < newLimits.rlim_cur) {
                    newLimits.rlim_cur = newLimits.rlim_max;
                    log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, TEXT("  Ignoring the configured hard limit. Decreasing the configured soft limit to the value of the hard limit."));
                } else {
                    log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, TEXT("  Ignoring the configured hard limit."));
                }
                /* Set again the limits. */
                if (setrlimit(resourceId, &newLimits) != 0) {
                    log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Unable to set the limits for %s (0x%x)."), resourceName, errno);
                    return 1;
                }
            } else {
                log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_FATAL, TEXT("Unable to set the limits for %s (0x%x)."), resourceName, errno);
                return 1;
            }
        }
    }

    return 0;
}

/**
 * Set the soft and hard resource limits.
 *  For each resource, we can set the limits being strict or not.
 *  - strict: the Wrapper will stop if it is not possible to set the limit as defined in the configuration.
 *  - not strict: the Wrapper will try to adjust the hard and soft limits to be as close as possible to the
 *                configuration and show warnings whenever a property is resolved to a different value.
 *  The constraints are the following: - the soft limit can't be greater than the hard limit.
 *                                     - the hard limit can only be raised by the root user.
 *
 * Returns 0 if no error. Otherwise returns 1.
 */
int loadResourcesLimitsConfiguration() {
    PResourceLimits nofileLimits;
    PResourceLimits dataLimits;
    int nofileStrict;
    int dataStrict;
    
    /* number of open file descriptors */
    nofileLimits = getResourcePropertyPair(properties, TEXT("wrapper.ulimit.nofile"), 1);
    if (!nofileLimits) {
        return 1;
    }
    nofileStrict = getBooleanProperty(properties, TEXT("wrapper.ulimit.nofile.strict"), TRUE);
    
    if (setResourceLimits(RLIMIT_NOFILE, TEXT("the number of open file descriptors"), TEXT("wrapper.ulimit.nofile"), nofileLimits, nofileStrict, 1)) {
        disposeResourceLimits(nofileLimits);
        return 1;
    }
    disposeResourceLimits(nofileLimits);
    
    /* size of a process's data segment */
    dataLimits = getResourcePropertyPair(properties, TEXT("wrapper.ulimit.data"), 1024);
    if (!dataLimits) {
        return 1;
    }
    dataStrict = getBooleanProperty(properties, TEXT("wrapper.ulimit.data.strict"), TRUE);
    
    if (setResourceLimits(RLIMIT_DATA, TEXT("the size of a process's data segment"), TEXT("wrapper.ulimit.data"), dataLimits, dataStrict, 1024)) {
        disposeResourceLimits(dataLimits);
        return 1;
    }
    disposeResourceLimits(dataLimits);
    
    return 0;
}

/** 
 * Print out the soft and hard resource limits.
 */
void showResourceslimits() {
    struct rlimit limits;
    TCHAR limBuf1[32];
    TCHAR limBuf2[32];
    
    int logLevel = getLogLevelForName(getStringProperty(properties, TEXT("wrapper.ulimit.loglevel"), TEXT("DEBUG")));
    
    if ((getLowLogLevel() <= logLevel) && (logLevel != LEVEL_NONE)) {
        if (getrlimit(RLIMIT_NOFILE, &limits) != 0) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("Unable to get the limits for the number of open file descriptors: (0x%x)"), errno);
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, TEXT("Number of open file descriptors limits: %s (soft), %s (hard)."), printRlim(limits.rlim_cur, limBuf1, 1), printRlim(limits.rlim_max, limBuf2, 1));
        }
        if (getrlimit(RLIMIT_DATA, &limits) != 0) {
            log_printf(WRAPPER_SOURCE_WRAPPER, LEVEL_ERROR, TEXT("Unable to get the limits for the data segment size: (0x%x)"), errno);
        } else {
            log_printf(WRAPPER_SOURCE_WRAPPER, logLevel, TEXT("Data segment size limits: %s (soft), %s (hard)."), printRlim(limits.rlim_cur, limBuf1, 1024), printRlim(limits.rlim_max, limBuf2, 1024));
        }
    }
}
#endif
