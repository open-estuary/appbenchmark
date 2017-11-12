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
 #include <stdlib.h>
 #include <stdio.h>
#endif
#include <string.h>
#include "wrapper_hashmap.h"
#include "wrapper_i18n.h"

#ifndef TRUE
 #define TRUE -1
#endif

#ifndef FALSE
 #define FALSE 0
#endif

/**
 * Compare two memory blocks.
 *
 * @param vA First memory block.
 * @param vALen Size of first memory block.
 * @param vB Second memory block.
 * @param vBLen Size of second memory block.
 *
 * @return -1 if vA is smaller, 0 if equal, 1 if vA is greater than vB.
 */
int memcmpHM(const void *vA, size_t vALen, const void *vB, size_t vBLen) {
    size_t i;
    unsigned char cA, cB;
    
    for (i = 0; (i < vALen) && (i < vBLen); i++) {
        cA = ((unsigned char *)vA)[i];
        cB = ((unsigned char *)vB)[i];
        
        if (cA < cB) {
            return -1;
        } else if (cA > cB) {
            return 1;
        }
    }
    /* Lengths are different. */
    if (vALen < vBLen) {
        return -1;
    } else if (vALen > vBLen ) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * Frees up any memory used by a HashMap.  Any values returned from the HashMap will also
 *  cease to be valid.
 *
 * @param hashMap HashMap to be freed.
 */
void freeHashMap(PHashMap hashMap) {
    int i;
    PHashBucket bucket;
    PHashEntry thisEntry;
    PHashEntry nextEntry;
    
    if (!hashMap) {
        return;
    }
    
    if (hashMap->buckets) {
        for (i = 0; i < hashMap->bucketCount; i++) {
            bucket = hashMap->buckets[i];
            
            /* Free up the entries */
            thisEntry = bucket->firstEntry;
            while (thisEntry) {
                nextEntry = thisEntry->nextEntry;
                
                /* Free up the entry. */
                if (thisEntry->key) {
                    free(thisEntry->key);
                }
                if (thisEntry->value) {
                    free(thisEntry->value);
                }
                free(thisEntry);
                
                thisEntry = nextEntry;
            }
            
            /* Free up the bucket. */
            free(bucket);
        }
        
        free(hashMap->buckets);
    }
    
    free(hashMap);
}

/**
 * Creates an empty HashMap with the specified number of buckets.
 *
 * @param bucketCount The number of buckets.  The search within a bucket is
 *                    linear so it should be large enough to store all data
 *                    without too many items in each bucket.
 *
 * @return The new HashMap.
 */
PHashMap newHashMap(int bucketCount) {
    int i;
    PHashMap hashMap;
    PHashBucket bucket;
    
    /* Build up an empty HashMap.  Be careful about clearing memory so a freeHashMap call will always work. */
    hashMap = malloc(sizeof(HashMap));
    if (!hashMap) {
        _tprintf(TEXT("Out of memory (%s)\n"), TEXT("NHM1"));
        return NULL;
    }
    memset(hashMap, 0, sizeof(HashMap));
    
    hashMap->bucketCount = bucketCount;
    
    hashMap->buckets = malloc(sizeof(PHashBucket) * bucketCount);
    if (!hashMap->buckets) {
        _tprintf(TEXT("Out of memory (%s)\n"), TEXT("NHM2"));
        freeHashMap(hashMap);
        return NULL;
    }
    memset(hashMap->buckets, 0, sizeof(PHashBucket) * bucketCount);
    
    for (i = 0; i < hashMap->bucketCount; i++) {
        bucket = malloc(sizeof(HashBucket));
        if (!bucket) {
            _tprintf(TEXT("Out of memory (%s)\n"), TEXT("NHM3"));
            freeHashMap(hashMap);
            return NULL;
        }
        memset(bucket, 0, sizeof(HashBucket));
        hashMap->buckets[i] = bucket;
    }
    
    return hashMap;
}

/**
 * Calculate a bucketId based on the key.
 *
 * @param hashMap HashMap for which the bucket Id is being calculated.
 * @param key Key whose bucket Id is being calculated.
 *
 * @return The bucket Id.
 */
int getBucketId(PHashMap hashMap, const TCHAR *key) {
    size_t len = _tcslen(key);
    size_t i;
    TCHAR hash = 0;
    
    /* Loop over the characters and add all of the characters together. */
    for (i = 0; i < len; i++) {
        hash = (hash + key[i]) & 0xffff;
    }
    
    return hash % hashMap->bucketCount;
}

/**
 * Puts a value into the HashMap.  The key and value will both be cloned.
 *
 * @param hashMap HashMap to store the value into.
 * @param key The key to reference the value.
 * @param keySize The size of the key.
 * @param value The value to store.
 * @param valueSize The size of the value.
 *
 * @return TRUE if there were any problems.
 */
int hashMapPutKVVV(PHashMap hashMap, const void *key, size_t keySize, const void *value, size_t valueSize) {
    void *keyCopy;
    void *valueCopy;
    int bucketId;
    PHashBucket bucket;
    PHashEntry *thisEntryLoc;
    PHashEntry thisEntry;
    int cmp;
    PHashEntry newEntry;
    
#ifdef _DEBUG_HASHMAP_DETAILED
    _tprintf(TEXT("hashMapPutKVVV(%p, %p, %d, %p, %d)\n"), hashMap, key, keySize, value, valueSize);
#endif
    
    /* First create copies of the items being stored. */
    keyCopy = malloc(keySize);
    if (!keyCopy) {
        _tprintf(TEXT("Out of memory (%s)\n"), TEXT("HMP1"));
        return TRUE;
    }
    memcpy(keyCopy, key, keySize);
    valueCopy = malloc(valueSize);
    if (!valueCopy) {
        _tprintf(TEXT("Out of memory (%s)\n"), TEXT("HMP2"));
        free(keyCopy);
        return TRUE;
    }
    memcpy(valueCopy, value, valueSize);
    
    /* Locate the bucket where the value should be stored. */
    bucketId = getBucketId(hashMap, key);
    bucket = hashMap->buckets[bucketId];
    
#ifdef _DEBUG_HASHMAP_DETAILED
    _tprintf(TEXT("  keyCopy=%p, valueCopy=%p, bucketId=%d\n"), keyCopy, valueCopy, bucketId);
#endif
    
    /* Figure out where in the bucket to store the value. */
    thisEntryLoc = &(bucket->firstEntry);
    thisEntry = bucket->firstEntry;
    
    while (thisEntry) {
        cmp = memcmpHM(thisEntry->key, thisEntry->keySize, key, keySize);
        if (cmp > 0) {
            /* This entry's key is bigger, so we should be before it. */
            newEntry = malloc(sizeof(HashEntry));
            if (!newEntry) {
                _tprintf(TEXT("Out of memory (%s)\n"), TEXT("HMP3"));
                free(keyCopy);
                free(valueCopy);
                return TRUE;
            }
            newEntry->key = keyCopy;
            newEntry->keySize = keySize;
            newEntry->value = valueCopy;
            newEntry->valueSize = valueSize;
            newEntry->nextEntry = thisEntry;
            *thisEntryLoc = newEntry;
            bucket->size++;
            hashMap->size++;
#ifdef _DEBUG_HASHMAP_DETAILED
            _tprintf(TEXT("  inserted entry -> bucketSize=%d hashMapSize=%d\n"), bucket->size, hashMap->size);
#endif
            return FALSE;
        } else if (cmp == 0) {
            /* This is the exact same key so we are replacing the value. */
            free(thisEntry->value);
            thisEntry->value = valueCopy;
            thisEntry->valueSize = valueSize;
            free(keyCopy); /* Not needed. */
#ifdef _DEBUG_HASHMAP_DETAILED
            _tprintf(TEXT("  replaced entry -> bucketSize=%d hashMapSize=%d\n"), bucket->size, hashMap->size);
#endif
            return FALSE;
        } else {
            /* This entry's key is smaller so we should be after it.  Keep looking. */
            thisEntryLoc = &(thisEntry->nextEntry);
            thisEntry = thisEntry->nextEntry;
        }
    }
    
    /* If we get here then we need to append our value to the end. */
    newEntry = malloc(sizeof(HashEntry));
    if (!newEntry) {
        _tprintf(TEXT("Out of memory (%s)\n"), TEXT("HMP4"));
        free(keyCopy);
        free(valueCopy);
        return TRUE;
    }
    newEntry->key = keyCopy;
    newEntry->keySize = keySize;
    newEntry->value = valueCopy;
    newEntry->valueSize = valueSize;
    newEntry->nextEntry = NULL; /* This is the end. */
    *thisEntryLoc = newEntry;
    bucket->size++;
    hashMap->size++;
#ifdef _DEBUG_HASHMAP_DETAILED
    _tprintf(TEXT("  append entry -> bucketSize=%d hashMapSize=%d\n"), bucket->size, hashMap->size);
#endif
    return FALSE;
}

/**
 * Puts a value into the HashMap.  The key and value will both be cloned.
 *
 * @param hashMap HashMap to store the value into.
 * @param key The key to reference the value.
 * @param value The value to store.
 */
void hashMapPutKWVW(PHashMap hashMap, const TCHAR *key, const TCHAR *value) {
    size_t keySize = sizeof(TCHAR) * (_tcslen(key) + 1);
    size_t valueSize = sizeof(TCHAR) * (_tcslen(value) + 1);
    
#ifdef _DEBUG_HASHMAP
    /* Can't use gtSTATIC here because of recursion. */
 #if defined(UNICODE) && !defined(WIN32)
    _tprintf(TEXT("hashMapPutKWVW(map, \"%S\", \"%S\")\n"), key, value);
 #else
    _tprintf(TEXT("hashMapPutKWVW(map, \"%s\", \"%s\")\n"), key, value);
 #endif
#endif
    
    hashMapPutKVVV(hashMap, key, keySize, value, valueSize);
}

/**
 * Puts a value into the HashMap.  The key and value will both be cloned.
 *
 * @param hashMap HashMap to store the value into.
 * @param key The key to reference the value.
 * @param value The value to store.
 */
void hashMapPutKMBVW(PHashMap hashMap, const char *key, const TCHAR *value) {
    size_t keySize = sizeof(char) * (strlen(key) + 1);
    size_t valueSize = sizeof(TCHAR) * (_tcslen(value) + 1);
    
    hashMapPutKVVV(hashMap, key, keySize, value, valueSize);
}

/**
 * Gets a value from the HashMap.
 *
 * @param hashMap HashMap from which to lookup the value.
 * @param key Key of the value being looked up.
 * @param keySize Size of the key.
 * @param valueSize Pointer to a size_t which will store the size of the returned value if non-NULL.
 *
 * @return a reference to the value.  It should not be modified or freed.
 */
const void *hashMapGetKVVV(PHashMap hashMap, const void *key, size_t keySize, size_t *valueSize) {
    int bucketId;
    PHashBucket bucket;
    PHashEntry thisEntry;
    int cmp;
    
    /* Initialize the return size. */
    if (valueSize) {
        *valueSize = 0;
    }
    
    /* Locate the bucket where the value should be stored. */
    bucketId = getBucketId(hashMap, key);
    bucket = hashMap->buckets[bucketId];
    
    /* Figure out where in the bucket to store the value. */
    thisEntry = bucket->firstEntry;
    
    while (thisEntry) {
        cmp = memcmpHM(thisEntry->key, thisEntry->keySize, key, keySize);
        if (cmp > 0) {
            /* This entry's key is bigger, so we are past where it should be. */
            return NULL;
        } else if (cmp == 0) {
            /* This is the value we were looking for. */
            if (valueSize) {
                *valueSize = thisEntry->valueSize;
            }
            return thisEntry->value;
        } else {
            /* Not yet. */
            thisEntry = thisEntry->nextEntry;
        }
    }
    /* We didn't find it. */
    return NULL;
}

/**
 * Gets a value from the HashMap.
 *
 * @param hashMap HashMap from which to lookup the value.
 * @param key Key of the value being looked up.
 *
 * @return a reference to the value.  It should not be modified or freed.
 */
const TCHAR *hashMapGetKWVW(PHashMap hashMap, const TCHAR *key) {
    size_t keySize = sizeof(TCHAR) * (_tcslen(key) + 1);
    
    return (const TCHAR *)hashMapGetKVVV(hashMap, key, keySize, NULL);
}

/**
 * Gets a value from the HashMap.
 *
 * @param hashMap HashMap from which to lookup the value.
 * @param key Key of the value being looked up.
 *
 * @return a reference to the value.  It should not be modified or freed.
 */
const TCHAR *hashMapGetKMBVW(PHashMap hashMap, const char *key) {
    size_t keySize = sizeof(char) * (strlen(key) + 1);
    
    return (const TCHAR *)hashMapGetKVVV(hashMap, key, keySize, NULL);
}

#ifdef _DEBUG_HASHMAP
/**
 * Dumps the statistics of a HashMap.
 */
void dumpHashMapStats(PHashMap hashMap) {
    int i;
#ifdef _DEBUG_HASHMAP_DETAILED
    int j;
    PHashEntry entry;
#endif
    
    _tprintf(TEXT("HashMap: %p\n"), hashMap);
    _tprintf(TEXT("  size: %d\n"), hashMap->size);
    _tprintf(TEXT("  bucketCount: %d\n"), hashMap->bucketCount);
    if (hashMap->buckets) {
        for (i = 0; i < hashMap->bucketCount; i++) {
            PHashBucket bucket = hashMap->buckets[i];
            if (bucket) {
                _tprintf(TEXT("  bucket[%d]: size: %d\n"), i, bucket->size);
#ifdef _DEBUG_HASHMAP_DETAILED
                j = 0;
                entry = bucket->firstEntry;
                while (entry) {
                    _tprintf(TEXT("    entry[%d]: key=%p (size=%d)  value=%p (size=%d)\n"), j, entry->key, entry->keySize, entry->value, entry->valueSize);
                    entry = entry->nextEntry;
                    j++;
                }
#endif
            } else {
                _tprintf(TEXT("  bucket[%d]: NULL\n"), i);
            }
        }
    }
}
#endif

