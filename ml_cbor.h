#ifndef ML_CBOR_H
#define ML_CBOR_H

#include <stdint.h>

#include "minilang.h"
#include "stringmap.h"

void ml_cbor_init(stringmap_t *Globals);

void ml_cbor_write(ml_value_t *Value, void *Data, int (*WriteFn)(void *Data, const unsigned char *Bytes, unsigned Size));

typedef struct ml_cbor_reader_t ml_cbor_reader_t;

typedef ml_value_t *(*ml_tag_t)(void *Data, ml_value_t *Value);

ml_cbor_reader_t *ml_cbor_reader_new(void *TagFnData, ml_tag_t (*TagFn)(uint64_t, void *, void **));
void ml_cbor_reader_read(ml_cbor_reader_t *Reader, unsigned char *Bytes, int Size);
ml_value_t *ml_cbor_reader_get(ml_cbor_reader_t *Reader);

typedef struct {const void *Data; size_t Length;} ml_cbor_t;

ml_cbor_t ml_to_cbor(ml_value_t *Value);
ml_value_t *ml_from_cbor(ml_cbor_t Cbor, void *TagFnData, ml_tag_t (*TagFn)(uint64_t, void *, void **));

typedef int (*ml_cbor_write_fn)(void *UserData, const unsigned char *Bytes, unsigned Size);

void ml_cbor_write_integer(void *UserData, ml_cbor_write_fn WriteFn, int64_t Number);
void ml_cbor_write_positive(void *UserData, ml_cbor_write_fn WriteFn, uint64_t Number);
void ml_cbor_write_negative(void *UserData, ml_cbor_write_fn WriteFn, uint64_t Number);
void ml_cbor_write_bytes(void *UserData, ml_cbor_write_fn WriteFn, unsigned Size);
void ml_cbor_write_indef_bytes(void *UserData, ml_cbor_write_fn WriteFn);
void ml_cbor_write_string(void *UserData, ml_cbor_write_fn WriteFn, unsigned Size);
void ml_cbor_write_indef_string(void *UserData, ml_cbor_write_fn WriteFn);
void ml_cbor_write_array(void *UserData, ml_cbor_write_fn WriteFn, unsigned Size);
void ml_cbor_write_indef_array(void *UserData, ml_cbor_write_fn WriteFn);
void ml_cbor_write_map(void *UserData, ml_cbor_write_fn WriteFn, unsigned Size);
void ml_cbor_write_indef_map(void *UserData, ml_cbor_write_fn WriteFn);
void ml_cbor_write_float2(void *UserData, ml_cbor_write_fn WriteFn, double Number);
void ml_cbor_write_float4(void *UserData, ml_cbor_write_fn WriteFn, double Number);
void ml_cbor_write_float8(void *UserData, ml_cbor_write_fn WriteFn, double Number);
void ml_cbor_write_simple(void *UserData, ml_cbor_write_fn WriteFn, unsigned char Simple);
void ml_cbor_write_break(void *UserData, ml_cbor_write_fn WriteFn);
void ml_cbor_write_tag(void *UserData, ml_cbor_write_fn WriteFn, uint64_t Tag);

#endif
