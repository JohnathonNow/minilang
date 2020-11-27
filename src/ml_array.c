#include "ml_array.h"
#include "ml_macros.h"
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

ML_TYPE(MLArrayT, (MLBufferT, MLIteratableT), "array");
// Base type for multidimensional arrays.

/*

ML_CONSTRUCTOR(array);
//<List:list
// Returns a new array containing the values in :mini:`List`.
// The shape and type of the array is determined from the elements in :mini:`List`.

*/

extern ml_type_t MLArrayInt8T[];
extern ml_type_t MLArrayUInt8T[];
extern ml_type_t MLArrayInt16T[];
extern ml_type_t MLArrayUInt16T[];
extern ml_type_t MLArrayInt32T[];
extern ml_type_t MLArrayUInt32T[];
extern ml_type_t MLArrayInt64T[];
extern ml_type_t MLArrayUInt64T[];
extern ml_type_t MLArrayFloat32T[];
extern ml_type_t MLArrayFloat64T[];
extern ml_type_t MLArrayAnyT[];

/*

ML_TYPE(MLArrayInt8T, (MLArrayT), "int8t-array");
//@array::int8
// Arrays of signed 8 bit integers.

ML_TYPE(MLArrayUInt8T, (MLArrayT), "uint8t-array");
//@array::uint8
// Arrays of unsigned 8 bit integers.

ML_TYPE(MLArrayInt16T, (MLArrayT), "int16t-array");
//@array::int16
// Arrays of signed 16 bit integers.

ML_TYPE(MLArrayUInt16T, (MLArrayT), "uint16t-array");
//@array::uint16
// Arrays of unsigned 16 bit integers.

ML_TYPE(MLArrayInt32T, (MLArrayT), "int32t-array");
//@array::int32
// Arrays of signed 32 bit integers.

ML_TYPE(MLArrayUInt32T, (MLArrayT), "uint32t-array");
//@array::uint32
// Arrays of unsigned 32 bit integers.

ML_TYPE(MLArrayInt64T, (MLArrayT), "int64t-array");
//@array::int64
// Arrays of signed 64 bit integers.

ML_TYPE(MLArrayUInt64T, (MLArrayT), "uint64t-array");
//@array::uint64
// Arrays of unsigned 64 bit integers.

ML_TYPE(MLArrayFloat32T, (MLArrayT), "float-array");
//@array::float32
// Arrays of 32 bit reals.

ML_TYPE(MLArrayFloat64T, (MLArrayT), "double-array");
//@array::float64
// Arrays of 64 bit reals.

ML_TYPE(MLArrayAnyT, (MLArrayT), "value-array");
//@array::any
// Arrays of any *Minilang* values.

*/

size_t MLArraySizes[] = {
	[ML_ARRAY_FORMAT_NONE] = sizeof(ml_value_t *),
	[ML_ARRAY_FORMAT_I8] = sizeof(int8_t),
	[ML_ARRAY_FORMAT_U8] = sizeof(uint8_t),
	[ML_ARRAY_FORMAT_I16] = sizeof(int16_t),
	[ML_ARRAY_FORMAT_U16] = sizeof(uint16_t),
	[ML_ARRAY_FORMAT_I32] = sizeof(int32_t),
	[ML_ARRAY_FORMAT_U32] = sizeof(uint32_t),
	[ML_ARRAY_FORMAT_I64] = sizeof(int64_t),
	[ML_ARRAY_FORMAT_U64] = sizeof(uint64_t),
	[ML_ARRAY_FORMAT_F32] = sizeof(float),
	[ML_ARRAY_FORMAT_F64] = sizeof(double),
	[ML_ARRAY_FORMAT_ANY] = sizeof(ml_value_t *)
};

ml_array_t *ml_array_new(ml_array_format_t Format, int Degree) {
	ml_type_t *Type = MLArrayT;
	switch (Format) {
	case ML_ARRAY_FORMAT_NONE: Type = MLArrayAnyT; break;
	case ML_ARRAY_FORMAT_I8: Type = MLArrayInt8T; break;
	case ML_ARRAY_FORMAT_U8: Type = MLArrayUInt8T; break;
	case ML_ARRAY_FORMAT_I16: Type = MLArrayInt16T; break;
	case ML_ARRAY_FORMAT_U16: Type = MLArrayUInt16T; break;
	case ML_ARRAY_FORMAT_I32: Type = MLArrayInt32T; break;
	case ML_ARRAY_FORMAT_U32: Type = MLArrayUInt32T; break;
	case ML_ARRAY_FORMAT_I64: Type = MLArrayInt64T; break;
	case ML_ARRAY_FORMAT_U64: Type = MLArrayUInt64T; break;
	case ML_ARRAY_FORMAT_F32: Type = MLArrayFloat32T; break;
	case ML_ARRAY_FORMAT_F64: Type = MLArrayFloat64T; break;
	case ML_ARRAY_FORMAT_ANY: Type = MLArrayAnyT; break;
	};
	ml_array_t *Array = xnew(ml_array_t, Degree, ml_array_dimension_t);
	Array->Base.Type = Type;
	Array->Degree = Degree;
	Array->Format = Format;
	return Array;
}

ml_array_t *ml_array(ml_array_format_t Format, int Degree, ...) {
	ml_array_t *Array = ml_array_new(Format, Degree);
	int DataSize = MLArraySizes[Format];
	va_list Sizes;
	va_start(Sizes, Degree);
	for (int I = Degree; --I >= 0;) {
		Array->Dimensions[I].Stride = DataSize;
		int Size = Array->Dimensions[I].Size = va_arg(Sizes, int);
		DataSize *= Size;
	}
	va_end(Sizes);
	Array->Base.Address = GC_MALLOC_ATOMIC(DataSize);
	Array->Base.Size = DataSize;
	return Array;
}

int ml_array_degree(ml_value_t *Value) {
	return ((ml_array_t *)Value)->Degree;
}

int ml_array_size(ml_value_t *Value, int Dim) {
	ml_array_t *Array = (ml_array_t *)Value;
	if (Dim < 0 || Dim >= Array->Degree) return 0;
	return Array->Dimensions[Dim].Size;
}

typedef struct ml_array_init_state_t {
	ml_state_t Base;
	char *Address;
	ml_array_t *Array;
	ml_value_t *Function;
	ml_value_t *Args[];
} ml_array_init_state_t;

static void ml_array_init_run(ml_array_init_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	Value = ml_typeof(Value)->deref(Value);
	ml_array_t *Array = State->Array;
	switch (Array->Format) {
	case ML_ARRAY_FORMAT_NONE:
		break;
	case ML_ARRAY_FORMAT_ANY:
		*(ml_value_t **)State->Address = Value;
		State->Address += sizeof(ml_value_t *);
		break;
	case ML_ARRAY_FORMAT_I8:
		*(int8_t *)State->Address = ml_integer_value(Value);
		State->Address += sizeof(int8_t);
		break;
	case ML_ARRAY_FORMAT_U8:
		*(uint8_t *)State->Address = ml_integer_value(Value);
		State->Address += sizeof(uint8_t);
		break;
	case ML_ARRAY_FORMAT_I16:
		*(int16_t *)State->Address = ml_integer_value(Value);
		State->Address += sizeof(int16_t);
		break;
	case ML_ARRAY_FORMAT_U16:
		*(uint16_t *)State->Address = ml_integer_value(Value);
		State->Address += sizeof(uint16_t);
		break;
	case ML_ARRAY_FORMAT_I32:
		*(int32_t *)State->Address = ml_integer_value(Value);
		State->Address += sizeof(int32_t);
		break;
	case ML_ARRAY_FORMAT_U32:
		*(uint32_t *)State->Address = ml_integer_value(Value);
		State->Address += sizeof(uint32_t);
		break;
	case ML_ARRAY_FORMAT_I64:
		*(int64_t *)State->Address = ml_integer_value(Value);
		State->Address += sizeof(int64_t);
		break;
	case ML_ARRAY_FORMAT_U64:
		*(uint64_t *)State->Address = ml_integer_value(Value);
		State->Address += sizeof(uint64_t);
		break;
	case ML_ARRAY_FORMAT_F32:
		*(float *)State->Address = ml_real_value(Value);
		State->Address += sizeof(float);
		break;
	case ML_ARRAY_FORMAT_F64:
		*(double *)State->Address = ml_real_value(Value);
		State->Address += sizeof(double);
		break;
	}
	for (int I = Array->Degree; --I >= 0;) {
		int Next = ml_integer_value(State->Args[I]) + 1;
		if (Next <= Array->Dimensions[I].Size) {
			State->Args[I] = ml_integer(Next);
			return ml_call(State, State->Function, Array->Degree, State->Args);
		} else {
			State->Args[I] = ml_integer(1);
		}
	}
	ML_CONTINUE(State->Base.Caller, Array);
}

static void ml_array_typed_new_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLListT);
	ml_array_format_t Format = (intptr_t)Data;
	ml_array_t *Array;
	int Degree = ml_list_length(Args[0]);
	Array = ml_array_new(Format, Degree);
	int I = 0;
	ML_LIST_FOREACH(Args[0], Iter) {
		if (!ml_is(Iter->Value, MLIntegerT)) ML_RETURN(ml_error("TypeError", "Dimension is not an integer"));
		Array->Dimensions[I++].Size = ml_integer_value(Iter->Value);
	}
	int DataSize = MLArraySizes[Format];
	for (int I = Array->Degree; --I >= 0;) {
		Array->Dimensions[I].Stride = DataSize;
		DataSize *= Array->Dimensions[I].Size;
	}
	Array->Base.Address = GC_MALLOC_ATOMIC(DataSize);
	Array->Base.Size = DataSize;
	if (Count == 1) {
		if (Format == ML_ARRAY_FORMAT_ANY) {
			ml_value_t **Values = (ml_value_t **)Array->Base.Address;
			for (int I = DataSize / sizeof(ml_value_t *); --I >= 0;) {
				*Values++ = MLNil;
			}
		} else {
			memset(Array->Base.Address, 0, DataSize);
		}
		ML_RETURN(Array);
	}
	ml_array_init_state_t *State = xnew(ml_array_init_state_t, Array->Degree, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_array_init_run;
	State->Base.Context = Caller->Context;
	State->Address = Array->Base.Address;
	State->Array = Array;
	ml_value_t *Function = State->Function = Args[1];
	for (int I = 0; I < Array->Degree; ++I) State->Args[I] = ml_integer(1);
	return ml_call(State, Function, Array->Degree, State->Args);
}

static void ml_array_new_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(2);
	ML_CHECKX_ARG_TYPE(0, MLTypeT);
	ML_CHECKX_ARG_TYPE(1, MLListT);
	ml_array_format_t Format;
	if (Args[0] == (ml_value_t *)MLArrayAnyT) {
		Format = ML_ARRAY_FORMAT_ANY;
	} else if (Args[0] == (ml_value_t *)MLArrayInt8T) {
		Format = ML_ARRAY_FORMAT_I8;
	} else if (Args[0] == (ml_value_t *)MLArrayUInt8T) {
		Format = ML_ARRAY_FORMAT_U8;
	} else if (Args[0] == (ml_value_t *)MLArrayInt16T) {
		Format = ML_ARRAY_FORMAT_I16;
	} else if (Args[0] == (ml_value_t *)MLArrayUInt16T) {
		Format = ML_ARRAY_FORMAT_U16;
	} else if (Args[0] == (ml_value_t *)MLArrayInt32T) {
		Format = ML_ARRAY_FORMAT_I32;
	} else if (Args[0] == (ml_value_t *)MLArrayUInt32T) {
		Format = ML_ARRAY_FORMAT_U32;
	} else if (Args[0] == (ml_value_t *)MLArrayInt64T) {
		Format = ML_ARRAY_FORMAT_I64;
	} else if (Args[0] == (ml_value_t *)MLArrayUInt64T) {
		Format = ML_ARRAY_FORMAT_U64;
	} else if (Args[0] == (ml_value_t *)MLArrayFloat32T) {
		Format = ML_ARRAY_FORMAT_F32;
	} else if (Args[0] == (ml_value_t *)MLArrayFloat64T) {
		Format = ML_ARRAY_FORMAT_F64;
	} else {
		ML_RETURN(ml_error("TypeError", "Unknown type for array"));
	}
	return ml_array_typed_new_fnx(Caller, (void *)Format, Count - 1, Args + 1);
}

static __attribute__ ((malloc)) ml_value_t *ml_array_wrap_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(3);
	ML_CHECK_ARG_TYPE(1, MLBufferT);
	ML_CHECK_ARG_TYPE(2, MLListT);
	ML_CHECK_ARG_TYPE(3, MLListT);
	ml_array_format_t Format;
	if (Args[0] == (ml_value_t *)MLArrayAnyT) {
		Format = ML_ARRAY_FORMAT_ANY;
	} else if (Args[0] == (ml_value_t *)MLArrayInt8T) {
		Format = ML_ARRAY_FORMAT_I8;
	} else if (Args[0] == (ml_value_t *)MLArrayUInt8T) {
		Format = ML_ARRAY_FORMAT_U8;
	} else if (Args[0] == (ml_value_t *)MLArrayInt16T) {
		Format = ML_ARRAY_FORMAT_I16;
	} else if (Args[0] == (ml_value_t *)MLArrayUInt16T) {
		Format = ML_ARRAY_FORMAT_U16;
	} else if (Args[0] == (ml_value_t *)MLArrayInt32T) {
		Format = ML_ARRAY_FORMAT_I32;
	} else if (Args[0] == (ml_value_t *)MLArrayUInt32T) {
		Format = ML_ARRAY_FORMAT_U32;
	} else if (Args[0] == (ml_value_t *)MLArrayInt64T) {
		Format = ML_ARRAY_FORMAT_I64;
	} else if (Args[0] == (ml_value_t *)MLArrayUInt64T) {
		Format = ML_ARRAY_FORMAT_U64;
	} else if (Args[0] == (ml_value_t *)MLArrayFloat32T) {
		Format = ML_ARRAY_FORMAT_F32;
	} else if (Args[0] == (ml_value_t *)MLArrayFloat64T) {
		Format = ML_ARRAY_FORMAT_F64;
	} else {
		return ml_error("TypeError", "Unknown type for array");
	}
	int Degree = ml_list_length(Args[2]);
	if (Degree != ml_list_length(Args[3])) return ml_error("ValueError", "Dimensions and strides must have same length");
	ml_array_t *Array = ml_array_new(Format, Degree);
	for (int I = 0; I < Degree; ++I) {
		ml_value_t *Size = ml_list_get(Args[2], I + 1);
		ml_value_t *Stride = ml_list_get(Args[3], I + 1);
		if (!ml_is(Size, MLIntegerT)) return ml_error("TypeError", "Dimension is not an integer");
		if (!ml_is(Stride, MLIntegerT)) return ml_error("TypeError", "Stride is not an integer");
		Array->Dimensions[I].Size = ml_integer_value(Size);
		Array->Dimensions[I].Stride = ml_integer_value(Stride);
	}
	Array->Base.Address = ((ml_buffer_t *)Args[1])->Address;
	Array->Base.Size = ((ml_buffer_t *)Args[1])->Size;
	return (ml_value_t *)Array;
}

ML_METHOD("degree", MLArrayT) {
//<Array
//>integer
// Return the degree of :mini:`Array`.
	ml_array_t *Array = (ml_array_t *)Args[0];
	return ml_integer(Array->Degree);
}

ML_METHOD("shape", MLArrayT) {
//<Array
//>list
// Return the shape of :mini:`Array`.
	ml_array_t *Array = (ml_array_t *)Args[0];
	ml_value_t *Shape = ml_list();
	for (int I = 0; I < Array->Degree; ++I) {
		ml_list_put(Shape, ml_integer(Array->Dimensions[I].Size));
	}
	return Shape;
}

ML_METHOD("count", MLArrayT) {
//<Array
//>integer
// Return the number of elements in :mini:`Array`.
	ml_array_t *Array = (ml_array_t *)Args[0];
	size_t Size = 1;
	for (int I = 0; I < Array->Degree; ++I) Size *= Array->Dimensions[I].Size;
	return ml_integer(Size);
}

ML_METHOD("transpose", MLArrayT) {
//<Array
//>array
// Returns the transpose of :mini:`Array`, sharing the underlying data.
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Degree = Source->Degree;
	ml_array_t *Target = ml_array_new(Source->Format, Degree);
	for (int I = 0; I < Degree; ++I) {
		Target->Dimensions[I] = Source->Dimensions[Degree - I - 1];
	}
	Target->Base = Source->Base;
	return (ml_value_t *)Target;
}

ML_METHOD("permute", MLArrayT, MLListT) {
//<Array
//<Indices
//>array
// Returns an array sharing the underlying data with :mini:`Array`, permuting the axes according to :mini:`Indices`.
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Degree = Source->Degree;
	if (Degree > 64) return ml_error("ArrayError", "Not implemented for degree > 64 yet");
	if (ml_list_length(Args[1]) != Degree) return ml_error("ArrayError", "List length must match degree");
	ml_array_t *Target = ml_array_new(Source->Format, Degree);
	int I = 0;
	size_t Actual = 0;
	ML_LIST_FOREACH(Args[1], Iter) {
		if (!ml_is(Iter->Value, MLIntegerT)) return ml_error("ArrayError", "Invalid index");
		int J = ml_integer_value(Iter->Value);
		if (J <= 0) J += Degree + 1;
		if (J < 1 || J > Degree) return ml_error("ArrayError", "Invalid index");
		Actual += 1 << (J - 1);
		Target->Dimensions[I++] = Source->Dimensions[J - 1];
	}
	size_t Expected = (1 << Degree) - 1;
	if (Actual != Expected) return ml_error("ArrayError", "Invalid permutation");
	Target->Base = Source->Base;
	return (ml_value_t *)Target;
}

ML_METHOD("expand", MLArrayT, MLListT) {
//<Array
//<Indices
//>array
// Returns an array sharing the underlying data with :mini:`Array` with additional unit-length axes at the specified :mini:`Indices`.
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Degree = Source->Degree + ml_list_length(Args[1]);
	int Expands[Source->Degree + 1];
	for (int I = 0; I <= Source->Degree; ++I) Expands[I] = 0;
	ML_LIST_FOREACH(Args[1], Iter) {
		if (!ml_is(Iter->Value, MLIntegerT)) return ml_error("ArrayError", "Invalid index");
		int J = ml_integer_value(Iter->Value);
		if (J <= 0) J += Degree + 1;
		if (J < 1 || J >= Degree + 1) return ml_error("ArrayError", "Invalid index");
		Expands[J - 1] += 1;
	}
	ml_array_t *Target = ml_array_new(Source->Format, Degree);
	ml_array_dimension_t *Dim = Target->Dimensions;
	for (int I = 0; I < Degree; ++I) {
		for (int J = 0; J < Expands[I]; ++J) {
			Dim->Size = 1;
			Dim->Stride = 0;
			++Dim;
		}
		*Dim++ = Source->Dimensions[I];
	}
	for (int J = 0; J < Expands[Source->Degree]; ++J) {
		Dim->Size = 1;
		Dim->Stride = 0;
		++Dim;
	}
	Target->Base = Source->Base;
	return (ml_value_t *)Target;
}

ML_METHOD("strides", MLArrayT) {
//<Array
//>list
// Return the strides of :mini:`Array` in bytes.
	ml_array_t *Array = (ml_array_t *)Args[0];
	ml_value_t *Strides = ml_list();
	for (int I = 0; I < Array->Degree; ++I) {
		ml_list_put(Strides, ml_integer(Array->Dimensions[I].Stride));
	}
	return Strides;
}

ML_METHOD("size", MLArrayT) {
//<Array
//>integer
// Return the size of :mini:`Array` in bytes.
	ml_array_t *Array = (ml_array_t *)Args[0];
	size_t Size = Array->Dimensions[Array->Degree - 1].Stride;
	for (int I = 1; I < Array->Degree; ++I) Size *= Array->Dimensions[I].Size;
	if (Array->Dimensions[0].Stride == Size) return ml_integer(Size * Array->Dimensions[0].Size);
	return MLNil;
}

typedef struct ml_integer_range_t {
	const ml_type_t *Type;
	long Start, Limit, Step;
} ml_integer_range_t;

extern ml_type_t MLIntegerRangeT[1];
static ML_METHOD_DECL(Range, "..");
static ML_METHOD_DECL(Symbol, "::");

static ml_value_t *ml_array_value(ml_array_t *Array, char *Address) {
	typeof(ml_array_value) *function = ml_typed_fn_get(Array->Base.Type, ml_array_value);
	return function(Array, Address);
}

ml_value_t *ml_array_index(ml_array_t *Source, int Count, ml_value_t **Indices) {
	ml_array_dimension_t TargetDimensions[Source->Degree];
	ml_array_dimension_t *TargetDimension = TargetDimensions;
	ml_array_dimension_t *SourceDimension = Source->Dimensions;
	ml_array_dimension_t *Limit = SourceDimension + Source->Degree;
	char *Address = Source->Base.Address;
	int Min, Max, Step, I;
	for (I = 0; I < Count; ++I) {
		ml_value_t *Index = Indices[I];
		if (Index == RangeMethod) {
			ml_array_dimension_t *Skip = Limit - (Count - (I + 1));
			if (Skip > Limit) return ml_error("RangeError", "Too many indices");
			while (SourceDimension < Skip) {
				*TargetDimension = *SourceDimension;
				++TargetDimension;
				++SourceDimension;
			}
			continue;
		}
		if (SourceDimension >= Limit) return ml_error("RangeError", "Too many indices");
		if (ml_is(Index, MLIntegerT)) {
			int IndexValue = ml_integer_value(Index);
			if (IndexValue <= 0) IndexValue += SourceDimension->Size + 1;
			if (--IndexValue < 0) return MLNil;
			if (IndexValue >= SourceDimension->Size) return MLNil;
			if (SourceDimension->Indices) IndexValue = SourceDimension->Indices[IndexValue];
			Address += SourceDimension->Stride * IndexValue;
		} else if (ml_is(Index, MLListT)) {
			int Size = TargetDimension->Size = ml_list_length(Index);
			if (!Size) return ml_error("IndexError", "Empty dimension");
			int *Indices = TargetDimension->Indices = (int *)GC_MALLOC_ATOMIC(Size * sizeof(int));
			int *IndexPtr = Indices;
			ML_LIST_FOREACH(Index, Iter) {
				int IndexValue = ml_integer_value(Iter->Value);
				if (IndexValue <= 0) IndexValue += SourceDimension->Size + 1;
				if (--IndexValue < 0) return MLNil;
				if (IndexValue >= SourceDimension->Size) return MLNil;
				*IndexPtr++ = IndexValue;
			}
			int First = Indices[0];
			for (int I = 0; I < Size; ++I) Indices[I] -= First;
			TargetDimension->Stride = SourceDimension->Stride;
			Address += SourceDimension->Stride * First;
			++TargetDimension;
		} else if (ml_is(Index, MLIntegerRangeT)) {
			ml_integer_range_t *IndexValue = (ml_integer_range_t *)Index;
			Min = IndexValue->Start;
			Max = IndexValue->Limit;
			Step = IndexValue->Step;
			if (Min < 1) Min += SourceDimension->Size + 1;
			if (Max < 1) Max += SourceDimension->Size + 1;
			if (--Min < 0) return MLNil;
			if (Min >= SourceDimension->Size) return MLNil;
			if (--Max < 0) return MLNil;
			if (Max >= SourceDimension->Size) return MLNil;
			if (Step == 0) return MLNil;
			int Size = TargetDimension->Size = (Max - Min) / Step + 1;
			if (Size < 0) return MLNil;
			TargetDimension->Indices = 0;
			TargetDimension->Stride = SourceDimension->Stride * Step;
			Address += SourceDimension->Stride * Min;
			++TargetDimension;
		} else if (Index == MLNil) {
			*TargetDimension = *SourceDimension;
			++TargetDimension;
		} else {
			return ml_error("TypeError", "Unknown index type: %s", ml_typeof(Index)->Name);
		}
		++SourceDimension;
	}
	while (SourceDimension < Limit) {
		*TargetDimension = *SourceDimension;
		++TargetDimension;
		++SourceDimension;
	}
	int Degree = TargetDimension - TargetDimensions;
	ml_array_t *Target = ml_array_new(Source->Format, Degree);
	for (int I = 0; I < Degree; ++I) Target->Dimensions[I] = TargetDimensions[I];
	Target->Base.Address = Address;
	return (ml_value_t *)Target;
}

ML_METHODV("[]", MLArrayT) {
//<Array
//<Indices...:any
//>array
// Returns a sub-array of :mini:`Array` sharing the underlying data.
// The :mini:`i`-th dimension is indexed by the corresponding :mini:`Index/i`.
// * If :mini:`Index/i` is :mini:`nil` then the :mini:`i`-th dimension is copied unchanged.
// * If :mini:`Index/i` is an integer then the :mini:`Index/i`-th value is selected and the :mini:`i`-th dimension is dropped from the result.
// * If :mini:`Index/i` is a list of integers then the :mini:`i`-th dimension is copied as a sparse dimension with the respective entries.
// If fewer than :mini:`A:degree` indices are provided then the remaining dimensions are copied unchanged.
	ml_array_t *Source = (ml_array_t *)Args[0];
	return ml_array_index(Source, Count - 1, Args + 1);
}

ML_METHOD("[]", MLArrayT, MLMapT) {
//<Array
//<Indices
//>array
// Returns a sub-array of :mini:`Array` sharing the underlying data.
// The :mini:`i`-th dimension is indexed by :mini:`Indices[i]` if present, and :mini:`nil` otherwise.
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Degree = Source->Degree;
	ml_value_t *Indices[Degree];
	for (int I = 0; I < Degree; ++I) Indices[I] = MLNil;
	ML_MAP_FOREACH(Args[1], Iter) {
		int Index = ml_integer_value(Iter->Key) - 1;
		if (Index < 0) Index += Degree + 1;
		if (Index < 0 || Index >= Degree) return ml_error("RangeError", "Index out of range");
		Indices[Index] = Iter->Value;
	}
	return ml_array_index(Source, Degree, Indices);
}

static ml_value_t *ml_array_of_fn(void *Data, int Count, ml_value_t **Args);

static char *ml_array_indexv(ml_array_t *Array, va_list Indices) {
	ml_array_dimension_t *Dimension = Array->Dimensions;
	char *Address = Array->Base.Address;
	for (int I = 0; I < Array->Degree; ++I) {
		int Index = va_arg(Indices, int);
		if (Index < 0 || Index >= Dimension->Size) return 0;
		if (Dimension->Indices) {
			Address += Dimension->Stride * Dimension->Indices[Index];
		} else {
			Address += Dimension->Stride * Index;
		}
		++Dimension;
	}
	return Address;
}

typedef struct {
	char *Address;
	int *Indices;
	int Size, Stride, Index;
} ml_array_iter_dim_t;

typedef ml_value_t *(*ml_array_iter_val_t)(char *);

#define ML_ARRAY_ITER_FN(CTYPE, TO_VAL) \
\
static ml_value_t *ml_array_iter_val_ ## CTYPE(char *Address) { \
	return TO_VAL(*(CTYPE *)Address); \
}

ML_ARRAY_ITER_FN(int8_t, ml_integer);
ML_ARRAY_ITER_FN(uint8_t, ml_integer);
ML_ARRAY_ITER_FN(int16_t, ml_integer);
ML_ARRAY_ITER_FN(uint16_t, ml_integer);
ML_ARRAY_ITER_FN(int32_t, ml_integer);
ML_ARRAY_ITER_FN(uint32_t, ml_integer);
ML_ARRAY_ITER_FN(int64_t, ml_integer);
ML_ARRAY_ITER_FN(uint64_t, ml_integer);
ML_ARRAY_ITER_FN(float, ml_real);
ML_ARRAY_ITER_FN(double, ml_real);

static ml_value_t *ml_array_iter_val_any(char *Address) {
	return *(ml_value_t **)Address;
}

typedef struct {
	const ml_type_t *Type;
	char *Address;
	ml_array_iter_val_t ToVal;
	int Degree;
	ml_array_iter_dim_t Dimensions[];
} ml_array_iterator_t;

ML_TYPE(MLArrayIteratorT, (), "array-iterator");
//!internal

static void ML_TYPED_FN(ml_iter_next, MLArrayIteratorT, ml_state_t *Caller, ml_array_iterator_t *Iterator) {
	int I = Iterator->Degree;
	ml_array_iter_dim_t *Dimensions = Iterator->Dimensions;
	for (;;) {
		if (--I < 0) ML_RETURN(MLNil);
		if (++Dimensions[I].Index < Dimensions[I].Size) {
			char *Address = Dimensions[I].Address;
			if (Dimensions[I].Indices) {
				Address += Dimensions[I].Indices[Dimensions[I].Index] * Dimensions[I].Stride;
			} else {
				Address += Dimensions[I].Index * Dimensions[I].Stride;
			}
			for (int J = I + 1; J < Iterator->Degree; ++J) {
				Dimensions[J].Address = Address;
				Dimensions[J].Index = 0;
			}
			Iterator->Address = Address;
			ML_RETURN(Iterator);
		}
	}
}

static void ML_TYPED_FN(ml_iter_value, MLArrayIteratorT, ml_state_t *Caller, ml_array_iterator_t *Iterator) {
	ML_RETURN(Iterator->ToVal(Iterator->Address));
}

static void ML_TYPED_FN(ml_iter_key, MLArrayIteratorT, ml_state_t *Caller, ml_array_iterator_t *Iterator) {
	ml_value_t *Tuple = ml_tuple(Iterator->Degree);
	for (int I = 0; I < Iterator->Degree; ++I) {
		ml_tuple_set(Tuple, I + 1, ml_integer(Iterator->Dimensions[I].Index + 1));
	}
	ML_RETURN(Tuple);
}

static void ML_TYPED_FN(ml_iterate, MLArrayT, ml_state_t *Caller, ml_array_t *Array) {
	ml_array_iterator_t *Iterator = xnew(ml_array_iterator_t, Array->Degree, ml_array_iter_dim_t);
	Iterator->Type = MLArrayIteratorT;
	Iterator->Address = Array->Base.Address;
	Iterator->Degree = Array->Degree;
	switch (Array->Format) {
	case ML_ARRAY_FORMAT_I8:
		Iterator->ToVal = ml_array_iter_val_int8_t;
		break;
	case ML_ARRAY_FORMAT_U8:
		Iterator->ToVal = ml_array_iter_val_uint8_t;
		break;
	case ML_ARRAY_FORMAT_I16:
		Iterator->ToVal = ml_array_iter_val_int16_t;
		break;
	case ML_ARRAY_FORMAT_U16:
		Iterator->ToVal = ml_array_iter_val_uint16_t;
		break;
	case ML_ARRAY_FORMAT_I32:
		Iterator->ToVal = ml_array_iter_val_int32_t;
		break;
	case ML_ARRAY_FORMAT_U32:
		Iterator->ToVal = ml_array_iter_val_uint32_t;
		break;
	case ML_ARRAY_FORMAT_I64:
		Iterator->ToVal = ml_array_iter_val_int64_t;
		break;
	case ML_ARRAY_FORMAT_U64:
		Iterator->ToVal = ml_array_iter_val_uint64_t;
		break;
	case ML_ARRAY_FORMAT_F32:
		Iterator->ToVal = ml_array_iter_val_float;
		break;
	case ML_ARRAY_FORMAT_F64:
		Iterator->ToVal = ml_array_iter_val_double;
		break;
	case ML_ARRAY_FORMAT_ANY:
		Iterator->ToVal = ml_array_iter_val_any;
		break;
	default:
		ML_ERROR("TypeError", "Invalid array type for iteration");
	}
	for (int I = 0; I < Array->Degree; ++I) {
		Iterator->Dimensions[I].Size = Array->Dimensions[I].Size;
		Iterator->Dimensions[I].Stride = Array->Dimensions[I].Stride;
		Iterator->Dimensions[I].Indices = Array->Dimensions[I].Indices;
		Iterator->Dimensions[I].Index = 0;
		Iterator->Dimensions[I].Address = Array->Base.Address;
	}
	ML_RETURN(Iterator);
}

#include "array/update_decl.h"

#define UPDATE_ROW_ENTRY(INDEX, NAME, TARGET, SOURCE) \
	[INDEX] = NAME ## _row_ ## TARGET ## _ ## SOURCE

#define UPDATE_ROW_VALUE_ENTRY(INDEX, NAME, SOURCE) \
	[INDEX] = NAME ## _row_value_ ## SOURCE

#define UPDATE_ROW_ENTRY_VALUE(INDEX, NAME, TARGET) \
	[INDEX] = NAME ## _row_ ## TARGET ## _value

#define UPDATE_ROW_VALUE_ENTRY_VALUE(INDEX, NAME) \
	[INDEX] = NAME ## _row_value_value

#define MAX_FORMATS 16

#define UPDATE_ROW_TARGET_ENTRIES(INDEX, NAME, TARGET) \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I8, NAME, TARGET, int8_t), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U8, NAME, TARGET, uint8_t), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I16, NAME, TARGET, int16_t), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U16, NAME, TARGET, uint16_t), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I32, NAME, TARGET, int32_t), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U32, NAME, TARGET, uint32_t), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I64, NAME, TARGET, int64_t), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U64, NAME, TARGET, uint64_t), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F32, NAME, TARGET, float), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F64, NAME, TARGET, double), \
UPDATE_ROW_ENTRY_VALUE(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_ANY, NAME, TARGET)

#define UPDATE_ROW_VALUE_TARGET_ENTRIES(INDEX, NAME) \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I8, NAME, int8_t), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U8, NAME, uint8_t), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I16, NAME, int16_t), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U16, NAME, uint16_t), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I32, NAME, int32_t), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U32, NAME, uint32_t), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I64, NAME, int64_t), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U64, NAME, uint64_t), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F32, NAME, float), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F64, NAME, double), \
UPDATE_ROW_VALUE_ENTRY_VALUE(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_ANY, NAME)

#define UPDATE_ROW_OPS_ENTRIES(INDEX, NAME) \
UPDATE_ROW_TARGET_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I8, NAME, int8_t), \
UPDATE_ROW_TARGET_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U8, NAME, uint8_t), \
UPDATE_ROW_TARGET_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I16, NAME, int16_t), \
UPDATE_ROW_TARGET_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U16, NAME, uint16_t), \
UPDATE_ROW_TARGET_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I32, NAME, int32_t), \
UPDATE_ROW_TARGET_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U32, NAME, uint32_t), \
UPDATE_ROW_TARGET_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I64, NAME, int64_t), \
UPDATE_ROW_TARGET_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U64, NAME, uint64_t), \
UPDATE_ROW_TARGET_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F32, NAME, float), \
UPDATE_ROW_TARGET_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F64, NAME, double), \
UPDATE_ROW_VALUE_TARGET_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_ANY, NAME)

typedef void (*update_row_fn_t)(ml_array_dimension_t *TargetDimension, char *TargetData, ml_array_dimension_t *SourceDimension, char *SourceData);

static update_row_fn_t UpdateRowFns[] = {
	UPDATE_ROW_OPS_ENTRIES(0, set),
	UPDATE_ROW_OPS_ENTRIES(1, add),
	UPDATE_ROW_OPS_ENTRIES(2, sub),
	UPDATE_ROW_OPS_ENTRIES(3, mul),
	UPDATE_ROW_OPS_ENTRIES(4, div)
};

static void update_array(int Op, ml_array_dimension_t *TargetDimension, char *TargetData, int SourceDegree, ml_array_dimension_t *SourceDimension, char *SourceData) {
	if (SourceDegree == 0) {
		ml_array_dimension_t ConstantDimension[1] = {{TargetDimension->Size, 0, NULL}};
		return UpdateRowFns[Op](TargetDimension, TargetData, ConstantDimension, SourceData);
	}
	if (SourceDegree == 1) {
		return UpdateRowFns[Op](TargetDimension, TargetData, SourceDimension, SourceData);
	}
	int Size = TargetDimension->Size;
	if (TargetDimension->Indices) {
		int *TargetIndices = TargetDimension->Indices;
		if (SourceDimension->Indices) {
			int *SourceIndices = SourceDimension->Indices;
			for (int I = 0; I < Size; ++I) {
				update_array(Op, TargetDimension + 1, TargetData + TargetIndices[I] * TargetDimension->Stride, SourceDegree - 1, SourceDimension + 1, SourceData + SourceIndices[I] * SourceDimension->Stride);
			}
		} else {
			int SourceStride = SourceDimension->Stride;
			for (int I = 0; I < Size; ++I) {
				update_array(Op, TargetDimension + 1, TargetData + TargetIndices[I] * TargetDimension->Stride, SourceDegree - 1, SourceDimension + 1, SourceData);
				SourceData += SourceStride;
			}
		}
	} else {
		int TargetStride = TargetDimension->Stride;
		if (SourceDimension->Indices) {
			int *SourceIndices = SourceDimension->Indices;
			for (int I = 0; I < Size; ++I) {
				update_array(Op, TargetDimension + 1, TargetData, SourceDegree - 1, SourceDimension + 1, SourceData + SourceIndices[I] * SourceDimension->Stride);
				TargetData += TargetStride;
			}
		} else {
			int SourceStride = SourceDimension->Stride;
			for (int I = Size; --I >= 0;) {
				update_array(Op, TargetDimension + 1, TargetData, SourceDegree - 1, SourceDimension + 1, SourceData);
				TargetData += TargetStride;
				SourceData += SourceStride;
			}
		}
	}
}

static void update_prefix(int Op, int PrefixDegree, ml_array_dimension_t *TargetDimension, char *TargetData, int SourceDegree, ml_array_dimension_t *SourceDimension, char *SourceData) {
	if (PrefixDegree == 0) return update_array(Op, TargetDimension, TargetData, SourceDegree, SourceDimension, SourceData);
	int Size = TargetDimension->Size;
	if (TargetDimension->Indices) {
		int *TargetIndices = TargetDimension->Indices;
		for (int I = Size; --I >= 0;) {
			update_prefix(Op, PrefixDegree - 1, TargetDimension + 1, TargetData + TargetIndices[I] * TargetDimension->Stride, SourceDegree, SourceDimension, SourceData);
		}
	} else {
		int Stride = TargetDimension->Stride;
		for (int I = Size; --I >= 0;) {
			update_prefix(Op, PrefixDegree - 1, TargetDimension + 1, TargetData, SourceDegree, SourceDimension, SourceData);
			TargetData += Stride;
		}
	}
}

static ml_value_t *update_array_fn(void *Data, int Count, ml_value_t **Args) {
	ml_array_t *Target = (ml_array_t *)Args[0];
	ml_array_t *Source = (ml_array_t *)Args[1];
	if (Source->Degree > Target->Degree) return ml_error("ArrayError", "Incompatible assignment (%d)", __LINE__);
	int PrefixDegree = Target->Degree - Source->Degree;
	for (int I = 0; I < Source->Degree; ++I) {
		if (Target->Dimensions[PrefixDegree + I].Size != Source->Dimensions[I].Size) return ml_error("ArrayError", "Incompatible assignment (%d)", __LINE__);
	}
	int Op = ((char *)Data - (char *)0) * MAX_FORMATS * MAX_FORMATS + Target->Format * MAX_FORMATS + Source->Format;
	if (!UpdateRowFns[Op]) return ml_error("ArrayError", "Unsupported array format pair (%s, %s)", Target->Base.Type->Name, Source->Base.Type->Name);
	if (Target->Degree) {
		update_prefix(Op, PrefixDegree, Target->Dimensions, Target->Base.Address, Source->Degree, Source->Dimensions, Source->Base.Address);
	} else {
		ml_array_dimension_t ValueDimension[1] = {{1, 0, NULL}};
		UpdateRowFns[Op](ValueDimension, Target->Base.Address, ValueDimension, Source->Base.Address);
	}
	return Args[0];
}

#define UPDATE_METHOD(NAME, BASE, ATYPE, CTYPE, FROM_VAL, FORMAT) \
\
ML_METHOD(#NAME, ATYPE, MLNumberT) { \
	ml_array_t *Array = (ml_array_t *)Args[0]; \
	CTYPE Value = FROM_VAL(Args[1]); \
	ml_array_dimension_t ValueDimension[1] = {{1, 0, NULL}}; \
	int Op = (BASE) * MAX_FORMATS * MAX_FORMATS + Array->Format * MAX_FORMATS + FORMAT; \
	if (!UpdateRowFns[Op]) return ml_error("ArrayError", "Unsupported array format pair (%s, %s)", ml_typeof(Args[0])->Name, ml_typeof(Args[1])->Name); \
	if (Array->Degree == 0) { \
		UpdateRowFns[Op](ValueDimension, Array->Base.Address, ValueDimension, (char *)&Value); \
	} else { \
		update_prefix(Op, Array->Degree - 1, Array->Dimensions, Array->Base.Address, 0, ValueDimension, (char *)&Value); \
	} \
	return Args[0]; \
}

#define UPDATE_METHODS(ATYPE, CTYPE, FROM_VAL, FORMAT) \
UPDATE_METHOD(set, 0, ATYPE, CTYPE, FROM_VAL, FORMAT); \
UPDATE_METHOD(add, 1, ATYPE, CTYPE, FROM_VAL, FORMAT); \
UPDATE_METHOD(sub, 2, ATYPE, CTYPE, FROM_VAL, FORMAT); \
UPDATE_METHOD(mul, 3, ATYPE, CTYPE, FROM_VAL, FORMAT); \
UPDATE_METHOD(div, 4, ATYPE, CTYPE, FROM_VAL, FORMAT);

#include "array/compare_decl.h"

#define COMPARE_ROW_ENTRY(INDEX, NAME, LEFT, RIGHT) \
	[INDEX] = NAME ## _row_ ## LEFT ## _ ## RIGHT

#define COMPARE_ROW_LEFT_ENTRIES(INDEX, NAME, LEFT) \
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

#define COMPARE_ROW_OPS_ENTRIES(INDEX, NAME) \
COMPARE_ROW_LEFT_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I8, NAME, int8_t), \
COMPARE_ROW_LEFT_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U8, NAME, uint8_t), \
COMPARE_ROW_LEFT_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I16, NAME, int16_t), \
COMPARE_ROW_LEFT_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U16, NAME, uint16_t), \
COMPARE_ROW_LEFT_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I32, NAME, int32_t), \
COMPARE_ROW_LEFT_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U32, NAME, uint32_t), \
COMPARE_ROW_LEFT_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I64, NAME, int64_t), \
COMPARE_ROW_LEFT_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U64, NAME, uint64_t), \
COMPARE_ROW_LEFT_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F32, NAME, float), \
COMPARE_ROW_LEFT_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F64, NAME, double)

typedef void (*compare_row_fn_t)(char *Target, ml_array_dimension_t *LeftDimension, char *LeftData, ml_array_dimension_t *RightDimension, char *RightData);

static compare_row_fn_t CompareRowFns[] = {
	COMPARE_ROW_OPS_ENTRIES(0, eq),
	COMPARE_ROW_OPS_ENTRIES(1, ne),
	COMPARE_ROW_OPS_ENTRIES(2, lt),
	COMPARE_ROW_OPS_ENTRIES(3, gt),
	COMPARE_ROW_OPS_ENTRIES(4, le),
	COMPARE_ROW_OPS_ENTRIES(5, ge)
};

static void compare_array(int Op, ml_array_dimension_t *TargetDimension, char *TargetData, ml_array_dimension_t *LeftDimension, char *LeftData, int RightDegree, ml_array_dimension_t *RightDimension, char *RightData) {
	if (RightDegree == 0) {
		ml_array_dimension_t ConstantDimension[1] = {{LeftDimension->Size, 0, NULL}};
		return CompareRowFns[Op](TargetData, LeftDimension, LeftData, ConstantDimension, RightData);
	}
	if (RightDegree == 1) {
		return CompareRowFns[Op](TargetData, LeftDimension, LeftData, RightDimension, RightData);
	}
	int Size = LeftDimension->Size;
	int TargetStride = TargetDimension->Stride;
	if (LeftDimension->Indices) {
		int *LeftIndices = LeftDimension->Indices;
		if (RightDimension->Indices) {
			int *RightIndices = RightDimension->Indices;
			for (int I = 0; I < Size; ++I) {
				compare_array(Op, TargetDimension + 1, TargetData, LeftDimension + 1, LeftData + LeftIndices[I] * LeftDimension->Stride, RightDegree - 1, RightDimension + 1, RightData + RightIndices[I] * RightDimension->Stride);
				TargetData += TargetStride;
			}
		} else {
			int RightStride = RightDimension->Stride;
			for (int I = 0; I < Size; ++I) {
				compare_array(Op, TargetDimension + 1, TargetData, LeftDimension + 1, LeftData + LeftIndices[I] * LeftDimension->Stride, RightDegree - 1, RightDimension + 1, RightData);
				RightData += RightStride;
				TargetData += TargetStride;
			}
		}
	} else {
		int LeftStride = LeftDimension->Stride;
		if (RightDimension->Indices) {
			int *RightIndices = RightDimension->Indices;
			for (int I = 0; I < Size; ++I) {
				compare_array(Op, TargetDimension + 1, TargetData, LeftDimension + 1, LeftData, RightDegree - 1, RightDimension + 1, RightData + RightIndices[I] * RightDimension->Stride);
				LeftData += LeftStride;
				TargetData += TargetStride;
			}
		} else {
			int RightStride = RightDimension->Stride;
			for (int I = Size; --I >= 0;) {
				compare_array(Op, TargetDimension + 1, TargetData, LeftDimension + 1, LeftData, RightDegree - 1, RightDimension + 1, RightData);
				LeftData += LeftStride;
				RightData += RightStride;
				TargetData += TargetStride;
			}
		}
	}
}

static void compare_prefix(int Op, ml_array_dimension_t *TargetDimension, char *TargetData, int PrefixDegree, ml_array_dimension_t *LeftDimension, char *LeftData, int RightDegree, ml_array_dimension_t *RightDimension, char *RightData) {
	if (PrefixDegree == 0) return compare_array(Op, TargetDimension, TargetData, LeftDimension, LeftData, RightDegree, RightDimension, RightData);
	int Size = LeftDimension->Size;
	int TargetStride = TargetDimension->Stride;
	if (LeftDimension->Indices) {
		int *LeftIndices = LeftDimension->Indices;
		for (int I = Size; --I >= 0;) {
			compare_prefix(Op, TargetDimension + 1, TargetData, PrefixDegree - 1, LeftDimension + 1, LeftData + LeftIndices[I] * LeftDimension->Stride, RightDegree, RightDimension, RightData);
			TargetData += TargetStride;
		}
	} else {
		int Stride = LeftDimension->Stride;
		for (int I = Size; --I >= 0;) {
			compare_prefix(Op, TargetDimension + 1, TargetData, PrefixDegree - 1, LeftDimension + 1, LeftData, RightDegree, RightDimension, RightData);
			LeftData += Stride;
			TargetData += TargetStride;
		}
	}
}

static ml_value_t *compare_array_fn(void *Data, int Count, ml_value_t **Args) {
	ml_array_t *Left = (ml_array_t *)Args[0];
	ml_array_t *Right = (ml_array_t *)Args[1];
	int Degree = Left->Degree;
	if (Right->Degree > Degree) return ml_error("ArrayError", "Incompatible assignment (%d)", __LINE__);
	int PrefixDegree = Degree - Right->Degree;
	for (int I = 0; I < Right->Degree; ++I) {
		if (Left->Dimensions[PrefixDegree + I].Size != Right->Dimensions[I].Size) return ml_error("ArrayError", "Incompatible assignment (%d)", __LINE__);
	}
	ml_array_t *Target = ml_array_new(ML_ARRAY_FORMAT_I8, Degree);
	int DataSize = 1;
	for (int I = Degree; --I >= 0;) {
		Target->Dimensions[I].Stride = DataSize;
		int Size = Target->Dimensions[I].Size = Left->Dimensions[I].Size;
		DataSize *= Size;
	}
	Target->Base.Address = GC_MALLOC_ATOMIC(DataSize);
	int Op = ((char *)Data - (char *)0) * MAX_FORMATS * MAX_FORMATS + Left->Format * MAX_FORMATS + Right->Format;
	if (!CompareRowFns[Op]) return ml_error("ArrayError", "Unsupported array format pair (%s, %s)", Left->Base.Type->Name, Right->Base.Type->Name);
	if (Degree) {
		compare_prefix(Op, Target->Dimensions, Target->Base.Address, PrefixDegree, Left->Dimensions, Left->Base.Address, Right->Degree, Right->Dimensions, Right->Base.Address);
	} else {
		ml_array_dimension_t ValueDimension[1] = {{1, 0, NULL}};
		CompareRowFns[Op](Target->Base.Address, ValueDimension, Left->Base.Address, ValueDimension, Right->Base.Address);
	}
	return (ml_value_t *)Target;
}

#define BUFFER_APPEND(BUFFER, PRINTF, VALUE) ml_stringbuffer_append(BUFFER, VALUE)

static long srotl(long X, unsigned int N) {
	const unsigned int Mask = (CHAR_BIT * sizeof(long) - 1);
	return (X << (N & Mask)) | (X >> ((-N) & Mask ));
}

#define ml_number(X) _Generic(X, ml_value_t *: ml_nop, double: ml_real, default: ml_integer)(X)

#define ml_number_value(T, X) _Generic(T, double: ml_real_value, default: ml_integer_value)(X)

#define METHODS(ATYPE, CTYPE, APPEND, PRINTF, FROM_VAL, TO_VAL, FROM_NUM, TO_NUM, FORMAT, HASH) \
\
static ml_value_t *ML_TYPED_FN(ml_array_value, ATYPE, ml_array_t *Array, char *Address) { \
	return TO_VAL(*(CTYPE *)Array->Base.Address); \
} \
\
static void append_array_ ## CTYPE(ml_stringbuffer_t *Buffer, int Degree, ml_array_dimension_t *Dimension, char *Address) { \
	ml_stringbuffer_add(Buffer, "<", 1); \
	int Stride = Dimension->Stride; \
	if (Dimension->Indices) { \
		int *Indices = Dimension->Indices; \
		if (Dimension->Size) { \
			if (Degree == 1) { \
				APPEND(Buffer, PRINTF, *(CTYPE *)(Address + (Indices[0]) * Dimension->Stride)); \
				for (int I = 1; I < Dimension->Size; ++I) { \
					ml_stringbuffer_add(Buffer, " ", 1); \
					APPEND(Buffer, PRINTF, *(CTYPE *)(Address + (Indices[I]) * Stride)); \
				} \
			} else { \
				append_array_ ## CTYPE(Buffer, Degree - 1, Dimension + 1, Address + (Indices[0]) * Dimension->Stride); \
				for (int I = 1; I < Dimension->Size; ++I) { \
					ml_stringbuffer_add(Buffer, " ", 1); \
					append_array_ ## CTYPE(Buffer, Degree - 1, Dimension + 1, Address + (Indices[I]) * Dimension->Stride); \
				} \
			} \
		} \
	} else { \
		if (Degree == 1) { \
			APPEND(Buffer, PRINTF, *(CTYPE *)Address); \
			Address += Stride; \
			for (int I = Dimension->Size; --I > 0;) { \
				ml_stringbuffer_add(Buffer, " ", 1); \
				APPEND(Buffer, PRINTF, *(CTYPE *)Address); \
				Address += Stride; \
			} \
		} else { \
			append_array_ ## CTYPE(Buffer, Degree - 1, Dimension + 1, Address); \
			Address += Stride; \
			for (int I = Dimension->Size; --I > 0;) { \
				ml_stringbuffer_add(Buffer, " ", 1); \
				append_array_ ## CTYPE(Buffer, Degree - 1, Dimension + 1, Address); \
				Address += Stride; \
			} \
		} \
	} \
	ml_stringbuffer_add(Buffer, ">", 1); \
} \
\
ML_METHOD(MLStringOfMethod, ATYPE) { \
	ml_array_t *Array = (ml_array_t *)Args[0]; \
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT}; \
	if (Array->Degree == 0) { \
		APPEND(Buffer, PRINTF, *(CTYPE *)Array->Base.Address); \
	} else { \
		append_array_ ## CTYPE(Buffer, Array->Degree, Array->Dimensions, Array->Base.Address); \
	} \
	return ml_stringbuffer_value(Buffer); \
} \
\
ML_METHOD("append", MLStringBufferT, ATYPE) { \
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0]; \
	ml_array_t *Array = (ml_array_t *)Args[1]; \
	if (Array->Degree == 0) { \
		APPEND(Buffer, PRINTF, *(CTYPE *)Array->Base.Address); \
	} else { \
		append_array_ ## CTYPE(Buffer, Array->Degree, Array->Dimensions, Array->Base.Address); \
	} \
	return MLSome; \
} \
\
UPDATE_METHODS(ATYPE, CTYPE, FROM_VAL, FORMAT); \
static CTYPE ml_array_get0_ ## CTYPE(void *Address, int Format) { \
	switch (Format) { \
	case ML_ARRAY_FORMAT_NONE: break; \
	case ML_ARRAY_FORMAT_I8: return FROM_NUM(*(int8_t *)Address); \
	case ML_ARRAY_FORMAT_U8: return FROM_NUM(*(uint8_t *)Address); \
	case ML_ARRAY_FORMAT_I16: return FROM_NUM(*(int16_t *)Address); \
	case ML_ARRAY_FORMAT_U16: return FROM_NUM(*(uint16_t *)Address); \
	case ML_ARRAY_FORMAT_I32: return FROM_NUM(*(int32_t *)Address); \
	case ML_ARRAY_FORMAT_U32: return FROM_NUM(*(uint32_t *)Address); \
	case ML_ARRAY_FORMAT_I64: return FROM_NUM(*(int64_t *)Address); \
	case ML_ARRAY_FORMAT_U64: return FROM_NUM(*(uint64_t *)Address); \
	case ML_ARRAY_FORMAT_F32: return FROM_NUM(*(float *)Address); \
	case ML_ARRAY_FORMAT_F64: return FROM_NUM(*(double *)Address); \
	case ML_ARRAY_FORMAT_ANY: return FROM_VAL(*(ml_value_t **)Address); \
	} \
	return (CTYPE)0; \
} \
\
CTYPE ml_array_get_ ## CTYPE(ml_array_t *Array, ...) { \
	va_list Indices; \
	va_start(Indices, Array); \
	char *Address = ml_array_indexv(Array, Indices); \
	va_end(Indices); \
	if (!Address) return 0; \
	return ml_array_get0_ ## CTYPE(Address, Array->Format); \
} \
\
void ml_array_set_ ## CTYPE(CTYPE Value, ml_array_t *Array, ...) { \
	va_list Indices; \
	va_start(Indices, Array); \
	char *Address = ml_array_indexv(Array, Indices); \
	va_end(Indices); \
	if (!Address) return; \
	switch (Array->Format) { \
	case ML_ARRAY_FORMAT_NONE: break; \
	case ML_ARRAY_FORMAT_I8: *(int8_t *)Address = TO_NUM((int8_t)0, Value); break; \
	case ML_ARRAY_FORMAT_U8: *(uint8_t *)Address = TO_NUM((uint8_t)0, Value); break; \
	case ML_ARRAY_FORMAT_I16: *(int16_t *)Address = TO_NUM((int16_t)0, Value); break; \
	case ML_ARRAY_FORMAT_U16: *(uint16_t *)Address = TO_NUM((uint16_t)0, Value); break; \
	case ML_ARRAY_FORMAT_I32: *(int32_t *)Address = TO_NUM((int32_t)0, Value); break; \
	case ML_ARRAY_FORMAT_U32: *(uint32_t *)Address = TO_NUM((uint32_t)0, Value); break; \
	case ML_ARRAY_FORMAT_I64: *(int64_t *)Address = TO_NUM((int64_t)0, Value); break; \
	case ML_ARRAY_FORMAT_U64: *(uint64_t *)Address = TO_NUM((uint8_t)0, Value); break; \
	case ML_ARRAY_FORMAT_F32: *(float *)Address = TO_NUM((float)0, Value); break; \
	case ML_ARRAY_FORMAT_F64: *(double *)Address = TO_NUM((double)0, Value); break; \
	case ML_ARRAY_FORMAT_ANY: *(ml_value_t **)Address = TO_VAL(Value); break; \
	} \
} \
\
static long hash_array_ ## CTYPE(int Degree, ml_array_dimension_t *Dimension, char *Address) { \
	int Stride = Dimension->Stride; \
	if (Dimension->Indices) { \
		int *Indices = Dimension->Indices; \
		if (Dimension->Size) { \
			if (Degree == 1) { \
				long Hash = HASH(*(CTYPE *)(Address + (Indices[0]) * Dimension->Stride)); \
				for (int I = 1; I < Dimension->Size; ++I) { \
					Hash = srotl(Hash, 1) | HASH(*(CTYPE *)(Address + (Indices[I]) * Stride)); \
				} \
				return srotl(Hash, Degree); \
			} else { \
				long Hash = hash_array_ ## CTYPE(Degree - 1, Dimension + 1, Address + (Indices[0]) * Dimension->Stride); \
				for (int I = 1; I < Dimension->Size; ++I) { \
					Hash = srotl(Hash, 1) | hash_array_ ## CTYPE(Degree - 1, Dimension + 1, Address + (Indices[I]) * Dimension->Stride); \
				} \
				return srotl(Hash, Degree); \
			} \
		} \
		return 0; \
	} else { \
		if (Degree == 1) { \
			long Hash = HASH(*(CTYPE *)Address); \
			Address += Stride; \
			for (int I = Dimension->Size; --I > 0;) { \
				Hash = srotl(Hash, 1) | HASH(*(CTYPE *)Address); \
				Address += Stride; \
			} \
			return srotl(Hash, Degree); \
		} else { \
			long Hash = hash_array_ ## CTYPE(Degree - 1, Dimension + 1, Address); \
			Address += Stride; \
			for (int I = Dimension->Size; --I > 0;) { \
				Hash = srotl(Hash, 1) | hash_array_ ## CTYPE(Degree - 1, Dimension + 1, Address); \
				Address += Stride; \
			} \
			return srotl(Hash, Degree); \
		} \
	} \
} \
\
static long ml_array_ ## CTYPE ## _hash(ml_array_t *Array) { \
	if (Array->Degree == 0) { \
		return Array->Format + (long)*(CTYPE *)Array->Base.Address; \
	} else { \
		return Array->Format + hash_array_ ## CTYPE(Array->Degree, Array->Dimensions, Array->Base.Address); \
	} \
} \
\
static ml_value_t *ml_array_ ## CTYPE ## _deref(ml_array_t *Target, ml_value_t *Value) { \
	if (Target->Degree == 0)  return TO_VAL(*(CTYPE *)Target->Base.Address); \
	return (ml_value_t *)Target; \
} \
\
static ml_value_t *ml_array_ ## CTYPE ## _assign(ml_array_t *Target, ml_value_t *Value) { \
	for (;;) if (FORMAT == ML_ARRAY_FORMAT_ANY && !Target->Degree) { \
		return *(ml_value_t **)Target->Base.Address = Value; \
	} else if (ml_is(Value, MLNumberT)) { \
		CTYPE CValue = FROM_VAL(Value); \
		ml_array_dimension_t ValueDimension[1] = {{1, 0, NULL}}; \
		int Op = Target->Format * MAX_FORMATS + Target->Format; \
		if (!UpdateRowFns[Op]) return ml_error("ArrayError", "Unsupported array format pair (%s, %s)", Target->Base.Type->Name, ml_typeof(Value)->Name); \
		if (Target->Degree == 0) { \
			UpdateRowFns[Op](ValueDimension, Target->Base.Address, ValueDimension, (char *)&CValue); \
		} else { \
			update_prefix(Op, Target->Degree - 1, Target->Dimensions, Target->Base.Address, 0, ValueDimension, (char *)&CValue); \
		} \
		return Value; \
	} else if (ml_is(Value, MLArrayT)) { \
		ml_array_t *Source = (ml_array_t *)Value; \
		if (Source->Degree > Target->Degree) return ml_error("ArrayError", "Incompatible assignment (%d)", __LINE__); \
		int PrefixDegree = Target->Degree - Source->Degree; \
		for (int I = 0; I < Source->Degree; ++I) { \
			if (Target->Dimensions[PrefixDegree + I].Size != Source->Dimensions[I].Size) return ml_error("ArrayError", "Incompatible assignment (%d)", __LINE__); \
		} \
		int Op = Target->Format * MAX_FORMATS + Source->Format; \
		if (!UpdateRowFns[Op]) return ml_error("ArrayError", "Unsupported array format pair (%s, %s)", Target->Base.Type->Name, Source->Base.Type->Name); \
		if (Target->Degree) { \
			update_prefix(Op, PrefixDegree, Target->Dimensions, Target->Base.Address, Source->Degree, Source->Dimensions, Source->Base.Address); \
		} else { \
			ml_array_dimension_t ValueDimension[1] = {{1, 0, NULL}}; \
			UpdateRowFns[Op](ValueDimension, Target->Base.Address, ValueDimension, Source->Base.Address); \
		} \
		return Value; \
	} else { \
		Value = ml_array_of_fn(NULL, 1, &Value); \
	} \
	return NULL; \
} \
\
ML_TYPE(ATYPE, (MLArrayT), #CTYPE "-array", \
	.hash = (void *)ml_array_ ## CTYPE ## _hash, \
	.deref = (void *)ml_array_ ## CTYPE ## _deref, \
	.assign = (void *)ml_array_ ## CTYPE ## _assign \
);

typedef ml_value_t *value;

#define NOP_VAL(T, X) X

METHODS(MLArrayInt8T, int8_t, ml_stringbuffer_addf, "%d", ml_integer_value, ml_integer, , NOP_VAL, ML_ARRAY_FORMAT_I8, (long));
METHODS(MLArrayUInt8T, uint8_t, ml_stringbuffer_addf, "%ud", ml_integer_value, ml_integer, , NOP_VAL, ML_ARRAY_FORMAT_U8, (long));
METHODS(MLArrayInt16T, int16_t, ml_stringbuffer_addf, "%d", ml_integer_value, ml_integer, , NOP_VAL, ML_ARRAY_FORMAT_I16, (long));
METHODS(MLArrayUInt16T, uint16_t, ml_stringbuffer_addf, "%ud", ml_integer_value, ml_integer, , NOP_VAL, ML_ARRAY_FORMAT_U16, (long));
METHODS(MLArrayInt32T, int32_t, ml_stringbuffer_addf, "%d", ml_integer_value, ml_integer, , NOP_VAL, ML_ARRAY_FORMAT_I32, (long));
METHODS(MLArrayUInt32T, uint32_t, ml_stringbuffer_addf, "%ud", ml_integer_value, ml_integer, , NOP_VAL, ML_ARRAY_FORMAT_U32, (long));
METHODS(MLArrayInt64T, int64_t, ml_stringbuffer_addf, "%ld", ml_integer_value, ml_integer, , NOP_VAL, ML_ARRAY_FORMAT_I64, (long));
METHODS(MLArrayUInt64T, uint64_t, ml_stringbuffer_addf, "%lud", ml_integer_value, ml_integer, , NOP_VAL, ML_ARRAY_FORMAT_U64, (long));
METHODS(MLArrayFloat32T, float, ml_stringbuffer_addf, "%f", ml_real_value, ml_real, , NOP_VAL, ML_ARRAY_FORMAT_F32, (long));
METHODS(MLArrayFloat64T, double, ml_stringbuffer_addf, "%f", ml_real_value, ml_real, , NOP_VAL, ML_ARRAY_FORMAT_F64, (long));
METHODS(MLArrayAnyT, value, BUFFER_APPEND, "?", ml_nop, ml_nop, ml_number, ml_number_value, ML_ARRAY_FORMAT_ANY, ml_hash);

#define PARTIAL_FUNCTIONS(CTYPE) \
\
static void partial_sums_ ## CTYPE(int Target, int Degree, ml_array_dimension_t *Dimension, char *Address, int LastRow) { \
	if (Degree == 0) { \
		*(CTYPE *)Address += *(CTYPE *)(Address - LastRow); \
	} else if (Target == Degree) { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices; \
			for (int I = 1; I < Dimension->Size; ++I) { \
				partial_sums_ ## CTYPE(Target, Degree - 1, Dimension + 1, Address + Indices[I] * Stride, (Indices[I] - Indices[I - 1]) * Stride); \
			} \
		} else { \
			for (int I = 1; I < Dimension->Size; ++I) { \
				Address += Stride; \
				partial_sums_ ## CTYPE(Target, Degree - 1, Dimension + 1, Address, Stride); \
			} \
		} \
	} else { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				partial_sums_ ## CTYPE(Target, Degree - 1, Dimension + 1, Address + Indices[I] * Stride, LastRow); \
			} \
		} else { \
			for (int I = 0; I < Dimension->Size; ++I) { \
				partial_sums_ ## CTYPE(Target, Degree - 1, Dimension + 1, Address, LastRow); \
				Address += Stride; \
			} \
		} \
	} \
} \
\
static void partial_prods_ ## CTYPE(int Target, int Degree, ml_array_dimension_t *Dimension, char *Address, int LastRow) { \
	if (Degree == 0) { \
		*(CTYPE *)Address *= *(CTYPE *)(Address - LastRow); \
	} else if (Target == Degree) { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices; \
			for (int I = 1; I < Dimension->Size; ++I) { \
				partial_prods_ ## CTYPE(Target, Degree - 1, Dimension + 1, Address + Indices[I] * Stride, (Indices[I] - Indices[I - 1]) * Stride); \
			} \
		} else { \
			for (int I = 1; I < Dimension->Size; ++I) { \
				Address += Stride; \
				partial_prods_ ## CTYPE(Target, Degree - 1, Dimension + 1, Address, Stride); \
			} \
		} \
	} else { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				partial_prods_ ## CTYPE(Target, Degree - 1, Dimension + 1, Address + Indices[I] * Stride, LastRow); \
			} \
		} else { \
			for (int I = 0; I < Dimension->Size; ++I) { \
				partial_prods_ ## CTYPE(Target, Degree - 1, Dimension + 1, Address, LastRow); \
				Address += Stride; \
			} \
		} \
	} \
} \

#define COMPLETE_FUNCTIONS(CTYPE1, CTYPE2) \
\
static CTYPE1 compute_sums_ ## CTYPE1 ## _ ## CTYPE2(int Degree, ml_array_dimension_t *Dimension, void *Address) { \
	CTYPE1 Sum = 0; \
	if (Degree > 1) { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Sum += compute_sums_ ## CTYPE1 ## _ ## CTYPE2(Degree - 1, Dimension + 1, Address + Indices[I] * Stride); \
			} \
		} else { \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Sum += compute_sums_ ## CTYPE1 ## _ ## CTYPE2(Degree - 1, Dimension + 1, Address); \
				Address += Stride; \
			} \
		} \
	} else { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Sum += *(CTYPE2 *)(Address + Indices[I] * Stride); \
			} \
		} else { \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Sum += *(CTYPE2 *)Address; \
				Address += Stride; \
			} \
		} \
	} \
	return Sum; \
} \
\
static void fill_sums_ ## CTYPE1 ## _ ## CTYPE2(int TargetDegree, ml_array_dimension_t *TargetDimension, void *TargetAddress, int SourceDegree, ml_array_dimension_t *SourceDimension, void *SourceAddress) { \
	if (TargetDegree == 0) { \
		*(CTYPE1 *)TargetAddress = compute_sums_ ## CTYPE1 ## _ ## CTYPE2(SourceDegree, SourceDimension, SourceAddress); \
	} else { \
		int TargetStride = TargetDimension->Stride; \
		int SourceStride = SourceDimension->Stride; \
		if (SourceDimension->Indices) { \
			int *Indices = SourceDimension->Indices; \
			for (int I = 0; I < SourceDimension->Size; ++I) { \
				fill_sums_ ## CTYPE1 ## _ ## CTYPE2(TargetDegree - 1, TargetDimension + 1, TargetAddress, SourceDegree - 1, SourceDimension + 1, SourceAddress + Indices[I] * SourceStride); \
				TargetAddress += TargetStride; \
			} \
		} else { \
			for (int I = 0; I < SourceDimension->Size; ++I) { \
				fill_sums_ ## CTYPE1 ## _ ## CTYPE2(TargetDegree - 1, TargetDimension + 1, TargetAddress, SourceDegree - 1, SourceDimension + 1, SourceAddress); \
				TargetAddress += TargetStride; \
				SourceAddress += SourceStride; \
			} \
		} \
	} \
} \
\
static CTYPE1 compute_prods_ ## CTYPE1 ## _ ## CTYPE2(int Degree, ml_array_dimension_t *Dimension, void *Address) { \
	CTYPE1 Prod = 1; \
	if (Degree > 1) { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Prod += compute_prods_ ## CTYPE1 ## _ ## CTYPE2(Degree - 1, Dimension + 1, Address + Indices[I] * Stride); \
			} \
		} else { \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Prod += compute_prods_ ## CTYPE1 ## _ ## CTYPE2(Degree - 1, Dimension + 1, Address); \
				Address += Stride; \
			} \
		} \
	} else { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Prod += *(CTYPE2 *)(Address + Indices[I] * Stride); \
			} \
		} else { \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Prod += *(CTYPE2 *)Address; \
				Address += Stride; \
			} \
		} \
	} \
	return Prod; \
} \
\
static void fill_prods_ ## CTYPE1 ## _ ## CTYPE2(int TargetDegree, ml_array_dimension_t *TargetDimension, void *TargetAddress, int SourceDegree, ml_array_dimension_t *SourceDimension, void *SourceAddress) { \
	if (TargetDegree == 0) { \
		*(CTYPE1 *)TargetAddress = compute_prods_ ## CTYPE1 ## _ ## CTYPE2(SourceDegree, SourceDimension, SourceAddress); \
	} else { \
		int TargetStride = TargetDimension->Stride; \
		int SourceStride = SourceDimension->Stride; \
		if (SourceDimension->Indices) { \
			int *Indices = SourceDimension->Indices; \
			for (int I = 0; I < SourceDimension->Size; ++I) { \
				fill_prods_ ## CTYPE1 ## _ ## CTYPE2(TargetDegree - 1, TargetDimension + 1, TargetAddress, SourceDegree - 1, SourceDimension + 1, SourceAddress + Indices[I] * SourceStride); \
				TargetAddress += TargetStride; \
			} \
		} else { \
			for (int I = 0; I < SourceDimension->Size; ++I) { \
				fill_prods_ ## CTYPE1 ## _ ## CTYPE2(TargetDegree - 1, TargetDimension + 1, TargetAddress, SourceDegree - 1, SourceDimension + 1, SourceAddress); \
				TargetAddress += TargetStride; \
				SourceAddress += SourceStride; \
			} \
		} \
	} \
}

PARTIAL_FUNCTIONS(int64_t);
PARTIAL_FUNCTIONS(uint64_t);
PARTIAL_FUNCTIONS(double);

COMPLETE_FUNCTIONS(int64_t, int8_t);
COMPLETE_FUNCTIONS(uint64_t, uint8_t);
COMPLETE_FUNCTIONS(int64_t, int16_t);
COMPLETE_FUNCTIONS(uint64_t, uint16_t);
COMPLETE_FUNCTIONS(int64_t, int32_t);
COMPLETE_FUNCTIONS(uint64_t, uint32_t);
COMPLETE_FUNCTIONS(int64_t, int64_t);
COMPLETE_FUNCTIONS(uint64_t, uint64_t);
COMPLETE_FUNCTIONS(double, float);
COMPLETE_FUNCTIONS(double, double);

static int array_copy(ml_array_t *Target, ml_array_t *Source) {
	int Degree = Source->Degree;
	int DataSize = MLArraySizes[Target->Format];
	for (int I = Degree; --I >= 0;) {
		Target->Dimensions[I].Stride = DataSize;
		int Size = Target->Dimensions[I].Size = Source->Dimensions[I].Size;
		DataSize *= Size;
	}
	Target->Base.Address = GC_MALLOC_ATOMIC(DataSize);
	int Op = Target->Format * MAX_FORMATS + Source->Format;
	update_array(Op, Target->Dimensions, Target->Base.Address, Degree, Source->Dimensions, Source->Base.Address);
	return DataSize;
}

ML_METHOD("sums", MLArrayT, MLIntegerT) {
//<Array
//<Index
//>array
// Returns a new array with the partial sums of :mini:`Array` in the :mini:`Index`-th dimension.
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Index = ml_integer_value(Args[1]);
	if (Index <= 0) Index += Source->Degree + 1;
	if (Index < 1 || Index > Source->Degree) return ml_error("ArrayError", "Dimension index invalid");
	Index = Source->Degree + 1 - Index;
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_I8:
	case ML_ARRAY_FORMAT_I16:
	case ML_ARRAY_FORMAT_I32:
	case ML_ARRAY_FORMAT_I64: {
		ml_array_t *Target = ml_array_new(ML_ARRAY_FORMAT_I64, Source->Degree);
		array_copy(Target, Source);
		partial_sums_int64_t(Index, Target->Degree, Target->Dimensions, Target->Base.Address, 0);
		return (ml_value_t *)Target;
	}
	case ML_ARRAY_FORMAT_U8:
	case ML_ARRAY_FORMAT_U16:
	case ML_ARRAY_FORMAT_U32:
	case ML_ARRAY_FORMAT_U64: {
		ml_array_t *Target = ml_array_new(ML_ARRAY_FORMAT_U64, Source->Degree);
		array_copy(Target, Source);
		partial_sums_uint64_t(Index, Target->Degree, Target->Dimensions, Target->Base.Address, 0);
		return (ml_value_t *)Target;
	}
	case ML_ARRAY_FORMAT_F32:
	case ML_ARRAY_FORMAT_F64: {
		ml_array_t *Target = ml_array_new(ML_ARRAY_FORMAT_F64, Source->Degree);
		array_copy(Target, Source);
		partial_sums_double(Index, Target->Degree, Target->Dimensions, Target->Base.Address, 0);
		return (ml_value_t *)Target;
	}
	default:
		return ml_error("ArrayError", "Invalid array format");
	}
}

ML_METHOD("prods", MLArrayT, MLIntegerT) {
//<Array
//<Index
//>array
// Returns a new array with the partial products of :mini:`Array` in the :mini:`Index`-th dimension.
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Index = ml_integer_value(Args[1]);
	if (Index <= 0) Index += Source->Degree + 1;
	if (Index < 1 || Index > Source->Degree) return ml_error("ArrayError", "Dimension index invalid");
	Index = Source->Degree + 1 - Index;
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_I8:
	case ML_ARRAY_FORMAT_I16:
	case ML_ARRAY_FORMAT_I32:
	case ML_ARRAY_FORMAT_I64: {
		ml_array_t *Target = ml_array_new(ML_ARRAY_FORMAT_I64, Source->Degree);
		array_copy(Target, Source);
		partial_prods_int64_t(Index, Target->Degree, Target->Dimensions, Target->Base.Address, 0);
		return (ml_value_t *)Target;
	}
	case ML_ARRAY_FORMAT_U8:
	case ML_ARRAY_FORMAT_U16:
	case ML_ARRAY_FORMAT_U32:
	case ML_ARRAY_FORMAT_U64: {
		ml_array_t *Target = ml_array_new(ML_ARRAY_FORMAT_U64, Source->Degree);
		array_copy(Target, Source);
		partial_prods_uint64_t(Index, Target->Degree, Target->Dimensions, Target->Base.Address, 0);
		return (ml_value_t *)Target;
	}
	case ML_ARRAY_FORMAT_F32:
	case ML_ARRAY_FORMAT_F64: {
		ml_array_t *Target = ml_array_new(ML_ARRAY_FORMAT_F64, Source->Degree);
		array_copy(Target, Source);
		partial_prods_double(Index, Target->Degree, Target->Dimensions, Target->Base.Address, 0);
		return (ml_value_t *)Target;
	}
	default:
		return ml_error("ArrayError", "Invalid array format");
	}
}

ML_METHOD("sum", MLArrayT) {
//<Array
//>integer|real
// Returns the sum of the values in :mini:`Array`.
	ml_array_t *Source = (ml_array_t *)Args[0];
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_I8:
		return ml_integer(compute_sums_int64_t_int8_t(Source->Degree, Source->Dimensions, Source->Base.Address));
	case ML_ARRAY_FORMAT_U8:
		return ml_integer(compute_sums_uint64_t_uint8_t(Source->Degree, Source->Dimensions, Source->Base.Address));
	case ML_ARRAY_FORMAT_I16:
		return ml_integer(compute_sums_int64_t_int16_t(Source->Degree, Source->Dimensions, Source->Base.Address));
	case ML_ARRAY_FORMAT_U16:
		return ml_integer(compute_sums_uint64_t_uint16_t(Source->Degree, Source->Dimensions, Source->Base.Address));
	case ML_ARRAY_FORMAT_I32:
		return ml_integer(compute_sums_int64_t_int32_t(Source->Degree, Source->Dimensions, Source->Base.Address));
	case ML_ARRAY_FORMAT_U32:
		return ml_integer(compute_sums_uint64_t_uint32_t(Source->Degree, Source->Dimensions, Source->Base.Address));
	case ML_ARRAY_FORMAT_I64:
		return ml_integer(compute_sums_int64_t_int64_t(Source->Degree, Source->Dimensions, Source->Base.Address));
	case ML_ARRAY_FORMAT_U64:
		return ml_integer(compute_sums_uint64_t_uint64_t(Source->Degree, Source->Dimensions, Source->Base.Address));
	case ML_ARRAY_FORMAT_F32:
		return ml_real(compute_sums_double_float(Source->Degree, Source->Dimensions, Source->Base.Address));
	case ML_ARRAY_FORMAT_F64:
		return ml_real(compute_sums_double_double(Source->Degree, Source->Dimensions, Source->Base.Address));
	default:
		return ml_error("ArrayError", "Invalid array format");
	}
}

ML_METHOD("sum", MLArrayT, MLIntegerT) {
//<Array
//<Index
//>array
// Returns a new array with the sums of :mini:`Array` in the :mini:`Index`-th dimension.
	ml_array_t *Source = (ml_array_t *)Args[0];
	int SumDegree = ml_integer_value(Args[1]);
	if (SumDegree <= 0 || SumDegree >= Source->Degree) return ml_error("RangeError", "Invalid axes count for sum");
	ml_array_format_t Format;
	void (*fill_sums)(int, ml_array_dimension_t *, void *, int, ml_array_dimension_t *, void *);
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_I8:
		Format = ML_ARRAY_FORMAT_I64;
		fill_sums = fill_sums_int64_t_int8_t;
		break;
	case ML_ARRAY_FORMAT_U8:
		Format = ML_ARRAY_FORMAT_U64;
		fill_sums = fill_sums_uint64_t_uint8_t;
		break;
	case ML_ARRAY_FORMAT_I16:
		Format = ML_ARRAY_FORMAT_I64;
		fill_sums = fill_sums_int64_t_int16_t;
		break;
	case ML_ARRAY_FORMAT_U16:
		Format = ML_ARRAY_FORMAT_U64;
		fill_sums = fill_sums_uint64_t_uint16_t;
		break;
	case ML_ARRAY_FORMAT_I32:
		Format = ML_ARRAY_FORMAT_I64;
		fill_sums = fill_sums_int64_t_int32_t;
		break;
	case ML_ARRAY_FORMAT_U32:
		Format = ML_ARRAY_FORMAT_U64;
		fill_sums = fill_sums_uint64_t_uint32_t;
		break;
	case ML_ARRAY_FORMAT_I64:
		Format = ML_ARRAY_FORMAT_I64;
		fill_sums = fill_sums_int64_t_int64_t;
		break;
	case ML_ARRAY_FORMAT_U64:
		Format = ML_ARRAY_FORMAT_U64;
		fill_sums = fill_sums_uint64_t_uint64_t;
		break;
	case ML_ARRAY_FORMAT_F32:
		Format = ML_ARRAY_FORMAT_F64;
		fill_sums = fill_sums_double_float;
		break;
	case ML_ARRAY_FORMAT_F64:
		Format = ML_ARRAY_FORMAT_F64;
		fill_sums = fill_sums_double_double;
		break;
	default:
		return ml_error("ArrayError", "Invalid array format");
	}
	ml_array_t *Target = ml_array_new(Format, Source->Degree - SumDegree);
	int DataSize = MLArraySizes[Target->Format];
	for (int I = Target->Degree; --I >= 0;) {
		Target->Dimensions[I].Stride = DataSize;
		int Size = Target->Dimensions[I].Size = Source->Dimensions[I].Size;
		DataSize *= Size;
	}
	Target->Base.Address = GC_MALLOC_ATOMIC(DataSize);
	fill_sums(Target->Degree, Target->Dimensions, Target->Base.Address, Source->Degree, Source->Dimensions, Source->Base.Address);
	return (ml_value_t *)Target;
}

ML_METHOD("prod", MLArrayT) {
//<Array
//>integer|real
// Returns the product of the values in :mini:`Array`.
	ml_array_t *Source = (ml_array_t *)Args[0];
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_I8:
		return ml_integer(compute_prods_int64_t_int8_t(Source->Degree, Source->Dimensions, Source->Base.Address));
	case ML_ARRAY_FORMAT_U8:
		return ml_integer(compute_prods_uint64_t_uint8_t(Source->Degree, Source->Dimensions, Source->Base.Address));
	case ML_ARRAY_FORMAT_I16:
		return ml_integer(compute_prods_int64_t_int16_t(Source->Degree, Source->Dimensions, Source->Base.Address));
	case ML_ARRAY_FORMAT_U16:
		return ml_integer(compute_prods_uint64_t_uint16_t(Source->Degree, Source->Dimensions, Source->Base.Address));
	case ML_ARRAY_FORMAT_I32:
		return ml_integer(compute_prods_int64_t_int32_t(Source->Degree, Source->Dimensions, Source->Base.Address));
	case ML_ARRAY_FORMAT_U32:
		return ml_integer(compute_prods_uint64_t_uint32_t(Source->Degree, Source->Dimensions, Source->Base.Address));
	case ML_ARRAY_FORMAT_I64:
		return ml_integer(compute_prods_int64_t_int64_t(Source->Degree, Source->Dimensions, Source->Base.Address));
	case ML_ARRAY_FORMAT_U64:
		return ml_integer(compute_prods_uint64_t_uint64_t(Source->Degree, Source->Dimensions, Source->Base.Address));
	case ML_ARRAY_FORMAT_F32:
		return ml_real(compute_prods_double_float(Source->Degree, Source->Dimensions, Source->Base.Address));
	case ML_ARRAY_FORMAT_F64:
		return ml_real(compute_prods_double_double(Source->Degree, Source->Dimensions, Source->Base.Address));
	default:
		return ml_error("ArrayError", "Invalid array format");
	}
}

ML_METHOD("prod", MLArrayT, MLIntegerT) {
//<Array
//<Index
//>array
// Returns a new array with the products of :mini:`Array` in the :mini:`Index`-th dimension.
	ml_array_t *Source = (ml_array_t *)Args[0];
	int SumDegree = ml_integer_value(Args[1]);
	if (SumDegree <= 0 || SumDegree >= Source->Degree) return ml_error("RangeError", "Invalid axes count for prod");
	ml_array_format_t Format;
	void (*fill_prods)(int, ml_array_dimension_t *, void *, int, ml_array_dimension_t *, void *);
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_I8:
		Format = ML_ARRAY_FORMAT_I64;
		fill_prods = fill_prods_int64_t_int8_t;
		break;
	case ML_ARRAY_FORMAT_U8:
		Format = ML_ARRAY_FORMAT_U64;
		fill_prods = fill_prods_uint64_t_uint8_t;
		break;
	case ML_ARRAY_FORMAT_I16:
		Format = ML_ARRAY_FORMAT_I64;
		fill_prods = fill_prods_int64_t_int16_t;
		break;
	case ML_ARRAY_FORMAT_U16:
		Format = ML_ARRAY_FORMAT_U64;
		fill_prods = fill_prods_uint64_t_uint16_t;
		break;
	case ML_ARRAY_FORMAT_I32:
		Format = ML_ARRAY_FORMAT_I64;
		fill_prods = fill_prods_int64_t_int32_t;
		break;
	case ML_ARRAY_FORMAT_U32:
		Format = ML_ARRAY_FORMAT_U64;
		fill_prods = fill_prods_uint64_t_uint32_t;
		break;
	case ML_ARRAY_FORMAT_I64:
		Format = ML_ARRAY_FORMAT_I64;
		fill_prods = fill_prods_int64_t_int64_t;
		break;
	case ML_ARRAY_FORMAT_U64:
		Format = ML_ARRAY_FORMAT_U64;
		fill_prods = fill_prods_uint64_t_uint64_t;
		break;
	case ML_ARRAY_FORMAT_F32:
		Format = ML_ARRAY_FORMAT_F64;
		fill_prods = fill_prods_double_float;
		break;
	case ML_ARRAY_FORMAT_F64:
		Format = ML_ARRAY_FORMAT_F64;
		fill_prods = fill_prods_double_double;
		break;
	default:
		return ml_error("ArrayError", "Invalid array format");
	}
	ml_array_t *Target = ml_array_new(Format, Source->Degree - SumDegree);
	int DataSize = MLArraySizes[Target->Format];
	for (int I = Target->Degree; --I >= 0;) {
		Target->Dimensions[I].Stride = DataSize;
		int Size = Target->Dimensions[I].Size = Source->Dimensions[I].Size;
		DataSize *= Size;
	}
	Target->Base.Address = GC_MALLOC_ATOMIC(DataSize);
	fill_prods(Target->Degree, Target->Dimensions, Target->Base.Address, Source->Degree, Source->Dimensions, Source->Base.Address);
	return (ml_value_t *)Target;
}

ML_METHOD("-", MLArrayT) {
//<Array
//>array
// Returns an array with the negated values from :mini:`Array`.
	ml_array_t *A = (ml_array_t *)Args[0];
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation");
	int Degree = A->Degree;
	ml_array_t *C = ml_array_new(A->Format, Degree);
	int DataSize = array_copy(C, A);
	switch (C->Format) {
	case ML_ARRAY_FORMAT_I8: {
		int8_t *Values = (int8_t *)C->Base.Address;
		for (int I = DataSize / sizeof(int8_t); --I >= 0; ++Values) *Values = -*Values;
		break;
	}
	case ML_ARRAY_FORMAT_U8: {
		uint8_t *Values = (uint8_t *)C->Base.Address;
		for (int I = DataSize / sizeof(uint8_t); --I >= 0; ++Values) *Values = -*Values;
		break;
	}
	case ML_ARRAY_FORMAT_I16: {
		int16_t *Values = (int16_t *)C->Base.Address;
		for (int I = DataSize / sizeof(int16_t); --I >= 0; ++Values) *Values = -*Values;
		break;
	}
	case ML_ARRAY_FORMAT_U16: {
		uint16_t *Values = (uint16_t *)C->Base.Address;
		for (int I = DataSize / sizeof(uint16_t); --I >= 0; ++Values) *Values = -*Values;
		break;
	}
	case ML_ARRAY_FORMAT_I32: {
		int32_t *Values = (int32_t *)C->Base.Address;
		for (int I = DataSize / sizeof(int32_t); --I >= 0; ++Values) *Values = -*Values;
		break;
	}
	case ML_ARRAY_FORMAT_U32: {
		uint32_t *Values = (uint32_t *)C->Base.Address;
		for (int I = DataSize / sizeof(uint32_t); --I >= 0; ++Values) *Values = -*Values;
		break;
	}
	case ML_ARRAY_FORMAT_I64: {
		int64_t *Values = (int64_t *)C->Base.Address;
		for (int I = DataSize / sizeof(int64_t); --I >= 0; ++Values) *Values = -*Values;
		break;
	}
	case ML_ARRAY_FORMAT_U64: {
		uint64_t *Values = (uint64_t *)C->Base.Address;
		for (int I = DataSize / sizeof(uint64_t); --I >= 0; ++Values) *Values = -*Values;
		break;
	}
	case ML_ARRAY_FORMAT_F32: {
		float *Values = (float *)C->Base.Address;
		for (int I = DataSize / sizeof(float); --I >= 0; ++Values) *Values = -*Values;
		break;
	}
	case ML_ARRAY_FORMAT_F64: {
		double *Values = (double *)C->Base.Address;
		for (int I = DataSize / sizeof(double); --I >= 0; ++Values) *Values = -*Values;
		break;
	}
	default: {
		return ml_error("TypeError", "Invalid types for array operation");
	}
	}
	return (ml_value_t *)C;
}

#define MAX(X, Y) ((X > Y) ? X : Y)

static ml_value_t *array_infix_fn(void *Data, int Count, ml_value_t **Args) {
	ml_array_t *A = (ml_array_t *)Args[0];
	ml_array_t *B = (ml_array_t *)Args[1];
	int Degree = A->Degree;
	ml_array_t *C = ml_array_new(MAX(A->Format, B->Format), Degree);
	array_copy(C, A);
	int Op2 = ((char *)Data - (char *)0) * MAX_FORMATS * MAX_FORMATS + C->Format * MAX_FORMATS + B->Format;
	update_prefix(Op2, C->Degree - B->Degree, C->Dimensions, C->Base.Address, B->Degree, B->Dimensions, B->Base.Address);
	return (ml_value_t *)C;
}

#define ML_ARITH_METHOD(BASE, OP) \
\
ML_METHOD(#OP, MLArrayT, MLIntegerT) { \
	ml_array_t *A = (ml_array_t *)Args[0]; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	int64_t B = ml_integer_value(Args[1]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_new(MAX(A->Format, ML_ARRAY_FORMAT_I64), Degree); \
	int DataSize = array_copy(C, A); \
	switch (C->Format) { \
	case ML_ARRAY_FORMAT_I64: { \
		int64_t *Values = (int64_t *)C->Base.Address; \
		for (int I = DataSize / sizeof(int64_t); --I >= 0; ++Values) *Values = *Values OP B; \
		break; \
	} \
	case ML_ARRAY_FORMAT_U64: { \
		uint64_t *Values = (uint64_t *)C->Base.Address; \
		for (int I = DataSize / sizeof(uint64_t); --I >= 0; ++Values) *Values = *Values OP B; \
		break; \
	} \
	case ML_ARRAY_FORMAT_F32: { \
		float *Values = (float *)C->Base.Address; \
		for (int I = DataSize / sizeof(float); --I >= 0; ++Values) *Values = *Values OP B; \
		break; \
	} \
	case ML_ARRAY_FORMAT_F64: { \
		double *Values = (double *)C->Base.Address; \
		for (int I = DataSize / sizeof(double); --I >= 0; ++Values) *Values = *Values OP B; \
		break; \
	} \
	default: { \
		return ml_error("TypeError", "Invalid types for array operation"); \
	} \
	} \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#OP, MLIntegerT, MLArrayT) { \
	ml_array_t *A = (ml_array_t *)Args[1]; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	int64_t B = ml_integer_value(Args[0]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_new(MAX(A->Format, ML_ARRAY_FORMAT_I64), Degree); \
	int DataSize = array_copy(C, A); \
	switch (C->Format) { \
	case ML_ARRAY_FORMAT_I64: { \
		int64_t *Values = (int64_t *)C->Base.Address; \
		for (int I = DataSize / sizeof(int64_t); --I >= 0; ++Values) *Values = B OP *Values; \
		break; \
	} \
	case ML_ARRAY_FORMAT_U64: { \
		uint64_t *Values = (uint64_t *)C->Base.Address; \
		for (int I = DataSize / sizeof(uint64_t); --I >= 0; ++Values) *Values = B OP *Values; \
		break; \
	} \
	case ML_ARRAY_FORMAT_F32: { \
		float *Values = (float *)C->Base.Address; \
		for (int I = DataSize / sizeof(float); --I >= 0; ++Values) *Values = B OP *Values; \
		break; \
	} \
	case ML_ARRAY_FORMAT_F64: { \
		double *Values = (double *)C->Base.Address; \
		for (int I = DataSize / sizeof(double); --I >= 0; ++Values) *Values = B OP *Values; \
		break; \
	} \
	default: { \
		return ml_error("TypeError", "Invalid types for array operation"); \
	} \
	} \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#OP, MLArrayT, MLRealT) { \
	ml_array_t *A = (ml_array_t *)Args[0]; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	double B = ml_integer_value(Args[1]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_new(ML_ARRAY_FORMAT_F64, Degree); \
	int DataSize = array_copy(C, A); \
	double *Values = (double *)C->Base.Address; \
	for (int I = DataSize / sizeof(double); --I >= 0; ++Values) *Values = *Values OP B; \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#OP, MLRealT, MLArrayT) { \
	ml_array_t *A = (ml_array_t *)Args[1]; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	double B = ml_integer_value(Args[0]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_new(ML_ARRAY_FORMAT_F64, Degree); \
	int DataSize = array_copy(C, A); \
	double *Values = (double *)C->Base.Address; \
	for (int I = DataSize / sizeof(double); --I >= 0; ++Values) *Values = B OP *Values; \
	return (ml_value_t *)C; \
}

ML_ARITH_METHOD(1, +);
ML_ARITH_METHOD(2, -);
ML_ARITH_METHOD(3, *);
ML_ARITH_METHOD(4, /);

#define ML_COMPARE_METHOD(BASE, BASE2, OP) \
\
ML_METHOD(#OP, MLArrayT, MLIntegerT) { \
	ml_array_t *A = (ml_array_t *)Args[0]; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	int64_t B = ml_integer_value(Args[1]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_new(ML_ARRAY_FORMAT_I8, Degree); \
	int DataSize = 1; \
	for (int I = Degree; --I >= 0;) { \
		C->Dimensions[I].Stride = DataSize; \
		int Size = C->Dimensions[I].Size = A->Dimensions[I].Size; \
		DataSize *= Size; \
	} \
	C->Base.Address = GC_MALLOC_ATOMIC(DataSize); \
	int Op = BASE * MAX_FORMATS * MAX_FORMATS + A->Format * MAX_FORMATS + ML_ARRAY_FORMAT_I64; \
	if (!CompareRowFns[Op]) return ml_error("ArrayError", "Unsupported array format pair (%s, integer)", A->Base.Type->Name); \
	compare_prefix(Op, C->Dimensions, C->Base.Address, Degree - 1, A->Dimensions, A->Base.Address, 0, NULL, (char *)&B); \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#OP, MLIntegerT, MLArrayT) { \
	ml_array_t *A = (ml_array_t *)Args[1]; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	int64_t B = ml_integer_value(Args[0]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_new(ML_ARRAY_FORMAT_I8, Degree); \
	int DataSize = 1; \
	for (int I = Degree; --I >= 0;) { \
		C->Dimensions[I].Stride = DataSize; \
		int Size = C->Dimensions[I].Size = A->Dimensions[I].Size; \
		DataSize *= Size; \
	} \
	C->Base.Address = GC_MALLOC_ATOMIC(DataSize); \
	int Op = BASE2 * MAX_FORMATS * MAX_FORMATS + A->Format * MAX_FORMATS + ML_ARRAY_FORMAT_I64; \
	if (!CompareRowFns[Op]) return ml_error("ArrayError", "Unsupported array format pair (integer, %s)", A->Base.Type->Name); \
	compare_prefix(Op, C->Dimensions, C->Base.Address, Degree - 1, A->Dimensions, A->Base.Address, 0, NULL, (char *)&B); \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#OP, MLArrayT, MLRealT) { \
	ml_array_t *A = (ml_array_t *)Args[0]; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	double B = ml_real_value(Args[1]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_new(ML_ARRAY_FORMAT_I8, Degree); \
	int DataSize = 1; \
	for (int I = Degree; --I >= 0;) { \
		C->Dimensions[I].Stride = DataSize; \
		int Size = C->Dimensions[I].Size = A->Dimensions[I].Size; \
		DataSize *= Size; \
	} \
	C->Base.Address = GC_MALLOC_ATOMIC(DataSize); \
	int Op = BASE * MAX_FORMATS * MAX_FORMATS + A->Format * MAX_FORMATS + ML_ARRAY_FORMAT_F64; \
	if (!CompareRowFns[Op]) return ml_error("ArrayError", "Unsupported array format pair (%s, integer)", A->Base.Type->Name); \
	compare_prefix(Op, C->Dimensions, C->Base.Address, Degree - 1, A->Dimensions, A->Base.Address, 0, NULL, (char *)&B); \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#OP, MLRealT, MLArrayT) { \
	ml_array_t *A = (ml_array_t *)Args[1]; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	double B = ml_real_value(Args[0]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_new(ML_ARRAY_FORMAT_I8, Degree); \
	int DataSize = 1; \
	for (int I = Degree; --I >= 0;) { \
		C->Dimensions[I].Stride = DataSize; \
		int Size = C->Dimensions[I].Size = A->Dimensions[I].Size; \
		DataSize *= Size; \
	} \
	C->Base.Address = GC_MALLOC_ATOMIC(DataSize); \
	int Op = BASE2 * MAX_FORMATS * MAX_FORMATS + A->Format * MAX_FORMATS + ML_ARRAY_FORMAT_F64; \
	if (!CompareRowFns[Op]) return ml_error("ArrayError", "Unsupported array format pair (%s, integer)", A->Base.Type->Name); \
	compare_prefix(Op, C->Dimensions, C->Base.Address, Degree - 1, A->Dimensions, A->Base.Address, 0, NULL, (char *)&B); \
	return (ml_value_t *)C; \
}

ML_COMPARE_METHOD(0, 0, =);
ML_COMPARE_METHOD(1, 1, !=);
ML_COMPARE_METHOD(2, 3, <);
ML_COMPARE_METHOD(3, 2, >);
ML_COMPARE_METHOD(4, 5, <=);
ML_COMPARE_METHOD(5, 4, >=);

static ml_array_t *ml_array_of_create(ml_value_t *Value, int Degree, ml_array_format_t Format) {
	if (ml_is(Value, MLListT)) {
		int Size = ml_list_length(Value);
		if (!Size) return (ml_array_t *)ml_error("ValueError", "Empty dimension in array");
		ml_array_t *Array = ml_array_of_create(ml_list_get(Value, 1), Degree + 1, Format);
		if (Array->Base.Type == MLErrorT) return Array;
		Array->Dimensions[Degree].Size = Size;
		if (Degree < Array->Degree - 1) {
			Array->Dimensions[Degree].Stride = Array->Dimensions[Degree + 1].Size * Array->Dimensions[Degree + 1].Stride;
		}
		return Array;
	} else if (ml_is(Value, MLTupleT)) {
		int Size = ml_tuple_size(Value);
		if (!Size) return (ml_array_t *)ml_error("ValueError", "Empty dimension in array");
		ml_array_t *Array = ml_array_of_create(ml_tuple_get(Value, 1), Degree + 1, Format);
		if (Array->Base.Type == MLErrorT) return Array;
		Array->Dimensions[Degree].Size = Size;
		if (Degree < Array->Degree - 1) {
			Array->Dimensions[Degree].Stride = Array->Dimensions[Degree + 1].Size * Array->Dimensions[Degree + 1].Stride;
		}
		return Array;
	} else if (ml_is(Value, MLArrayT)) {
		ml_array_t *Nested = (ml_array_t *)Value;
		ml_array_t *Array = ml_array_new(Format, Degree + Nested->Degree);
		memcpy(Array->Dimensions + Degree, Nested->Dimensions, Nested->Degree * sizeof(ml_array_dimension_t));
		return Array;
	} else {
		ml_array_t *Array = ml_array_new(Format, Degree);
		if (Degree) {
			Array->Dimensions[Degree - 1].Size = 1;
			Array->Dimensions[Degree - 1].Stride = MLArraySizes[Format];
		}
		return Array;
	}
}

static ml_value_t *ml_array_of_fill(ml_array_format_t Format, ml_array_dimension_t *Dimension, char *Address, int Degree, ml_value_t *Value) {
	if (ml_is(Value, MLListT)) {
		if (!Degree) return ml_error("ValueError", "Inconsistent depth in array");
		if (ml_list_length(Value) != Dimension->Size) return ml_error("ValueError", "Inconsistent lengths in array");
		ML_LIST_FOREACH(Value, Iter) {
			ml_value_t *Error = ml_array_of_fill(Format, Dimension + 1, Address, Degree - 1, Iter->Value);
			if (Error) return Error;
			Address += Dimension->Stride;
		}
	} else if (ml_is(Value, MLTupleT)) {
		if (!Degree) return ml_error("ValueError", "Inconsistent depth in array");
		if (ml_tuple_size(Value) != Dimension->Size) return ml_error("ValueError", "Inconsistent lengths in array");
		ml_tuple_t *Tuple = (ml_tuple_t *)Value;
		for (int I = 0; I < Tuple->Size; ++I) {
			ml_value_t *Error = ml_array_of_fill(Format, Dimension + 1, Address, Degree - 1, Tuple->Values[I]);
			if (Error) return Error;
			Address += Dimension->Stride;
		}
	} else if (ml_is(Value, MLArrayT)) {
		ml_array_t *Source = (ml_array_t *)Value;
		if (Source->Degree != Degree) return ml_error("ArrayError", "Incompatible assignment (%d)", __LINE__);
		for (int I = 0; I < Degree; ++I) {
			if (Dimension[I].Size != Source->Dimensions[I].Size) return ml_error("ArrayError", "Incompatible assignment (%d)", __LINE__);
		}
		int Op = Format * MAX_FORMATS + Source->Format;
		update_array(Op, Dimension, Address, Degree, Source->Dimensions, Source->Base.Address);
	} else {
		if (Degree) return ml_error("ValueError", "Inconsistent depth in array");
		switch (Format) {
		case ML_ARRAY_FORMAT_NONE: break;
		case ML_ARRAY_FORMAT_I8: *(int8_t *)Address = ml_integer_value(Value); break;
		case ML_ARRAY_FORMAT_U8: *(uint8_t *)Address = ml_integer_value(Value); break;
		case ML_ARRAY_FORMAT_I16: *(int16_t *)Address = ml_integer_value(Value); break;
		case ML_ARRAY_FORMAT_U16: *(uint16_t *)Address = ml_integer_value(Value); break;
		case ML_ARRAY_FORMAT_I32: *(int32_t *)Address = ml_integer_value(Value); break;
		case ML_ARRAY_FORMAT_U32: *(uint32_t *)Address = ml_integer_value(Value); break;
		case ML_ARRAY_FORMAT_I64: *(int64_t *)Address = ml_integer_value(Value); break;
		case ML_ARRAY_FORMAT_U64: *(uint64_t *)Address = ml_integer_value(Value); break;
		case ML_ARRAY_FORMAT_F32: *(float *)Address = ml_real_value(Value); break;
		case ML_ARRAY_FORMAT_F64: *(double *)Address = ml_real_value(Value); break;
		case ML_ARRAY_FORMAT_ANY: *(ml_value_t **)Address = Value; break;
		}
	}
	return NULL;
}

static __attribute__ ((pure)) ml_array_format_t ml_array_of_type_guess(ml_value_t *Value, ml_array_format_t Format) {
	if (ml_is(Value, MLListT)) {
		ML_LIST_FOREACH(Value, Iter) {
			Format = ml_array_of_type_guess(Iter->Value, Format);
		}
	} else if (ml_is(Value, MLTupleT)) {
		ml_tuple_t *Tuple = (ml_tuple_t *)Value;
		for (int I = 0; I < Tuple->Size; ++I) {
			Format = ml_array_of_type_guess(Tuple->Values[I], Format);
		}
	} else if (ml_is(Value, MLArrayT)) {
		ml_array_t *Array = (ml_array_t *)Value;
		if (Format <= Array->Format) Format = Array->Format;
	} else if (ml_is(Value, MLRealT)) {
		if (Format < ML_ARRAY_FORMAT_F64) Format = ML_ARRAY_FORMAT_F64;
	} else if (ml_is(Value, MLIntegerT)) {
		if (Format < ML_ARRAY_FORMAT_I64) Format = ML_ARRAY_FORMAT_I64;
	} else {
		Format = ML_ARRAY_FORMAT_ANY;
	}
	return Format;
}

static ml_value_t *ml_array_of_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ml_value_t *Source = Args[0];
	ml_array_format_t Format = ML_ARRAY_FORMAT_NONE;
	if (Count == 2) {
		if (Args[0] == (ml_value_t *)MLArrayAnyT) {
			Format = ML_ARRAY_FORMAT_ANY;
		} else if (Args[0] == (ml_value_t *)MLArrayInt8T) {
			Format = ML_ARRAY_FORMAT_I8;
		} else if (Args[0] == (ml_value_t *)MLArrayUInt8T) {
			Format = ML_ARRAY_FORMAT_U8;
		} else if (Args[0] == (ml_value_t *)MLArrayInt16T) {
			Format = ML_ARRAY_FORMAT_I16;
		} else if (Args[0] == (ml_value_t *)MLArrayUInt16T) {
			Format = ML_ARRAY_FORMAT_U16;
		} else if (Args[0] == (ml_value_t *)MLArrayInt32T) {
			Format = ML_ARRAY_FORMAT_I32;
		} else if (Args[0] == (ml_value_t *)MLArrayUInt32T) {
			Format = ML_ARRAY_FORMAT_U32;
		} else if (Args[0] == (ml_value_t *)MLArrayInt64T) {
			Format = ML_ARRAY_FORMAT_I64;
		} else if (Args[0] == (ml_value_t *)MLArrayUInt64T) {
			Format = ML_ARRAY_FORMAT_U64;
		} else if (Args[0] == (ml_value_t *)MLArrayFloat32T) {
			Format = ML_ARRAY_FORMAT_F32;
		} else if (Args[0] == (ml_value_t *)MLArrayFloat64T) {
			Format = ML_ARRAY_FORMAT_F64;
		} else {
			return ml_error("TypeError", "Unknown type for array");
		}
		Source = Args[1];
	} else {
		Format = ml_array_of_type_guess(Args[0], ML_ARRAY_FORMAT_NONE);
	}
	ml_array_t *Array = ml_array_of_create(Source, 0, Format);
	if (Array->Base.Type == MLErrorT) return (ml_value_t *)Array;
	size_t Size = MLArraySizes[Array->Format];
	ml_array_dimension_t *Dimension = Array->Dimensions + Array->Degree;
	for (int I = Array->Degree; --I >= 0;) {
		--Dimension;
		Dimension->Stride = Size;
		Size *= Dimension->Size;
	}
	char *Address = Array->Base.Address = GC_MALLOC_ATOMIC(Size);
	ml_value_t *Error = ml_array_of_fill(Array->Format, Array->Dimensions, Address, Array->Degree, Source);
	return Error ?: (ml_value_t *)Array;
}

ML_METHOD("copy", MLArrayT) {
//<Array
//>array
// Return a new array with the same values of :mini:`Array` but not sharing the underlying data.
	ml_array_t *Source = (ml_array_t *)Args[0];
	ml_array_t *Target = ml_array_new(Source->Format, Source->Degree);
	array_copy(Target, Source);
	return (ml_value_t *)Target;
}

typedef struct {
	ml_state_t Base;
	union {
		void *Values;
		int8_t *I8;
		uint8_t *U8;
		int16_t *I16;
		uint16_t *U16;
		int32_t *I32;
		uint32_t *U32;
		int64_t *I64;
		uint64_t *U64;
		float *F32;
		double *F64;
		ml_value_t **Any;
	};
	ml_value_t *Function;
	ml_value_t *Array;
	ml_value_t *Args[1];
	int Remaining;
} ml_array_apply_state_t;

#define ARRAY_APPLY(NAME, CTYPE, TO_VAL, TO_NUM) \
static void ml_array_apply_ ## CTYPE(ml_array_apply_state_t *State, ml_value_t *Value) { \
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value); \
	*State->NAME++ = TO_NUM(Value); \
	if (--State->Remaining) { \
		State->Args[0] = TO_VAL(*State->NAME); \
		return ml_call(State, State->Function, 1, State->Args); \
	} else { \
		ML_CONTINUE(State->Base.Caller, State->Array); \
	} \
}

ARRAY_APPLY(I8, int8_t, ml_integer, ml_integer_value);
ARRAY_APPLY(U8, uint8_t, ml_integer, ml_integer_value);
ARRAY_APPLY(I16, int16_t, ml_integer, ml_integer_value);
ARRAY_APPLY(U16, uint16_t, ml_integer, ml_integer_value);
ARRAY_APPLY(I32, int32_t, ml_integer, ml_integer_value);
ARRAY_APPLY(U32, uint32_t, ml_integer, ml_integer_value);
ARRAY_APPLY(I64, int64_t, ml_integer, ml_integer_value);
ARRAY_APPLY(U64, uint64_t, ml_integer, ml_integer_value);
ARRAY_APPLY(F32, float, ml_real, ml_real_value);
ARRAY_APPLY(F64, double, ml_real, ml_real_value);
ARRAY_APPLY(Any, value, , );

ML_METHODX("copy", MLArrayT, MLFunctionT) {
//<Array
//<Function
//>array
// Return a new array with the results of applying :mini:`Function` to each value of :mini:`Array`.
	ml_array_t *A = (ml_array_t *)Args[0];
	int Degree = A->Degree;
	ml_array_t *C = ml_array_new(A->Format, Degree);
	int Remaining = array_copy(C, A) / MLArraySizes[C->Format];
	if (Remaining == 0) ML_RETURN(C);
	ml_array_apply_state_t *State = new(ml_array_apply_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	ml_value_t *Function = State->Function = Args[1];
	State->Remaining = Remaining;
	State->Values = C->Base.Address;
	State->Array = (ml_value_t *)C;
	switch (C->Format) {
	case ML_ARRAY_FORMAT_I8: {
		State->Base.run = (void *)ml_array_apply_int8_t;
		State->Args[0] = ml_integer(*State->I8);
		break;
	}
	case ML_ARRAY_FORMAT_U8: {
		State->Base.run = (void *)ml_array_apply_uint8_t;
		State->Args[0] = ml_integer(*State->U8);
		break;
	}
	case ML_ARRAY_FORMAT_I16: {
		State->Base.run = (void *)ml_array_apply_int16_t;
		State->Args[0] = ml_integer(*State->I16);
		break;
	}
	case ML_ARRAY_FORMAT_U16: {
		State->Base.run = (void *)ml_array_apply_uint16_t;
		State->Args[0] = ml_integer(*State->U16);
		break;
	}
	case ML_ARRAY_FORMAT_I32: {
		State->Base.run = (void *)ml_array_apply_int32_t;
		State->Args[0] = ml_integer(*State->I32);
		break;
	}
	case ML_ARRAY_FORMAT_U32: {
		State->Base.run = (void *)ml_array_apply_uint32_t;
		State->Args[0] = ml_integer(*State->U32);
		break;
	}
	case ML_ARRAY_FORMAT_I64: {
		State->Base.run = (void *)ml_array_apply_int64_t;
		State->Args[0] = ml_integer(*State->I64);
		break;
	}
	case ML_ARRAY_FORMAT_U64: {
		State->Base.run = (void *)ml_array_apply_uint64_t;
		State->Args[0] = ml_integer(*State->U64);
		break;
	}
	case ML_ARRAY_FORMAT_F32: {
		State->Base.run = (void *)ml_array_apply_float;
		State->Args[0] = ml_real(*State->F32);
		break;
	}
	case ML_ARRAY_FORMAT_F64: {
		State->Base.run = (void *)ml_array_apply_double;
		State->Args[0] = ml_real(*State->F64);
		break;
	}
	case ML_ARRAY_FORMAT_ANY: {
		State->Base.run = (void *)ml_array_apply_value;
		State->Args[0] = *State->Any;
		break;
	}
	default: ML_ERROR("ArrayError", "Unsupported format");
	}
	return ml_call(State, Function, 1, State->Args);
}

typedef struct {
	ml_state_t Base;
	char *Address;
	ml_value_t *Function;
	ml_array_t *Array;
	ml_value_t *Args[1];
	int Degree;
	int Indices[];
} ml_array_update_state_t;

#define ARRAY_UPDATE(CTYPE, TO_VAL, TO_NUM) \
static void ml_array_update_ ## CTYPE(ml_array_update_state_t *State, ml_value_t *Value) { \
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value); \
	*(CTYPE *)State->Address = TO_NUM(Value); \
	int *Indices = State->Indices; \
	for (int I = State->Degree; --I >= 0;) { \
		ml_array_dimension_t *Dimension = State->Array->Dimensions + I; \
		int Index = Indices[I]; \
		if (Index + 1 < Dimension->Size) { \
			if (Dimension->Indices) { \
				State->Address += (Dimension->Indices[Index + 1] - Dimension->Indices[Index]) * Dimension->Stride; \
			} else { \
				State->Address += Dimension->Stride; \
			} \
			State->Indices[I] = Index + 1; \
			State->Args[0] = TO_VAL(*(CTYPE *)State->Address); \
			return ml_call(State, State->Function, 1, State->Args); \
		} else { \
			if (Dimension->Indices) { \
				State->Address -= (Dimension->Indices[Index] - Dimension->Indices[0]) * Dimension->Stride; \
			} else { \
				State->Address -= Index * Dimension->Stride; \
			} \
			State->Indices[I] = 0; \
		} \
	} \
	ML_CONTINUE(State->Base.Caller, State->Array); \
}

ARRAY_UPDATE(int8_t, ml_integer, ml_integer_value);
ARRAY_UPDATE(uint8_t, ml_integer, ml_integer_value);
ARRAY_UPDATE(int16_t, ml_integer, ml_integer_value);
ARRAY_UPDATE(uint16_t, ml_integer, ml_integer_value);
ARRAY_UPDATE(int32_t, ml_integer, ml_integer_value);
ARRAY_UPDATE(uint32_t, ml_integer, ml_integer_value);
ARRAY_UPDATE(int64_t, ml_integer, ml_integer_value);
ARRAY_UPDATE(uint64_t, ml_integer, ml_integer_value);
ARRAY_UPDATE(float, ml_real, ml_real_value);
ARRAY_UPDATE(double, ml_real, ml_real_value);
ARRAY_UPDATE(value, , );

ML_METHODX("update", MLArrayT, MLFunctionT) {
//<Array
//<Function
//>array
// Update the values in :mini:`Array` in place by applying :mini:`Function` to each value.
	ml_array_t *A = (ml_array_t *)Args[0];
	int Degree = A->Degree;
	ml_array_update_state_t *State = xnew(ml_array_update_state_t, Degree, int);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	ml_value_t *Function = State->Function = Args[1];
	State->Array = A;
	State->Degree = Degree;
	char *Address = A->Base.Address;
	ml_array_dimension_t *Dimension = A->Dimensions;
	for (int I = 0; I < Degree; ++I, ++Dimension) {
		if (Dimension->Indices) {
			Address += Dimension->Indices[0] * Dimension->Stride;
		}
	}
	State->Address = Address;
	switch (A->Format) {
	case ML_ARRAY_FORMAT_I8: {
		State->Base.run = (void *)ml_array_update_int8_t;
		State->Args[0] = ml_integer(*(int8_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_U8: {
		State->Base.run = (void *)ml_array_update_uint8_t;
		State->Args[0] = ml_integer(*(uint8_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_I16: {
		State->Base.run = (void *)ml_array_update_int16_t;
		State->Args[0] = ml_integer(*(int16_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_U16: {
		State->Base.run = (void *)ml_array_update_uint16_t;
		State->Args[0] = ml_integer(*(uint16_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_I32: {
		State->Base.run = (void *)ml_array_update_int32_t;
		State->Args[0] = ml_integer(*(int32_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_U32: {
		State->Base.run = (void *)ml_array_update_uint32_t;
		State->Args[0] = ml_integer(*(uint32_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_I64: {
		State->Base.run = (void *)ml_array_update_int64_t;
		State->Args[0] = ml_integer(*(int64_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_U64: {
		State->Base.run = (void *)ml_array_update_uint64_t;
		State->Args[0] = ml_integer(*(uint64_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_F32: {
		State->Base.run = (void *)ml_array_update_float;
		State->Args[0] = ml_real(*(float *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_F64: {
		State->Base.run = (void *)ml_array_update_double;
		State->Args[0] = ml_real(*(double *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_ANY: {
		State->Base.run = (void *)ml_array_update_value;
		State->Args[0] = *(ml_value_t **)Address;
		break;
	}
	default: ML_ERROR("ArrayError", "Unsupported format");
	}
	return ml_call(State, Function, 1, State->Args);
}

#define ML_ARRAY_DOT(CTYPE) \
static CTYPE ml_array_dot_ ## CTYPE( \
	void *DataA, ml_array_dimension_t *DimA, int FormatA, \
	void *DataB, ml_array_dimension_t *DimB, int FormatB \
) { \
	CTYPE Sum = 0; \
	int Size = DimA->Size; \
	int StrideA = DimA->Stride; \
	int StrideB = DimB->Stride; \
	if (DimA->Indices) { \
		int *IndicesA = DimA->Indices; \
		if (DimB->Indices) { \
			int *IndicesB = DimB->Indices; \
			for (int I = 0; I < Size; ++I) { \
				CTYPE ValueA = ml_array_get0_ ## CTYPE(DataA + IndicesA[I] * StrideA, FormatA); \
				CTYPE ValueB = ml_array_get0_ ## CTYPE(DataB + IndicesB[I] * StrideB, FormatB); \
				Sum += ValueA * ValueB; \
			} \
		} else { \
			for (int I = 0; I < Size; ++I) { \
				CTYPE ValueA = ml_array_get0_ ## CTYPE(DataA + IndicesA[I] * StrideA, FormatA); \
				CTYPE ValueB = ml_array_get0_ ## CTYPE(DataB, FormatB); \
				Sum += ValueA * ValueB; \
				DataB += StrideB; \
			} \
		} \
	} else { \
		if (DimB->Indices) { \
			int *IndicesB = DimB->Indices; \
			for (int I = 0; I < Size; ++I) { \
				CTYPE ValueA = ml_array_get0_ ## CTYPE(DataA, FormatA); \
				CTYPE ValueB = ml_array_get0_ ## CTYPE(DataB + IndicesB[I] * StrideB, FormatB); \
				Sum += ValueA * ValueB; \
				DataA += StrideA; \
			} \
		} else { \
			for (int I = 0; I < Size; ++I) { \
				CTYPE ValueA = ml_array_get0_ ## CTYPE(DataA, FormatA); \
				CTYPE ValueB = ml_array_get0_ ## CTYPE(DataB, FormatB); \
				Sum += ValueA * ValueB; \
				DataA += StrideA; \
				DataB += StrideB; \
			} \
		} \
	} \
	return Sum; \
}

ML_ARRAY_DOT(int8_t);
ML_ARRAY_DOT(uint8_t);
ML_ARRAY_DOT(int16_t);
ML_ARRAY_DOT(uint16_t);
ML_ARRAY_DOT(int32_t);
ML_ARRAY_DOT(uint32_t);
ML_ARRAY_DOT(int64_t);
ML_ARRAY_DOT(uint64_t);
ML_ARRAY_DOT(float);
ML_ARRAY_DOT(double);

static void ml_array_dot_fill(
	void *DataA, ml_array_dimension_t *DimA, int FormatA, int DegreeA,
	void *DataB, ml_array_dimension_t *DimB, int FormatB, int DegreeB,
	void *DataC, ml_array_dimension_t *DimC, int FormatC, int DegreeC
) {
	if (DegreeA > 1) {
		int StrideA = DimA->Stride;
		int StrideC = DimC->Stride;
		if (DimA->Indices) {
			int *Indices = DimA->Indices;
			for (int I = 0; I < DimA->Size; ++I) {
				ml_array_dot_fill(
					DataA + (Indices[I]) * StrideA, DimA + 1, FormatA, DegreeA - 1,
					DataB, DimB, FormatB, DegreeB,
					DataC, DimC + 1, FormatC, DegreeC - 1
				);
				DataC += StrideC;
			}
		} else {
			for (int I = DimA->Size; --I >= 0;) {
				ml_array_dot_fill(
					DataA, DimA + 1, FormatA, DegreeA - 1,
					DataB, DimB, FormatB, DegreeB,
					DataC, DimC + 1, FormatC, DegreeC - 1
				);
				DataA += StrideA;
				DataC += StrideC;
			}
		}
	} else if (DegreeB > 1) {
		int StrideB = DimB->Stride;
		int StrideC = (DimC + DegreeC - 1)->Stride;
		if (DimB->Indices) {
			int *Indices = DimB->Indices;
			for (int I = 0; I < DimB->Size; ++I) {
				ml_array_dot_fill(
					DataA, DimA, FormatA, DegreeA,
					DataB + (Indices[I]) * StrideB, DimB - 1, FormatB, DegreeB - 1,
					DataC, DimC, FormatC, DegreeC - 1
				);
				DataC += StrideC;
			}
		} else {
			for (int I = DimB->Size; --I >= 0;) {
				ml_array_dot_fill(
					DataA, DimA, FormatA, DegreeA,
					DataB, DimB - 1, FormatB, DegreeB - 1,
					DataC, DimC, FormatC, DegreeC - 1
				);
				DataB += StrideB;
				DataC += StrideC;
			}
		}
	} else {
		switch (FormatC) {
		case ML_ARRAY_FORMAT_I8:
			*(int8_t *)DataC = ml_array_dot_int8_t(DataA, DimA, FormatA, DataB, DimB + (DegreeB - 1), FormatB);
			break;
		case ML_ARRAY_FORMAT_U8:
			*(uint8_t *)DataC = ml_array_dot_uint8_t(DataA, DimA, FormatA, DataB, DimB + (DegreeB - 1), FormatB);
			break;
		case ML_ARRAY_FORMAT_I16:
			*(int16_t *)DataC = ml_array_dot_int16_t(DataA, DimA, FormatA, DataB, DimB + (DegreeB - 1), FormatB);
			break;
		case ML_ARRAY_FORMAT_U16:
			*(uint16_t *)DataC = ml_array_dot_uint16_t(DataA, DimA, FormatA, DataB, DimB + (DegreeB - 1), FormatB);
			break;
		case ML_ARRAY_FORMAT_I32:
			*(int32_t *)DataC = ml_array_dot_int32_t(DataA, DimA, FormatA, DataB, DimB + (DegreeB - 1), FormatB);
			break;
		case ML_ARRAY_FORMAT_U32:
			*(uint32_t *)DataC = ml_array_dot_uint32_t(DataA, DimA, FormatA, DataB, DimB + (DegreeB - 1), FormatB);
			break;
		case ML_ARRAY_FORMAT_I64:
			*(int64_t *)DataC = ml_array_dot_int64_t(DataA, DimA, FormatA, DataB, DimB + (DegreeB - 1), FormatB);
			break;
		case ML_ARRAY_FORMAT_U64:
			*(uint64_t *)DataC = ml_array_dot_uint64_t(DataA, DimA, FormatA, DataB, DimB + (DegreeB - 1), FormatB);
			break;
		case ML_ARRAY_FORMAT_F32:
			*(float *)DataC = ml_array_dot_float(DataA, DimA, FormatA, DataB, DimB + (DegreeB - 1), FormatB);
			break;
		case ML_ARRAY_FORMAT_F64:
			*(double *)DataC = ml_array_dot_double(DataA, DimA, FormatA, DataB, DimB + (DegreeB - 1), FormatB);
			break;
		}
	}
}

ML_METHOD(".", MLArrayT, MLArrayT) {
//<A
//<B
//>array
// Returns the inner product of :mini:`A` and :mini:`B`.
	ml_array_t *A = (ml_array_t *)Args[0];
	ml_array_t *B = (ml_array_t *)Args[1];
	if (!A->Degree || !B->Degree) return ml_error("ShapeError", "Empty array");
	int Size = A->Dimensions[A->Degree - 1].Size;
	if (Size != B->Dimensions[0].Size) return ml_error("ShapeError", "Incompatible arrays");
	if (A->Degree == 1 && B->Degree == 1) {
		double Sum = 0;
		for (int I = 0; I < Size; ++I) {
			Sum += ml_array_get_double(A, I) * ml_array_get_double(B, I);
		}
		return ml_real(Sum);
	}
	int Degree = A->Degree + B->Degree - 2;
	ml_array_t *C = ml_array_new(MAX(A->Format, B->Format), Degree);
	int DataSize = MLArraySizes[C->Format];
	int Base = A->Degree - 2;
	for (int I = B->Degree; --I >= 1;) {
		C->Dimensions[Base + I].Stride = DataSize;
		int Size = C->Dimensions[Base + I].Size = B->Dimensions[I].Size;
		DataSize *= Size;
	}
	for (int I = A->Degree - 1; --I >= 0;) {
		C->Dimensions[I].Stride = DataSize;
		int Size = C->Dimensions[I].Size = A->Dimensions[I].Size;
		DataSize *= Size;
	}
	C->Base.Address = GC_MALLOC_ATOMIC(DataSize);
	C->Base.Size = DataSize;
	ml_array_dot_fill(
		A->Base.Address, A->Dimensions, A->Format, A->Degree,
		B->Base.Address, B->Dimensions + (B->Degree - 1), B->Format, B->Degree,
		C->Base.Address, C->Dimensions, C->Format, C->Degree
	);
	return (ml_value_t *)C;
}

void ml_array_init(stringmap_t *Globals) {
	MLArrayAnyT->Constructor = ml_cfunctionx((void *)ML_ARRAY_FORMAT_ANY, ml_array_typed_new_fnx);
	MLArrayInt8T->Constructor = ml_cfunctionx((void *)ML_ARRAY_FORMAT_I8, ml_array_typed_new_fnx);
	MLArrayUInt8T->Constructor = ml_cfunctionx((void *)ML_ARRAY_FORMAT_U8, ml_array_typed_new_fnx);
	MLArrayInt16T->Constructor = ml_cfunctionx((void *)ML_ARRAY_FORMAT_I16, ml_array_typed_new_fnx);
	MLArrayUInt16T->Constructor = ml_cfunctionx((void *)ML_ARRAY_FORMAT_U16, ml_array_typed_new_fnx);
	MLArrayInt32T->Constructor = ml_cfunctionx((void *)ML_ARRAY_FORMAT_I32, ml_array_typed_new_fnx);
	MLArrayUInt32T->Constructor = ml_cfunctionx((void *)ML_ARRAY_FORMAT_U32, ml_array_typed_new_fnx);
	MLArrayInt64T->Constructor = ml_cfunctionx((void *)ML_ARRAY_FORMAT_I64, ml_array_typed_new_fnx);
	MLArrayUInt64T->Constructor = ml_cfunctionx((void *)ML_ARRAY_FORMAT_U64, ml_array_typed_new_fnx);
	MLArrayFloat32T->Constructor = ml_cfunctionx((void *)ML_ARRAY_FORMAT_F32, ml_array_typed_new_fnx);
	MLArrayFloat64T->Constructor = ml_cfunctionx((void *)ML_ARRAY_FORMAT_F64, ml_array_typed_new_fnx);
	MLArrayT->Constructor = ml_cfunction(NULL, ml_array_of_fn);
#include "ml_array_init.c"
	ml_method_by_name("set", 0 + (char *)0, update_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("add", 1 + (char *)0, update_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("sub", 2 + (char *)0, update_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("mul", 3 + (char *)0, update_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("div", 4 + (char *)0, update_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("+", 1 + (char *)0, array_infix_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("-", 2 + (char *)0, array_infix_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("*", 3 + (char *)0, array_infix_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("/", 4 + (char *)0, array_infix_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("=", 0 + (char *)0, compare_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("!=", 1 + (char *)0, compare_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("<", 2 + (char *)0, compare_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name(">", 3 + (char *)0, compare_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("<=", 4 + (char *)0, compare_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name(">=", 5 + (char *)0, compare_array_fn, MLArrayT, MLArrayT, NULL);
	stringmap_insert(MLArrayT->Exports, "new", ml_cfunctionx(NULL, ml_array_new_fnx));
	stringmap_insert(MLArrayT->Exports, "wrap", ml_cfunction(NULL, ml_array_wrap_fn));
	stringmap_insert(MLArrayT->Exports, "any", MLArrayAnyT);
	stringmap_insert(MLArrayT->Exports, "int8", MLArrayInt8T);
	stringmap_insert(MLArrayT->Exports, "uint8", MLArrayUInt8T);
	stringmap_insert(MLArrayT->Exports, "int16", MLArrayInt16T);
	stringmap_insert(MLArrayT->Exports, "uint16", MLArrayUInt16T);
	stringmap_insert(MLArrayT->Exports, "int32", MLArrayInt32T);
	stringmap_insert(MLArrayT->Exports, "uint32", MLArrayUInt32T);
	stringmap_insert(MLArrayT->Exports, "int64", MLArrayInt64T);
	stringmap_insert(MLArrayT->Exports, "uint64", MLArrayUInt64T);
	stringmap_insert(MLArrayT->Exports, "float32", MLArrayFloat32T);
	stringmap_insert(MLArrayT->Exports, "float64", MLArrayFloat64T);
	if (Globals) {
		stringmap_insert(Globals, "array", MLArrayT);
	}
}
