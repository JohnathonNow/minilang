#include "../ml_array.h"

#ifdef ML_COMPLEX
#include <complex.h>
#undef I
#endif

#define COMPARE_ROW_IMPL(NAME, OP, LEFT, RIGHT, LTYPE, RTYPE, LCONV, RCONV) \
\
void NAME ## _row_ ## LEFT ## _ ## RIGHT(char *Target, ml_array_dimension_t *LeftDimension, char *LeftData, ml_array_dimension_t *RightDimension, char *RightData) { \
	int Size = LeftDimension->Size; \
	if (LeftDimension->Indices) { \
		int *LeftIndices = LeftDimension->Indices; \
		if (RightDimension->Indices) { \
			int *RightIndices = RightDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				LTYPE Left = LCONV(*(LEFT *)(LeftData + LeftIndices[I] * LeftDimension->Stride)); \
				RTYPE Right = RCONV(*(RIGHT *)(RightData + RightIndices[I] * RightDimension->Stride)); \
				*(Target++) = OP(Left, Right); \
			} \
		} else { \
			int RightStride = RightDimension->Stride; \
			for (int I = 0; I < Size; ++I) { \
				LTYPE Left = LCONV(*(LEFT *)(LeftData + LeftIndices[I] * LeftDimension->Stride)); \
				RTYPE Right = RCONV(*(RIGHT *)RightData); \
				*(Target++) = OP(Left, Right); \
				RightData += RightStride; \
			} \
		} \
	} else { \
		int LeftStride = LeftDimension->Stride; \
		if (RightDimension->Indices) { \
			int *RightIndices = RightDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				LTYPE Left = LCONV(*(LEFT *)LeftData); \
				RTYPE Right = RCONV(*(RIGHT *)(RightData + RightIndices[I] * RightDimension->Stride)); \
				*(Target++) = OP(Left, Right); \
				LeftData += LeftStride; \
			} \
		} else { \
			int RightStride = RightDimension->Stride; \
			for (int I = Size; --I >= 0;) { \
				LTYPE Left = LCONV(*(LEFT *)LeftData); \
				RTYPE Right = RCONV(*(RIGHT *)RightData); \
				*(Target++) = OP(Left, Right); \
				LeftData += LeftStride; \
				RightData += RightStride; \
			} \
		} \
	} \
}

#define COMPARE_ROW_LEFT_IMPL_BASE(NAME, OP, LEFT, LTYPE, LCONV) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, int8_t, LTYPE, int8_t, LCONV,) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, uint8_t, LTYPE, uint8_t, LCONV,) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, int16_t, LTYPE, int16_t, LCONV,) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, uint16_t, LTYPE, uint16_t, LCONV,) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, int32_t, LTYPE, int32_t, LCONV,) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, uint32_t, LTYPE, uint32_t, LCONV,) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, int64_t, LTYPE, int64_t, LCONV,) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, uint64_t, LTYPE, uint64_t, LCONV,) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, float, LTYPE, float, LCONV,) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, double, LTYPE, double, LCONV,)

#ifdef ML_COMPLEX

#define COMPARE_ROW_LEFT_IMPL(NAME, OP, LEFT, LTYPE, LCONV) \
COMPARE_ROW_LEFT_IMPL_BASE(NAME, OP, LEFT, LTYPE, LCONV) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, complex_float, LTYPE, double, LCONV, cabs) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, complex_double, LTYPE, double, LCONV, cabs)

#else

#define COMPARE_ROW_LEFT_IMPL(NAME, OP, LEFT, LTYPE, LCONV) \
COMPARE_ROW_LEFT_IMPL_BASE(NAME, OP, LEFT, LTYPE, LCONV)

#endif

#define COMPARE_ROW_LEFT_VALUE_IMPL_BASE(NAME, OP) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, int8_t, int8_t,) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, uint8_t, uint8_t,) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, int16_t, int16_t,) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, uint16_t, uint16_t,) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, int32_t, int32_t,) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, uint32_t, uint32_t,) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, int64_t, int64_t,) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, uint64_t, uint64_t,) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, float, float,) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, double, double,)

#ifdef ML_COMPLEX

#define COMPARE_ROW_LEFT_VALUE_IMPL(NAME, OP) \
COMPARE_ROW_LEFT_VALUE_IMPL_BASE(NAME, OP) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, complex_float, double, cabs) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, complex_double, double, cabs)

#else

#define COMPARE_ROW_LEFT_VALUE_IMPL(NAME, OP) \
COMPARE_ROW_LEFT_VALUE_IMPL_BASE(NAME, OP)

#endif

#define COMPARE_ROW_OPS_IMPL_BASE(NAME, OP) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, int8_t, int8_t,) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, uint8_t, uint8_t,) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, int16_t, int16_t,) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, uint16_t, uint16_t,) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, int32_t, int32_t,) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, uint32_t, uint32_t,) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, int64_t, int64_t,) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, uint64_t, uint64_t,) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, float, float,) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, double, double,)

#ifdef ML_COMPLEX

#define COMPARE_ROW_OPS_IMPL(NAME, OP) \
COMPARE_ROW_OPS_IMPL_BASE(NAME, OP) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, complex_float, double, cabs) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, complex_double, double, cabs)

#else

#define COMPARE_ROW_OPS_IMPL(NAME, OP) \
COMPARE_ROW_OPS_IMPL_BASE(NAME, OP)

#endif

#define COMPARE_ROW_ENTRY(INDEX, NAME, LEFT, RIGHT) \
	[INDEX] = NAME ## _row_ ## LEFT ## _ ## RIGHT

#define COMPARE_ROW_LEFT_ENTRIES_BASE(INDEX, NAME, LEFT) \
COMPARE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I8, NAME, LEFT, int8_t), \
COMPARE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U8, NAME, LEFT, uint8_t), \
COMPARE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I16, NAME, LEFT, int16_t), \
COMPARE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U16, NAME, LEFT, uint16_t), \
COMPARE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I32, NAME, LEFT, int32_t), \
COMPARE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U32, NAME, LEFT, uint32_t), \
COMPARE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I64, NAME, LEFT, int64_t), \
COMPARE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U64, NAME, LEFT, uint64_t), \
COMPARE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F32, NAME, LEFT, float), \
COMPARE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F64, NAME, LEFT, double)

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
COMPARE_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_I8, NAME, int8_t), \
COMPARE_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_U8, NAME, uint8_t), \
COMPARE_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_I16, NAME, int16_t), \
COMPARE_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_U16, NAME, uint16_t), \
COMPARE_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_I32, NAME, int32_t), \
COMPARE_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_U32, NAME, uint32_t), \
COMPARE_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_I64, NAME, int64_t), \
COMPARE_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_U64, NAME, uint64_t), \
COMPARE_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_F32, NAME, float), \
COMPARE_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_F64, NAME, double)

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

#define COMPARE_FNS(TITLE, NAME, OP) \
	COMPARE_ROW_OPS_IMPL(NAME, OP) \
\
compare_row_fn_t Compare ## TITLE ## RowFns[MAX_FORMATS * MAX_FORMATS] = { \
	COMPARE_ROW_OPS_ENTRIES(NAME) \
}
