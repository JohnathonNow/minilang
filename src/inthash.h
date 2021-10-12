#ifndef INTHASH_H
#define INTHASH_H

#include <stdlib.h>
#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct inthash_t inthash_t;

struct inthash_t {
	uintptr_t *Keys;
	void **Values;
	int Size, Space;
};

#define INTHASH_INDEX_SHIFT 6
#define INTHASH_INCR_SHIFT 9

#define INTHASH_INIT {NULL, 0, 0}

inthash_t *inthash_new() __attribute__ ((malloc));

void *inthash_search(const inthash_t *Map, uintptr_t Key) __attribute__ ((pure));
void *inthash_insert(inthash_t *Map, uintptr_t Key, void *Value);

static inline void *inthash_search_inline(const inthash_t *Map, uintptr_t Key) {
	if (!Map->Size) return NULL;
	uintptr_t *Keys = Map->Keys;
	size_t Mask = Map->Size - 1;
	size_t Index = (Key >> INTHASH_INDEX_SHIFT) & Mask;
	if (Keys[Index] == Key) return Map->Values[Index];
	if (Keys[Index] < Key) return NULL;
	size_t Incr = (Key >> INTHASH_INCR_SHIFT) | 1;
	do {
		Index = (Index + Incr) & Mask;
		if (Keys[Index] == Key) return Map->Values[Index];
	} while (Keys[Index] > Key);
	return NULL;
}

int inthash_foreach(inthash_t *Map, void *Data, int (*callback)(uintptr_t, void *, void *));

typedef struct {void *Value; int Present;} inthash_result_t;

inthash_result_t inthash_search2(const inthash_t *Map, uintptr_t Key) __attribute__ ((pure));

#ifdef	__cplusplus
}
#endif


#endif
