#include "ml_number.h"
#include "minilang.h"
#include "ml_macros.h"
#include <math.h>
#include <float.h>
#include <limits.h>
#include <inttypes.h>

#ifdef ML_COMPLEX
#include <complex.h>
#undef I
#endif

#undef ML_CATEGORY
#define ML_CATEGORY "number"

typedef struct {
	ml_state_t Base;
	ml_value_t *Function;
	int Index, Count;
	ml_value_t *Args[];
} ml_infix_state_t;

static void ml_infix_run(ml_infix_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	int Index = State->Index;
	if (Index + 1 == State->Count) ML_RETURN(Value);
	State->Args[Index] = Value;
	State->Index = Index + 1;
	return ml_call(State, State->Function, 2, State->Args + Index);
}

static void ml_infix_many_fn(ml_state_t *Caller, void *Infix, int Count, ml_value_t **Args) {
	ml_infix_state_t *State = xnew(ml_infix_state_t, Count, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_infix_run;
	State->Function = (ml_value_t *)Infix;
	State->Count = Count;
	State->Index = 1;
	for (int I = 0; I < Count; ++I) State->Args[I] = Args[I];
	return ml_call(State, Infix, 2, State->Args);
}

static void ml_infix_many(const char *Name) {
	ml_value_t *Method = ml_method(Name);
	ml_method_define(Method, ml_cfunctionx(Method, ml_infix_many_fn), 1, MLAnyT, MLAnyT, MLAnyT, NULL);
}

ML_TYPE(MLNumberT, (), "number");
// Base type for numbers.

#ifdef ML_COMPLEX

static long ml_complex_hash(ml_complex_t *Complex, ml_hash_chain_t *Chain) {
	return (long)creal(Complex->Value);
}

ML_TYPE(MLComplexT, (MLNumberT), "complex",
	.hash = (void *)ml_complex_hash
);

ml_value_t *ml_complex(complex double Value) {
	ml_complex_t *Complex = new(ml_complex_t);
	Complex->Type = MLComplexT;
	Complex->Value = Value;
	return (ml_value_t *)Complex;
}

ML_METHOD(MLComplexT, MLRealT) {
	return ml_complex(ml_real_value(Args[0]));
}

ML_METHOD(MLRealT, MLComplexT) {
	return ml_real(creal(ml_complex_value(Args[0])));
}

extern complex double ml_complex_value_fast(const ml_value_t *Value);

complex double ml_complex_value(const ml_value_t *Value) {
#ifdef ML_NANBOXING
	int Tag = ml_tag(Value);
	if (Tag == 1) return (int32_t)(intptr_t)Value;
	if (Tag >= 7) return ml_double_value_fast(Value);
	if (Tag == 0) {
		if (Value->Type == MLInt64T) {
			return ((ml_int64_t *)Value)->Value;
		} else if (Value->Type == MLComplexT) {
			return ((ml_complex_t *)Value)->Value;
		}
	}
	return 0;
#else
	if (Value->Type == MLIntegerT) {
		return ((ml_integer_t *)Value)->Value;
	} else if (Value->Type == MLDoubleT) {
		return ((ml_double_t *)Value)->Value;
	} else if (ml_is(Value, MLIntegerT)) {
		return ((ml_integer_t *)Value)->Value;
	} else if (ml_is(Value, MLDoubleT)) {
		return ((ml_double_t *)Value)->Value;
	} else if (ml_is(Value, MLComplexT)) {
		return ((ml_complex_t *)Value)->Value;
	} else {
		return 0;
	}
#endif
}

#define ml_arith_method_complex(NAME, SYMBOL) \
ML_METHOD(NAME, MLComplexT) { \
/*<A
//>complex
// Returns :mini:`SYMBOLA`.
*/\
	complex double ComplexA = ml_complex_value_fast(Args[0]); \
	complex double ComplexB = SYMBOL(ComplexA); \
	if (fabs(cimag(ComplexB)) <= DBL_EPSILON) { \
		return ml_real(creal(ComplexB)); \
	} else { \
		return ml_complex(ComplexB); \
	} \
}

#define ml_arith_method_complex_complex(NAME, SYMBOL) \
ML_METHOD(NAME, MLComplexT, MLComplexT) { \
/*<A
//<B
//>real
// complex :mini:`A SYMBOL B`.
*/\
	complex double ComplexA = ml_complex_value_fast(Args[0]); \
	complex double ComplexB = ml_complex_value_fast(Args[1]); \
	complex double ComplexC = ComplexA SYMBOL ComplexB; \
	if (fabs(cimag(ComplexC)) <= DBL_EPSILON) { \
		return ml_real(creal(ComplexC)); \
	} else { \
		return ml_complex(ComplexC); \
	} \
}

#define ml_arith_method_complex_integer(NAME, SYMBOL) \
ML_METHOD(NAME, MLComplexT, MLIntegerT) { \
/*<A
//<B
//>complex
// Returns :mini:`A SYMBOL B`.
*/\
	complex double ComplexA = ml_complex_value_fast(Args[0]); \
	int64_t IntegerB = ml_integer_value_fast(Args[1]); \
	complex double ComplexC = ComplexA SYMBOL IntegerB; \
	if (fabs(cimag(ComplexC)) <= DBL_EPSILON) { \
		return ml_real(creal(ComplexC)); \
	} else { \
		return ml_complex(ComplexC); \
	} \
}

#define ml_arith_method_integer_complex(NAME, SYMBOL) \
ML_METHOD(NAME, MLIntegerT, MLComplexT) { \
/*<A
//<B
//>complex
// Returns :mini:`A SYMBOL B`.
*/\
	int64_t IntegerA = ml_integer_value_fast(Args[0]); \
	complex double ComplexB = ml_complex_value_fast(Args[1]); \
	complex double ComplexC = IntegerA SYMBOL ComplexB; \
	if (fabs(cimag(ComplexC)) <= DBL_EPSILON) { \
		return ml_real(creal(ComplexC)); \
	} else { \
		return ml_complex(ComplexC); \
	} \
}

#define ml_arith_method_complex_real(NAME, SYMBOL) \
ML_METHOD(NAME, MLComplexT, MLDoubleT) { \
/*<A
//<B
//>complex
// Returns :mini:`A SYMBOL B`.
*/\
	complex double ComplexA = ml_complex_value_fast(Args[0]); \
	double RealB = ml_double_value_fast(Args[1]); \
	complex double ComplexC = ComplexA SYMBOL RealB; \
	if (fabs(cimag(ComplexC)) <= DBL_EPSILON) { \
		return ml_real(creal(ComplexC)); \
	} else { \
		return ml_complex(ComplexC); \
	} \
}

#define ml_arith_method_real_complex(NAME, SYMBOL) \
ML_METHOD(NAME, MLDoubleT, MLComplexT) { \
/*<A
//<B
//>complex
// Returns :mini:`A SYMBOL B`.
*/\
	double RealA = ml_double_value_fast(Args[0]); \
	complex double ComplexB = ml_complex_value_fast(Args[1]); \
	complex double ComplexC = RealA SYMBOL ComplexB; \
	if (fabs(cimag(ComplexC)) <= DBL_EPSILON) { \
		return ml_real(creal(ComplexC)); \
	} else { \
		return ml_complex(ComplexC); \
	} \
}

ML_METHOD("r", MLComplexT) {
//<Z
//>real
// Returns the real component of :mini:`Z`.
	return ml_real(creal(ml_complex_value_fast(Args[0])));
}

ML_METHOD("i", MLComplexT) {
//<Z
//>real
// Returns the imaginary component of :mini:`Z`.
	return ml_real(cimag(ml_complex_value_fast(Args[0])));
}

#endif

#ifdef ML_COMPLEX
ML_TYPE(MLRealT, (MLComplexT), "real");
#else
ML_TYPE(MLRealT, (MLNumberT), "real");
//!internal
#endif
// Base type for real numbers.

/*
ML_DEF(Inf);
//!number
//@real::Inf
//>real
// Positive infinity.

ML_DEF(NaN);
//!number
//@real::NaN
//>real
// Not a number.
*/

#ifdef ML_NANBOXING

static long ml_int32_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	return (int32_t)(intptr_t)Value;
}

static void ml_int32_call(ml_state_t *Caller, ml_value_t *Value, int Count, ml_value_t **Args) {
	long Index = (int32_t)(intptr_t)Value;
	if (Index <= 0) Index += Count + 1;
	if (Index <= 0) ML_RETURN(MLNil);
	if (Index > Count) ML_RETURN(MLNil);
	ML_RETURN(Args[Index - 1]);
}

ML_TYPE(MLIntegerT, (MLRealT, MLFunctionT), "integer");
//!internal

ML_TYPE(MLInt32T, (MLIntegerT), "int32",
//!internal
	.hash = (void *)ml_int32_hash,
	.call = (void *)ml_int32_call,
	.NoInherit = 1
);

static long ml_int64_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	return ((ml_int64_t *)Value)->Value;
}

ML_TYPE(MLInt64T, (MLIntegerT), "int64",
//!internal
	.hash = (void *)ml_int64_hash,
	.NoInherit = 1
);

ml_value_t *ml_int64(int64_t Integer) {
	ml_int64_t *Value = new(ml_int64_t);
	Value->Type = MLInt64T;
	Value->Value = Integer;
	return (ml_value_t *)Value;
}

int64_t ml_integer_value(const ml_value_t *Value) {
	int Tag = ml_tag(Value);
	if (Tag == 1) return (int32_t)(intptr_t)Value;
	if (Tag >= 7) return ml_double_value_fast(Value);
	if (Tag == 0) {
		if (Value->Type == MLInt64T) {
			return ((ml_int64_t *)Value)->Value;
		}
	}
	return 0;
}

ML_METHOD(MLRealT, MLInt32T) {
//!internal
	return ml_real((int32_t)(intptr_t)Args[0]);
}

ML_METHOD(MLRealT, MLInt64T) {
//!internal
	return ml_real(((ml_int64_t *)Args[0])->Value);
}

#else

static long ml_integer_hash(ml_integer_t *Integer, ml_hash_chain_t *Chain) {
	return Integer->Value;
}

static void ml_integer_call(ml_state_t *Caller, ml_integer_t *Integer, int Count, ml_value_t **Args) {
	long Index = Integer->Value;
	if (Index <= 0) Index += Count + 1;
	if (Index <= 0) ML_RETURN(MLNil);
	if (Index > Count) ML_RETURN(MLNil);
	ML_RETURN(Args[Index - 1]);
}

ML_TYPE(MLIntegerT, (MLRealT, MLFunctionT), "integer",
	.hash = (void *)ml_integer_hash,
	.call = (void *)ml_integer_call
);

ml_value_t *ml_integer(int64_t Value) {
	ml_integer_t *Integer = new(ml_integer_t);
	Integer->Type = MLIntegerT;
	Integer->Value = Value;
	return (ml_value_t *)Integer;
}

extern int64_t ml_integer_value_fast(const ml_value_t *Value);

int64_t ml_integer_value(const ml_value_t *Value) {
	if (Value->Type == MLIntegerT) {
		return ((ml_integer_t *)Value)->Value;
	} else if (Value->Type == MLDoubleT) {
		return ((ml_double_t *)Value)->Value;
	} else if (ml_is(Value, MLIntegerT)) {
		return ((ml_integer_t *)Value)->Value;
	} else if (ml_is(Value, MLDoubleT)) {
		return ((ml_double_t *)Value)->Value;
	} else {
		return 0;
	}
}

ML_METHOD(MLRealT, MLIntegerT) {
	return ml_real(ml_integer_value_fast(Args[0]));
}

#endif

static long ml_double_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	return (long)ml_double_value_fast(Value);
}

ML_TYPE(MLDoubleT, (MLRealT), "double",
	.hash = (void *)ml_double_hash,
	.NoInherit = 1
);

#ifdef ML_NANBOXING

double ml_real_value(const ml_value_t *Value) {
	int Tag = ml_tag(Value);
	if (Tag == 1) return (int32_t)(intptr_t)Value;
	if (Tag >= 7) return ml_double_value_fast(Value);
	if (Tag == 0) {
		if (Value->Type == MLInt64T) {
			return ((ml_int64_t *)Value)->Value;
		}
	}
	return 0;
}

ML_METHOD(MLDoubleT, MLInt32T) {
//!internal
	return ml_real((int32_t)(intptr_t)Args[0]);
}

ML_METHOD(MLDoubleT, MLInt64T) {
//!internal
	return ml_real(((ml_int64_t *)Args[0])->Value);
}

#else

ml_value_t *ml_real(double Value) {
	ml_double_t *Real = new(ml_double_t);
	Real->Type = MLDoubleT;
	Real->Value = Value;
	return (ml_value_t *)Real;
}

double ml_real_value(const ml_value_t *Value) {
	if (Value->Type == MLIntegerT) {
		return ((ml_integer_t *)Value)->Value;
	} else if (Value->Type == MLDoubleT) {
		return ((ml_double_t *)Value)->Value;
	} else if (ml_is(Value, MLIntegerT)) {
		return ((ml_integer_t *)Value)->Value;
	} else if (ml_is(Value, MLDoubleT)) {
		return ((ml_double_t *)Value)->Value;
	} else {
		return 0;
	}
}

ML_METHOD(MLDoubleT, MLIntegerT) {
//!internal
	return ml_real(ml_integer_value_fast(Args[0]));
}

#endif

ML_METHOD(MLIntegerT, MLDoubleT) {
//<Real
//>integer
// Converts :mini:`Real` to an integer (using default rounding).
	return ml_integer(ml_double_value_fast(Args[0]));
}

#define ml_arith_method_integer(NAME, SYMBOL) \
ML_METHOD(NAME, MLIntegerT) { \
/*<A
//>integer
// Returns :mini:`SYMBOLA`.
*/\
	int64_t IntegerA = ml_integer_value_fast(Args[0]); \
	return ml_integer(SYMBOL(IntegerA)); \
}

#define ml_arith_method_integer_integer(NAME, SYMBOL) \
ML_METHOD(NAME, MLIntegerT, MLIntegerT) { \
/*<A
//<B
//>integer
// Returns :mini:`A SYMBOL B`.
*/\
	int64_t IntegerA = ml_integer_value_fast(Args[0]); \
	int64_t IntegerB = ml_integer_value_fast(Args[1]); \
	return ml_integer(IntegerA SYMBOL IntegerB); \
}

#define ml_arith_method_real(NAME, SYMBOL) \
ML_METHOD(NAME, MLDoubleT) { \
/*<A
//>real
// Returns :mini:`SYMBOLA`.
*/\
	double RealA = ml_double_value_fast(Args[0]); \
	return ml_real(SYMBOL(RealA)); \
}

#define ml_arith_method_real_real(NAME, SYMBOL) \
ML_METHOD(NAME, MLDoubleT, MLDoubleT) { \
/*<A
//<B
//>real
// Returns :mini:`A SYMBOL B`.
*/\
	double RealA = ml_double_value_fast(Args[0]); \
	double RealB = ml_double_value_fast(Args[1]); \
	return ml_real(RealA SYMBOL RealB); \
}

#define ml_arith_method_real_integer(NAME, SYMBOL) \
ML_METHOD(NAME, MLDoubleT, MLIntegerT) { \
/*<A
//<B
//>real
// Returns :mini:`A SYMBOL B`.
*/\
	double RealA = ml_double_value_fast(Args[0]); \
	int64_t IntegerB = ml_integer_value_fast(Args[1]); \
	return ml_real(RealA SYMBOL IntegerB); \
}

#define ml_arith_method_integer_real(NAME, SYMBOL) \
ML_METHOD(NAME, MLIntegerT, MLDoubleT) { \
/*<A
//<B
//>real
// Returns :mini:`A SYMBOL B`.
*/\
	int64_t IntegerA = ml_integer_value_fast(Args[0]); \
	double RealB = ml_double_value_fast(Args[1]); \
	return ml_real(IntegerA SYMBOL RealB); \
}

#ifdef ML_COMPLEX

#define ml_arith_method_number(NAME, SYMBOL) \
ml_arith_method_integer(NAME, SYMBOL) \
ml_arith_method_real(NAME, SYMBOL) \
ml_arith_method_complex(NAME, SYMBOL)

#define ml_arith_method_number_number(NAME, SYMBOL) \
ml_arith_method_integer_integer(NAME, SYMBOL) \
ml_arith_method_real_real(NAME, SYMBOL) \
ml_arith_method_real_integer(NAME, SYMBOL) \
ml_arith_method_integer_real(NAME, SYMBOL) \
ml_arith_method_complex_complex(NAME, SYMBOL) \
ml_arith_method_complex_real(NAME, SYMBOL) \
ml_arith_method_complex_integer(NAME, SYMBOL) \
ml_arith_method_integer_complex(NAME, SYMBOL) \
ml_arith_method_real_complex(NAME, SYMBOL) \

#else

#define ml_arith_method_number(NAME, SYMBOL) \
ml_arith_method_integer(NAME, SYMBOL) \
ml_arith_method_real(NAME, SYMBOL)

#define ml_arith_method_number_number(NAME, SYMBOL) \
ml_arith_method_integer_integer(NAME, SYMBOL) \
ml_arith_method_real_real(NAME, SYMBOL) \
ml_arith_method_real_integer(NAME, SYMBOL) \
ml_arith_method_integer_real(NAME, SYMBOL)

#endif

ml_arith_method_number("-", -)
ml_arith_method_number_number("+", +)
ml_arith_method_number_number("-", -)
ml_arith_method_number_number("*", *)
ml_arith_method_integer("~", ~);
ml_arith_method_integer_integer("/\\", &);
ml_arith_method_integer_integer("\\/", |);
ml_arith_method_integer_integer("><", ^);

ML_METHOD("<<", MLIntegerT, MLIntegerT) {
//<A
//<B
//>integer
// Returns :mini:`A << B`.
	int64_t IntegerA = ml_integer_value_fast(Args[0]);
	int64_t IntegerB = ml_integer_value_fast(Args[1]);
	int64_t IntegerC;
	if (IntegerB > 0) {
		IntegerC = IntegerA << IntegerB;
	} else if (IntegerB < 0) {
		if (IntegerA < 0) {
			IntegerC = -(-IntegerA >> -IntegerB);
		} else {
			IntegerC = IntegerA >> -IntegerB;
		}
	} else {
		IntegerC = IntegerA;
	}
	return ml_integer(IntegerC);
}

ML_METHOD(">>", MLIntegerT, MLIntegerT) {
//<A
//<B
//>integer
// Returns :mini:`A >> B`.
	int64_t IntegerA = ml_integer_value_fast(Args[0]);
	int64_t IntegerB = ml_integer_value_fast(Args[1]);
	int64_t IntegerC;
	if (IntegerB > 0) {
		if (IntegerA < 0) {
			IntegerC = -(-IntegerA >> IntegerB);
		} else {
			IntegerC = IntegerA >> IntegerB;
		}
	} else if (IntegerB < 0) {
		IntegerC = IntegerA << -IntegerB;
	} else {
		IntegerC = IntegerA;
	}
	return ml_integer(IntegerC);
}

ML_METHOD("++", MLIntegerT) {
//<Int
//>integer
// Returns :mini:`Int + 1`
	return ml_integer(ml_integer_value_fast(Args[0]) + 1);
}

ML_METHOD("--", MLIntegerT) {
//<Int
//>integer
// Returns :mini:`Int - 1`
	return ml_integer(ml_integer_value_fast(Args[0]) - 1);
}

ML_METHOD("++", MLDoubleT) {
//<Real
//>real
// Returns :mini:`Real + 1`
	return ml_real(ml_double_value_fast(Args[0]) + 1);
}

ML_METHOD("--", MLDoubleT) {
//<Real
//>real
// Returns :mini:`Real - 1`
	return ml_real(ml_double_value_fast(Args[0]) - 1);
}

ml_arith_method_real_real("/", /)
ml_arith_method_real_integer("/", /)
ml_arith_method_integer_real("/", /)

#ifdef ML_COMPLEX

ml_arith_method_complex_complex("/", /)
ml_arith_method_complex_integer("/", /)
ml_arith_method_integer_complex("/", /)
ml_arith_method_complex_real("/", /)
ml_arith_method_real_complex("/", /)
ml_arith_method_complex("~", ~);

#endif

ML_METHOD("/", MLIntegerT, MLIntegerT) {
//<Int/1
//<Int/2
//>integer | real
// Returns :mini:`Int/1 / Int/2` as an integer if the division is exact, otherwise as a real.
	int64_t IntegerA = ml_integer_value_fast(Args[0]);
	int64_t IntegerB = ml_integer_value_fast(Args[1]);
	if (!IntegerB) return ml_error("ValueError", "Division by 0");
	if (IntegerA % IntegerB == 0) {
		return ml_integer(IntegerA / IntegerB);
	} else {
		return ml_real((double)IntegerA / (double)IntegerB);
	}
}

ML_METHOD("%", MLIntegerT, MLIntegerT) {
//<Int/1
//<Int/2
//>integer
// Returns the remainder of :mini:`Int/1` divided by :mini:`Int/2`.
// Note: the result is calculated by rounding towards 0. In particular, if :mini:`Int/1` is negative, the result will be negative.
// For a nonnegative remainder, use :mini:`Int/1 mod Int/2`.
	int64_t IntegerA = ml_integer_value_fast(Args[0]);
	int64_t IntegerB = ml_integer_value_fast(Args[1]);
	if (!IntegerB) return ml_error("ValueError", "Division by 0");
	return ml_integer(IntegerA % IntegerB);
}

ML_METHOD("|", MLIntegerT, MLIntegerT) {
//<Int/1
//<Int/2
//>integer
// Returns :mini:`Int/2` if it is divisible by :mini:`Int/1` and :mini:`nil` otherwise.
	int64_t IntegerA = ml_integer_value_fast(Args[0]);
	int64_t IntegerB = ml_integer_value_fast(Args[1]);
	if (!IntegerA) return ml_error("ValueError", "Division by 0");
	return (IntegerB % IntegerA) ? MLNil : Args[1];
}

ML_METHOD("!|", MLIntegerT, MLIntegerT) {
//<Int/1
//<Int/2
//>integer
// Returns :mini:`Int/2` if it is not divisible by :mini:`Int/1` and :mini:`nil` otherwise.
	int64_t IntegerA = ml_integer_value_fast(Args[0]);
	int64_t IntegerB = ml_integer_value_fast(Args[1]);
	if (!IntegerA) return ml_error("ValueError", "Division by 0");
	return (IntegerB % IntegerA) ? Args[1] : MLNil;
}

ML_METHOD("div", MLIntegerT, MLIntegerT) {
//<Int/1
//<Int/2
//>integer
// Returns the quotient of :mini:`Int/1` divided by :mini:`Int/2`.
// The result is calculated by rounding down in all cases.
	int64_t IntegerA = ml_integer_value_fast(Args[0]);
	int64_t IntegerB = ml_integer_value_fast(Args[1]);
	if (!IntegerB) return ml_error("ValueError", "Division by 0");
	long A = IntegerA;
	long B = IntegerB;
	long Q = A / B;
	if (A < 0 && B * Q != A) {
		if (B < 0) ++Q; else --Q;
	}
	return ml_integer(Q);
}

ML_METHOD("mod", MLIntegerT, MLIntegerT) {
//<Int/1
//<Int/2
//>integer
// Returns the remainder of :mini:`Int/1` divided by :mini:`Int/2`.
// Note: the result is calculated by rounding down in all cases. In particular, the result is always nonnegative.
	int64_t IntegerA = ml_integer_value_fast(Args[0]);
	int64_t IntegerB = ml_integer_value_fast(Args[1]);
	if (!IntegerB) return ml_error("ValueError", "Division by 0");
	long A = IntegerA;
	long B = IntegerB;
	long R = A % B;
	if (R < 0) R += labs(B);
	return ml_integer(R);
}

#define ml_comp_method_integer_integer(NAME, SYMBOL) \
ML_METHOD(NAME, MLIntegerT, MLIntegerT) { \
/*<A
//<B
//>integer
// Returns :mini:`B` if :mini:`A SYMBOL B`, otherwise returns :mini:`nil`.
*/\
	int64_t IntegerA = ml_integer_value_fast(Args[0]); \
	int64_t IntegerB = ml_integer_value_fast(Args[1]); \
	return IntegerA SYMBOL IntegerB ? Args[1] : MLNil; \
}

#define ml_comp_method_real_real(NAME, SYMBOL) \
ML_METHOD(NAME, MLDoubleT, MLDoubleT) { \
/*<A
//<B
//>real
// Returns :mini:`B` if :mini:`A SYMBOL B`, otherwise returns :mini:`nil`.
*/\
	double RealA = ml_double_value_fast(Args[0]); \
	double RealB = ml_double_value_fast(Args[1]); \
	return RealA SYMBOL RealB ? Args[1] : MLNil; \
}

#define ml_comp_method_real_integer(NAME, SYMBOL) \
ML_METHOD(NAME, MLDoubleT, MLIntegerT) { \
/*<A
//<B
//>real
// Returns :mini:`B` if :mini:`A SYMBOL B`, otherwise returns :mini:`nil`.
*/\
	double RealA = ml_double_value_fast(Args[0]); \
	int64_t IntegerB = ml_integer_value_fast(Args[1]); \
	return RealA SYMBOL IntegerB ? Args[1] : MLNil; \
}

#define ml_comp_method_integer_real(NAME, SYMBOL) \
ML_METHOD(NAME, MLIntegerT, MLDoubleT) { \
/*<A
//<B
//>real
// Returns :mini:`B` if :mini:`A SYMBOL B`, otherwise returns :mini:`nil`.
*/\
	int64_t IntegerA = ml_integer_value_fast(Args[0]); \
	double RealB = ml_double_value_fast(Args[1]); \
	return IntegerA SYMBOL RealB ? Args[1] : MLNil; \
}

#define ml_comp_method_number_number(NAME, SYMBOL) \
ml_comp_method_integer_integer(NAME, SYMBOL) \
ml_comp_method_real_real(NAME, SYMBOL) \
ml_comp_method_real_integer(NAME, SYMBOL) \
ml_comp_method_integer_real(NAME, SYMBOL)

ml_comp_method_number_number("=", ==)
ml_comp_method_number_number("!=", !=)
ml_comp_method_number_number("<", <)
ml_comp_method_number_number(">", >)
ml_comp_method_number_number("<=", <=)
ml_comp_method_number_number(">=", >=)

#ifdef ML_NANBOXING

#define NegOne ml_int32(-1)
#define One ml_int32(1)
#define Zero ml_int32(0)

#else

static ml_integer_t One[1] = {{MLIntegerT, 1}};
static ml_integer_t NegOne[1] = {{MLIntegerT, -1}};
static ml_integer_t Zero[1] = {{MLIntegerT, 0}};

#endif

ML_METHOD("<>", MLIntegerT, MLIntegerT) {
//<Int/1
//<Int/2
//>integer
// Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Int/1` is less than, equal to or greater than :mini:`Int/2`.
	int64_t IntegerA = ml_integer_value_fast(Args[0]);
	int64_t IntegerB = ml_integer_value_fast(Args[1]);
	if (IntegerA < IntegerB) return (ml_value_t *)NegOne;
	if (IntegerA > IntegerB) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

ML_METHOD("<>", MLDoubleT, MLIntegerT) {
//<Real/1
//<Int/2
//>integer
// Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Real/1` is less than, equal to or greater than :mini:`Int/2`.
	double RealA = ml_double_value_fast(Args[0]);
	int64_t IntegerB = ml_integer_value_fast(Args[1]);
	if (RealA < IntegerB) return (ml_value_t *)NegOne;
	if (RealA > IntegerB) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

ML_METHOD("<>", MLIntegerT, MLDoubleT) {
//<Int/1
//<Real/2
//>integer
// Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Int/1` is less than, equal to or greater than :mini:`Real/2`.
	int64_t IntegerA = ml_integer_value_fast(Args[0]);
	double RealB = ml_double_value_fast(Args[1]);
	if (IntegerA < RealB) return (ml_value_t *)NegOne;
	if (IntegerA > RealB) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

ML_METHOD("<>", MLDoubleT, MLDoubleT) {
//<Real/1
//<Real/2
//>integer
// Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Real/1` is less than, equal to or greater than :mini:`Real/2`.
	double RealA = ml_double_value_fast(Args[0]);
	double RealB = ml_double_value_fast(Args[1]);
	if (RealA < RealB) return (ml_value_t *)NegOne;
	if (RealA > RealB) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

ML_METHOD("isfinite", MLDoubleT) {
//<Number:number
//>number|nil
// Returns :mini:`Number` if it is finite (neither |plusmn|\ |infin| nor ``NaN``), otherwise returns :mini:`nil`.
	double X = ml_double_value_fast(Args[0]);
	if (!isfinite(X)) return MLNil;
	return Args[0];
}

ML_METHOD("isnan", MLDoubleT) {
//<Number:number
//>number|nil
// Returns :mini:`Number` if it is ``NaN``, otherwise returns :mini:`Number`.
	double X = ml_double_value_fast(Args[0]);
	if (!isnan(X)) return MLNil;
	return Args[0];
}

typedef struct ml_integer_iter_t {
	const ml_type_t *Type;
	long Current, Step, Limit;
	long Index;
} ml_integer_iter_t;

static void ML_TYPED_FN(ml_iter_value, MLIntegerIterT, ml_state_t *Caller, ml_integer_iter_t *Iter) {
	ML_RETURN(ml_integer(Iter->Current));
}

static void ML_TYPED_FN(ml_iter_next, MLIntegerIterT, ml_state_t *Caller, ml_integer_iter_t *Iter) {
	Iter->Current += Iter->Step;
	if (Iter->Step > 0) {
		if (Iter->Current > Iter->Limit) ML_RETURN(MLNil);
	} else if (Iter->Step < 0) {
		if (Iter->Current < Iter->Limit) ML_RETURN(MLNil);
	}
	++Iter->Index;
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLIntegerIterT, ml_state_t *Caller, ml_integer_iter_t *Iter) {
	ML_RETURN(ml_integer(Iter->Index));
}

ML_TYPE(MLIntegerIterT, (), "integer-iter");
//!range

static void ML_TYPED_FN(ml_iterate, MLIntegerRangeT, ml_state_t *Caller, ml_value_t *Value) {
	ml_integer_range_t *Range = (ml_integer_range_t *)Value;
	if (Range->Step > 0 && Range->Start > Range->Limit) ML_RETURN(MLNil);
	if (Range->Step < 0 && Range->Start < Range->Limit) ML_RETURN(MLNil);
	ml_integer_iter_t *Iter = new(ml_integer_iter_t);
	Iter->Type = MLIntegerIterT;
	Iter->Index = 1;
	Iter->Current = Range->Start;
	Iter->Limit = Range->Limit;
	Iter->Step = Range->Step;
	ML_RETURN(Iter);
}

ML_TYPE(MLIntegerRangeT, (MLSequenceT), "integer-range");
//!range

ML_METHOD("..", MLIntegerT, MLIntegerT) {
//!range
//<Start
//<Limit
//>integer::range
// Returns a range from :mini:`Start` to :mini:`Limit` (inclusive).
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = ml_integer_value_fast(Args[0]);
	Range->Limit = ml_integer_value_fast(Args[1]);
	Range->Step = 1;
	return (ml_value_t *)Range;
}

ML_METHOD("..", MLIntegerT, MLIntegerT, MLIntegerT) {
//!range
//<Start
//<Limit
//<Step
//>integer::range
// Returns a range from :mini:`Start` to :mini:`Limit` (inclusive).
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = ml_integer_value_fast(Args[0]);
	Range->Limit = ml_integer_value_fast(Args[1]);
	Range->Step = ml_integer_value_fast(Args[2]);
	return (ml_value_t *)Range;
}

ML_METHOD("up", MLIntegerT) {
//!range
//<Start
//<Count
//>integer::range
// Returns an unlimited range from :mini:`Start`.
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = ml_integer_value_fast(Args[0]);
	Range->Limit = LONG_MAX;
	Range->Step = 1;
	return (ml_value_t *)Range;
}

ML_METHOD("up", MLIntegerT, MLIntegerT) {
//!range
//<Start
//<Count
//>integer::range
// Returns a range from :mini:`Start` to :mini:`Start + Count - 1` (inclusive).
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = ml_integer_value_fast(Args[0]);
	Range->Limit = Range->Start + ml_integer_value_fast(Args[1]) - 1;
	Range->Step = 1;
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLIntegerT, MLIntegerT) {
//!range
//<Start
//<Step
//>integer::range
// Returns a unlimited range from :mini:`Start` in steps of :mini:`Step`.
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = ml_integer_value_fast(Args[0]);
	Range->Step = ml_integer_value_fast(Args[1]);
	if (Range->Step < 0) {
		Range->Limit = LONG_MIN;
	} else if (Range->Step > 0) {
		Range->Limit = LONG_MAX;
	} else {
		Range->Limit = Range->Start;
	}
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLIntegerRangeT, MLIntegerT) {
//!range
//<Range
//<Step
//>integer::range
// Returns a range with the same limits as :mini:`Range` but with step :mini:`Step`.
	ml_integer_range_t *Range0 = (ml_integer_range_t *)Args[0];
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = Range0->Start;
	Range->Limit = Range0->Limit;
	Range->Step = ml_integer_value_fast(Args[1]);
	return (ml_value_t *)Range;
}

ML_METHOD("count", MLIntegerRangeT) {
//!range
//<Range
//>integer
// Returns the number of values in :mini:`Range`.
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[0];
	int64_t Diff = Range->Limit - Range->Start;
	if (!Range->Step) {
		return (ml_value_t *)Zero;
	} else if (Diff < 0 && Range->Step > 0) {
		return (ml_value_t *)Zero;
	} else if (Diff > 0 && Range->Step < 0) {
		return (ml_value_t *)Zero;
	} else {
		return ml_integer(Diff / Range->Step + 1);
	}
}

ML_METHOD("start", MLIntegerRangeT) {
//!range
//<Range
//>integer
// Returns the start of :mini:`Range`.
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[0];
	return ml_integer(Range->Start);
}

ML_METHOD("limit", MLIntegerRangeT) {
//!range
//<Range
//>integer
// Returns the limit of :mini:`Range`.
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[0];
	return ml_integer(Range->Limit);
}

ML_METHOD("step", MLIntegerRangeT) {
//!range
//<Range
//>integer
// Returns the limit of :mini:`Range`.
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[0];
	return ml_integer(Range->Step);
}

ML_METHOD("in", MLIntegerT, MLIntegerRangeT) {
//!range
//<X
//<Range
//>X | nil
	long Value = ml_integer_value_fast(Args[0]);
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[1];
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return Args[0];
}

ML_METHOD("in", MLDoubleT, MLIntegerRangeT) {
//!range
//<X
//<Range
//>X | nil
	double Value = ml_double_value_fast(Args[0]);
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[1];
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return Args[0];
}

typedef struct ml_real_iter_t {
	const ml_type_t *Type;
	double Current, Step, Limit;
	long Index, Remaining;
} ml_real_iter_t;

static void ML_TYPED_FN(ml_iter_value, MLRealIterT, ml_state_t *Caller, ml_real_iter_t *Iter) {
	ML_RETURN(ml_real(Iter->Current));
}

static void ML_TYPED_FN(ml_iter_next, MLRealIterT, ml_state_t *Caller, ml_real_iter_t *Iter) {
	Iter->Current += Iter->Step;
	if (--Iter->Remaining == 0) ML_RETURN(MLNil);
	++Iter->Index;
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLRealIterT, ml_state_t *Caller, ml_real_iter_t *Iter) {
	ML_RETURN(ml_integer(Iter->Index));
}

ML_TYPE(MLRealIterT, (), "real-iter");
//!range

static void ML_TYPED_FN(ml_iterate, MLRealRangeT, ml_state_t *Caller, ml_value_t *Value) {
	ml_real_range_t *Range = (ml_real_range_t *)Value;
	if (Range->Step > 0 && Range->Start > Range->Limit) ML_RETURN(MLNil);
	if (Range->Step < 0 && Range->Start < Range->Limit) ML_RETURN(MLNil);
	ml_real_iter_t *Iter = new(ml_real_iter_t);
	Iter->Type = MLRealIterT;
	Iter->Index = 1;
	Iter->Current = Range->Start;
	Iter->Limit = Range->Limit;
	Iter->Step = Range->Step;
	Iter->Remaining = Range->Count;
	ML_RETURN(Iter);
}

ML_TYPE(MLRealRangeT, (MLSequenceT), "real-range");
//!range

ML_METHOD("..", MLNumberT, MLNumberT) {
//!range
//<Start
//<Limit
//>real::range
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	Range->Start = ml_real_value(Args[0]);
	Range->Limit = ml_real_value(Args[1]);
	Range->Step = 1.0;
	Range->Count = floor(Range->Limit - Range->Start) + 1;
	return (ml_value_t *)Range;
}

ML_METHOD("..", MLNumberT, MLNumberT, MLNumberT) {
//!range
//<Start
//<Limit
//>real::range
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	Range->Start = ml_real_value(Args[0]);
	Range->Limit = ml_real_value(Args[1]);
	double Step = Range->Step = ml_real_value(Args[2]);
	double C = (Range->Limit - Range->Start) / Step + 1;
	if (C > LONG_MAX) C = 0;
	Range->Count = C;
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLNumberT, MLNumberT) {
//!range
//<Start
//<Step
//>real::range
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	Range->Start = ml_real_value(Args[0]);
	Range->Step = ml_real_value(Args[1]);
	Range->Limit = Range->Step > 0.0 ? INFINITY : -INFINITY;
	Range->Count = 0;
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLRealRangeT, MLNumberT) {
//!range
//<Range
//<Step
//>real::range
	ml_real_range_t *Range0 = (ml_real_range_t *)Args[0];
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	double Start = Range->Start = Range0->Start;
	double Limit = Range->Limit = Range0->Limit;
	Range->Step = ml_real_value(Args[1]);
	double C = (Limit - Start) / Range->Step + 1;
	if (C > LONG_MAX) C = 0;
	Range->Count = C;
	return (ml_value_t *)Range;
}

ML_METHOD("in", MLIntegerRangeT, MLIntegerT) {
//!range
//<Range
//<Count
//>real::range
	ml_integer_range_t *Range0 = (ml_integer_range_t *)Args[0];
	long C = ml_integer_value_fast(Args[1]);
	if (C <= 0) return ml_error("RangeError", "Invalid step count");
	if ((Range0->Limit - Range0->Start) % C) {
		ml_real_range_t *Range = new(ml_real_range_t);
		Range->Type = MLRealRangeT;
		Range->Start = Range0->Start;
		Range->Limit = Range0->Limit;
		Range->Step = (Range->Limit - Range->Start) / C;
		Range->Count = C + 1;
		return (ml_value_t *)Range;
	} else {
		ml_integer_range_t *Range = new(ml_integer_range_t);
		Range->Type = MLIntegerRangeT;
		Range->Start = Range0->Start;
		Range->Limit = Range0->Limit;
		Range->Step = (Range->Limit - Range->Start) / C;
		return (ml_value_t *)Range;
	}
}

ML_METHOD("in", MLRealRangeT, MLIntegerT) {
//!range
//<Range
//<Count
//>real::range
	ml_real_range_t *Range0 = (ml_real_range_t *)Args[0];
	long C = ml_integer_value_fast(Args[1]);
	if (C <= 0) return ml_error("RangeError", "Invalid step count");
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	Range->Start = Range0->Start;
	Range->Limit = Range0->Limit;
	Range->Step = (Range->Limit - Range->Start) / C;
	Range->Count = C + 1;
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLIntegerRangeT, MLDoubleT) {
//!range
//<Range
//<Step
//>real::range
	ml_integer_range_t *Range0 = (ml_integer_range_t *)Args[0];
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	double Start = Range->Start = Range0->Start;
	double Limit = Range->Limit = Range0->Limit;
	double Step = Range->Step = ml_double_value_fast(Args[1]);
	double C = (Limit - Start) / Step + 1;
	if (C > LONG_MAX) C = 0;
	Range->Count = C;
	return (ml_value_t *)Range;
}

ML_METHOD("bin", MLIntegerRangeT, MLIntegerT) {
//!range
//<Range
//<Value
//>integer | nil
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[0];
	int64_t Value = ml_integer_value_fast(Args[1]);
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return ml_integer((Value - Range->Start) / Range->Step + 1);
}

ML_METHOD("bin", MLIntegerRangeT, MLDoubleT) {
//!range
//<Range
//<Value
//>integer | nil
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[0];
	double Value = ml_real_value(Args[1]);
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return ml_integer(floor((Value - Range->Start) / Range->Step) + 1);
}

ML_METHOD("bin", MLRealRangeT, MLIntegerT) {
//!range
//<Range
//<Value
//>integer | nil
	ml_real_range_t *Range = (ml_real_range_t *)Args[0];
	int64_t Value = ml_integer_value_fast(Args[1]);
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return ml_integer((Value - Range->Start) / Range->Step + 1);
}

ML_METHOD("bin", MLRealRangeT, MLDoubleT) {
//!range
//<Range
//<Value
//>integer | nil
	ml_real_range_t *Range = (ml_real_range_t *)Args[0];
	double Value = ml_real_value(Args[1]);
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return ml_integer(floor((Value - Range->Start) / Range->Step) + 1);
}

ML_METHOD("count", MLRealRangeT) {
//!range
//<Range
//>integer
// Returns the number of values in :mini:`Range`.
	ml_real_range_t *Range = (ml_real_range_t *)Args[0];
	return ml_integer(Range->Count);
}

ML_METHOD("start", MLRealRangeT) {
//!range
//<Range
//>real
// Returns the start of :mini:`Range`.
	ml_real_range_t *Range = (ml_real_range_t *)Args[0];
	return ml_real(Range->Start);
}

ML_METHOD("limit", MLRealRangeT) {
//!range
//<Range
//>real
// Returns the limit of :mini:`Range`.
	ml_real_range_t *Range = (ml_real_range_t *)Args[0];
	return ml_real(Range->Limit);
}

ML_METHOD("step", MLRealRangeT) {
//!range
//<Range
//>real
// Returns the step of :mini:`Range`.
	ml_real_range_t *Range = (ml_real_range_t *)Args[0];
	return ml_real(Range->Step);
}

ML_METHOD("in", MLIntegerT, MLRealRangeT) {
//!range
//<X
//<Range
//>X | nil
	long Value = ml_integer_value_fast(Args[0]);
	ml_real_range_t *Range = (ml_real_range_t *)Args[1];
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return Args[0];
}

ML_METHOD("in", MLDoubleT, MLRealRangeT) {
//!range
//<X
//<Range
//>X | nil
	double Value = ml_double_value_fast(Args[0]);
	ml_real_range_t *Range = (ml_real_range_t *)Args[1];
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return Args[0];
}

// Switch Functions //
//!type

typedef struct {
	ml_value_t *Index;
	int64_t Min, Max;
} ml_integer_case_t;

typedef struct {
	ml_type_t *Type;
	ml_integer_case_t Cases[];
} ml_integer_switch_t;

static void ml_integer_switch(ml_state_t *Caller, ml_integer_switch_t *Switch, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Arg = ml_deref(Args[0]);
	if (!ml_is(Arg, MLNumberT)) {
		ML_ERROR("TypeError", "Expected number for argument 1");
	}
	int64_t Value = ml_integer_value(Arg);
	for (ml_integer_case_t *Case = Switch->Cases;; ++Case) {
		if (Case->Min <= Value && Value <= Case->Max) ML_RETURN(Case->Index);
	}
	ML_RETURN(MLNil);
}

ML_TYPE(MLIntegerSwitchT, (MLFunctionT), "integer-switch",
//!internal
	.call = (void *)ml_integer_switch
);

ML_FUNCTION(MLIntegerSwitch) {
//!internal
	int Total = 1;
	for (int I = 0; I < Count; ++I) {
		ML_CHECK_ARG_TYPE(I, MLListT);
		Total += ml_list_length(Args[I]);
	}
	ml_integer_switch_t *Switch = xnew(ml_integer_switch_t, Total, ml_integer_case_t);
	Switch->Type = MLIntegerSwitchT;
	ml_integer_case_t *Case = Switch->Cases;
	for (int I = 0; I < Count; ++I) {
		ML_LIST_FOREACH(Args[I], Iter) {
			ml_value_t *Value = Iter->Value;
			if (ml_is(Value, MLIntegerT)) {
				Case->Min = Case->Max = ml_integer_value(Value);
			} else if (ml_is(Value, MLDoubleT)) {
				double Real = ml_real_value(Value), Int = floor(Real);
				if (Real != Int) return ml_error("ValueError", "Non-integer value in integer case");
				Case->Min = Case->Max = Int;
			} else if (ml_is(Value, MLIntegerRangeT)) {
				ml_integer_range_t *Range = (ml_integer_range_t *)Value;
				Case->Min = Range->Start;
				Case->Max = Range->Limit;
			} else if (ml_is(Value, MLRealRangeT)) {
				ml_real_range_t *Range = (ml_real_range_t *)Value;
				Case->Min = ceil(Range->Start);
				Case->Max = floor(Range->Limit);
			} else {
				return ml_error("ValueError", "Unsupported value in integer case");
			}
			Case->Index = ml_integer(I);
			++Case;
		}
	}
	Case->Min = LONG_MIN;
	Case->Max = LONG_MAX;
	Case->Index = ml_integer(Count);
	return (ml_value_t *)Switch;
}

typedef struct {
	ml_value_t *Index;
	double Min, Max;
} ml_real_case_t;

typedef struct {
	ml_type_t *Type;
	ml_real_case_t Cases[];
} ml_real_switch_t;

static void ml_real_switch(ml_state_t *Caller, ml_real_switch_t *Switch, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Arg = ml_deref(Args[0]);
	if (!ml_is(Arg, MLNumberT)) {
		ML_ERROR("TypeError", "Expected number for argument 1");
	}
	double Value = ml_real_value(Arg);
	for (ml_real_case_t *Case = Switch->Cases;; ++Case) {
		if (Case->Min <= Value && Value <= Case->Max) ML_RETURN(Case->Index);
	}
	ML_RETURN(MLNil);
}

ML_TYPE(MLRealSwitchT, (MLFunctionT), "real-switch",
//!internal
	.call = (void *)ml_real_switch
);

ML_FUNCTION(MLRealSwitch) {
//!internal
	int Total = 1;
	for (int I = 0; I < Count; ++I) {
		ML_CHECK_ARG_TYPE(I, MLListT);
		Total += ml_list_length(Args[I]);
	}
	ml_real_switch_t *Switch = xnew(ml_real_switch_t, Total, ml_real_case_t);
	Switch->Type = MLRealSwitchT;
	ml_real_case_t *Case = Switch->Cases;
	for (int I = 0; I < Count; ++I) {
		ML_LIST_FOREACH(Args[I], Iter) {
			ml_value_t *Value = Iter->Value;
			if (ml_is(Value, MLIntegerT)) {
				Case->Min = Case->Max = ml_integer_value(Value);
			} else if (ml_is(Value, MLDoubleT)) {
				Case->Min = Case->Max = ml_real_value(Value);
			} else if (ml_is(Value, MLIntegerRangeT)) {
				ml_integer_range_t *Range = (ml_integer_range_t *)Value;
				Case->Min = Range->Start;
				Case->Max = Range->Limit;
			} else if (ml_is(Value, MLRealRangeT)) {
				ml_real_range_t *Range = (ml_real_range_t *)Value;
				Case->Min = Range->Start;
				Case->Max = Range->Limit;
			} else {
				return ml_error("ValueError", "Unsupported value in real case");
			}
			Case->Index = ml_integer(I);
			++Case;
		}
	}
	Case->Min = -INFINITY;
	Case->Max = INFINITY;
	Case->Index = ml_integer(Count);
	return (ml_value_t *)Switch;
}

void ml_number_init() {
#include "ml_number_init.c"
	stringmap_insert(MLIntegerT->Exports, "range", MLIntegerRangeT);
	stringmap_insert(MLIntegerT->Exports, "switch", ml_inline_call_macro((ml_value_t *)MLIntegerSwitch));
	stringmap_insert(MLRealT->Exports, "range", MLRealRangeT);
	stringmap_insert(MLRealT->Exports, "switch", ml_inline_call_macro((ml_value_t *)MLRealSwitch));
	ml_method_by_value(MLIntegerT->Constructor, NULL, ml_identity, MLIntegerT, NULL);
	ml_method_by_name("isfinite", NULL, ml_identity, MLIntegerT, NULL);
	ml_method_by_name("isnan", NULL, ml_return_nil, MLIntegerT, NULL);
	ml_method_by_value(MLDoubleT->Constructor, NULL, ml_identity, MLDoubleT, NULL);
	ml_method_by_value(MLRealT->Constructor, NULL, ml_identity, MLDoubleT, NULL);
	stringmap_insert(MLRealT->Exports, "Inf", ml_real(INFINITY));
	stringmap_insert(MLRealT->Exports, "NaN", ml_real(NAN));
	ml_method_by_value(MLNumberT->Constructor, NULL, ml_identity, MLNumberT, NULL);
	ml_infix_many("+");
	ml_infix_many("*");
	ml_infix_many("/\\");
	ml_infix_many("\\/");
}