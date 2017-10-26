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

#ifdef WIN32
#include <windows.h>
#include <tchar.h>
#else
#ifndef FREEBSD
#include <iconv.h>
#endif
#include <langinfo.h>
#include <errno.h>
#include <limits.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include "logger_base.h"
#ifdef FREEBSD
#include "wrapperinfo.h"
#endif

#if defined(IRIX)
 #define PATH_MAX FILENAME_MAX
#endif

#ifndef TRUE
#define TRUE -1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/**
 * Dynamically load the symbols for the iconv library
 */
#ifdef FREEBSD
typedef void *iconv_t;
static iconv_t (*wrapper_iconv_open)(const char *, const char *);  
static size_t (*wrapper_iconv)(iconv_t, const char **, size_t *, char **, size_t *);  
static int (*wrapper_iconv_close)(iconv_t);
static char iconvLibNameMB[128];
static TCHAR iconvLibNameW[128];
#else
#define wrapper_iconv_open iconv_open
#define wrapper_iconv iconv
#define wrapper_iconv_close iconv_close
#endif

#if defined(UNICODE) && defined(WIN32)
/**
 * @param multiByteChars The MultiByte encoded source string.
 * @param encoding Encoding of the MultiByte characters.
 * @param outputBuffer If return is TRUE then this will be an error message.  If return is FALSE then this will contain the
 *                     requested WideChars string.  If there were any memory problems, the return will be TRUE and the
 *                     buffer will be set to NULL.  In any case, it is the responsibility of the caller to free the output
 *                     buffer memory. 
 * @param localizeErrorMessage TRUE if the error message can be localized.
 *
 * @return TRUE if there were problems, FALSE if Ok.
 *
 */
int multiByteToWideChar(const char *multiByteChars, int encoding, TCHAR **outputBufferW, int localizeErrorMessage) {
    const TCHAR *errorTemplate;
    size_t errorTemplateLen;
    int req;
    
    /* Clear the output buffer as a sanity check.  Shouldn't be needed. */
    *outputBufferW = NULL;
    
    req = MultiByteToWideChar(encoding, MB_ERR_INVALID_CHARS, multiByteChars, -1, NULL, 0);
    if (req <= 0) {
        if (GetLastError() == ERROR_NO_UNICODE_TRANSLATION) {
            errorTemplate = (localizeErrorMessage ? TEXT("Invalid multibyte sequence.") : TEXT("Invalid multibyte sequence."));
            errorTemplateLen = _tcslen(errorTemplate) + 1;
            *outputBufferW = malloc(sizeof(TCHAR) * errorTemplateLen);
            if (*outputBufferW) {
                _sntprintf(*outputBufferW, errorTemplateLen, TEXT("%s"), errorTemplate);
            } else {
                /* Out of memory. *outputBufferW already NULL. */
            }
            return TRUE;
        } else {
            errorTemplate = (localizeErrorMessage ? TEXT("Unexpected conversion error: %d") : TEXT("Unexpected conversion error: %d"));
            errorTemplateLen = _tcslen(errorTemplate) + 10 + 1;
            *outputBufferW = malloc(sizeof(TCHAR) * errorTemplateLen);
            if (*outputBufferW) {
                _sntprintf(*outputBufferW, errorTemplateLen, errorTemplate, GetLastError());
            } else {
                /* Out of memory. *outputBufferW already NULL. */
            }
            return TRUE;
        }
    }
    *outputBufferW = malloc((req + 1) * sizeof(TCHAR));
    if (!(*outputBufferW)) {
        _tprintf(TEXT("Out of memory (%s%02d)"), TEXT("MBTWC"), 1);
        /* Out of memory. *outputBufferW already NULL. */
        return TRUE;
    }
    
    MultiByteToWideChar(encoding, MB_ERR_INVALID_CHARS, multiByteChars, -1, *outputBufferW, req + 1);
    return FALSE;
}
#endif


#if defined(UNICODE) && !defined(WIN32)
#include <fcntl.h>



/**
 * Converts a MultiByte encoded string to a WideChars (UNICODE/Locale dependant) string.
 *
 * @param multiByteChars The MultiByte encoded source string.
 * @param multiByteEncoding The source encoding.
 * @param interumEncoding The interum encoding before transforming to Wide Chars (On solaris this is the result encoding.)
 *                        If the ecoding is appended by "//TRANSLIT", "//IGNORE", "//TRANSLIT//IGNORE" then the conversion
 *                        will try to transliterate and or ignore invalid characters without warning.
 * @param outputBufferW If return is TRUE then this will be an error message.  If return is FALSE then this will contain the
 *                      requested WideChars string.  If there were any memory problems, the return will be TRUE and the
 *                      buffer will be set to NULL.  In any case, it is the responsibility of the caller to free the output
 *                      buffer memory. 
 * @param localizeErrorMessage TRUE if the error message can be localized.
 *
 * @return TRUE if there were problems, FALSE if Ok.
 */
int multiByteToWideChar(const char *multiByteChars, const char *multiByteEncoding, char *interumEncoding, wchar_t **outputBufferW, int localizeErrorMessage)
{
    const TCHAR *errorTemplate;
    size_t errorTemplateLen;
    size_t iconv_value;
    char *nativeCharStart;
    char *nativeCharStartCopy;
    size_t multiByteCharsLen;
    size_t nativeCharLen;
    size_t nativeCharLenCopy;
    size_t multiByteCharsLenStart;
#if defined(FREEBSD) || defined(SOLARIS) || (defined(AIX) && defined(USE_LIBICONV_GNU))
    const char* multiByteCharsStart;
#else
    char* multiByteCharsStart;
#endif
    iconv_t conv_desc;
    int didIConv;
    int redoIConv;
    size_t wideCharLen;
    
    /* Clear the output buffer as a sanity check.  Shouldn't be needed. */
    *outputBufferW = NULL;

    /* First we need to convert from the multi-byte string to native. */
    /* If the multiByteEncoding and interumEncoding encodings are equal then there is nothing to do. */
    if (strcmp(multiByteEncoding, interumEncoding) != 0 && strcmp(interumEncoding, "646") != 0) {
        conv_desc = wrapper_iconv_open(interumEncoding, multiByteEncoding); /* convert multiByte encoding to interum-encoding*/
        if (conv_desc == (iconv_t)(-1)) {
            /* Initialization failure. */
            if (errno == EINVAL) {
                errorTemplate = (localizeErrorMessage ? TEXT("Conversion from '% s' to '% s' is not supported.") : TEXT("Conversion from '% s' to '% s' is not supported."));
                errorTemplateLen = _tcslen(errorTemplate) + strlen(multiByteEncoding) + strlen(interumEncoding) + 1;
                *outputBufferW = malloc(sizeof(TCHAR) * errorTemplateLen);
                if (*outputBufferW) {
                    _sntprintf(*outputBufferW, errorTemplateLen, errorTemplate, multiByteEncoding, interumEncoding);
                } else {
                    /* Out of memory. *outputBufferW already NULL. */
                }
                return TRUE;
            } else {
                errorTemplate = (localizeErrorMessage ? TEXT("Initialization failure in iconv: %d") : TEXT("Initialization failure in iconv: %d"));
                errorTemplateLen = _tcslen(errorTemplate) + 10 + 1;
                *outputBufferW = malloc(sizeof(TCHAR) * errorTemplateLen);
                if (*outputBufferW) {
                    _sntprintf(*outputBufferW, errorTemplateLen, errorTemplate, errno);
                } else {
                    /* Out of memory. *outputBufferW already NULL. */
                }
                return TRUE;
            }
        }
        multiByteCharsLen = strlen(multiByteChars);
        if (!multiByteCharsLen) {
            wrapper_iconv_close(conv_desc);
            /* The input string is empty, so the output will be as well. */
            *outputBufferW = malloc(sizeof(TCHAR));
            if (*outputBufferW) {
                (*outputBufferW)[0] = TEXT('\0');
                return FALSE;
            } else {
                /* Out of memory. *outputBufferW already NULL. */
                return TRUE;
            }
        }
        ++multiByteCharsLen; /* add 1 in order to store \0 - especially necessary in UTF-8 -> UTF-8 conversions*/
        
        /* We need to figure out how many bytes we need to store the native encoded string. */
        nativeCharLen = multiByteCharsLen;
        nativeCharStart = NULL;
        do {
            redoIConv = FALSE;
            
            if (nativeCharStart) {
                free(nativeCharStart);
            }
            
            multiByteCharsLenStart = multiByteCharsLen;
#if defined(FREEBSD) || defined(SOLARIS) || (defined(AIX) && defined(USE_LIBICONV_GNU))
            multiByteCharsStart = multiByteChars;
#else
            multiByteCharsStart = (char *)multiByteChars;
#endif
            nativeCharStart = malloc(nativeCharLen);
            if (!nativeCharStart) {
                wrapper_iconv_close(conv_desc);
                /* Out of memory. */
                *outputBufferW = NULL;
                return TRUE;
            }
            
            /* Make a copy of the nativeCharLen as this call will replace it with the number of chars used. */
            nativeCharLenCopy = nativeCharLen;
            nativeCharStartCopy = nativeCharStart;
            iconv_value = wrapper_iconv(conv_desc, &multiByteCharsStart, &multiByteCharsLenStart, &nativeCharStartCopy, &nativeCharLenCopy);
             /* Handle failures. */
            if (iconv_value == (size_t)-1) {
                /* See "man 3 iconv" for an explanation. */
                switch (errno) {
                case EILSEQ:
                    wrapper_iconv_close(conv_desc);
                    free(nativeCharStart);
                    errorTemplate = (localizeErrorMessage ? TEXT("Invalid multibyte sequence.") : TEXT("Invalid multibyte sequence."));
                    errorTemplateLen = _tcslen(errorTemplate) + 1;
                    *outputBufferW = malloc(sizeof(TCHAR) * errorTemplateLen);
                    if (*outputBufferW) {
                        _sntprintf(*outputBufferW, errorTemplateLen, errorTemplate);
                    } else {
                        /* Out of memory. *outputBufferW already NULL. */
                    }
                    return TRUE;
                    
                case EINVAL:
                    wrapper_iconv_close(conv_desc);
                    free(nativeCharStart);
                    errorTemplate = (localizeErrorMessage ? TEXT("Incomplete multibyte sequence.") : TEXT("Incomplete multibyte sequence."));
                    errorTemplateLen = _tcslen(errorTemplate) + 1;
                    *outputBufferW = malloc(sizeof(TCHAR) * errorTemplateLen);
                    if (*outputBufferW) {
                        _sntprintf(*outputBufferW, errorTemplateLen, errorTemplate);
                    } else {
                        /* Out of memory. *outputBufferW already NULL. */
                    }
                    return TRUE;
                    
                case E2BIG:
                    /* native char buffer was too small, extend buffer and redo */
                    nativeCharLen += multiByteCharsLen;
                    redoIConv = TRUE;
                    break;
                    
                default:
                    wrapper_iconv_close(conv_desc);
                    free(nativeCharStart);
                    errorTemplate = (localizeErrorMessage ? TEXT("Unexpected iconv error: %d") : TEXT("Unexpected iconv error: %d"));
                    errorTemplateLen = _tcslen(errorTemplate) + 10 + 1;
                    *outputBufferW = malloc(sizeof(TCHAR) * errorTemplateLen);
                    if (*outputBufferW) {
                        _sntprintf(*outputBufferW, errorTemplateLen, errorTemplate, errno);
                    } else {
                        /* Out of memory. *outputBufferW already NULL. */
                    }
                    return TRUE;
                }
            }
        } while (redoIConv);
        
        /* finish iconv */
        if (wrapper_iconv_close(conv_desc)) {
            free(nativeCharStart);
            errorTemplate = (localizeErrorMessage ? TEXT("Cleanup failure in iconv: %d") : TEXT("Cleanup failure in iconv: %d"));
            errorTemplateLen = _tcslen(errorTemplate) + 10 + 1;
            *outputBufferW = malloc(sizeof(TCHAR) * errorTemplateLen);
            if (*outputBufferW) {
                _sntprintf(*outputBufferW, errorTemplateLen, errorTemplate, errno);
            } else {
                /* Out of memory. *outputBufferW already NULL. */
            }
            return TRUE;
        }
        didIConv = TRUE;
    } else {
        nativeCharStart = (char *)multiByteChars;
        didIConv = FALSE;
    }

    /* now store the result into a wchar_t */
    wideCharLen = mbstowcs(NULL, nativeCharStart, MBSTOWCS_QUERY_LENGTH);
    if (wideCharLen == (size_t)-1) {
        if (didIConv) {
            free(nativeCharStart);
        }
        if (errno == EILSEQ) {
            errorTemplate = (localizeErrorMessage ? TEXT("Invalid multibyte sequence.") : TEXT("Invalid multibyte sequence."));
            errorTemplateLen = _tcslen(errorTemplate) + 1;
        } else {
            errorTemplate = (localizeErrorMessage ? TEXT("Unexpected iconv error: %d") : TEXT("Unexpected iconv error: %d"));
            errorTemplateLen = _tcslen(errorTemplate) + 10 + 1;
        }
        *outputBufferW = malloc(sizeof(TCHAR) * errorTemplateLen);
        if (*outputBufferW) {
            _sntprintf(*outputBufferW, errorTemplateLen, errorTemplate, errno);
        } else {
            /* Out of memory. *outputBufferW already NULL. */
        }
        return TRUE;
    }
    *outputBufferW = malloc(sizeof(wchar_t) * (wideCharLen + 1));
    if (!(*outputBufferW)) {
        /* Out of memory. *outputBufferW already NULL. */
        if (didIConv) {
            free(nativeCharStart);
        }
        return TRUE;
    }
    mbstowcs(*outputBufferW, nativeCharStart, wideCharLen + 1);
    (*outputBufferW)[wideCharLen] = TEXT('\0'); /* Avoid bufferflows caused by badly encoded characters. */
    
    /* free the native char */
    if (didIConv) {
        free(nativeCharStart);
    }
    return FALSE;
}

size_t _treadlink(TCHAR* exe, TCHAR* fullPath, size_t size) {
    char* cExe;
    char* cFullPath;
    size_t req;

    req = wcstombs(NULL, exe, 0);
    if (req == (size_t)-1) {
        return (size_t)-1;
    }
    cExe = malloc(req + 1);
    if (cExe) {
        wcstombs(cExe, exe, req + 1);
        cFullPath = malloc(size);
        if (cFullPath) {
            req = readlink(cExe, cFullPath, size);
            if (req == (size_t)-1) {
                free(cFullPath);
                free(cExe);
                return (size_t)-1;
            }
            
            req = mbstowcs(fullPath, cFullPath, size);
            if (req == (size_t)-1) {
                free(cFullPath);
                free(cExe);
                return (size_t)-1;
            }
            fullPath[size - 1] = TEXT('\0'); /* Avoid bufferflows caused by badly encoded characters. */
            
            free(cFullPath);
            free(cExe);
            return req * sizeof(TCHAR);
        } else {
            free(cExe);
        }
    }
    return (size_t)-1;
}

/**
 * This Wrapper function internally does a malloc to generate the
 *  Wide-char version of the return string.  This must be freed by the caller.
 */
TCHAR* _tgetcwd(TCHAR *buf, size_t size) {
    char* cBuf;
    size_t len;
    
    if (buf) {
        cBuf = malloc(size);
        if (cBuf) {
            if (getcwd(cBuf, size) != NULL) {
                len = mbstowcs(buf, cBuf, size);
                if (len == (size_t)-1) {
                    /* Failed. */
                    free(cBuf);
                    return NULL;
                }
                buf[size - 1] = TEXT('\0'); /* Avoid bufferflows caused by badly encoded characters. */
                free(cBuf);
                return buf;
            }
            free(cBuf);
        }
    } 
    return NULL;
}

long _tpathconf(const TCHAR *path, int name) {
    char* cPath;
    size_t req;
    long retVal;

    req = wcstombs(NULL, path, 0);
    if (req == (size_t)-1) {
        return -1;
    }
    cPath = malloc(req + 1);
    if (cPath) {
        wcstombs(cPath, path, req + 1);
        retVal = pathconf(cPath, name);
        free(cPath);
        return retVal;
    }
    return -1;
}

/**
 * Set the current locale.
 *
 * This Wrapper function internally does a malloc to generate the
 *  Wide-char version of the return string.  This must be freed by the caller.
 *
 * @param category
 * @param locale The requested locale. TEXT("") for the default.
 *
 * @return NULL if there are any errors, otherwise return locale.
 */
TCHAR *_tsetlocale(int category, const TCHAR *locale) {
    char* cLocale;
    char* cReturn;
    TCHAR* tReturn;
    size_t req;

    if (locale) {
        req = wcstombs(NULL, locale, 0);
        if (req == (size_t)-1) {
            return NULL;
        }
        cLocale = malloc(sizeof(char) * (req + 1));
        if (!cLocale) {
            return NULL;
        }
        wcstombs(cLocale, locale, req + 1);
    } else {
        cLocale = NULL;
    }
    
    cReturn = setlocale(category, cLocale);
    if (cLocale) {
        free(cLocale);
    }
    
    if (cReturn) {
        req = mbstowcs(NULL, cReturn, MBSTOWCS_QUERY_LENGTH);
        if (req != (size_t)-1) {
            tReturn = malloc(sizeof(TCHAR) * (req + 1));
            if (tReturn) {
                mbstowcs(tReturn, cReturn, req + 1);
                tReturn[req] = TEXT('\0'); /* Avoid bufferflows caused by badly encoded characters. */
                return tReturn;
            }
        }
    }
    return NULL;
}

int _tprintf(const wchar_t *fmt,...) {
    int i, flag;
    wchar_t *msg;
    va_list args;

    if (wcsstr(fmt, TEXT("%s")) != NULL) {
        msg = malloc(sizeof(wchar_t) * (wcslen(fmt) + 1));
        if (msg) {
            wcsncpy(msg, fmt, wcslen(fmt) + 1);
            for (i = 0; i < wcslen(fmt); i++){
                if (fmt[i] == TEXT('%') && i  < wcslen(fmt) && fmt[i + 1] == TEXT('s') && (i == 0 || fmt[i - 1] != TEXT('%'))) {
                    msg[i + 1] = TEXT('S');
                    i++;
                }
            }
            msg[wcslen(fmt)] = TEXT('\0');
        }
        flag = TRUE;
    } else {
         msg = (wchar_t*)fmt;
         flag = FALSE;
    }
    if (msg) {
        va_start(args, fmt);
        i = vwprintf(msg, args);
        va_end (args);
        if (flag == TRUE) {
            free(msg);
        }
        return i;
    }
    return -1;
}

int _ftprintf(FILE *stream, const wchar_t *fmt, ...) {
    int i, flag;
    wchar_t *msg;
    va_list args;
    if (wcsstr(fmt, TEXT("%s")) != NULL) {
        msg = malloc(sizeof(wchar_t) * (wcslen(fmt) + 1));
        if (msg) {
            wcsncpy(msg, fmt, wcslen(fmt) + 1);
            for (i = 0; i < wcslen(fmt); i++){
                if (fmt[i] == TEXT('%') && i  < wcslen(fmt) && fmt[i + 1] == TEXT('s') && (i == 0 || fmt[i - 1] != TEXT('%'))) {
                    msg[i + 1] = TEXT('S');
                    i++;
                }
            }
            msg[wcslen(fmt)] = TEXT('\0');
        }
        flag = TRUE;
    } else {
         msg = (wchar_t*)fmt;
         flag = FALSE;
    }
    if (msg) {
        va_start(args, fmt);
        i = vfwprintf(stream, msg, args);
        va_end (args);
        if (flag == TRUE) {
            free(msg);
        }
        return i;
    }
    return -1;
}

int _sntprintf(TCHAR *str, size_t size, const TCHAR *fmt, ...) {
    int i, flag;
    wchar_t *msg;
    va_list args;
    if (wcsstr(fmt, TEXT("%s")) != NULL) {
        msg = malloc(sizeof(wchar_t) * (wcslen(fmt) + 1));
        if (msg) {
            wcsncpy(msg, fmt, wcslen(fmt) + 1);
            for (i = 0; i < wcslen(fmt); i++){
                if (fmt[i] == TEXT('%') && i  < wcslen(fmt) && fmt[i + 1] == TEXT('s') && (i == 0 || fmt[i - 1] != TEXT('%'))) {
                    msg[i + 1] = TEXT('S');
                    i++;
                }
            }
            msg[wcslen(fmt)] = TEXT('\0');
        }
        flag = TRUE;
    } else {
         msg = (wchar_t*)fmt;
         flag = FALSE;
    }
    if (msg) {
        va_start(args, fmt);
        i = vswprintf(str, size, msg, args);
        va_end (args);
        if (flag == TRUE) {
            free(msg);
        }
        return i;
    }
    return -1;
}

int _tremove(const TCHAR *path) {
    char* cPath;
    size_t req;
    int result;

    req = wcstombs(NULL, path, 0);
    if (req == (size_t)-1) {
        return -1;
    }

    cPath = malloc(req + 1);
    if (cPath) {
        wcstombs(cPath, path, req + 1);
        result = remove(cPath);
        free(cPath);
        return result;
    }
    return -1;
}

int _trename(const TCHAR *path, const TCHAR *to) {
    char* cPath;
    char* cTo;
    size_t req;
    int ret;

    ret = -1;
    req = wcstombs(NULL, path, 0);
    if (req == (size_t)-1) {
        return ret;
    }
    
    cPath = malloc(req + 1);
    if (cPath) {
        wcstombs(cPath, path, req + 1);

        req  = wcstombs(NULL, to, 0);
        if (req == (size_t)-1) {
            free(cPath);
            return ret;
        }
        
        cTo = malloc(req + 1);
        if (cTo) {
            wcstombs(cTo, to, req + 1);
            ret = rename(cPath, cTo);
            free(cTo);
        }
        free(cPath);
    }
    return ret;
}

void _tsyslog(int priority, const TCHAR *message) {
    char* cMessage;
    size_t req;

    req = wcstombs(NULL, message, 0);
    if (req == (size_t)-1) {
        return;
    }

    cMessage = malloc(req + 1);
    if (cMessage) {
        wcstombs(cMessage, message, req + 1);
        syslog(priority, "%s", cMessage);
        free(cMessage);
    }
}

/**
 * This Wrapper function internally does a malloc to generate the
 *  Wide-char version of the return string.  This must be freed by the caller.
 *  Only needed inside the following:
 *  #if !defined(WIN32) && defined(UNICODE)
 *  #endif
 */
TCHAR * _tgetenv( const TCHAR * name ) {
    char* cName;
    TCHAR* val;
    size_t req;
    char *cVal;

    req = wcstombs(NULL, name, 0);
    if (req == (size_t)-1) {
        return NULL;
    }
    cName = malloc(sizeof(char) * (req + 1));
    if (cName) {
        wcstombs(cName, name, req + 1);
        cVal = getenv(cName);
        free(cName);
        if (cVal == NULL) {
            return NULL;
        }
        
        req = mbstowcs(NULL, cVal, MBSTOWCS_QUERY_LENGTH);
        if (req == (size_t)-1) {
            /* Failed. */
            return NULL;
        }
        val = malloc(sizeof(TCHAR) * (req + 1));
        if (val) {
            mbstowcs(val, cVal, req + 1);
            val[req] = TEXT('\0'); /* Avoid bufferflows caused by badly encoded characters. */
            return val;
        }
    }
    return NULL;
}

FILE* _tfopen(const wchar_t* file, const wchar_t* mode) {
    int sizeFile, sizeMode;
    char* cFile;
    char* cMode;
    FILE *f = NULL;

    sizeFile = wcstombs(NULL, (wchar_t*)file, 0);
    if (sizeFile == (size_t)-1) {
        return NULL;
    }

    cFile= malloc(sizeFile + 1);
    if (cFile) {
        wcstombs(cFile, (wchar_t*) file, sizeFile + 1);

        sizeMode = wcstombs(NULL, (wchar_t*)mode, 0);
        if (sizeMode == (size_t)-1) {
            free(cFile);
            return NULL;
        }

        cMode= malloc(sizeMode + 1);
        if (cMode) {
            wcstombs(cMode, (wchar_t*) mode, sizeMode + 1);
            f = fopen(cFile, cMode);
            free(cMode);
        }
        free(cFile);
    }
    return f;
}

FILE* _tpopen(const wchar_t* command, const wchar_t* mode) {
    int sizeFile, sizeMode;
    char* cFile;
    char* cMode;
    FILE *f = NULL;

    sizeFile = wcstombs(NULL, (wchar_t*)command, 0);
    if (sizeFile == (size_t)-1) {
        return NULL;
    }

    cFile= malloc(sizeFile + 1);
    if (cFile) {
        wcstombs(cFile, (wchar_t*) command, sizeFile + 1);

        sizeMode = wcstombs(NULL, (wchar_t*)mode, 0);
        if (sizeMode == (size_t)-1) {
            free(cFile);
            return NULL;
        }

        cMode= malloc(sizeMode + 1);
        if (cMode) {
            wcstombs(cMode, (wchar_t*) mode, sizeMode + 1);
            f = popen(cFile, cMode);
            free(cMode);
        }
        free(cFile);
    }
    return f;
}

int _tunlink(const wchar_t* address) {
    int size;
    char *cAddress;

    size = wcstombs(NULL, (wchar_t*)address, 0);
    if (size == (size_t)-1) {
        return -1;
    }

    cAddress= malloc(size + 1);
    if (cAddress) {
        wcstombs(cAddress, (wchar_t*) address, size + 1);
        size = unlink(cAddress);
        free(cAddress);
        return size;
    }
    return -1;
}


int _tmkfifo(TCHAR* arg, mode_t mode) {
    size_t size;
    char *cStr;
    int r; 

    r = -1;
    size = wcstombs(NULL, arg, 0);
    if (size == (size_t)-1) {
        return r;
    }

    cStr = malloc(size + 1);
    if (cStr) {
        wcstombs(cStr, arg, size + 1);
        r = mkfifo(cStr, mode);
        free(cStr);
    }
    return r;
}

int _tchdir(const TCHAR *path) {
    int r;
    size_t size;
    char *cStr;

    r = -1;
    size = wcstombs(NULL, path, 0);
    if (size == (size_t)-1) {
        return r;
    }

    cStr = malloc(size + 1);
    if (cStr) {
        wcstombs(cStr, path, size + 1);
        r = chdir(cStr);
        free(cStr);
    }
    return r;
}

int _texecvp(TCHAR* arg, TCHAR **cmd) {
    char** cCmd;
    char *cArg;
    int i, size;
    size_t req;

    for (i = 0; cmd[i] != NULL; i++) {
        ;
    }
    size = i;
    cCmd = malloc((i + 1) * sizeof *cCmd);
    if (cCmd) {
        for (i = 0; i < size; i++) {
            req  = wcstombs(NULL, cmd[i], 0);
            if (req == (size_t)-1) {
                i--;
                for (; i > 0; i--) {
                    free(cCmd[i]);
                }
                free(cCmd);
                return -1;
            }

            cCmd[i] = malloc(req + 1);
            if (cCmd[i]) {
                wcstombs(cCmd[i], cmd[i], req + 1);
            } else {
                i--;
                for (; i > 0; i--) {
                    free(cCmd[i]);
                }
                free(cCmd);
                return -1;
            }
        }
        cCmd[size] = '\0';

        req = wcstombs(NULL, arg, 0);
        if (req == (size_t)-1) {
            for (; size >= 0; size--) {
                free(cCmd[size]);
            }
            free(cCmd);
            return -1;
        }

        cArg = malloc(req + 1);
        if (cArg) {
            wcstombs(cArg, arg, req + 1);
            i = execvp(cArg, cCmd);
            free(cArg);
        } else {
            i = -1;
        }
        for (; size >= 0; size--) {
            free(cCmd[size]);
        }
        free(cCmd);
        return i;
    }
    return -1;
}

#ifdef ECSCASECMP
int wcscasecmp(const wchar_t* s1, const wchar_t* s2) {
    wint_t a1, a2;

    if (s1 == s2) {
        return 0;
    }

    do {
        a1 = towlower(*s1++);
        a2 = towlower(*s2++);
        if (a1 == L'\0') {
            break;
        }
    } while (a1 == a2);

    return a1 - a2;
}
#endif


#if defined(HPUX) 
int _vsntprintf(wchar_t *ws, size_t n, const wchar_t *format, va_list arg) {
    /* z/OS shows unexpected behaviour if the format string is empty */
    if (ws) {
        ws[0] = TEXT('\0');
    }
    return vswprintf(ws, n, format, arg);
}
#endif

int _texecve(TCHAR* arg, TCHAR **cmd, TCHAR** env) {
    char **cCmd;
    char *cArg;
    char **cEnv;
    int i, sizeCmd, sizeEnv;
    size_t req;

    for (i = 0; cmd[i] != NULL; i++) {
        ;
    }
    sizeCmd = i;
    cCmd = malloc((i + 1) * sizeof *cCmd);
    if (cCmd) {
        for (i = 0; i < sizeCmd; i++) {
            req  = wcstombs(NULL, cmd[i], 0);
            if (req == (size_t)-1) {
                i--;
                for (; i > 0; i--) {
                    free(cCmd[i]);
                }
                free(cCmd);
                return -1;
            }

            cCmd[i] = malloc(req + 1);
            if (cCmd[i]) {
                wcstombs(cCmd[i], cmd[i], req + 1);
            } else {
                i--;
                for (; i > 0; i--) {
                    free(cCmd[i]);
                }
                free(cCmd);
                return -1;
            }
        }
        cCmd[sizeCmd] = '\0';
        for (i = 0; env[i] != NULL; i++) {
            ;
        }
        sizeEnv = i;
        cEnv = malloc((i + 1) * sizeof *cEnv);
        if (!cEnv) {
            for (; sizeCmd >= 0; sizeCmd--) {
                free(cCmd[sizeCmd]);
            }
            free(cCmd);
            return -1;
        }
        for (i = 0; i < sizeEnv; i++) {
            req = wcstombs(NULL, env[i], 0);
            if (req == (size_t)-1) {
                i--;
                for (; i > 0; i--) {
                    free(cEnv[i]);
                }
                free(cEnv);
                for (; sizeCmd >= 0; sizeCmd--) {
                    free(cCmd[sizeCmd]);
                }
                free(cCmd);
                return -1;
            }

            cEnv[i] = malloc(req + 1);
            if (cEnv[i]) {
                wcstombs(cEnv[i], env[i], req + 1);
            } else {
                i--;
                for (; i > 0; i--) {
                    free(cEnv[i]);
                }
                free(cEnv);
                for (; sizeCmd >= 0; sizeCmd--) {
                    free(cCmd[sizeCmd]);
                }
                free(cCmd);
                return -1;
            }
        }
        cEnv[sizeEnv] = '\0';

        req  = wcstombs(NULL, arg, 0);
        if (req == (size_t)-1) {
            for (; sizeEnv >= 0; sizeEnv--) {
                free(cEnv[sizeEnv]);
            }
            free(cEnv);
            for (; sizeCmd >= 0; sizeCmd--) {
                free(cCmd[sizeCmd]);
            }
            free(cCmd);
            return -1;
        }

        cArg = malloc(req + 1);
        if (cArg) {
            wcstombs(cArg, arg, req + 1);
            i = execve(cArg, cCmd, cEnv);
            free(cArg);
        } else {
            i = -1;
        }
        for (; sizeEnv >= 0; sizeEnv--) {
            free(cEnv[sizeEnv]);
        }
        free(cEnv);
        for (; sizeCmd >= 0; sizeCmd--) {
            free(cCmd[sizeCmd]);
        }
        free(cCmd);
        return i;
    }
    return -1;
}

int _topen(const TCHAR *path, int oflag, mode_t mode) {
    char* cPath;
    int r;
    size_t size;

    size = wcstombs(NULL, path, 0);
    if (size == (size_t)-1) {
        return -1;
    }

    cPath = malloc(size + 1);
    if (cPath) {
        wcstombs(cPath, path, size + 1);
        r = open(cPath, oflag, mode);
        free(cPath);
        return r;
    }
    return -1;
}

#if defined(WRAPPER_USE_PUTENV)
/**
 * Normal calls to putenv do not free the string parameter, but UNICODE calls can and should.
 */
int _tputenv(const TCHAR *string) {
    int r;
    size_t size;
    char *cStr;

    size = wcstombs(NULL, (wchar_t*)string, 0);
    if (size == (size_t)-1) {
        return -1;
    }

    cStr = malloc(size + 1);
    if (cStr) {
        wcstombs(cStr, string, size + 1);
        r = putenv(cStr);
        /* Can't free cStr as it becomes part of the environment. */
        /*  free(cstr); */
        return r;
    }
    return -1;
}
#else
int _tsetenv(const TCHAR *name, const TCHAR *value, int overwrite) {
    int r = -1;
    size_t size;
    char *cName;
    char *cValue;

    size = wcstombs(NULL, (wchar_t*)name, 0);
    if (size == (size_t)-1) {
        return -1;
    }

    cName = malloc(size + 1);
    if (cName) {
        wcstombs(cName, name, size + 1);

        size = wcstombs(NULL, (wchar_t*)value, 0);
        if (size == (size_t)-1) {
            free(cName);
            return -1;
        }

        cValue = malloc(size + 1);
        if (cValue) {
            wcstombs(cValue, value, size + 1);

            r = setenv(cName, cValue, overwrite);

            free(cValue);
        }

        free(cName);
    }
    return r;
}

void _tunsetenv(const TCHAR *name) {
    size_t size;
    char *cName;

    size = wcstombs(NULL, (wchar_t*)name, 0);
    if (size == (size_t)-1) {
        return;
    }

    cName = malloc(size + 1);
    if (cName) {
        wcstombs(cName, name, size + 1);

        unsetenv(cName);

        free(cName);
    }
}
#endif

int _tstat(const wchar_t* filename, struct stat *buf) {
    int size;
    char *cFileName;

    size = wcstombs(NULL, (wchar_t*)filename, 0);
    if (size == (size_t)-1) {
        return -1;
    }

    cFileName = malloc(size + 1);
    if (cFileName) {
        wcstombs(cFileName, (wchar_t*) filename, size + 1);
        size = stat(cFileName, buf);
        free(cFileName);
    }
    return size;
}

/**
 * Expands symlinks and resolves /./, /../ and extra '/' characters to produce a
 *  canonicalized absolute pathname.
 *  On some platforms (e.g MACOSX), even if the full path could not be resolved,
 *  the valid part will be copied to resolvedName until a non-existant folder is 
 *  encountered. resolvedName can then be used to point out where the problem was.
 *
 * @param fileName The file name to be resolved.
 * @param resolvedName A buffer large enough to hold the expanded path.
 * @param resolvedNameSize The size of the resolvedName buffer, should usually be PATH_MAX + 1.
 *
 * @return pointer to resolvedName if successful, otherwise NULL and errno is set to indicate the error.
 */
wchar_t* _trealpathN(const wchar_t* fileName, wchar_t *resolvedName, size_t resolvedNameSize) {
    char *cFile;
#if defined(IRIX)
    char resolved[FILENAME_MAX + 1];
#else
    char resolved[PATH_MAX + 1];
#endif
    int sizeFile;
    int req;
    char* returnVal;
    int err = 0;

    sizeFile = wcstombs(NULL, fileName, 0);
    if (sizeFile == (size_t)-1) {
        return NULL;
    }

    cFile = malloc(sizeFile + 1);
    if (cFile) {
        /* Initialize the return value. */
        resolvedName[0] = TEXT('\0');

        wcstombs(cFile, fileName, sizeFile + 1);
        
        /* get the canonicalized absolute pathname */
        resolved[0] = '\0';
        returnVal = realpath(cFile, resolved);
        err = errno;

        free(cFile);

        if (strlen(resolved) > 0) {
            /* In case realpath failed, 'resolved' may contain a part of the path (until the invalid folder).
             * Example: cFile is "/home/user/alex/../nina" but "/home/user/nina" doesn't exist.
             *          => realpath will return NULL and resolved will be set to "/home/user" */
            req = mbstowcs(NULL, resolved, MBSTOWCS_QUERY_LENGTH);
            if (req == (size_t)-1) {
                if (err != 0) {
                    /* use errno set by realpath() if it failed. */
                    errno = err;
                }
                return NULL;
            }
            mbstowcs(resolvedName, resolved, resolvedNameSize);
            resolvedName[resolvedNameSize - 1] = TEXT('\0'); /* Avoid bufferflows caused by badly encoded characters. */
        }
        
        errno = err;
        if (returnVal == NULL) {
            return NULL;
        } else {
            return resolvedName;
        }
    }
    return NULL;
}
#endif

/**
 * Convert a string to lowercase. A new string will be allocated.
 *
 * @param value Input string
 *
 * @return The converted string.
 */
TCHAR* toLower(TCHAR* value) {
    TCHAR* result;
    size_t len;
    size_t i;
    
    len = _tcslen(value);
    result = malloc(sizeof(TCHAR) * (len + 1));
    if (!result) {
        outOfMemory(TEXT("TL"), 1);
        return NULL;
    }
    
    for (i = 0; i < len; i++) {
        result[i] = _totlower(value[i]);
    }
    result[len] = TEXT('\0');
    
    return result;
}


/**
 * Function to get the system encoding name/number for the encoding
 * of the conf file
 *
 * @para String holding the encoding from the conf file
 *
 * @return TRUE if not found, FALSE otherwise
 *
 */
#ifdef WIN32
int getEncodingByName(char* encodingMB, int *encoding) {
#else
int getEncodingByName(char* encodingMB, char** encoding) {
#endif
    if (strIgnoreCaseCmp(encodingMB, "Shift_JIS") == 0) {
#if defined(FREEBSD) || defined (AIX) || defined(MACOSX)
        *encoding = "SJIS";
#elif defined(WIN32)
        *encoding = 932;
#else
        *encoding = "shiftjis";
#endif
    } else if (strIgnoreCaseCmp(encodingMB, "eucJP") == 0) {
#if defined(AIX)
        *encoding = "IBM-eucJP";
#elif defined(WIN32)
        *encoding = 20932;
#else
        *encoding = "eucJP";
#endif
    } else if (strIgnoreCaseCmp(encodingMB, "UTF-8") == 0) {
#if defined(HPUX)
        *encoding = "utf8";
#elif defined(WIN32)
        *encoding = 65001;
#else
        *encoding = "UTF-8";
#endif
    } else if (strIgnoreCaseCmp(encodingMB, "ISO-8859-1") == 0) {
#if defined(WIN32)
        *encoding = 28591;
#elif defined(LINUX)
        *encoding = "ISO-8859-1";
#else
        *encoding = "ISO8859-1";
#endif
    } else if (strIgnoreCaseCmp(encodingMB, "CP1252") == 0) {
#if defined(WIN32)
        *encoding = 1252;
#else
        *encoding = "CP1252";
#endif
    } else if (strIgnoreCaseCmp(encodingMB, "ISO-8859-2") == 0) {
#if defined(WIN32)
        *encoding = 28592;
#elif defined(LINUX)
        *encoding = "ISO-8859-2";
#else
        *encoding = "ISO8859-2";
#endif
    } else if (strIgnoreCaseCmp(encodingMB, "ISO-8859-3") == 0) {
#if defined(WIN32)
        *encoding = 28593;
#elif defined(LINUX)
        *encoding = "ISO-8859-3";
#else
        *encoding = "ISO8859-3";
#endif
    } else if (strIgnoreCaseCmp(encodingMB, "ISO-8859-4") == 0) {
#if defined(WIN32)
        *encoding = 28594;
#elif defined(LINUX)
        *encoding = "ISO-8859-4";
#else
        *encoding = "ISO8859-4";
#endif
    } else if (strIgnoreCaseCmp(encodingMB, "ISO-8859-5") == 0) {
#if defined(WIN32)
        *encoding = 28595;
#elif defined(LINUX)
        *encoding = "ISO-8859-5";
#else
        *encoding = "ISO8859-5";
#endif
    } else if (strIgnoreCaseCmp(encodingMB, "ISO-8859-6") == 0) {
#if defined(WIN32)
        *encoding = 28596;
#elif defined(LINUX)
        *encoding = "ISO-8859-6";
#else
        *encoding = "ISO8859-6";
#endif
    } else if (strIgnoreCaseCmp(encodingMB, "ISO-8859-7") == 0) {
#if defined(WIN32)
        *encoding = 28597;
#elif defined(LINUX)
        *encoding = "ISO-8859-7";
#else
        *encoding = "ISO8859-7";
#endif
    } else if (strIgnoreCaseCmp(encodingMB, "ISO-8859-8") == 0) {
#if defined(WIN32)
        *encoding = 28598;
#elif defined(LINUX)
        *encoding = "ISO-8859-8";
#else
        *encoding = "ISO8859-8";
#endif
    } else if (strIgnoreCaseCmp(encodingMB, "ISO-8859-9") == 0) {
#if defined(WIN32)
        *encoding = 28599;
#elif defined(LINUX)
        *encoding = "ISO-8859-9";
#else
        *encoding = "ISO8859-9";
#endif
    } else if (strIgnoreCaseCmp(encodingMB, "ISO-8859-10") == 0) {
#if defined(WIN32)
        *encoding = 28600;
#elif defined(LINUX)
        *encoding = "ISO-8859-10";
#else
        *encoding = "ISO8859-10";
#endif
    } else if (strIgnoreCaseCmp(encodingMB, "ISO-8859-11") == 0) {
#if defined(WIN32)
        *encoding = 28601;
#elif defined(LINUX)
        *encoding = "ISO-8859-11";
#else
        *encoding = "ISO8859-11";
#endif
    } else if (strIgnoreCaseCmp(encodingMB, "ISO-8859-13") == 0) {
#if defined(WIN32)
        *encoding = 28603;
#elif defined(LINUX)
        *encoding = "ISO-8859-13";
#else
        *encoding = "ISO8859-13";
#endif
    } else if (strIgnoreCaseCmp(encodingMB, "ISO-8859-14") == 0) {
#if defined(WIN32)
        *encoding = 28604;
#elif defined(LINUX)
        *encoding = "ISO-8859-14";
#else
        *encoding = "ISO8859-14";
#endif
    } else if (strIgnoreCaseCmp(encodingMB, "ISO-8859-15") == 0) {
#if defined(WIN32)
        *encoding = 28605;
#elif defined(LINUX)
        *encoding = "ISO-8859-15";
#else
        *encoding = "ISO8859-15";
#endif
    } else if (strIgnoreCaseCmp(encodingMB, "ISO-8859-16") == 0) {
#if defined(WIN32)
        *encoding = 28606;
#elif defined(LINUX)
        *encoding = "ISO-8859-16";
#else
        *encoding = "ISO8859-16";
#endif
    } else if (strIgnoreCaseCmp(encodingMB, "CP1250") == 0) {
#if defined(WIN32)
        *encoding = 1250;
#else
        *encoding = "CP1250";
#endif
    } else if (strIgnoreCaseCmp(encodingMB, "CP1251") == 0) {
#if defined(WIN32)
        *encoding = 1251;
#else
        *encoding = "CP1251";
#endif
    } else if (strIgnoreCaseCmp(encodingMB, "KOI8-R") == 0) {
#if defined(WIN32)
        *encoding = 20866;
#else
        *encoding = "KOI8-R";
#endif
    } else if (strIgnoreCaseCmp(encodingMB, "KOI8-U") == 0) {
#if defined(WIN32)
        *encoding = 21866;
#else
        *encoding = "KOI8-U";
#endif
    } else if (strIgnoreCaseCmp(encodingMB, "DEFAULT") == 0) {
#ifdef WIN32
            *encoding = GetACP();
#else 
            *encoding = nl_langinfo(CODESET);
 #ifdef MACOSX
            if (strlen(*encoding) == 0) {
                *encoding = "UTF-8";
            }
 #endif
#endif
    } else {
        return TRUE;
    }
    return FALSE;
}

/**
 * Gets the error code for the last operation that failed.
 */
int wrapperGetLastError() {
#ifdef WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

/*
 * Corrects a path in place by replacing all '/' characters with '\'
 *  on Windows platforms.  Does nothing on NIX platforms.
 *
 * filename - Filename to be modified.  Could be null.
 */
void wrapperCorrectWindowsPath(TCHAR *filename) {
#ifdef WIN32
    TCHAR *c;

    if (filename) {
        c = (TCHAR *)filename;
        while((c = _tcschr(c, TEXT('/'))) != NULL) {
            c[0] = TEXT('\\');
        }
    }
#endif
}

/*
 * Corrects a path in place by replacing all '\' characters with '/'
 *  on NIX platforms.  Does nothing on Windows platforms.
 *
 * filename - Filename to be modified.  Could be null.
 */
void wrapperCorrectNixPath(TCHAR *filename) {
#ifndef WIN32
    TCHAR *c;

    if (filename) {
        c = (TCHAR *)filename;
        while((c = _tcschr(c, TEXT('\\'))) != NULL) {
            c[0] = TEXT('/');
        }
    }
#endif
}

#ifdef FREEBSD
/**
 * Get the name of the iconv library that was loaded.
 *
 * @return the name of the iconv library (wide chars).
 */
TCHAR* getIconvLibName() {
    mbstowcs(iconvLibNameW, iconvLibNameMB, 128);
    return iconvLibNameW;
} 

/**
 * Locate an iconv function in the dynamically loaded library.
 *
 * @param libHandle handle returned from a call to dlopen()
 * @param fptr      pointer to the function
 * @param fname1    first name to search
 * @param fname2    second name to search
 * @param fname3    third name to search
 *
 * @return TRUE if there were any problems, FALSE otherwise.
 */
int locateIconvFunction(void *libHandle, void **fptr, const char *fname1, const char *fname2, const char *fname3) {
    const char *error1;
    const char *error2;
    const char *error3;
    void *func = *fptr;
    
    *(void**)(&func) = dlsym(libHandle, fname1);
    if (!func) {
        /* The string that dlerror is in a static buffer and should not be freed. It must be immediately used or copied. */
        error1 = dlerror();
        *(void**)(&func) = dlsym(libHandle, fname2);
        if (!func) {
            error2 = dlerror();
            *(void**)(&func) = dlsym(libHandle, fname3);
            if (!func) {
                error3 = dlerror();
                printf("Failed to locate the %s function from the iconv library (%s): %s\n", fname1, iconvLibNameMB, (error1 ? error1 : "<null>"));
                printf("Failed to locate the %s function from the iconv library (%s): %s\n", fname2, iconvLibNameMB, (error2 ? error2 : "<null>"));
                printf("Failed to locate the %s function from the iconv library (%s): %s\n", fname3, iconvLibNameMB, (error3 ? error3 : "<null>"));
                printf("Unable to continue.\n");
                return TRUE;
            }
        }
    }
    *fptr = func;
    
    return FALSE;
}

/**
 * Tries to load libiconv and then fallback in FreeBSD.
 * Unfortunately we can not do any pretty logging here as iconv is
 *  required for all of that to work.
 * Limitation: currently the function will not try the next library
 *  if the iconv functions failed to load correctly.
 *
 * @return TRUE if there were any problems, FALSE otherwise.
 */
int loadIconvLibrary() {
    void *libHandle;
    const char *error;
    
    /* After 2013-10-08 (254273), FreeBSD 10-x have the iconv functions in libc. 
     *  Unfortunately there is a problem on the handle when opening libc dynamically.
     *  We assume there is always at least one of the following libraries on the system. */
    
    /* iconv library name present from FreeBSD 7 to 9 */
    strncpy(iconvLibNameMB, "/usr/local/lib/libiconv.so", 128);
    libHandle = dlopen(iconvLibNameMB, RTLD_NOW);

    /* Falling back to libbiconv library in FreeBSD 10 */
    if (libHandle == NULL) {
        strncpy(iconvLibNameMB, "/usr/local/lib/libbiconv.so", 128);
        libHandle = dlopen(iconvLibNameMB, RTLD_NOW);
    }

    /* Falling back to libkiconv.4 in FreeBSD 10 */
    if (libHandle == NULL && _tcscmp(wrapperBits, TEXT("32")) == 0) {
        /* If the 32-bit version of the Wrapper is running on a 64-bit system,
         *  the correct library is /usr/lib32/libkiconv.so.4.
         *  Be careful here as not being able to find the library doesn't
         *  necessarily mean that the system is 32-bit. */
        strncpy(iconvLibNameMB, "/usr/lib32/libkiconv.so.4", 128);
        libHandle = dlopen(iconvLibNameMB, RTLD_NOW);
    }
    
    if (libHandle == NULL) {
        strncpy(iconvLibNameMB, "/lib/libkiconv.so.4", 128);
        libHandle = dlopen(iconvLibNameMB, RTLD_NOW);
    }

    /* No library found, we cannot continue as we need iconv support */
    if (!libHandle) {
        /* The string that dlerror is in a static buffer and should not be freed. It must be immediately used or copied. */
        error = dlerror();
        printf("Failed to locate the iconv library: %s\n", (error ? error : "<null>"));
        printf("Unable to continue.\n");
        return TRUE;
    }
    
    /* Look up the required functions. Return true if any of them could not be found. */
    return locateIconvFunction(libHandle, (void**)&wrapper_iconv_open,  "iconv_open",  "libiconv_open",  "__bsd_iconv_open") ||
           locateIconvFunction(libHandle, (void**)&wrapper_iconv,       "iconv",       "libiconv",       "__bsd_iconv")      ||
           locateIconvFunction(libHandle, (void**)&wrapper_iconv_close, "iconv_close", "libiconv_close", "__bsd_iconv_close");
}
#endif

#ifdef DEBUG_MALLOC
 /* There can't be any more malloc calls after the malloc2 function in this file. */
 #undef malloc
void *malloc2(size_t size, const char *file, int line, const char *func, const char *sizeVar) {
    void *ptr;
 #ifdef WIN32
    wprintf(L"%S:%d:%S malloc(%S) -> malloc(%d)", file, line, func, sizeVar, size);
 #else	
    wprintf(L"%s:%d:%s malloc(%s) -> malloc(%d)", file, line, func, sizeVar, size);
 #endif
    ptr = malloc(size);
    wprintf(L" -> %p\n", ptr);
    return ptr;
}
#endif

