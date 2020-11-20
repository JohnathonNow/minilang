#include <gc.h>
#include "ml_runtime.h"
#include <string.h>
#include "minilang.h"
#include "ml_macros.h"
#include "ml_iterfns.h"

//!iterator

/****************************** Chained ******************************/

typedef struct ml_filter_t {
	const ml_type_t *Type;
	ml_value_t *Function;
} ml_filter_t;

static void ml_chained_filter_call(ml_state_t *Caller, ml_filter_t *Filter, int Count, ml_value_t **Args) {
	return ml_call(Caller, Filter->Function, Count, Args);
}

ML_TYPE(FilterT, (MLFunctionT), "chained-filter",
//@filter
	.call = (void *)ml_chained_filter_call
);

static ml_filter_t *FilterNil;

ML_FUNCTION(Filter) {
//@filter
//<?Function
//>filter
// Returns a filter for use in chained functions and iterators.
	if (Count == 0) return (ml_value_t *)FilterNil;
	ml_filter_t *Filter = new(ml_filter_t);
	Filter->Type = FilterT;
	Filter->Function = Args[0];
	return (ml_value_t *)Filter;
}

typedef struct ml_chained_state_t {
	ml_state_t Base;
	ml_value_t *Value;
	ml_value_t **Current;
} ml_chained_state_t;

static void ml_chained_state_value(ml_chained_state_t *State, ml_value_t *Value);

static void ml_chained_state_filter(ml_chained_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	ml_value_t **Entry = State->Current;
	if (!Entry[0]) ML_CONTINUE(State->Base.Caller, State->Value);
	State->Current = Entry + 1;
	ml_value_t *Function = Entry[0];
	if (ml_typeof(Function) != FilterT) {
		State->Base.run = (void *)ml_chained_state_value;
	}
	return ml_call(State, Function, 1, &State->Value);
}

static void ml_chained_state_value(ml_chained_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_value_t **Entry = State->Current;
	if (!Entry[0]) ML_CONTINUE(State->Base.Caller, Value);
	State->Current = Entry + 1;
	State->Value = Value;
	ml_value_t *Function = Entry[0];
	if (ml_typeof(Function) == FilterT) {
		State->Base.run = (void *)ml_chained_state_filter;
	}
	return ml_call(State, Function, 1, &State->Value);
}

typedef struct ml_chained_function_t {
	const ml_type_t *Type;
	ml_value_t *Entries[];
} ml_chained_function_t;

static void ml_chained_function_call(ml_state_t *Caller, ml_chained_function_t *Chained, int Count, ml_value_t **Args) {
	ml_chained_state_t *State = new(ml_chained_state_t);
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_chained_state_value;
	State->Base.Context = Caller->Context;
	State->Current = Chained->Entries + 1;
	return ml_call(State, Chained->Entries[0], Count, Args);
}

typedef struct ml_chained_iterator_t {
	ml_state_t Base;
	ml_value_t *Iterator;
	ml_value_t *Value;
	ml_value_t **Current, **Entries;
} ml_chained_iterator_t;

ML_TYPE(MLChainedStateT, (), "chained-iterator");

ML_TYPE(MLChainedFunctionT, (MLFunctionT, MLIteratableT), "chained-function",
	.call = (void *)ml_chained_function_call
);

static ml_value_t *ml_chained(int Count, ml_value_t **Functions) {
	if (Count == 1) return Functions[0];
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, Count + 1, ml_value_t *);
	Chained->Type = MLChainedFunctionT;
	for (int I = 0; I < Count; ++I) Chained->Entries[I] = *Functions++;
	return (ml_value_t *)Chained;
}

static void ML_TYPED_FN(ml_iter_key, MLChainedStateT, ml_state_t *Caller, ml_chained_iterator_t *State) {
	return ml_iter_key(Caller, State->Iterator);
}

static void ML_TYPED_FN(ml_iter_value, MLChainedStateT, ml_state_t *Caller, ml_chained_iterator_t *State) {
	ML_RETURN(State->Value);
}

static void ml_chained_iterator_next(ml_chained_iterator_t *State, ml_value_t *Iter);

static void ml_chained_iterator_value(ml_chained_iterator_t *State, ml_value_t *Value);

static void ml_chained_iterator_filter(ml_chained_iterator_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) {
		State->Base.run = (void *)ml_chained_iterator_next;
		State->Current = State->Entries;
		return ml_iter_next((ml_state_t *)State, State->Iterator);
	}
	ml_value_t **Entry = State->Current;
	if (!Entry[0]) ML_CONTINUE(State->Base.Caller, State);
	State->Current = Entry + 1;
	ml_value_t *Function = Entry[0];
	if (ml_typeof(Function) != FilterT) {
		State->Base.run = (void *)ml_chained_iterator_value;
	}
	return ml_call(State, Function, 1, &State->Value);
}

static void ml_chained_iterator_value(ml_chained_iterator_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Value = Value;
	ml_value_t **Entry = State->Current;
	if (!Entry[0]) ML_CONTINUE(State->Base.Caller, State);
	State->Current = Entry + 1;
	ml_value_t *Function = Entry[0];
	if (ml_typeof(Function) == FilterT) {
		State->Base.run = (void *)ml_chained_iterator_filter;
	}
	return ml_call(State, Function, 1, &State->Value);
}

static void ml_chained_iterator_next(ml_chained_iterator_t *State, ml_value_t *Iter) {
	if (ml_is_error(Iter)) ML_CONTINUE(State->Base.Caller, Iter);
	if (Iter == MLNil) ML_CONTINUE(State->Base.Caller, Iter);
	State->Base.run = (void *)ml_chained_iterator_value;
	State->Current = State->Entries;
	return ml_iter_value((ml_state_t *)State, State->Iterator = Iter);
}

static void ML_TYPED_FN(ml_iter_next, MLChainedStateT, ml_state_t *Caller, ml_chained_iterator_t *State) {
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)ml_chained_iterator_next;
	return ml_iter_next((ml_state_t *)State, State->Iterator);
}

static void ML_TYPED_FN(ml_iterate, MLChainedFunctionT, ml_state_t *Caller, ml_chained_function_t *Chained) {
	ml_chained_iterator_t *State = new(ml_chained_iterator_t);
	State->Base.Type =  MLChainedStateT;
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)ml_chained_iterator_next;
	State->Entries = Chained->Entries + 1;
	return ml_iterate((ml_state_t *)State, Chained->Entries[0]);
}

ML_METHOD("->", MLIteratableT, MLFunctionT) {
//<Iteratable
//<Function
//>chainedfunction
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, 3, ml_value_t *);
	Chained->Type = MLChainedFunctionT;
	Chained->Entries[0] = Args[0];
	Chained->Entries[1] = Args[1];
	return (ml_value_t *)Chained;
}

ML_METHOD("->", MLFunctionT, MLFunctionT) {
//<Iteratable
//<Function
//>chainedfunction
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, 3, ml_value_t *);
	Chained->Type = MLChainedFunctionT;
	Chained->Entries[0] = Args[0];
	Chained->Entries[1] = Args[1];
	return (ml_value_t *)Chained;
}

ML_METHOD("->", MLFunctionT, MLChainedFunctionT) {
//<Iteratable
//<Function
//>chainedfunction
	ml_chained_function_t *Rest = (ml_chained_function_t *)Args[1];
	int N = 0;
	while (Rest->Entries[N]) ++N;
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, N + 2, ml_value_t *);
	Chained->Type = MLChainedFunctionT;
	Chained->Entries[0] = Args[0];
	for (int I = 0; I < N; ++I) Chained->Entries[I + 1] = Rest->Entries[I];
	return (ml_value_t *)Chained;
}

ML_METHOD("->", MLChainedFunctionT, MLFunctionT) {
//<ChainedFunction
//<Function
//>chainedfunction
	ml_chained_function_t *Base = (ml_chained_function_t *)Args[0];
	int N = 0;
	while (Base->Entries[N]) ++N;
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, N + 2, ml_value_t *);
	Chained->Type = MLChainedFunctionT;
	for (int I = 0; I < N; ++I) Chained->Entries[I] = Base->Entries[I];
	Chained->Entries[N] = Args[1];
	return (ml_value_t *)Chained;
}

ML_METHOD("->", MLChainedFunctionT, MLChainedFunctionT) {
//<ChainedFunction/1
//<ChainedFunction/2
//>chainedfunction
	ml_chained_function_t *Base = (ml_chained_function_t *)Args[0];
	int N = 0;
	while (Base->Entries[N]) ++N;
	ml_chained_function_t *Rest = (ml_chained_function_t *)Args[1];
	int M = 0;
	while (Rest->Entries[M]) ++M;
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, N + M + 1, ml_value_t *);
	Chained->Type = MLChainedFunctionT;
	for (int I = 0; I < N; ++I) Chained->Entries[I] = Base->Entries[I];
	for (int I = 0; I < M; ++I) Chained->Entries[I + N] = Rest->Entries[I];
	return (ml_value_t *)Chained;
}

/****************************** Doubled ******************************/

typedef struct ml_double_t {
	const ml_type_t *Type;
	ml_value_t *Iteratable, *Function;
} ml_double_t;

ML_TYPE(MLDoubledIteratorT, (MLIteratableT), "double");

typedef struct ml_double_state_t {
	ml_state_t Base;
	ml_value_t *Iterator0;
	ml_value_t *Iterator;
	ml_value_t *Function;
	ml_value_t *Arg;
} ml_double_state_t;

ML_TYPE(MLDoubledIteratorStateT, (MLStateT), "double-state");

static void ml_double_iter0_next(ml_double_state_t *State, ml_value_t *Value);

static void ml_double_iter_next(ml_double_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) {
		State->Base.run = (void *)ml_double_iter0_next;
		return ml_iter_next((ml_state_t *)State, State->Iterator0);
	}
	State->Iterator = Value;
	ML_CONTINUE(State->Base.Caller, State);
}

static void ml_double_function_call(ml_double_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)ml_double_iter_next;
	return ml_iterate((ml_state_t *)State, Value);
}

static void ml_double_value0(ml_double_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)ml_double_function_call;
	State->Arg = Value;
	ml_value_t *Function = State->Function;
	return ml_call(State, Function, 1, &State->Arg);
}

static void ml_double_iter0_next(ml_double_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)ml_double_value0;
	return ml_iter_value((ml_state_t *)State, State->Iterator0 = Value);
}

static void ML_TYPED_FN(ml_iterate, MLDoubledIteratorT, ml_state_t *Caller, ml_double_t *Double) {
	ml_double_state_t *State = new(ml_double_state_t);
	State->Base.Type = MLDoubledIteratorStateT;
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)ml_double_iter0_next;
	State->Function = Double->Function;
	return ml_iterate((ml_state_t *)State, Double->Iteratable);
}

static void ML_TYPED_FN(ml_iter_key, MLDoubledIteratorStateT, ml_state_t *Caller, ml_double_state_t *State) {
	return ml_iter_key(Caller, State->Iterator);
}

static void ML_TYPED_FN(ml_iter_value, MLDoubledIteratorStateT, ml_state_t *Caller, ml_double_state_t *State) {
	return ml_iter_value(Caller, State->Iterator);
}

static void ML_TYPED_FN(ml_iter_next, MLDoubledIteratorStateT, ml_state_t *Caller, ml_double_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)ml_double_iter_next;
	return ml_iter_next((ml_state_t *)State, State->Iterator);
}

ML_METHOD("^", MLIteratableT, MLFunctionT) {
	ml_double_t *Double = new(ml_double_t);
	Double->Type = MLDoubledIteratorT;
	Double->Iteratable = Args[0];
	Double->Function = Args[1];
	return (ml_value_t *)Double;
}

/****************************** All ******************************/

typedef struct ml_iter_state_t {
	ml_state_t Base;
	ml_value_t *Iter;
	ml_value_t *Values[];
} ml_iter_state_t;

static void all_iterate(ml_iter_state_t *State, ml_value_t *Value);

static void all_iter_value(ml_iter_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)all_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void all_iterate(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, MLSome);
	State->Base.run = (void *)all_iter_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

ML_FUNCTIONX(All) {
//<Iteratable
//>some | nil
// Returns :mini:`nil` if :mini:`nil` is produced by :mini:`Iterable`. Otherwise returns :mini:`some`.
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_iter_state_t *State = new(ml_iter_state_t);
	State->Base.Caller = Caller;
	State->Base.run = (void *)all_iterate;
	State->Base.Context = Caller->Context;
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

/****************************** First ******************************/

static void first_iterate(ml_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Caller, Value);
	return ml_iter_value(State->Caller, Value);
}

ML_FUNCTIONX(First) {
//<Iteratable
//>any | nil
// Returns the first value produced by :mini:`Iteratable`.
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_state_t *State = new(ml_state_t);
	State->Caller = Caller;
	State->run = first_iterate;
	State->Context = Caller->Context;
	return ml_iterate(State, ml_chained(Count, Args));
}

static void first2_iter_value(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_tuple_set(State->Values[0], 2, Value);
	ML_CONTINUE(State->Base.Caller, State->Values[0]);
}

static void first2_iter_key(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_tuple_set(State->Values[0], 1, Value);
	State->Base.run = (ml_state_fn)first2_iter_value;
	return ml_iter_value((ml_state_t *)State, State->Iter);
}

static void first2_iterate(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (ml_state_fn)first2_iter_key;
	return ml_iter_key((ml_state_t *)State, State->Iter = Value);
}

ML_FUNCTIONX(First2) {
//<Iteratable
//>tuple(any, any) | nil
// Returns the first key and value produced by :mini:`Iteratable`.
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 1, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (ml_state_fn)first2_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = ml_tuple(2);
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

static void last_iterate(ml_iter_state_t *State, ml_value_t *Value);

static void last_value(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[0] = Value;
	State->Base.run = (void *)last_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void last_iterate(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, State->Values[0]);
	State->Base.run = (void *)last_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

ML_FUNCTIONX(Last) {
//<Iteratable
//>any | nil
// Returns the last value produced by :mini:`Iteratable`.
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 1, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)last_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = MLNil;
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

static void last2_iterate(ml_iter_state_t *State, ml_value_t *Value);

static void last2_value(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[1] = Value;
	State->Base.run = (void *)last2_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void last2_key(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[0] = Value;
	State->Base.run = (void *)last2_value;
	return ml_iter_value((ml_state_t *)State, State->Iter);
}

static void last2_iterate(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) {
		if (State->Values[0]) {
			ml_value_t *Tuple = ml_tuple(2);
			ml_tuple_set(Tuple, 1, State->Values[0]);
			ml_tuple_set(Tuple, 2, State->Values[1]);
			ML_CONTINUE(State->Base.Caller, Tuple);
		} else {
			ML_CONTINUE(State->Base.Caller, MLNil);
		}
	}
	State->Base.run = (void *)last2_key;
	return ml_iter_key((ml_state_t *)State, State->Iter = Value);
}

ML_FUNCTIONX(Last2) {
//<Iteratable
//>tuple(any, any) | nil
// Returns the last key and value produced by :mini:`Iteratable`.
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)last2_iterate;
	State->Base.Context = Caller->Context;
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

static void list_iterate(ml_iter_state_t *State, ml_value_t *Value);

static void list_iter_value(ml_iter_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_list_put(State->Values[0], Value);
	State->Base.run = (void *)list_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void list_iterate(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)list_iter_value;
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, State->Values[0]);
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

extern ml_value_t *MLListOfMethod;

ML_METHODVX(MLListOfMethod, MLIteratableT) {
//!list
//<Iteratable
//>list
// Returns a list of all of the values produced by :mini:`Iteratable`.
	ml_iter_state_t *State = xnew(ml_iter_state_t, 1, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)list_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = ml_list();
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

static void map_iterate(ml_iter_state_t *State, ml_value_t *Value);

static void map_iter_value(ml_iter_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_map_insert(State->Values[0], State->Values[1], Value);
	State->Base.run = (void *)map_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void map_iter_key(ml_iter_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) Value = ml_integer(ml_map_size(State->Values[0]) + 1);
	State->Values[1] = Value;
	State->Base.run = (void *)map_iter_value;
	return ml_iter_value((ml_state_t *)State, State->Iter);
}

static void map_iterate(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, State->Values[0]);
	State->Base.run = (void *)map_iter_key;
	return ml_iter_key((ml_state_t *)State, State->Iter = Value);
}

ML_FUNCTIONX(All2) {
//!deprecated
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 2, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)map_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = ml_map();
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

extern ml_value_t *MLMapOfMethod;

ML_METHODVX(MLMapOfMethod, MLIteratableT) {
//!map
//<Iteratable
//>map
// Returns a map of all the key and value pairs produced by :mini:`Iteratable`.
	ml_iter_state_t *State = xnew(ml_iter_state_t, 2, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)map_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = ml_map();
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

typedef struct ml_count_state_t {
	ml_state_t Base;
	ml_value_t *Iter;
	long Count;
} ml_count_state_t;

static void count_iterate(ml_count_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, ml_integer(State->Count));
	++State->Count;
	return ml_iter_next((ml_state_t *)State, State->Iter = Value);
}

ML_FUNCTIONX(Count) {
//<Iteratable
//>integer
// Returns the count of the values produced by :mini:`Iteratable`.
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_count_state_t *State = new(ml_count_state_t);
	State->Base.Caller = Caller;
	State->Base.run = (void *)count_iterate;
	State->Base.Context = Caller->Context;
	State->Count = 0;
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

typedef struct ml_count2_state_t {
	ml_state_t Base;
	ml_value_t *Iter;
	ml_value_t *Counts;
} ml_count2_state_t;

static void count2_iterate(ml_count2_state_t *State, ml_value_t *Value);

static void count2_value(ml_count2_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value != MLNil) {
		ml_map_node_t *Node = ml_map_slot(State->Counts, Value);
		Node->Value = (ml_value_t *)((char *)Node->Value + 1);
	}
	State->Base.run = (void *)count2_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void count2_iterate(ml_count2_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) {
		ML_MAP_FOREACH(State->Counts, Iter) Iter->Value = ml_integer((char *)Iter->Value - (char *)0);
		ML_CONTINUE(State->Base.Caller, State->Counts);
	}
	State->Base.run = (void *)count2_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

ML_FUNCTIONX(Count2) {
//<Iteratable
//>map
// Returns a map of the values produced by :mini:`Iteratable` with associated counts.
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_count2_state_t *State = new(ml_count2_state_t);
	State->Base.Caller = Caller;
	State->Base.run = (void *)count2_iterate;
	State->Base.Context = Caller->Context;
	State->Counts = ml_map();
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

static ML_METHOD_DECL(Less, "<");
static ML_METHOD_DECL(Greater, ">");
static ML_METHOD_DECL(Add, "+");
static ML_METHOD_DECL(Mul, "*");

static void reduce_iter_next(ml_iter_state_t *State, ml_value_t *Value);

static void reduce_call(ml_iter_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value != MLNil) State->Values[1] = Value;
	State->Base.run = (void *)reduce_iter_next;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void reduce_next_value(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_value_t *Compare = State->Values[0];
	State->Values[2] = Value;
	State->Base.run = (void *)reduce_call;
	return ml_call(State, Compare, 2, State->Values + 1);
}

static void reduce_iter_next(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, State->Values[1] ?: MLNil);
	State->Base.run = (void *)reduce_next_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

static void reduce_first_value(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[1] = Value;
	State->Base.run = (void *)reduce_iter_next;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void reduce_iterate(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, MLNil);
	State->Base.run = State->Values[1] ? (void *)reduce_next_value : (void *)reduce_first_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

ML_FUNCTIONX(Reduce) {
//<?Initial:any
//<Iteratable:iteratable
//<Fn:function
//>any | nil
// Returns :mini:`Fn(Fn( ... Fn(Initial, V/1), V/2) ..., V/n)` where :mini:`V/i` are the values produced by :mini:`Iteratable`.
// If :mini:`Initial` is omitted, first value produced by :mini:`Iteratable` is used.
	ML_CHECKX_ARG_COUNT(2);
	if (Count == 2) {
		ML_CHECKX_ARG_TYPE(0, MLIteratableT);
		ML_CHECKX_ARG_TYPE(1, MLFunctionT);
		ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
		State->Base.Caller = Caller;
		State->Base.run = (void *)reduce_iterate;
		State->Base.Context = Caller->Context;
		State->Values[0] = Args[1];
		return ml_iterate((ml_state_t *)State, Args[0]);
	} else {
		ML_CHECKX_ARG_TYPE(1, MLIteratableT);
		ML_CHECKX_ARG_TYPE(2, MLFunctionT);
		ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
		State->Base.Caller = Caller;
		State->Base.run = (void *)reduce_iterate;
		State->Base.Context = Caller->Context;
		State->Values[0] = Args[2];
		State->Values[1] = Args[0];
		return ml_iterate((ml_state_t *)State, Args[1]);
	}
}

ML_FUNCTIONX(Min) {
//<Iteratable
//>any | nil
// Returns the smallest value (based on :mini:`<`) produced by :mini:`Iteratable`.
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)reduce_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = GreaterMethod;
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

ML_FUNCTIONX(Max) {
//<Iteratable
//>any | nil
// Returns the largest value (based on :mini:`>`) produced by :mini:`Iteratable`.
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)reduce_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = LessMethod;
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

ML_FUNCTIONX(Sum) {
//<Iteratable
//>any | nil
// Returns the sum of the values (based on :mini:`+`) produced by :mini:`Iteratable`.
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)reduce_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = AddMethod;
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

ML_FUNCTIONX(Prod) {
//<Iteratable
//>any | nil
// Returns the product of the values (based on :mini:`*`) produced by :mini:`Iteratable`.
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)reduce_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = MulMethod;
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

typedef struct ml_join_state_t {
	ml_state_t Base;
	const char *Separator;
	ml_value_t *Iter;
	ml_stringbuffer_t Buffer[1];
	size_t SeparatorLength;
} ml_join_state_t;

static void join_append(ml_join_state_t *State, ml_value_t *Value);

static void join_next(ml_join_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, ml_stringbuffer_value(State->Buffer));
	ml_stringbuffer_add(State->Buffer, State->Separator, State->SeparatorLength);
	State->Base.run = (void *)join_append;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

static void join_append(ml_join_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_stringbuffer_append(State->Buffer, Value);
	State->Base.run = (void *)join_next;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void join_first(ml_join_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, ml_stringbuffer_value(State->Buffer));
	State->Base.run = (void *)join_append;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

ML_METHODVX("join", MLIteratableT, MLStringT) {
//<Iteratable
//<Separator
//>string
// Joins the elements of :mini:`Iteratable` into a string using :mini:`Separator` between elements.
	ml_join_state_t *State = new(ml_join_state_t);
	State->Base.Caller = Caller;
	State->Base.run = (void *)join_first;
	State->Base.Context = Caller->Context;
	State->Separator = ml_string_value(Args[1]);
	State->SeparatorLength = ml_string_length(Args[1]);
	State->Buffer[0] = (ml_stringbuffer_t)ML_STRINGBUFFER_INIT;
	return ml_iterate((ml_state_t *)State, Args[0]);
}

static void reduce2_iter_next(ml_iter_state_t *State, ml_value_t *Value);

static void reduce2_next_key(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[1] = Value;
	State->Base.run = (void *)reduce2_iter_next;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void reduce2_call(ml_iter_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value != MLNil) {
		State->Values[2] = Value;
		State->Base.run = (void *)reduce2_next_key;
		return ml_iter_key((ml_state_t *)State, State->Iter);
	} else {
		State->Base.run = (void *)reduce2_iter_next;
		return ml_iter_next((ml_state_t *)State, State->Iter);
	}
}

static void reduce2_next_value(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_value_t *Function = State->Values[0];
	State->Values[3] = Value;
	State->Base.run = (void *)reduce2_call;
	return ml_call(State, Function, 2, State->Values + 2);
}

static void reduce2_iter_next(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) {
		if (State->Values[1]) {
			ml_value_t *Tuple = ml_tuple(2);
			ml_tuple_set(Tuple, 1, State->Values[1]);
			ml_tuple_set(Tuple, 2, State->Values[2]);
			ML_CONTINUE(State->Base.Caller, Tuple);
		} else {
			ML_CONTINUE(State->Base.Caller, MLNil);
		}
	}
	State->Base.run = (void *)reduce2_next_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

static void reduce2_first_value(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[2] = Value;
	State->Base.run = (void *)reduce2_iter_next;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void reduce2_first_key(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[1] = Value;
	State->Base.run = (void *)reduce2_first_value;
	return ml_iter_value((ml_state_t *)State, State->Iter);
}

static void reduce2_iterate(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, MLNil);
	State->Base.run = (void *)reduce2_first_key;
	return ml_iter_key((ml_state_t *)State, State->Iter = Value);
}

ML_FUNCTIONX(Reduce2) {
	ML_CHECKX_ARG_COUNT(2);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ML_CHECKX_ARG_TYPE(1, MLFunctionT);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 4, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)reduce2_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = Args[1];
	return ml_iterate((ml_state_t *)State, Args[0]);
}

ML_FUNCTIONX(Min2) {
//<Iteratable
//>any | nil
// Returns a tuple with the key and value of the smallest value (based on :mini:`<`) produced by :mini:`Iteratable`.
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 4, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)reduce2_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = GreaterMethod;
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

ML_FUNCTIONX(Max2) {
//<Iteratable
//>any | nil
// Returns a tuple with the key and value of the largest value (based on :mini:`>`) produced by :mini:`Iteratable`.
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 4, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)reduce2_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = LessMethod;
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

typedef struct ml_stacked_t {
	const ml_type_t *Type;
	ml_value_t *Value, *ReduceFn;
} ml_stacked_t;

ML_TYPE(MLStackedT, (MLIteratableT), "stacked");
//!internal

ML_TYPE(MLStackedStateT, (), "stacked-state");
//!internal

static void stacked_iter_next(ml_iter_state_t *State, ml_value_t *Value);

static void stacked_call(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[1] = Value;
	State->Base.run = (void *)stacked_iter_next;
	ML_CONTINUE(State->Base.Caller, State);
}

static void stacked_next_value(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_value_t *ReduceFn = State->Values[0];
	State->Values[2] = Value;
	State->Base.run = (void *)stacked_call;
	return ml_call(State, ReduceFn, 2, State->Values + 1);
}

static void stacked_iter_next(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, MLNil);
	State->Base.run = (void *)stacked_next_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

static void stacked_first_value(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[1] = Value;
	State->Base.run = (void *)stacked_iter_next;
	ML_CONTINUE(State->Base.Caller, State);
}

static void stacked_iterate(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, MLNil);
	State->Base.run = (void *)stacked_first_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}


static void ML_TYPED_FN(ml_iter_key, MLStackedStateT, ml_state_t *Caller, ml_iter_state_t *State) {
	State->Base.Caller = Caller;
	return ml_iter_key(Caller, State->Iter);
}

static void ML_TYPED_FN(ml_iter_value, MLStackedStateT, ml_state_t *Caller, ml_iter_state_t *State) {
	State->Base.Caller = Caller;
	ML_RETURN(State->Values[1]);
}

static void ML_TYPED_FN(ml_iter_next, MLStackedStateT, ml_state_t *Caller, ml_iter_state_t *State) {
	State->Base.Caller = Caller;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void ML_TYPED_FN(ml_iterate, MLStackedT, ml_state_t *Caller, ml_stacked_t *Stacked) {
	ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
	State->Base.Type = MLStackedStateT;
	State->Base.Caller = Caller;
	State->Base.run = (void *)stacked_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = Stacked->ReduceFn;
	return ml_iterate((ml_state_t *)State, Stacked->Value);
}

ML_METHOD("//", MLIteratableT, MLFunctionT) {
//<Iteratable
//<Fn
//>iteratable
// Returns an iteratable that produces :mini:`V/1`, :mini:`Fn(V/1, V/2)`, :mini:`Fn(Fn(V/1, V/2), V/3)`, ... .
	ml_stacked_t *Stacked = new(ml_stacked_t);
	Stacked->Type = MLStackedT;
	Stacked->Value = Args[0];
	Stacked->ReduceFn = Args[1];
	return (ml_value_t *)Stacked;
}

typedef struct ml_limited_t {
	const ml_type_t *Type;
	ml_value_t *Value;
	int Remaining;
} ml_limited_t;

ML_TYPE(MLLimitedT, (MLIteratableT), "limited");
//!internal

ML_METHOD("count", MLLimitedT) {
//!internal
	ml_limited_t *Limited = (ml_limited_t *)Args[0];
	return ml_integer(Limited->Remaining);
}

typedef struct ml_limited_state_t {
	ml_state_t Base;
	ml_value_t *Iter;
	int Remaining;
} ml_limited_state_t;

ML_TYPE(MLLimitedStateT, (), "limited-state");
//!internal

static void limited_iterate(ml_limited_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Iter = Value;
	--State->Remaining;
	ML_CONTINUE(State->Base.Caller, State);
}

static void ML_TYPED_FN(ml_iterate, MLLimitedT, ml_state_t *Caller, ml_limited_t *Limited) {
	if (Limited->Remaining) {
		ml_limited_state_t *State = new(ml_limited_state_t);
		State->Base.Type = MLLimitedStateT;
		State->Base.Caller = Caller;
		State->Base.run = (void *)limited_iterate;
		State->Base.Context = Caller->Context;
		State->Remaining = Limited->Remaining;
		return ml_iterate((ml_state_t *)State, Limited->Value);
	} else {
		ML_RETURN(MLNil);
	}
}

static void ML_TYPED_FN(ml_iter_key, MLLimitedStateT, ml_state_t *Caller, ml_limited_state_t *State) {
	return ml_iter_key(Caller, State->Iter);
}

static void ML_TYPED_FN(ml_iter_value, MLLimitedStateT, ml_state_t *Caller, ml_limited_state_t *State) {
	return ml_iter_value(Caller, State->Iter);
}

static void ML_TYPED_FN(ml_iter_next, MLLimitedStateT, ml_state_t *Caller, ml_limited_state_t *State) {
	if (State->Remaining) {
		State->Base.Caller = Caller;
		State->Base.run = (void *)limited_iterate;
		return ml_iter_next((ml_state_t *)State, State->Iter);
	} else {
		ML_RETURN(MLNil);
	}
}

ML_METHOD("limit", MLIteratableT, MLIntegerT) {
//<Iteratable
//<Limit
//>iteratable
// Returns an iteratable that produces at most :mini:`Limit` values from :mini:`Iteratable`.
	ml_limited_t *Limited = new(ml_limited_t);
	Limited->Type = MLLimitedT;
	Limited->Value = Args[0];
	Limited->Remaining = ml_integer_value_fast(Args[1]);
	return (ml_value_t *)Limited;
}

typedef struct ml_skipped_t {
	const ml_type_t *Type;
	ml_value_t *Value;
	long Remaining;
} ml_skipped_t;

ML_TYPE(MLSkippedT, (MLIteratableT), "skipped");
//!internal

typedef struct ml_skipped_state_t {
	ml_state_t Base;
	long Remaining;
} ml_skipped_state_t;

static void skipped_iterate(ml_skipped_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	if (State->Remaining) {
		--State->Remaining;
		return ml_iter_next((ml_state_t *)State, Value);
	} else {
		ML_CONTINUE(State->Base.Caller, Value);
	}
}

static void ML_TYPED_FN(ml_iterate, MLSkippedT, ml_state_t *Caller, ml_skipped_t *Skipped) {
	if (Skipped->Remaining) {
		ml_skipped_state_t *State = new(ml_skipped_state_t);
		State->Base.Caller = Caller;
		State->Base.run = (void *)skipped_iterate;
		State->Base.Context = Caller->Context;
		State->Remaining = Skipped->Remaining;
		return ml_iterate((ml_state_t *)State, Skipped->Value);
	} else {
		return ml_iterate(Caller, Skipped->Value);
	}
}

ML_METHOD("skip", MLIteratableT, MLIntegerT) {
//<Iteratable
//<Skip
//>iteratable
// Returns an iteratable that skips the first :mini:`Skip` values from :mini:`Iteratable` and then produces the rest.
	ml_skipped_t *Skipped = new(ml_skipped_t);
	Skipped->Type = MLSkippedT;
	Skipped->Value = Args[0];
	Skipped->Remaining = ml_integer_value_fast(Args[1]);
	return (ml_value_t *)Skipped;
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Value;
	ml_state_t *Limited;
	size_t Waiting, Limit, Burst;
} ml_tasks_t;

static void ml_tasks_call(ml_state_t *Caller, ml_tasks_t *Tasks, int Count, ml_value_t **Args) {
	if (!Tasks->Waiting) ML_ERROR("TasksError", "Tasks have already completed");
	if (Tasks->Value != MLNil) ML_RETURN(Tasks->Value);
	ML_CHECKX_ARG_TYPE(Count - 1, MLFunctionT);
	ml_value_t *Function = Args[Count - 1];
	++Tasks->Waiting;
	ml_call(Tasks, Function, Count - 1, Args);
	if (Tasks->Waiting > Tasks->Limit && !Tasks->Limited) {
		Tasks->Limited = Caller;
	} else {
		ML_RETURN(Tasks->Value);
	}
}

ML_TYPE(MLTasksT, (MLFunctionT), "tasks",
//!tasks
// A dynamic set of tasks (function calls). Multiple tasks can run in parallel (depending on the availability of a scheduler and/or asynchronous function calls).
	.call = (void *)ml_tasks_call
);

static void ml_tasks_continue(ml_tasks_t *Tasks, ml_value_t *Value) {
	if (ml_is_error(Value)) Tasks->Value = Value;
	--Tasks->Waiting;
	if (Tasks->Limited && Tasks->Waiting <= Tasks->Burst) {
		ml_state_t *Caller = Tasks->Limited;
		Tasks->Limited = NULL;
		ML_RETURN(Tasks->Value);
	}
	if (Tasks->Waiting == 0) ML_CONTINUE(Tasks->Base.Caller, Tasks->Value);
}

ML_FUNCTIONX(Tasks) {
//!tasks
//<?Max:integer
//<?Min:integer
//>tasks
// Creates a new :mini:`tasks` set.
// If specified, at most :mini:`Max` functions will be called in parallel (the default is unlimited).
// If :mini:`Min` is also specified then the number of running tasks must drop below :mini:`Min` before more tasks are launched.
	ml_tasks_t *Tasks = new(ml_tasks_t);
	Tasks->Base.Type = MLTasksT;
	Tasks->Base.run = (void *)ml_tasks_continue;
	Tasks->Base.Caller = Caller;
	Tasks->Base.Context = Caller->Context;
	Tasks->Value = MLNil;
	Tasks->Waiting = 1;
	if (Count >= 2) {
		ML_CHECKX_ARG_TYPE(0, MLIntegerT);
		ML_CHECKX_ARG_TYPE(1, MLIntegerT);
		Tasks->Limit = ml_integer_value_fast(Args[1]);
		Tasks->Burst = ml_integer_value_fast(Args[0]) + 1;
	} else if (Count >= 1) {
		ML_CHECKX_ARG_TYPE(0, MLIntegerT);
		Tasks->Limit = ml_integer_value_fast(Args[0]);
		Tasks->Burst = SIZE_MAX;
	} else {
		Tasks->Limit = SIZE_MAX;
		Tasks->Burst = SIZE_MAX;
	}
	ML_RETURN(Tasks);
}

ML_METHODVX("add", MLTasksT, MLAnyT) {
//!tasks
//<Tasks
//<Function
//<?Args...
// Adds the function call :mini:`Function(Args...)` to a set of tasks.
// Adding a task to a completed tasks set returns an error.
	ml_tasks_t *Tasks = (ml_tasks_t *)Args[0];
	if (!Tasks->Waiting) ML_ERROR("TasksError", "Tasks have already completed");
	if (Tasks->Value != MLNil) ML_RETURN(Tasks->Value);
	ML_CHECKX_ARG_TYPE(Count - 1, MLFunctionT);
	ml_value_t *Function = Args[Count - 1];
	++Tasks->Waiting;
	ml_call(Tasks, Function, Count - 2, Args + 1);
	if (Tasks->Waiting > Tasks->Limit && !Tasks->Limited) {
		Tasks->Limited = Caller;
	} else {
		ML_RETURN(Tasks->Value);
	}
}

ML_METHODX("wait", MLTasksT) {
//!tasks
//<Tasks
//>nil | error
// Waits until all of the tasks in a tasks set have returned, or one of the tasks has returned an error (which is then returned from this call).
	ml_tasks_t *Tasks = (ml_tasks_t *)Args[0];
	Tasks->Base.Caller = Caller;
	Tasks->Base.Context = Caller->Context;
	ml_tasks_continue(Tasks, MLNil);
}

typedef struct ml_parallel_iter_t ml_parallel_iter_t;

typedef struct {
	ml_state_t Base;
	ml_state_t NextState[1];
	ml_state_t KeyState[1];
	ml_state_t ValueState[1];
	ml_value_t *Iter, *Function, *Error;
	ml_value_t *Args[2];
	size_t Waiting, Limit, Burst;
} ml_parallel_t;

static void parallel_iter_next(ml_state_t *State, ml_value_t *Iter) {
	ml_parallel_t *Parallel = (ml_parallel_t *)((char *)State - offsetof(ml_parallel_t, NextState));
	if (Parallel->Error) return;
	if (Iter == MLNil) {
		Parallel->Iter = NULL;
		ML_CONTINUE(Parallel, MLNil);
	}
	if (ml_is_error(Iter)) {
		Parallel->Error = Iter;
		ML_CONTINUE(Parallel->Base.Caller, Iter);
	}
	return ml_iter_key(Parallel->KeyState, Parallel->Iter = Iter);
}

static void parallel_iter_key(ml_state_t *State, ml_value_t *Value) {
	ml_parallel_t *Parallel = (ml_parallel_t *)((char *)State - offsetof(ml_parallel_t, KeyState));
	if (Parallel->Error) return;
	Parallel->Args[0] = Value;
	return ml_iter_value(Parallel->ValueState, Parallel->Iter);
}

static void parallel_iter_value(ml_state_t *State, ml_value_t *Value) {
	ml_parallel_t *Parallel = (ml_parallel_t *)((char *)State - offsetof(ml_parallel_t, ValueState));
	if (Parallel->Error) return;
	Parallel->Waiting += 1;
	Parallel->Args[1] = Value;
	ml_call(Parallel, Parallel->Function, 2, Parallel->Args);
	if (Parallel->Iter) {
		if (Parallel->Waiting > Parallel->Limit) return;
		return ml_iter_next(Parallel->NextState, Parallel->Iter);
	}
}

static void parallel_continue(ml_parallel_t *Parallel, ml_value_t *Value) {
	if (Parallel->Error) return;
	if (ml_is_error(Value)) {
		Parallel->Error = Value;
		ML_CONTINUE(Parallel->Base.Caller, Value);
	}
	--Parallel->Waiting;
	if (Parallel->Iter) {
		if (Parallel->Waiting > Parallel->Burst) return;
		return ml_iter_next(Parallel->NextState, Parallel->Iter);
	}
	if (Parallel->Waiting == 0) ML_CONTINUE(Parallel->Base.Caller, MLNil);
}

ML_FUNCTIONX(Parallel) {
//!tasks
//<Iteratable
//<Max:?integer
//<Min:?integer
//<Function:function
//>nil | error
// Iterates through :mini:`Iteratable` and calls :mini:`Function(Key, Value)` for each :mini:`Key, Value` pair produced **without** waiting for the call to return.
// The call to :mini:`parallel` returns when all calls to :mini:`Function` return, or an error occurs.
// If :mini:`Max` is given, at most :mini:`Max` calls to :mini:`Function` will run at a time by pausing iteration through :mini:`Iteratable`.
// If :mini:`Min` is also given then iteration will be resumed only when the number of calls to :mini:`Function` drops to :mini:`Min`.
	ML_CHECKX_ARG_COUNT(2);

	ml_parallel_t *Parallel = new(ml_parallel_t);
	Parallel->Base.Caller = Caller;
	Parallel->Base.run = (void *)parallel_continue;
	Parallel->Base.Context = Caller->Context;
	Parallel->Waiting = 1;
	Parallel->NextState->run = parallel_iter_next;
	Parallel->NextState->Context = Caller->Context;
	Parallel->KeyState->run = parallel_iter_key;
	Parallel->KeyState->Context = Caller->Context;
	Parallel->ValueState->run = parallel_iter_value;
	Parallel->ValueState->Context = Caller->Context;

	if (Count > 3) {
		ML_CHECKX_ARG_TYPE(1, MLIntegerT);
		ML_CHECKX_ARG_TYPE(2, MLIntegerT);
		ML_CHECKX_ARG_TYPE(3, MLFunctionT);
		Parallel->Limit = ml_integer_value_fast(Args[2]);
		Parallel->Burst = ml_integer_value_fast(Args[1]) + 1;
		Parallel->Function = Args[3];
	} else if (Count > 2) {
		ML_CHECKX_ARG_TYPE(1, MLIntegerT);
		ML_CHECKX_ARG_TYPE(2, MLFunctionT);
		Parallel->Limit = ml_integer_value_fast(Args[1]);
		Parallel->Burst = SIZE_MAX;
		Parallel->Function = Args[2];
	} else {
		ML_CHECKX_ARG_TYPE(1, MLFunctionT);
		Parallel->Limit = SIZE_MAX;
		Parallel->Burst = SIZE_MAX;
		Parallel->Function = Args[1];
	}

	return ml_iterate(Parallel->NextState, Args[0]);
}

typedef struct ml_unique_t {
	const ml_type_t *Type;
	ml_value_t *Iter;
} ml_unique_t;

ML_TYPE(MLUniqueT, (MLIteratableT), "unique");
//!internal

typedef struct ml_unique_state_t {
	ml_state_t Base;
	ml_value_t *Iter;
	ml_value_t *History;
	ml_value_t *Value;
	int Iteration;
} ml_unique_state_t;

ML_TYPE(MLUniqueStateT, (), "unique-state");
//!internal

static void ml_unique_fnx_iterate(ml_unique_state_t *State, ml_value_t *Value);

static void ml_unique_fnx_value(ml_unique_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (ml_map_insert(State->History, Value, MLSome) == MLNil) {
		State->Value = Value;
		++State->Iteration;
		ML_CONTINUE(State->Base.Caller, State);
	}
	State->Base.run = (void *)ml_unique_fnx_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void ml_unique_fnx_iterate(ml_unique_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)ml_unique_fnx_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

static void ML_TYPED_FN(ml_iterate, MLUniqueT, ml_state_t *Caller, ml_unique_t *Unique) {
	ml_unique_state_t *State = new(ml_unique_state_t);
	State->Base.Type = MLUniqueStateT;
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_unique_fnx_iterate;
	State->Base.Context = Caller->Context;
	State->History = ml_map();
	State->Iteration = 0;
	return ml_iterate((ml_state_t *)State, Unique->Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLUniqueStateT, ml_state_t *Caller, ml_unique_state_t *State) {
	ML_RETURN(ml_integer(State->Iteration));
}

static void ML_TYPED_FN(ml_iter_value, MLUniqueStateT, ml_state_t *Caller, ml_unique_state_t *State) {
	ML_RETURN(State->Value);
}

static void ML_TYPED_FN(ml_iter_next, MLUniqueStateT, ml_state_t *Caller, ml_unique_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_unique_fnx_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

ML_FUNCTION(Unique) {
//<Iteratable
//>iteratable
// Returns an iteratable that returns the unique values produced by :mini:`Iteratable` (based on inserting into a :mini:`map`).
	ML_CHECK_ARG_COUNT(1);
	ml_unique_t *Unique = new(ml_unique_t);
	Unique->Type = MLUniqueT;
	Unique->Iter = ml_chained(Count, Args);
	return (ml_value_t *)Unique;
}

typedef struct ml_zipped_t {
	const ml_type_t *Type;
	ml_value_t *Function;
	ml_value_t **Iters;
	int Count;
} ml_zipped_t;

ML_TYPE(MLZippedT, (MLIteratableT), "zipped");
//!internal

typedef struct ml_zipped_state_t {
	ml_state_t Base;
	ml_value_t *Function;
	ml_value_t **Iters;
	ml_value_t **Args;
	int Count, Index, Iteration;
} ml_zipped_state_t;

ML_TYPE(MLZippedStateT, (), "zipped-state");
//!internal

static void zipped_iterate(ml_zipped_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Iters[State->Index] = Value;
	if (++State->Index ==  State->Count) ML_CONTINUE(State->Base.Caller, State);
	return ml_iterate((ml_state_t *)State, State->Iters[State->Index]);
}

static void ML_TYPED_FN(ml_iterate, MLZippedT, ml_state_t *Caller, ml_zipped_t *Zipped) {
	ml_zipped_state_t *State = new(ml_zipped_state_t);
	State->Base.Type = MLZippedStateT;
	State->Base.Caller = Caller;
	State->Base.run = (void *)zipped_iterate;
	State->Base.Context = Caller->Context;
	State->Function = Zipped->Function;
	State->Iters = anew(ml_value_t *, Zipped->Count);
	State->Args = anew(ml_value_t *, Zipped->Count);
	for (int I = 0; I < Zipped->Count; ++I) State->Iters[I] = Zipped->Iters[I];
	State->Count = Zipped->Count;
	State->Iteration = 1;
	return ml_iterate((ml_state_t *)State, State->Iters[0]);
}

static void ML_TYPED_FN(ml_iter_key, MLZippedStateT, ml_state_t *Caller, ml_zipped_state_t *State) {
	ML_RETURN(ml_integer(State->Iteration));
}

static void ml_zipped_fnx_value(ml_zipped_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Args[State->Index] = Value;
	if (++State->Index ==  State->Count) {
		return ml_call(State->Base.Caller, State->Function, State->Count, State->Args);
	}
	return ml_iter_value((ml_state_t *)State, State->Iters[State->Index]);
}

static void ML_TYPED_FN(ml_iter_value, MLZippedStateT, ml_state_t *Caller, ml_zipped_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_zipped_fnx_value;
	State->Index = 0;
	return ml_iter_value((ml_state_t *)State, State->Iters[0]);
}

static void zipped_iter_next(ml_zipped_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Iters[State->Index] = Value;
	if (++State->Index ==  State->Count) ML_CONTINUE(State->Base.Caller, State);
	return ml_iter_next((ml_state_t *)State, State->Iters[State->Index]);
}

static void ML_TYPED_FN(ml_iter_next, MLZippedStateT, ml_state_t *Caller, ml_zipped_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)zipped_iter_next;
	++State->Iteration;
	State->Index = 0;
	return ml_iter_next((ml_state_t *)State, State->Iters[0]);
}

ML_FUNCTION(Zip) {
//@zip
//<Iteratable/1:iteratable
//<...:iteratable
//<Iteratable/n:iteratable
//<Function
//>iteratable
// Returns a new iteratable that draws values :mini:`V/i` from each of :mini:`Iteratable/i` and then produces :mini:`Functon(V/1, V/2, ..., V/n)`.
// The iteratable stops produces values when any of the :mini:`Iteratable/i` stops.
	ML_CHECK_ARG_COUNT(1);
	ml_zipped_t *Zipped = new(ml_zipped_t);
	Zipped->Type = MLZippedT;
	Zipped->Count = Count - 1;
	Zipped->Function = Args[Count - 1];
	Zipped->Iters = anew(ml_value_t *, Count - 1);
	for (int I = 0; I < Count - 1; ++I) Zipped->Iters[I] = Args[I];
	return (ml_value_t *)Zipped;
}

typedef struct ml_repeated_t {
	const ml_type_t *Type;
	ml_value_t *Value, *Update;
} ml_repeated_t;

ML_TYPE(MLRepeatedT, (MLIteratableT), "repeated");
//!internal

typedef struct ml_repeated_state_t {
	ml_state_t Base;
	ml_value_t *Value, *Update;
	int Iteration;
} ml_repeated_state_t;

ML_TYPE(MLRepeatedStateT, (), "repeated-state");
//!internal

static void repeated_update(ml_repeated_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Value = Value;
	ML_CONTINUE(State->Base.Caller, State);
}

static void ML_TYPED_FN(ml_iter_next, MLRepeatedStateT, ml_state_t *Caller, ml_repeated_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	++State->Iteration;
	if (State->Update) {
		return ml_call(State, State->Update, 1, &State->Value);
	} else {
		ML_RETURN(State);
	}
}

static void ML_TYPED_FN(ml_iter_key, MLRepeatedStateT, ml_state_t *Caller, ml_repeated_state_t *State) {
	ML_RETURN(ml_integer(State->Iteration));
}

static void ML_TYPED_FN(ml_iter_value, MLRepeatedStateT, ml_state_t *Caller, ml_repeated_state_t *State) {
	ML_RETURN(State->Value);
}

static void ML_TYPED_FN(ml_iterate, MLRepeatedT, ml_state_t *Caller, ml_repeated_t *Repeated) {
	ml_repeated_state_t *State = new(ml_repeated_state_t);
	State->Base.Type = MLRepeatedStateT;
	State->Base.run = (void *)repeated_update;
	State->Value = Repeated->Value;
	State->Update = Repeated->Update;
	State->Iteration = 1;
	ML_RETURN(State);
}

ML_METHOD("@", MLAnyT) {
//<Value
//>iteratable
// Returns an iteratable that repeatedly produces :mini:`Value`.
	ML_CHECK_ARG_COUNT(1);
	ml_repeated_t *Repeated = new(ml_repeated_t);
	Repeated->Type = MLRepeatedT;
	Repeated->Value = Args[0];
	return (ml_value_t *)Repeated;
}

ML_METHOD("@", MLAnyT, MLFunctionT) {
//<Value
//<Update:function
//>iteratable
// Returns an iteratable that repeatedly produces :mini:`Value`.
// :mini:`Value` is replaced with :mini:`Update(Value)` after each iteration.
	ML_CHECK_ARG_COUNT(1);
	ml_repeated_t *Repeated = new(ml_repeated_t);
	Repeated->Type = MLRepeatedT;
	Repeated->Value = Args[0];
	Repeated->Update = Args[1];
	return (ml_value_t *)Repeated;
}

typedef struct ml_sequenced_t {
	const ml_type_t *Type;
	ml_value_t *First, *Second;
} ml_sequenced_t;

ML_TYPE(MLSequencedT, (MLIteratableT), "sequenced");
//!internal

typedef struct ml_sequenced_state_t {
	ml_state_t Base;
	ml_value_t *Iter, *Next;
} ml_sequenced_state_t;

ML_TYPE(MLSequencedStateT, (), "sequenced-state");
//!internal

static void ml_sequenced_fnx_iterate(ml_sequenced_state_t *State, ml_value_t *Value);

static void ML_TYPED_FN(ml_iter_next, MLSequencedStateT, ml_state_t *Caller, ml_sequenced_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_sequenced_fnx_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLSequencedStateT, ml_state_t *Caller, ml_sequenced_state_t *State) {
	return ml_iter_key(Caller, State->Iter);
}

static void ML_TYPED_FN(ml_iter_value, MLSequencedStateT, ml_state_t *Caller, ml_sequenced_state_t *State) {
	return ml_iter_value(Caller, State->Iter);
}

static void ml_sequenced_fnx_iterate(ml_sequenced_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) {
		return ml_iterate(State->Base.Caller, State->Next);
	}
	State->Iter = Value;
	ML_CONTINUE(State->Base.Caller, State);
}

static void ML_TYPED_FN(ml_iterate, MLSequencedT, ml_state_t *Caller, ml_sequenced_t *Sequenced) {
	ml_sequenced_state_t *State = new(ml_sequenced_state_t);
	State->Base.Type = MLSequencedStateT;
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_sequenced_fnx_iterate;
	State->Next = Sequenced->Second;
	return ml_iterate((ml_state_t *)State, Sequenced->First);
}

ML_METHOD(">>", MLIteratableT, MLIteratableT) {
//<Iteratable/1
//<Iteratable/2
//>Iteratable
// Returns an iteratable that produces the values from :mini:`Iteratable/1` followed by those from :mini:`Iteratable/2`.
	ml_sequenced_t *Sequenced = xnew(ml_sequenced_t, 3, ml_value_t *);
	Sequenced->Type = MLSequencedT;
	Sequenced->First = Args[0];
	Sequenced->Second = Args[1];
	return (ml_value_t *)Sequenced;
}

ML_METHOD(">>", MLIteratableT) {
//<Iteratable
//>Iteratable
// Returns an iteratable that repeatedly produces the values from :mini:`Iteratable` (for use with :mini:`limit`).
	ml_sequenced_t *Sequenced = xnew(ml_sequenced_t, 3, ml_value_t *);
	Sequenced->Type = MLSequencedT;
	Sequenced->First = Args[0];
	Sequenced->Second = (ml_value_t *)Sequenced;
	return (ml_value_t *)Sequenced;
}

typedef struct ml_weaved_t {
	const ml_type_t *Type;
	int Count;
	ml_value_t *Iters[];
} ml_weaved_t;

ML_TYPE(MLWeavedT, (MLIteratableT), "weaved");
//!internal

typedef struct ml_weaved_state_t {
	ml_state_t Base;
	int Count, Index, Iteration;
	ml_value_t *Iters[];
} ml_weaved_state_t;

ML_TYPE(MLWeavedStateT, (), "weaved-state");
//!internal

static void weaved_iterate(ml_weaved_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Iters[State->Index] = Value;
	ML_CONTINUE(State->Base.Caller, State);
}

static void ML_TYPED_FN(ml_iterate, MLWeavedT, ml_state_t *Caller, ml_weaved_t *Weaved) {
	ml_weaved_state_t *State = xnew(ml_weaved_state_t, Weaved->Count, ml_value_t *);
	State->Base.Type = MLWeavedStateT;
	State->Base.Caller = Caller;
	State->Base.run = (void *)weaved_iterate;
	State->Base.Context = Caller->Context;
	for (int I = 0; I < Weaved->Count; ++I) State->Iters[I] = Weaved->Iters[I];
	State->Count = Weaved->Count;
	State->Iteration = 1;
	return ml_iterate((ml_state_t *)State, State->Iters[0]);
}

static void ML_TYPED_FN(ml_iter_key, MLWeavedStateT, ml_state_t *Caller, ml_weaved_state_t *State) {
	ML_RETURN(ml_integer(State->Iteration));
}

static void ML_TYPED_FN(ml_iter_value, MLWeavedStateT, ml_state_t *Caller, ml_weaved_state_t *State) {
	return ml_iter_value(Caller, State->Iters[State->Index]);
}

static void ML_TYPED_FN(ml_iter_next, MLWeavedStateT, ml_state_t *Caller, ml_weaved_state_t *State) {
	State->Base.Caller = Caller;
	if (++State->Index == State->Count) State->Index = 0;
	if (++State->Iteration > State->Count) {
		return ml_iter_next((ml_state_t *)State, State->Iters[State->Index]);
	} else {
		return ml_iterate((ml_state_t *)State, State->Iters[State->Index]);
	}
}

ML_FUNCTION(Weave) {
//@weave
//<Iteratable/1:iteratable
//<...:iteratable
//<Iteratable/n:iteratable
//>iteratable
// Returns a new iteratable that produces interleaved values :mini:`V/i` from each of :mini:`Iteratable/i`.
// The iteratable stops produces values when any of the :mini:`Iteratable/i` stops.
	ML_CHECK_ARG_COUNT(1);
	ml_weaved_t *Weaved = xnew(ml_weaved_t, Count, ml_value_t *);
	Weaved->Type = MLWeavedT;
	Weaved->Count = Count;
	for (int I = 0; I < Count; ++I) Weaved->Iters[I] = Args[I];
	return (ml_value_t *)Weaved;
}

typedef struct {
	const ml_type_t *Type;
	ml_value_t *Value;
} ml_swapped_t;

ML_TYPE(MLSwappedT, (MLIteratableT), "swapped");
//!internal

typedef struct {
	ml_state_t Base;
	ml_value_t *Iter;
} ml_swapped_state_t;

ML_TYPE(MLSwappedStateT, (), "swapped-state");
//!internal

static void swapped_iterate(ml_swapped_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Iter = Value;
	ML_CONTINUE(State->Base.Caller, State);
}

static void ML_TYPED_FN(ml_iterate, MLSwappedT, ml_state_t *Caller, ml_swapped_t *Swapped) {
	ml_swapped_state_t *State = new(ml_swapped_state_t);
	State->Base.Caller = Caller;
	State->Base.Type = MLSwappedStateT;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)swapped_iterate;
	return ml_iterate((ml_state_t *)State, Swapped->Value);
}

static void ML_TYPED_FN(ml_iter_key, MLSwappedStateT, ml_state_t *Caller, ml_swapped_state_t *State) {
	return ml_iter_value(Caller, State->Iter);
}

static void ML_TYPED_FN(ml_iter_value, MLSwappedStateT, ml_state_t *Caller, ml_swapped_state_t *State) {
	return ml_iter_key(Caller, State->Iter);
}

static void ML_TYPED_FN(ml_iter_next, MLSwappedStateT, ml_state_t *Caller, ml_swapped_state_t *State) {
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

ML_FUNCTION(Swap) {
//@swap
//<Iteratable:iteratable
// Returns a new iteratable which swaps the keys and values produced by :mini:`Iteratable`.
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLIteratableT);
	ml_swapped_t *Swapped = new(ml_swapped_t);
	Swapped->Type = MLSwappedT;
	Swapped->Value = Args[0];
	return (ml_value_t *)Swapped;
}

typedef struct {
	const ml_type_t *Type;
	ml_value_t *Value;
} ml_key_t;

ML_TYPE(MLKeyT, (MLIteratableT), "key");
//!internal

typedef struct {
	ml_state_t Base;
	ml_value_t *Iter;
	int Iteration;
} ml_key_state_t;

ML_TYPE(MLKeyStateT, (), "keys-state");
//!internal

static void key_iterate(ml_key_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Iter = Value;
	++State->Iteration;
	ML_CONTINUE(State->Base.Caller, State);
}

static void ML_TYPED_FN(ml_iterate, MLKeyT, ml_state_t *Caller, ml_key_t *Key) {
	ml_key_state_t *State = new(ml_key_state_t);
	State->Base.Caller = Caller;
	State->Base.Type = MLKeyStateT;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)key_iterate;
	return ml_iterate((ml_state_t *)State, Key->Value);
}

static void ML_TYPED_FN(ml_iter_key, MLKeyStateT, ml_state_t *Caller, ml_key_state_t *State) {
	ML_RETURN(ml_integer(State->Iteration));
}

static void ML_TYPED_FN(ml_iter_value, MLKeyStateT, ml_state_t *Caller, ml_key_state_t *State) {
	return ml_iter_key(Caller, State->Iter);
}

static void ML_TYPED_FN(ml_iter_next, MLKeyStateT, ml_state_t *Caller, ml_key_state_t *State) {
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

ML_FUNCTION(Key) {
//@key
//<Iteratable:iteratable
// Returns a new iteratable which produces the keys of :mini:`Iteratable`.
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLIteratableT);
	ml_key_t *Key = new(ml_key_t);
	Key->Type = MLKeyT;
	Key->Value = Args[0];
	return (ml_value_t *)Key;
}

void ml_iterfns_init(stringmap_t *Globals) {
	FilterNil = new(ml_filter_t);
	FilterNil->Type = FilterT;
	FilterNil->Function = ml_integer(1);
#include "ml_iterfns_init.c"
	stringmap_insert(Globals, "filter", Filter);
	stringmap_insert(Globals, "first", First);
	stringmap_insert(Globals, "first2", First2);
	stringmap_insert(Globals, "last", Last);
	stringmap_insert(Globals, "last2", Last2);
	stringmap_insert(Globals, "all", All);
	stringmap_insert(Globals, "all2", All2);
	stringmap_insert(Globals, "count", Count);
	stringmap_insert(Globals, "count2", Count2);
	stringmap_insert(Globals, "reduce", Reduce);
	stringmap_insert(Globals, "min", Min);
	stringmap_insert(Globals, "max", Max);
	stringmap_insert(Globals, "sum", Sum);
	stringmap_insert(Globals, "prod", Prod);
	stringmap_insert(Globals, "reduce2", Reduce2);
	stringmap_insert(Globals, "min2", Min2);
	stringmap_insert(Globals, "max2", Max2);
	stringmap_insert(Globals, "parallel", Parallel);
	stringmap_insert(Globals, "unique", Unique);
	stringmap_insert(Globals, "tasks", Tasks);
	stringmap_insert(Globals, "zip", Zip);
	stringmap_insert(Globals, "weave", Weave);
	stringmap_insert(Globals, "swap", Swap);
	stringmap_insert(Globals, "key", Key);
}
