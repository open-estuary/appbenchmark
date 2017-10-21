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

#ifndef _WRAPPER_HASHMAP
 #define _WRAPPER_HASHMAP

 /*#define _DEBUG_HASHMAP */
 /*#define _DEBUG_HASHMAP_DETAILED*/

 #include "wrapper_i18n.h"

typedef struct HashEntry HashEntry, *PHashEntry;
struct HashEntry {
    size_t keySize;
    void *key;
    size_t valueSize;
    void *value;
    PHashEntry nextEntry;
};

typedef struct {
    int size;
    PHashEntry firstEntry;
} HashBucket, *PHashBucket;

typedef struct {
    int bucketCount;
    int size;
    PHashBucket *buckets;
} HashMap, *PHashMap;

/**
 * Frees up any memory used by a HashMap.  Any values returned from the HashMap will also
 *  cease to be valid.
 *
 * @param hashMap HashMap to be freed.
 */
extern void freeHashMap(PHashMap hashMap);

/**
 * Creates an empty HashMap with the specified number of buckets.
 *
 * @param bucketCount The number of buckets.  The search within a bucket is
 *                    linear so it should be large enough to store all data
 *                    without too many items in each bucket.
 *
 * @return The new HashMap.
 */
extern PHashMap newHashMap(int bucketCount);

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
extern int hashMapPutKVVV(PHashMap hashMap, const void *key, size_t keySize, const void *value, size_t valueSize);

/**
 * Puts a value into the HashMap.  The key and value will both be cloned.
 *
 * @param hashMap HashMap to store the value into.
 * @param key The key to reference the value.
 * @param value The value to store.
 */
extern void hashMapPutKWVW(PHashMap hashMap, const TCHAR *key, const TCHAR *value);

/**
 * Puts a value into the HashMap.  The key and value will both be cloned.
 *
 * @param hashMap HashMap to store the value into.
 * @param key The key to reference the value.
 * @param value The value to store.
 */
extern void hashMapPutKMBVW(PHashMap hashMap, const char *key, const TCHAR *value);

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
extern const void *hashMapGetKVVV(PHashMap hashMap, const void *key, size_t keySize, size_t *valueSize);

/**
 * Gets a value from the HashMap.
 *
 * @param hashMap HashMap from which to lookup the value.
 * @param key Key of the value being looked up.
 *
 * @return a reference to the value.  It should not be modified or freed.
 */
extern const TCHAR *hashMapGetKWVW(PHashMap hashMap, const TCHAR *key);

/**
 * Gets a value from the HashMap.
 *
 * @param hashMap HashMap from which to lookup the value.
 * @param key Key of the value being looked up.
 *
 * @return a reference to the value.  It should not be modified or freed.
 */
extern const TCHAR *hashMapGetKMBVW(PHashMap hashMap, const char *key);

 #ifdef _DEBUG_HASHMAP
/**
 * Dumps the statistics of a HashMap.
 */
extern void dumpHashMapStats(PHashMap hashMap);
 #endif

#endif

