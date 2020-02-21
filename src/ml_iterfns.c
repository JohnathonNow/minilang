#include <gc.h>
#include "ml_runtime.h"
#include <string.h>
#include "minilang.h"
#include "ml_macros.h"
#include "ml_iterfns.h"

typedef struct ml_frame_iter_t {
	ml_state_t Base;
	ml_value_t *Iter;
	ml_value_t *Values[];
} ml_frame_iter_t;

static ml_value_t *ml_all_fnx_get_value(ml_frame_iter_t *Frame, ml_value_t *Result);

static ml_value_t *ml_all_fnx_append_value(ml_frame_iter_t *Frame, ml_value_t *Result) {
	Result = Result->Type->deref(Result);
	if (Result->Type == MLErrorT) ML_CONTINUE(Frame->Base.Caller, Result);
	ml_list_append(Frame->Values[0], Result);
	Frame->Base.run = (void *)ml_all_fnx_get_value;
	return ml_iter_next((ml_state_t *)Frame, Frame->Iter);
}

static ml_value_t *ml_all_fnx_get_value(ml_frame_iter_t *Frame, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(Frame->Base.Caller, Result);
	Frame->Base.run = (void *)ml_all_fnx_append_value;
	if (Result == MLNil) ML_CONTINUE(Frame->Base.Caller, Frame->Values[0]);
	return ml_iter_value((ml_state_t *)Frame, Frame->Iter = Result);
}

static ml_value_t *ml_all_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_frame_iter_t *Frame = xnew(ml_frame_iter_t, 1, ml_value_t *);
	Frame->Base.Caller = Caller;
	Frame->Base.run = (void *)ml_all_fnx_get_value;
	Frame->Values[0] = ml_list();
	return ml_iterate((ml_state_t *)Frame, Args[0]);
}

static ml_value_t *ml_map_fnx_get_key(ml_frame_iter_t *Frame, ml_value_t *Result);

static ml_value_t *ml_map_fnx_get_value(ml_frame_iter_t *Frame, ml_value_t *Result);

static ml_value_t *ml_map_fnx_insert_key_value(ml_frame_iter_t *Frame, ml_value_t *Result);

static ml_value_t *ml_map_fnx_get_key(ml_frame_iter_t *Frame, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(Frame->Base.Caller, Result);
	Frame->Base.run = (void *)ml_map_fnx_get_value;
	if (Result == MLNil) ML_CONTINUE(Frame->Base.Caller, Frame->Values[0]);
	return ml_iter_key((ml_state_t *)Frame, Frame->Iter = Result);
}

static ml_value_t *ml_map_fnx_get_value(ml_frame_iter_t *Frame, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(Frame->Base.Caller, Result);
	Frame->Base.run = (void *)ml_map_fnx_insert_key_value;
	if (Result == MLNil) ML_CONTINUE(Frame->Base.Caller, Frame->Values[0]);
	Frame->Values[1] = Result;
	return ml_iter_value((ml_state_t *)Frame, Frame->Iter);
}

static ml_value_t *ml_map_fnx_insert_key_value(ml_frame_iter_t *Frame, ml_value_t *Result) {
	Result = Result->Type->deref(Result);
	if (Result->Type == MLErrorT) ML_CONTINUE(Frame->Base.Caller, Result);
	ml_map_insert(Frame->Values[0], Frame->Values[1], Result);
	Frame->Base.run = (void *)ml_map_fnx_get_key;
	return ml_iter_next((ml_state_t *)Frame, Frame->Iter);
}

static ml_value_t *ml_map_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_frame_iter_t *Frame = xnew(ml_frame_iter_t, 1, ml_value_t *);
	Frame->Base.Caller = Caller;
	Frame->Base.run = (void *)ml_map_fnx_get_key;
	Frame->Values[0] = ml_map();
	return ml_iterate((ml_state_t *)Frame, Args[0]);
}

typedef struct ml_count_state_t {
	ml_state_t Base;
	ml_value_t *Iter;
	long Count;
} ml_count_state_t;

static ml_value_t *ml_count_fnx_increment(ml_count_state_t *Frame, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(Frame->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(Frame->Base.Caller, ml_integer(Frame->Count));
	++Frame->Count;
	return ml_iter_next((ml_state_t *)Frame, Frame->Iter = Result);
}

static ml_value_t *ml_count_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ml_count_state_t *Frame = xnew(ml_count_state_t, 1, ml_value_t *);
	Frame->Base.Caller = Caller;
	Frame->Base.run = (void *)ml_count_fnx_increment;
	Frame->Count = 0;
	return ml_iterate((ml_state_t *)Frame, Args[0]);
}

static ml_value_t *LessMethod, *GreaterMethod, *AddMethod, *MulMethod;

static ml_value_t *ml_fold_fnx_get_next(ml_frame_iter_t *Frame, ml_value_t *Result);

static ml_value_t *ml_fold_fnx_result(ml_frame_iter_t *Frame, ml_value_t *Result) {
	Result = Result->Type->deref(Result);
	if (Result->Type == MLErrorT) ML_CONTINUE(Frame->Base.Caller, Result);
	if (Result != MLNil) Frame->Values[1] = Result;
	Frame->Base.run = (void *)ml_fold_fnx_get_next;
	return ml_iter_next((ml_state_t *)Frame, Frame->Iter);
}

static ml_value_t *ml_fold_fnx_fold(ml_frame_iter_t *Frame, ml_value_t *Result) {
	Result = Result->Type->deref(Result);
	if (Result->Type == MLErrorT) ML_CONTINUE(Frame->Base.Caller, Result);
	ml_value_t *Compare = Frame->Values[0];
	Frame->Values[2] = Result;
	Frame->Base.run = (void *)ml_fold_fnx_result;
	return Compare->Type->call((ml_state_t *)Frame, Compare, 2, Frame->Values + 1);
}

static ml_value_t *ml_fold_fnx_get_next(ml_frame_iter_t *Frame, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(Frame->Base.Caller, Result);
	Frame->Base.run = (void *)ml_fold_fnx_fold;
	if (Result == MLNil) ML_CONTINUE(Frame->Base.Caller, Frame->Values[1] ?: MLNil);
	return ml_iter_value((ml_state_t *)Frame, Frame->Iter = Result);
}

static ml_value_t *ml_fold_fnx_first(ml_frame_iter_t *Frame, ml_value_t *Result) {
	Result = Result->Type->deref(Result);
	if (Result->Type == MLErrorT) ML_CONTINUE(Frame->Base.Caller, Result);
	Frame->Values[1] = Result;
	Frame->Base.run = (void *)ml_fold_fnx_get_next;
	return ml_iter_next((ml_state_t *)Frame, Frame->Iter);
}

static ml_value_t *ml_fold_fnx_get_first(ml_frame_iter_t *Frame, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(Frame->Base.Caller, Result);
	Frame->Base.run = (void *)ml_fold_fnx_first;
	if (Result == MLNil) ML_CONTINUE(Frame->Base.Caller, Frame->Values[1] ?: MLNil);
	return ml_iter_value((ml_state_t *)Frame, Frame->Iter = Result);
}

static ml_value_t *ml_min_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_frame_iter_t *Frame = xnew(ml_frame_iter_t, 3, ml_value_t *);
	Frame->Base.Caller = Caller;
	Frame->Base.run = (void *)ml_fold_fnx_get_first;
	Frame->Values[0] = GreaterMethod;
	return ml_iterate((ml_state_t *)Frame, Args[0]);
}

static ml_value_t *ml_max_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_frame_iter_t *Frame = xnew(ml_frame_iter_t, 3, ml_value_t *);
	Frame->Base.Caller = Caller;
	Frame->Base.run = (void *)ml_fold_fnx_get_first;
	Frame->Values[0] = LessMethod;
	return ml_iterate((ml_state_t *)Frame, Args[0]);
}

static ml_value_t *ml_sum_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_frame_iter_t *Frame = xnew(ml_frame_iter_t, 3, ml_value_t *);
	Frame->Base.Caller = Caller;
	Frame->Base.run = (void *)ml_fold_fnx_get_first;
	Frame->Values[0] = AddMethod;
	return ml_iterate((ml_state_t *)Frame, Args[0]);
}

static ml_value_t *ml_prod_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_frame_iter_t *Frame = xnew(ml_frame_iter_t, 3, ml_value_t *);
	Frame->Base.Caller = Caller;
	Frame->Base.run = (void *)ml_fold_fnx_get_first;
	Frame->Values[0] = MulMethod;
	return ml_iterate((ml_state_t *)Frame, Args[0]);
}

static ml_value_t *ml_fold_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(2);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ML_CHECKX_ARG_TYPE(1, MLFunctionT);
	ml_frame_iter_t *Frame = xnew(ml_frame_iter_t, 3, ml_value_t *);
	Frame->Base.Caller = Caller;
	Frame->Base.run = (void *)ml_fold_fnx_get_first;
	Frame->Values[0] = Args[1];
	return ml_iterate((ml_state_t *)Frame, Args[0]);
}

typedef struct ml_limited_t {
	const ml_type_t *Type;
	ml_value_t *Value;
	int Remaining;
} ml_limited_t;

static ml_type_t *MLLimitedT;

typedef struct ml_limited_state_t {
	ml_state_t Base;
	ml_value_t *Iter;
	int Remaining;
} ml_limited_state_t;

static ml_type_t *MLLimitedStateT;

static ml_value_t *ml_limited_fnx_iterate(ml_limited_state_t *State, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, Result);
	State->Iter = Result;
	--State->Remaining;
	ML_CONTINUE(State->Base.Caller, State);
}

static ml_value_t *ml_limited_iterate(ml_state_t *Caller, ml_limited_t *Limited) {
	if (Limited->Remaining) {
		ml_limited_state_t *State = new(ml_limited_state_t);
		State->Base.Type = MLLimitedStateT;
		State->Base.Caller = Caller;
		State->Base.run = (void *)ml_limited_fnx_iterate;
		State->Remaining = Limited->Remaining;
		return ml_iterate((ml_state_t *)State, Limited->Value);
	} else {
		ML_CONTINUE(Caller, MLNil);
	}
}

static ml_value_t *ml_limited_state_key(ml_state_t *Caller, ml_limited_state_t *State) {
	return ml_iter_key(Caller, State->Iter);
}

static ml_value_t *ml_limited_state_value(ml_state_t *Caller, ml_limited_state_t *State) {
	return ml_iter_value(Caller, State->Iter);
}

static ml_value_t *ml_limited_state_next(ml_state_t *Caller, ml_limited_state_t *State) {
	if (State->Remaining) {
		State->Base.Caller = Caller;
		State->Base.run = (void *)ml_limited_fnx_iterate;
		return ml_iter_next((ml_state_t *)State, State->Iter);
	} else {
		ML_CONTINUE(Caller, MLNil);
	}
}

ML_METHOD("limit", MLIteratableT, MLIntegerT) {
	ml_limited_t *Limited = new(ml_limited_t);
	Limited->Type = MLLimitedT;
	Limited->Value = Args[0];
	Limited->Remaining = ml_integer_value(Args[1]);
	return (ml_value_t *)Limited;
}

typedef struct ml_skipped_t {
	const ml_type_t *Type;
	ml_value_t *Value;
	long Remaining;
} ml_skipped_t;

static ml_type_t *MLSkippedT;

typedef struct ml_skipped_state_t {
	ml_state_t Base;
	long Remaining;
} ml_skipped_state_t;

static ml_value_t *ml_skipped_fnx_iterate(ml_skipped_state_t *State, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, Result);
	if (State->Remaining) {
		--State->Remaining;
		return ml_iter_next((ml_state_t *)State, Result);
	} else {
		ML_CONTINUE(State->Base.Caller, Result);
	}
}

static ml_value_t *ml_skipped_iterate(ml_state_t *Caller, ml_skipped_t *Skipped) {
	if (Skipped->Remaining) {
		ml_skipped_state_t *State = new(ml_skipped_state_t);
		State->Base.Caller = Caller;
		State->Base.run = (void *)ml_skipped_fnx_iterate;
		State->Remaining = Skipped->Remaining;
		return ml_iterate((ml_state_t *)State, Skipped->Value);
	} else {
		return ml_iterate(Caller, Skipped->Value);
	}
}

ML_METHOD("skip", MLIteratableT, MLIntegerT) {
	ml_skipped_t *Skipped = new(ml_skipped_t);
	Skipped->Type = MLSkippedT;
	Skipped->Value = Args[0];
	Skipped->Remaining = ml_integer_value(Args[1]);
	return (ml_value_t *)Skipped;
}

typedef struct {
	ml_state_t Base;
	size_t Waiting;
} ml_tasks_t;

static ml_type_t *MLTasksT;

static ml_value_t *ml_tasks_continue(ml_tasks_t *Tasks, ml_value_t *Value) {
	if (Value->Type == MLErrorT) {
		Tasks->Waiting = 0xFFFFFFFF;
		ML_CONTINUE(Tasks->Base.Caller, Value);
	}
	if (--Tasks->Waiting == 0) ML_CONTINUE(Tasks->Base.Caller, MLNil);
	return MLNil;
}

static ml_value_t *ml_tasks_fn(void *Data, int Count, ml_value_t **Args) {
	ml_tasks_t *Tasks = new(ml_tasks_t);
	Tasks->Base.Type = MLTasksT;
	Tasks->Base.run = (void *)ml_tasks_continue;
	Tasks->Waiting = 1;
	return (ml_value_t *)Tasks;
}

static ml_value_t *ml_tasks_call(ml_state_t *Caller, ml_tasks_t *Tasks, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_TYPE(Count - 1, MLFunctionT);
	ml_value_t *Function = Args[Count - 1];
	++Tasks->Waiting;
	Function->Type->call((ml_state_t *)Tasks, Function, Count - 1, Args);
	ML_CONTINUE(Caller, Tasks);
}

ML_METHODX("wait", MLTasksT) {
	ml_tasks_t *Tasks = (ml_tasks_t *)Args[0];
	Tasks->Base.Caller = Caller;
	if (--Tasks->Waiting == 0) ML_CONTINUE(Tasks->Base.Caller, MLNil);
	return MLNil;
}

typedef struct ml_parallel_iter_t ml_parallel_iter_t;

typedef struct {
	ml_state_t Base;
	ml_parallel_iter_t *Iter;
	size_t Waiting, Limit, Burst;
} ml_parallel_t;

struct ml_parallel_iter_t {
	ml_state_t Base;
	ml_value_t *Iter;
	ml_value_t *Function;
	ml_value_t *Args[2];
};

static ml_value_t *ml_parallel_iterate(ml_parallel_iter_t *State, ml_value_t *Iter);

static ml_value_t *ml_parallel_iter_value(ml_parallel_iter_t *State, ml_value_t *Value) {
	ml_parallel_t *Parallel = (ml_parallel_t *)State->Base.Caller;
	Parallel->Waiting += 1;
	State->Args[1] = Value;
	State->Function->Type->call((ml_state_t *)Parallel, State->Function, 2, State->Args);
	State->Base.run = (void *)ml_parallel_iterate;
	if (Parallel->Waiting > Parallel->Limit) return MLNil;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static ml_value_t *ml_parallel_iter_key(ml_parallel_iter_t *State, ml_value_t *Value) {
	State->Args[0] = Value;
	State->Base.run = (void *)ml_parallel_iter_value;
	return ml_iter_value((ml_state_t *)State, State->Iter);
}

static ml_value_t *ml_parallel_iterate(ml_parallel_iter_t *State, ml_value_t *Iter) {
	if (Iter == MLNil) {
		ml_parallel_t *Parallel = (ml_parallel_t *)State->Base.Caller;
		Parallel->Iter = NULL;
		ML_CONTINUE(Parallel, MLNil);
	}
	if (Iter->Type == MLErrorT) ML_CONTINUE(State->Base.Caller, Iter);
	State->Base.run = (void *)ml_parallel_iter_key;
	return ml_iter_key((ml_state_t *)State, State->Iter = Iter);
}

static ml_value_t *ml_parallel_continue(ml_parallel_t *Parallel, ml_value_t *Value) {
	if (Value->Type == MLErrorT) {
		Parallel->Waiting = 0xFFFFFFFF;
		ML_CONTINUE(Parallel->Base.Caller, Value);
	}
	--Parallel->Waiting;
	if (Parallel->Iter) {
		if (Parallel->Waiting > Parallel->Burst) return MLNil;
		return ml_iter_next(Parallel->Iter, Parallel->Iter->Iter);
	}
	if (Parallel->Waiting == 0) ML_CONTINUE(Parallel->Base.Caller, MLNil);
	return MLNil;
}

static ml_value_t *ml_parallel_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(2);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);

	ml_parallel_t *S0 = new(ml_parallel_t);
	S0->Base.Caller = Caller;
	S0->Base.run = (void *)ml_parallel_continue;
	S0->Waiting = 1;

	ml_parallel_iter_t *S1 = new(ml_parallel_iter_t);
	S1->Base.Caller = (ml_state_t *)S0;
	S1->Base.run = (void *)ml_parallel_iterate;
	S0->Iter = S1;

	if (Count > 3) {
		ML_CHECKX_ARG_TYPE(1, MLIntegerT);
		ML_CHECKX_ARG_TYPE(2, MLIntegerT);
		ML_CHECKX_ARG_TYPE(3, MLFunctionT);
		S0->Limit = ml_integer_value(Args[2]);
		S0->Burst = ml_integer_value(Args[1]) + 1;
		S1->Function = Args[3];
	} else if (Count > 2) {
		ML_CHECKX_ARG_TYPE(1, MLIntegerT);
		ML_CHECKX_ARG_TYPE(2, MLFunctionT);
		S0->Limit = ml_integer_value(Args[1]);
		S0->Burst = 0xFFFFFFFF;
		S1->Function = Args[2];
	} else {
		ML_CHECKX_ARG_TYPE(1, MLFunctionT);
		S0->Limit = 0xFFFFFFFF;
		S0->Burst = 0xFFFFFFFF;
		S1->Function = Args[1];
	}

	return ml_iterate((ml_state_t *)S1, Args[0]);
}

typedef struct ml_unique_t {
	const ml_type_t *Type;
	ml_value_t *Iter;
} ml_unique_t;

static ml_type_t *MLUniqueT;

typedef struct ml_unique_state_t {
	ml_state_t Base;
	ml_value_t *Iter;
	ml_value_t *History;
	ml_value_t *Value;
	int Iteration;
} ml_unique_state_t;

static ml_type_t *MLUniqueStateT;

static ml_value_t *ml_unique_fnx_iterate(ml_unique_state_t *State, ml_value_t *Result);

static ml_value_t *ml_unique_fnx_value(ml_unique_state_t *State, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(State->Base.Caller, Result);
	if (!ml_map_insert(State->History, Result, MLNil)) {
		State->Value = Result;
		++State->Iteration;
		ML_CONTINUE(State->Base.Caller, State);
	}
	State->Base.run = (void *)ml_unique_fnx_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static ml_value_t *ml_unique_fnx_iterate(ml_unique_state_t *State, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, Result);
	State->Base.run = (void *)ml_unique_fnx_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Result);
}

static ml_value_t *ml_unique_iterate(ml_state_t *Caller, ml_unique_t *Unique) {
	ml_unique_state_t *State = new(ml_unique_state_t);
	State->Base.Type = MLUniqueStateT;
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_unique_fnx_iterate;
	State->History = ml_map();
	State->Iteration = 0;
	return ml_iterate((ml_state_t *)State, Unique->Iter);
}

static ml_value_t *ml_unique_key(ml_state_t *Caller, ml_unique_state_t *State) {
	ML_CONTINUE(Caller, ml_integer(State->Iteration));
}

static ml_value_t *ml_unique_value(ml_state_t *Caller, ml_unique_state_t *State) {
	ML_CONTINUE(Caller, State->Value);
}

static ml_value_t *ml_unique_next(ml_state_t *Caller, ml_unique_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_unique_fnx_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static ml_value_t *ml_unique_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ml_unique_t *Unique = new(ml_unique_t);
	Unique->Type = MLUniqueT;
	Unique->Iter = Args[0];
	return (ml_value_t *)Unique;
}

typedef struct ml_grouped_t {
	const ml_type_t *Type;
	ml_value_t *Function;
	ml_value_t **Iters;
	int Count;
} ml_grouped_t;

static ml_type_t *MLGroupedT;

typedef struct ml_grouped_state_t {
	ml_state_t Base;
	ml_value_t *Function;
	ml_value_t **Iters;
	ml_value_t **Args;
	int Count, Index, Iteration;
} ml_grouped_state_t;

static ml_type_t *MLGroupedStateT;

static ml_value_t *ml_grouped_fnx_iterate(ml_grouped_state_t *State, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, Result);
	State->Iters[State->Index] = Result;
	if (++State->Index ==  State->Count) ML_CONTINUE(State->Base.Caller, State);
	return ml_iterate((ml_state_t *)State, State->Iters[State->Index]);
}

static ml_value_t *ml_grouped_iterate(ml_state_t *Caller, ml_grouped_t *Grouped) {
	ml_grouped_state_t *State = new(ml_grouped_state_t);
	State->Base.Type = MLGroupedStateT;
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_grouped_fnx_iterate;
	State->Function = Grouped->Function;
	State->Iters = anew(ml_value_t *, Grouped->Count);
	State->Args = anew(ml_value_t *, Grouped->Count);
	for (int I = 0; I < Grouped->Count; ++I) State->Iters[I] = Grouped->Iters[I];
	State->Count = Grouped->Count;
	State->Iteration = 1;
	return ml_iterate((ml_state_t *)State, State->Iters[0]);
}

static ml_value_t *ml_grouped_key(ml_state_t *Caller, ml_grouped_state_t *State) {
	ML_CONTINUE(Caller, ml_integer(State->Iteration));
}

static ml_value_t *ml_grouped_fnx_value(ml_grouped_state_t *State, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(State->Base.Caller, Result);
	State->Args[State->Index] = Result;
	if (++State->Index ==  State->Count) {
		return State->Function->Type->call(State->Base.Caller, State->Function, State->Count, State->Args);
	}
	return ml_iter_value((ml_state_t *)State, State->Iters[State->Index]);
}

static ml_value_t *ml_grouped_value(ml_state_t *Caller, ml_grouped_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_grouped_fnx_value;
	State->Index = 0;
	return ml_iter_value((ml_state_t *)State, State->Iters[0]);
}

static ml_value_t *ml_grouped_fnx_next(ml_grouped_state_t *State, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, Result);
	State->Iters[State->Index] = Result;
	if (++State->Index ==  State->Count) ML_CONTINUE(State->Base.Caller, State);
	return ml_iter_next((ml_state_t *)State, State->Iters[State->Index]);
}

static ml_value_t *ml_grouped_next(ml_state_t *Caller, ml_grouped_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_grouped_fnx_next;
	++State->Iteration;
	State->Index = 0;
	return ml_iter_next((ml_state_t *)State, State->Iters[0]);
}

static ml_value_t *ml_group_fn(void *Data, int Count, ml_value_t **Args) {
	ml_grouped_t *Grouped = new(ml_grouped_t);
	Grouped->Type = MLGroupedT;
	Grouped->Count = Count - 1;
	Grouped->Function = Args[Count - 1];
	Grouped->Iters = anew(ml_value_t *, Count - 1);
	for (int I = 0; I < Count - 1; ++I) Grouped->Iters[I] = Args[I];
	return (ml_value_t *)Grouped;
}

typedef struct ml_repeated_t {
	const ml_type_t *Type;
	ml_value_t *Value, *Function;
} ml_repeated_t;

static ml_type_t *MLRepeatedT;

typedef struct ml_repeated_state_t {
	ml_state_t Base;
	ml_value_t *Value, *Function;
	int Iteration;
} ml_repeated_state_t;

static ml_type_t *MLRepeatedStateT;

static ml_value_t *ml_repeated_fnx_value(ml_repeated_state_t *State, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(State->Base.Caller, Result);
	State->Value = Result;
	++State->Iteration;
	ML_CONTINUE(State->Base.Caller, State);
}

static ml_value_t *ml_repeated_next(ml_state_t *Caller, ml_repeated_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_repeated_fnx_value;
	return State->Function->Type->call((ml_state_t *)State, State->Function, 1, &State->Value);
}

static ml_value_t *ml_repeated_key(ml_state_t *Caller, ml_repeated_state_t *State) {
	ML_CONTINUE(Caller, ml_integer(State->Iteration));
}

static ml_value_t *ml_repeated_value(ml_state_t *Caller, ml_repeated_state_t *State) {
	ML_CONTINUE(Caller, State->Value);
}

static ml_value_t *ml_repeated_iterate(ml_state_t *Caller, ml_repeated_t *Repeated) {
	ml_repeated_state_t *State = new(ml_repeated_state_t);
	State->Base.Type = MLRepeatedStateT;
	State->Value = Repeated->Value;
	State->Function = Repeated->Function;
	State->Iteration = 1;
	ML_CONTINUE(Caller, State);
}

static ml_value_t *ml_repeat_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ml_repeated_t *Repeated = new(ml_repeated_t);
	Repeated->Type = MLRepeatedT;
	Repeated->Value = Args[0];
	Repeated->Function = Count > 1 ? Args[1] : ml_integer(1);
	return (ml_value_t *)Repeated;
}

typedef struct ml_sequenced_t {
	const ml_type_t *Type;
	ml_value_t *First, *Second;
} ml_sequenced_t;

static ml_type_t *MLSequencedT;

typedef struct ml_sequenced_state_t {
	ml_state_t Base;
	ml_value_t *Iter, *Next;
} ml_sequenced_state_t;

static ml_type_t *MLSequencedStateT;

static ml_value_t *ml_sequenced_fnx_iterate(ml_sequenced_state_t *State, ml_value_t *Result);

static ml_value_t *ml_sequenced_next(ml_state_t *Caller, ml_sequenced_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_sequenced_fnx_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static ml_value_t *ml_sequenced_key(ml_state_t *Caller, ml_sequenced_state_t *State) {
	return ml_iter_key(Caller, State->Iter);
}

static ml_value_t *ml_sequenced_value(ml_state_t *Caller, ml_sequenced_state_t *State) {
	return ml_iter_value(Caller, State->Iter);
}

static ml_value_t *ml_sequenced_fnx_iterate(ml_sequenced_state_t *State, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) {
		return ml_iterate(State->Base.Caller, State->Next);
	}
	State->Iter = Result;
	ML_CONTINUE(State->Base.Caller, State);
}

static ml_value_t *ml_sequenced_iterate(ml_state_t *Caller, ml_sequenced_t *Sequenced) {
	ml_sequenced_state_t *State = new(ml_sequenced_state_t);
	State->Base.Type = MLSequencedStateT;
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_sequenced_fnx_iterate;
	State->Next = Sequenced->Second;
	return ml_iterate((ml_state_t *)State, Sequenced->First);
}

ML_METHOD("||", MLIteratableT, MLIteratableT) {
	ml_sequenced_t *Sequenced = xnew(ml_sequenced_t, 3, ml_value_t *);
	Sequenced->Type = MLSequencedT;
	Sequenced->First = Args[0];
	Sequenced->Second = Args[1];
	return (ml_value_t *)Sequenced;
}

ML_METHOD("||", MLIteratableT) {
	ml_sequenced_t *Sequenced = xnew(ml_sequenced_t, 3, ml_value_t *);
	Sequenced->Type = MLSequencedT;
	Sequenced->First = Args[0];
	Sequenced->Second = (ml_value_t *)Sequenced;
	return (ml_value_t *)Sequenced;
}

void ml_iterfns_init(stringmap_t *Globals) {
	LessMethod = ml_method("<");
	GreaterMethod = ml_method(">");
	AddMethod = ml_method("+");
	MulMethod = ml_method("*");
	//stringmap_insert(Globals, "each", ml_functionx(0, ml_each_fnx));
	stringmap_insert(Globals, "all", ml_functionx(0, ml_all_fnx));
	stringmap_insert(Globals, "map", ml_functionx(0, ml_map_fnx));
	stringmap_insert(Globals, "unique", ml_function(0, ml_unique_fn));
	stringmap_insert(Globals, "count", ml_functionx(0, ml_count_fnx));
	stringmap_insert(Globals, "min", ml_functionx(0, ml_min_fnx));
	stringmap_insert(Globals, "max", ml_functionx(0, ml_max_fnx));
	stringmap_insert(Globals, "sum", ml_functionx(0, ml_sum_fnx));
	stringmap_insert(Globals, "prod", ml_functionx(0, ml_prod_fnx));
	stringmap_insert(Globals, "fold", ml_functionx(0, ml_fold_fnx));
	stringmap_insert(Globals, "parallel", ml_functionx(0, ml_parallel_fnx));
	stringmap_insert(Globals, "tasks", ml_function(0, ml_tasks_fn));
	stringmap_insert(Globals, "group", ml_function(0, ml_group_fn));
	stringmap_insert(Globals, "repeat", ml_function(0, ml_repeat_fn));

	stringmap_insert(Globals, "tuple", ml_function(0, ml_tuple_new));
	stringmap_insert(Globals, "list", ml_function(0, ml_list_new));

	MLLimitedT = ml_type(MLIteratableT, "limited");
	MLLimitedStateT = ml_type(MLAnyT, "limited-state");
	ml_typed_fn_set(MLLimitedT, ml_iterate, ml_limited_iterate);
	ml_typed_fn_set(MLLimitedStateT, ml_iter_next, ml_limited_state_next);
	ml_typed_fn_set(MLLimitedStateT, ml_iter_key, ml_limited_state_key);
	ml_typed_fn_set(MLLimitedStateT, ml_iter_value, ml_limited_state_value);

	MLSkippedT = ml_type(MLIteratableT, "skipped");
	ml_typed_fn_set(MLSkippedT, ml_iterate, ml_skipped_iterate);

	MLTasksT = ml_type(MLFunctionT, "tasks");
	MLTasksT->call = (void *)ml_tasks_call;

	MLUniqueT = ml_type(MLIteratableT, "unique");
	MLUniqueStateT = ml_type(MLAnyT, "unique-state");
	ml_typed_fn_set(MLUniqueT, ml_iterate, ml_unique_iterate);
	ml_typed_fn_set(MLUniqueStateT, ml_iter_next, ml_unique_next);
	ml_typed_fn_set(MLUniqueStateT, ml_iter_key, ml_unique_key);
	ml_typed_fn_set(MLUniqueStateT, ml_iter_value, ml_unique_value);

	MLGroupedT = ml_type(MLIteratableT, "grouped");
	MLGroupedStateT = ml_type(MLAnyT, "grouped-state");
	ml_typed_fn_set(MLGroupedT, ml_iterate, ml_grouped_iterate);
	ml_typed_fn_set(MLGroupedStateT, ml_iter_next, ml_grouped_next);
	ml_typed_fn_set(MLGroupedStateT, ml_iter_key, ml_grouped_key);
	ml_typed_fn_set(MLGroupedStateT, ml_iter_value, ml_grouped_value);

	MLRepeatedT = ml_type(MLIteratableT, "repeated");
	MLRepeatedStateT = ml_type(MLAnyT, "repeated-state");
	ml_typed_fn_set(MLRepeatedT, ml_iterate, ml_repeated_iterate);
	ml_typed_fn_set(MLRepeatedStateT, ml_iter_next, ml_repeated_next);
	ml_typed_fn_set(MLRepeatedStateT, ml_iter_key, ml_repeated_key);
	ml_typed_fn_set(MLRepeatedStateT, ml_iter_value, ml_repeated_value);

	MLSequencedT = ml_type(MLIteratableT, "sequenced");
	MLSequencedStateT = ml_type(MLAnyT, "sequenced-state");
	ml_typed_fn_set(MLSequencedT, ml_iterate, ml_sequenced_iterate);
	ml_typed_fn_set(MLSequencedStateT, ml_iter_next, ml_sequenced_next);
	ml_typed_fn_set(MLSequencedStateT, ml_iter_key, ml_sequenced_key);
	ml_typed_fn_set(MLSequencedStateT, ml_iter_value, ml_sequenced_value);

#include "ml_iterfns_init.c"
}
