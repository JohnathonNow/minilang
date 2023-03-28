#ifdef ML_COMPLEX
#include <complex.h>
#undef I
#endif

#include "compare_impl.h"

#define COMPARE_ROW_LEFT_IMPL_BASE(NAME, OP, METH, LEFT) \
COMPARE_ROW_IMPL(NAME, OP, METH, LEFT, uint8_t) \
COMPARE_ROW_IMPL(NAME, OP, METH, LEFT, int8_t) \
COMPARE_ROW_IMPL(NAME, OP, METH, LEFT, uint16_t) \
COMPARE_ROW_IMPL(NAME, OP, METH, LEFT, int16_t) \
COMPARE_ROW_IMPL(NAME, OP, METH, LEFT, uint32_t) \
COMPARE_ROW_IMPL(NAME, OP, METH, LEFT, int32_t) \
COMPARE_ROW_IMPL(NAME, OP, METH, LEFT, uint64_t) \
COMPARE_ROW_IMPL(NAME, OP, METH, LEFT, int64_t) \
COMPARE_ROW_IMPL(NAME, OP, METH, LEFT, float) \
COMPARE_ROW_IMPL(NAME, OP, METH, LEFT, double) \
COMPARE_ROW_IMPL_VALUE(NAME, OP, METH, LEFT)

#ifdef ML_COMPLEX

#define COMPARE_ROW_LEFT_IMPL(NAME, OP, METH, LEFT) \
COMPARE_ROW_LEFT_IMPL_BASE(NAME, OP, METH, LEFT) \
COMPARE_ROW_IMPL(NAME, OP, METH, LEFT, complex_float) \
COMPARE_ROW_IMPL(NAME, OP, METH, LEFT, complex_double)

#else

#define COMPARE_ROW_LEFT_IMPL(NAME, OP, METH, LEFT) \
COMPARE_ROW_LEFT_IMPL_BASE(NAME, OP, METH, LEFT)

#endif

#define COMPARE_ROW_LEFT_VALUE_IMPL_BASE(NAME, OP, METH) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, METH, uint8_t) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, METH, int8_t) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, METH, uint16_t) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, METH, int16_t) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, METH, uint32_t) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, METH, int32_t) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, METH, uint64_t) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, METH, int64_t) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, METH, float) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, METH, double) \
COMPARE_ROW_VALUE_IMPL_VALUE(NAME, OP, METH)

#ifdef ML_COMPLEX

#define COMPARE_ROW_LEFT_VALUE_IMPL(NAME, OP, METH) \
COMPARE_ROW_LEFT_VALUE_IMPL_BASE(NAME, OP, METH) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, METH, complex_float) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, METH, complex_double)

#else

#define COMPARE_ROW_LEFT_VALUE_IMPL(NAME, OP, METH) \
COMPARE_ROW_LEFT_VALUE_IMPL_BASE(NAME, OP, METH)

#endif

#define COMPARE_ROW_OPS_IMPL_BASE(NAME, OP, METH) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, METH, uint8_t) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, METH, int8_t) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, METH, uint16_t) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, METH, int16_t) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, METH, uint32_t) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, METH, int32_t) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, METH, uint64_t) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, METH, int64_t) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, METH, float) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, METH, double) \
COMPARE_ROW_LEFT_VALUE_IMPL(NAME, OP, METH)

#ifdef ML_COMPLEX

#define COMPARE_ROW_OPS_IMPL(NAME, OP, METH) \
COMPARE_ROW_OPS_IMPL_BASE(NAME, OP, METH) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, METH, complex_float) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, METH, complex_double)

#else

#define COMPARE_ROW_OPS_IMPL(NAME, OP, METH) \
COMPARE_ROW_OPS_IMPL_BASE(NAME, OP, METH)

#endif

#define COMPARE_ROW_ENTRY(INDEX, NAME, LEFT, RIGHT) \
	[INDEX] = NAME ## _row_ ## LEFT ## _ ## RIGHT

#define COMPARE_ROW_LEFT_ENTRIES_BASE(INDEX, NAME, LEFT) \
COMPARE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U8, NAME, LEFT, uint8_t), \
COMPARE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I8, NAME, LEFT, int8_t), \
COMPARE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U16, NAME, LEFT, uint16_t), \
COMPARE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I16, NAME, LEFT, int16_t), \
COMPARE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U32, NAME, LEFT, uint32_t), \
COMPARE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I32, NAME, LEFT, int32_t), \
COMPARE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U64, NAME, LEFT, uint64_t), \
COMPARE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I64, NAME, LEFT, int64_t), \
COMPARE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F32, NAME, LEFT, float), \
COMPARE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F64, NAME, LEFT, double), \
COMPARE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_ANY, NAME, LEFT, any)

#ifdef ML_COMPLEX

#define COMPARE_ROW_LEFT_ENTRIES(INDEX, NAME, LEFT) \
COMPARE_ROW_LEFT_ENTRIES_BASE(INDEX, NAME, LEFT), \
COMPARE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_C32, NAME, LEFT, complex_float), \
COMPARE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_C64, NAME, LEFT, complex_double)

#else

#define COMPARE_ROW_LEFT_ENTRIES(INDEX, NAME, LEFT) \
COMPARE_ROW_LEFT_ENTRIES_BASE(INDEX, NAME, LEFT)

#endif

#define COMPARE_ROW_OPS_ENTRIES_BASE(NAME) \
COMPARE_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_U8, NAME, uint8_t), \
COMPARE_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_I8, NAME, int8_t), \
COMPARE_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_U16, NAME, uint16_t), \
COMPARE_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_I16, NAME, int16_t), \
COMPARE_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_U32, NAME, uint32_t), \
COMPARE_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_I32, NAME, int32_t), \
COMPARE_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_U64, NAME, uint64_t), \
COMPARE_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_I64, NAME, int64_t), \
COMPARE_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_F32, NAME, float), \
COMPARE_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_F64, NAME, double), \
COMPARE_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_ANY, NAME, any)

#ifdef ML_COMPLEX

#define COMPARE_ROW_OPS_ENTRIES(NAME) \
COMPARE_ROW_OPS_ENTRIES_BASE(NAME), \
COMPARE_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_C32, NAME, complex_float), \
COMPARE_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_C64, NAME, complex_double)

#else

#define COMPARE_ROW_OPS_ENTRIES(NAME) \
COMPARE_ROW_OPS_ENTRIES_BASE(NAME)

#endif

typedef void (*compare_row_fn_t)(char *Target, ml_array_dimension_t *LeftDimension, char *LeftData, ml_array_dimension_t *RightDimension, char *RightData);

#define COMPARE_FNS(TITLE, NAME, OP, METH) \
	COMPARE_ROW_OPS_IMPL(NAME, OP, METH) \
\
compare_row_fn_t Compare ## TITLE ## RowFns[MAX_FORMATS * MAX_FORMATS] = { \
	COMPARE_ROW_OPS_ENTRIES(NAME) \
}
