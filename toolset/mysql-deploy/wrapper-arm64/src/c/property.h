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

#ifndef _PROPERTY_H
#define _PROPERTY_H

#include "wrapper_hashmap.h"

#ifndef TRUE
#define TRUE -1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* This defines the largest environment variable that we are able
 *  to work with.  It can be expanded if needed. */
#define MAX_PROPERTY_NAME_LENGTH 512
#define MAX_PROPERTY_VALUE_LENGTH 16384
#define MAX_PROPERTY_NAME_VALUE_LENGTH MAX_PROPERTY_NAME_LENGTH + 1 + MAX_PROPERTY_VALUE_LENGTH

#define ENV_SOURCE_PARENT      1
#define ENV_SOURCE_APPLICATION 2
#define ENV_SOURCE_CONFIG      4
#ifdef WIN32
#define ENV_SOURCE_REG_SYSTEM  8
#define ENV_SOURCE_REG_ACCOUNT 16
#endif

typedef struct EnvSrc EnvSrc;
struct EnvSrc {
    int     source;                     /* Source of the variable. */
    TCHAR*  name;                       /* Name of the environment variable. */
    EnvSrc *next;                       /* Next variable in the chain. */
};
extern EnvSrc *baseEnvSrc;

typedef struct Property Property;
struct Property {
    TCHAR *name;                        /* The name of the property. */
    TCHAR *value;                       /* The value of the property. */
    int finalValue;                     /* TRUE if the Property can not be changed. */
    int quotable;                       /* TRUE if quotes can be optionally added around the value. */
    int internal;                       /* TRUE if the Property is internal. */
    Property *next;                     /* Pointer to the next Property in a linked list */
    Property *previous;                 /* Pointer to the next Property in a linked list */
    int isGenerated;                    /* TRUE if the property did not exist in the configuration and was generated using a default value. */
};

typedef struct Properties Properties;
struct Properties {
    int debugProperties;                /* TRUE if debug information on Properties should be shown. */
    int exitOnOverwrite;                /* If TRUE, causes the wrapper to exit when any property is overwritten in the config files. */
    int logLevelOnOverwrite;            /* Defines the log level of the messages reported when properties are overwritten. */
    int overwrittenPropertyCausedExit;  /* Flag to keep trace whether at least one property was overridden */
    int logWarnings;                    /* Flag that controls whether or not warnings will be logged. */
    int logWarningLogLevel;             /* Log level at which any log warnings will be logged. */
    int dumpLogLevel;                   /* Log level at which properties will be dumped. */
    Property *first;                    /* Pointer to the first property. */
    Property *last;                     /* Pointer to the last property.  */
    PHashMap warnedVarMap;              /* Map of undefined environment variables for which the user was warned. */
};

/**
 * Get the log level of the messages reported when properties are overwritten.
 *
 * @param properties 
 *
 * @return log level 
 */
extern int GetLogLevelOnOverwrite(Properties *properties);

/**
 * Sets an environment variable with the specified value.
 *  The function will only set the variable if its value is changed, but if
 *  it does, the call will result in a memory leak the size of the string:
 *   "name=value".
 *
 * @param name Name of the variable being set.
 * @param value Value to be set, NULL to clear it.
 * @param source Where the variable came from.
 *               Must be one of ENV_SOURCE_PARENT, ENV_SOURCE_APPLICATION, ENV_SOURCE_CONFIG,
 *                 or ENV_SOURCE_REG_SYSTEM or ENV_SOURCE_REG_ACCOUNT on Windows.
 *               If value is ENV_SOURCE_PARENT then the value may be NULL and will never be
 *                 set to the environment.
 *
 * Return TRUE if there were any problems, FALSE otherwise.
 */
extern int setEnv(const TCHAR *name, const TCHAR *value, int source);

/**
 * Parses a property value and populates any environment variables.  If the expanded
 *  environment variable would result in a string that is longer than bufferLength
 *  the value is truncated.
 *
 * @param warnUndefinedVars Log warnings about missing environment variables.
 * @param warnedUndefVarMap Map of variables which have previously been logged, may be NULL if warnUndefinedVars false.
 * @param warnLogLevel Log level at which any warnings will be logged.
 */
extern void evaluateEnvironmentVariables(const TCHAR *propertyValue, TCHAR *buffer, int bufferLength, int warnUndefinedVars, PHashMap warnedUndefVarMap, int warnLogLevel);

/**
 * This function returns a reference to a static buffer and is NOT thread safe.
 *  Check implementation notes before using.
 */
extern TCHAR* generateTimeValue(const TCHAR* format, struct tm *timeTM);

/**
 * This function returns a reference to a static buffer and is NOT thread safe.
 *  Check implementation notes before using.
 */
extern TCHAR* generateRandValue(const TCHAR* format);

/**
 * Create a Properties structure loaded in from the specified file.
 *  Must call disposeProperties to free up allocated memory.
 *
 * @param properties Properties structure to load into.
 * @param filename File to load the properties from.
 * @param preload TRUE if this is a preload call that should have supressed error output.
 * @param argCommand Argument passed to the binary.
 * @param originalWorkingDir Working directory of the binary at the moment it was launched.
 * @param isDebugging Flag that controls whether or not debug output will be logged.
 *
 * @return TRUE if there were any problems, FALSE if successful.
 */
extern int loadProperties(Properties *properties,
                          const TCHAR* filename,
                          int preload,
                          const TCHAR *argCommand,
                          const TCHAR *originalWorkingDir,
                          int isDebugging);

/**
 * Create a Properties structure.  Must call disposeProperties to free up
 *  allocated memory.
 */
extern Properties* createProperties(int debug, int logLevelOnOverwrite, int exitOnOverwrite);

/**
 * Free all memory allocated by a Properties structure.  The properties
 *  pointer will no longer be valid.
 */
extern void disposeProperties(Properties *properties);

/**
 * Free all memory allocated by a Properties structure.  The properties
 *  pointer will no longer be valid.
 */
extern void disposeEnvironment();

/**
 * Remove a single Property from a Properties.  All associated memory is
 *  freed up.
 */
extern void removeProperty(Properties *properties, const TCHAR *propertyName);

/**
 * Used to set a NULL terminated list of property names whose values should be
 *  escaped when read in from a file.   '\\' will become '\' and '\n' will
 *  become '^J', all other characters following '\' will be left as is.
 *
 * @param propertyNames NULL terminated list of property names.  Property names
 *                      can contain a single '*' wildcard which will match 0 or
 *                      more characters.
 */
extern void setEscapedProperties(const TCHAR **propertyNames);

/**
 * Returns true if the specified property matches one of the property names
 *  previosly set in a call to setEscapableProperties()
 *
 * @param propertyName Property name to test.
 *
 * @return TRUE if the property should be escaped.  FALSE otherwise.
 */
extern int isEscapedProperty(const TCHAR *propertyName);

/**
 * Adds a single property to the properties structure.
 *
 * @param properties Properties structure to add to.
 * @param filename Name of the file from which the property was loaded.  NULL, if not from a file.
 * @param lineNum Line number of the property declaration in the file.  Ignored if filename is NULL.
 * @param propertyName Name of the new Property.
 * @param propertyValue Initial property value.
 * @param finalValue TRUE if the property should be set as static.
 * @param quotable TRUE if the property could contain quotes.
 * @param escapable TRUE if the propertyValue can be escaped if its propertyName
 *                  is in the list set with setEscapableProperties().
 * @param internal TRUE if the property is a Wrapper internal property.
 *
 * @return The newly created Property, or NULL if there was a reported error.
 */
extern Property* addProperty(Properties *properties, const TCHAR* filename, int lineNum, const TCHAR *propertyName, const TCHAR *propertyValue, int finalValue, int quotable, int escapable, int internal);

/**
 * Takes a name/value pair in the form <name>=<value> and attempts to add
 * it to the specified properties table.
 *
 * @param properties Properties structure to add to.
 * @param filename Name of the file from which the property was loaded.  NULL, if not from a file.
 * @param lineNum Line number of the property declaration in the file.  Ignored if filename is NULL.
 * @param propertyNameValue The "name=value" pair to create the property from.
 * @param finalValue TRUE if the property should be set as static.
 * @param quotable TRUE if the property could contain quotes.
 * @param internal TRUE if the property is a Wrapper internal property.
 *
 * Returns 0 if successful, otherwise 1
 */
extern int addPropertyPair(Properties *properties, const TCHAR* filename, int lineNum, const TCHAR *propertyNameValue, int finalValue, int quotable, int internal);

extern const TCHAR* getStringProperty(Properties *properties, const TCHAR *propertyName, const TCHAR *defaultValue);

extern const TCHAR* getFileSafeStringProperty(Properties *properties, const TCHAR *propertyName, const TCHAR *defaultValue);

/**
 * Returns a sorted array of all properties beginning with {propertyNameBase}.
 *  Only numerical characters can be returned between the two.
 *
 * The calling code must always call freeStringProperties to make sure that the
 *  malloced propertyNames, propertyValues, and propertyIndices arrays are freed
 *  up correctly.  This is only necessary if the function returns 0.
 *
 * @param properties The full properties structure.
 * @param propertyNameHead All matching properties must begin with this value.
 * @param propertyNameTail All matching properties must end with this value.
 * @param all If FALSE then the array will start with #1 and loop up until the
 *            next property is not found, if TRUE then all properties will be
 *            returned, even if there are gaps in the series.
 * @param matchAny If FALSE only numbers are allowed as placeholder
 * @param propertyNames Returns a pointer to a NULL terminated array of
 *                      property names.
 * @param propertyValues Returns a pointer to a NULL terminated array of
 *                       property values.
 * @param propertyIndices Returns a pointer to a 0 terminated array of
 *                        the index numbers used in each property name of
 *                        the propertyNames array.
 *
 * @return 0 if successful, -1 if there was an error.
 */
extern int getStringProperties(Properties *properties, const TCHAR *propertyNameHead, const TCHAR *propertyNameTail, int all, int matchAny, TCHAR ***propertyNames, TCHAR ***propertyValues, long unsigned int **propertyIndices);

/**
 * Frees up an array of properties previously returned by getStringProperties().
 */
extern void freeStringProperties(TCHAR **propertyNames, TCHAR **propertyValues, long unsigned int *propertyIndices);

extern int getIntProperty(Properties *properties, const TCHAR *propertyName, int defaultValue);

extern int getBooleanProperty(Properties *properties, const TCHAR *propertyName, int defaultValue);

extern int getBooleanProperties(Properties *properties, const TCHAR *propertyNameHead, const TCHAR *propertyNameTail, int all, int matchAny, TCHAR ***propertyNames, int **propertyValues, long unsigned int **propertyIndices, int defaultValue);

extern void freeBooleanProperties(TCHAR **propertyNames, int *propertyValues, long unsigned int *propertyIndices);

extern int isGeneratedProperty(Properties *properties, const TCHAR *propertyName);

extern int isQuotableProperty(Properties *properties, const TCHAR *propertyName);

extern void dumpProperties(Properties *properties);

/**
 * Level at which properties will be dumped.
 */
extern void setPropertiesDumpLogLevel(Properties *properties, int logLevel);

/**
 * Set to TRUE if warnings about property values should be logged.
 */
extern void setLogPropertyWarnings(Properties *properties, int logWarnings);

/**
 * Level at which any property warnings are logged.
 */
extern void setLogPropertyWarningLogLevel(Properties *properties, int logLevel);

/**
 * Returns the minimum value. This is used in place of the __min macro when the parameters should not be called more than once.
 */
extern int propIntMin(int value1, int value2);

/**
 * Returns the maximum value. This is used in place of the __max macro when the parameters should not be called more than once.
 */
extern int propIntMax(int value1, int value2);

/** Creates a linearized representation of all of the properties.
 *  The returned buffer must be freed by the calling code. */
extern TCHAR *linearizeProperties(Properties *properties, TCHAR separator);

#endif

