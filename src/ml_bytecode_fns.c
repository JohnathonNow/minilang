#include "ml_bytecode.h"
#include "ml_macros.h"

#define ERROR() { \
	Inst = Frame->OnError; \
	return Inst->run(Frame, Result, Top, Inst); \
}

#define ADVANCE(N) { \
	Inst = Inst->Params[N].Inst; \
	return Inst->run(Frame, Result, Top, Inst); \
}

#define ERROR_CHECK(VALUE) if (VALUE->Type == MLErrorT) { \
	ml_error_trace_add(VALUE, (ml_source_t){Frame->Source, Inst->LineNo}); \
	Result = VALUE; \
	ERROR(); \
}

static void DO_RETURN_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	ML_CONTINUE(Frame->Base.Caller, Result);
}

extern ml_type_t MLSuspensionT[];

static void DO_SUSPEND_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	Frame->Base.Type = MLSuspensionT;
	Frame->Inst = Inst->Params[0].Inst;
	Frame->Top = Top;
	ML_CONTINUE(Frame->Base.Caller, (ml_value_t *)Frame);
}

static void DO_RESUME_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	*--Top = 0;
	ADVANCE(0);
}

static void DO_NIL_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	Result = MLNil;
	ADVANCE(0);
}

static void DO_SOME_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	Result = MLSome;
	ADVANCE(0);
}

static void DO_IF_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	Result = Result->Type->deref(Result);
	ERROR_CHECK(Result);
	if (Result == MLNil) {
		ADVANCE(0);
	} else {
		ADVANCE(1);
	}
}

static void DO_ELSE_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	Result = Result->Type->deref(Result);
	ERROR_CHECK(Result);
	if (Result != MLNil) {
		ADVANCE(0);
	} else {
		ADVANCE(1);
	}
}

static void DO_PUSH_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	*Top++ = Result;
	ADVANCE(0);
}

static void DO_WITH_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	*Top++ = Result;
	ADVANCE(0);
}

static void DO_WITH_VAR_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	ml_reference_t *Local = xnew(ml_reference_t, 1, ml_value_t *);
	Local->Type = MLReferenceT;
	Local->Address = Local->Value;
	Local->Value[0] = Result;
	*Top++ = (ml_value_t *)Local;
	ADVANCE(0);
}

static void DO_WITHX_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	int Count = Inst->Params[1].Count;
	ml_unpacked_t Unpacked = ml_unpack(Result, Count);
	if (!Unpacked.Values) {
		Result = ml_error("TypeError", "Unable to unpack %s", Result->Type->Name);
		ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->LineNo});
		ERROR();
	}
	if (Unpacked.Count < Count) {
		Result = ml_error("ValueError", "Not enough values to unpack (%d < %d)", Unpacked.Count, Count);
		ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->LineNo});
		ERROR();
	}
	for (int I = 0; I < Count; ++I) *Top++ = Unpacked.Values[I];
	ADVANCE(0);
}

static void DO_POP_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	Result = *--Top;
	*Top = 0;
	ADVANCE(0);
}

static void DO_ENTER_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	for (int I = Inst->Params[1].Count; --I >= 0;) {
		ml_reference_t *Local = xnew(ml_reference_t, 1, ml_value_t *);
		Local->Type = MLReferenceT;
		Local->Address = Local->Value;
		Local->Value[0] = MLNil;
		*Top++ = (ml_value_t *)Local;
	}
	for (int I = Inst->Params[2].Count; --I >= 0;) {
		*Top++ = NULL;
	}
	ADVANCE(0);
}

static void DO_EXIT_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	for (int I = Inst->Params[1].Count; --I >= 0;) *--Top = 0;
	ADVANCE(0);
}

static void DO_LOOP_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	ADVANCE(0);
}

static void DO_TRY_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	Frame->OnError = Inst->Params[1].Inst;
	ADVANCE(0);
}

static void DO_CATCH_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	if (Result->Type != MLErrorT) {
		Result = ml_error("InternalError", "expected error value, not %s", Result->Type->Name);
		ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->LineNo});
		ERROR();
	}
	Result->Type = MLErrorValueT;
	ml_value_t **Old = Frame->Stack + Inst->Params[1].Index;
	while (Top > Old) *--Top = 0;
	*Top++ = Result;
	ADVANCE(0);
}

static void DO_LOAD_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	Result = Inst->Params[1].Value;
	ADVANCE(0);
}

static void DO_VAR_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	Result = Result->Type->deref(Result);
	ERROR_CHECK(Result);
	ml_reference_t *Local = (ml_reference_t *)Top[Inst->Params[1].Index];
	Local->Value[0] = Result;
	ADVANCE(0);
}

static void DO_VARX_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	Result = Result->Type->deref(Result);
	ERROR_CHECK(Result);
	int Count = Inst->Params[2].Count;
	ml_unpacked_t Unpacked = ml_unpack(Result, Count);
	if (!Unpacked.Values) {
		Result = ml_error("TypeError", "Unable to unpack %s", Result->Type->Name);
		ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->LineNo});
		ERROR();
	}
	if (Unpacked.Count < Count) {
		Result = ml_error("ValueError", "Not enough values to unpack (%d < %d)", Unpacked.Count, Count);
		ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->LineNo});
		ERROR();
	}
	ml_value_t **Base = Top + Inst->Params[1].Index;
	for (int I = 0; I < Count; ++I) {
		Result = Unpacked.Values[I]->Type->deref(Unpacked.Values[I]);
		ERROR_CHECK(Result);
		ml_reference_t *Local = (ml_reference_t *)Base[I];
		Local->Value[0] = Result;
	}
	ADVANCE(0);
}

static void DO_LET_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	Result = Result->Type->deref(Result);
	ERROR_CHECK(Result);
	Top[Inst->Params[1].Index] = Result;
	ADVANCE(0);
}

static void DO_LETI_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	Result = Result->Type->deref(Result);
	ERROR_CHECK(Result);
	ml_value_t *Uninitialized = Top[Inst->Params[1].Index];
	Top[Inst->Params[1].Index] = Result;
	if (Uninitialized) ml_uninitialized_set(Uninitialized, Result);
	ADVANCE(0);
}

static void DO_LETX_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	Result = Result->Type->deref(Result);
	ERROR_CHECK(Result);
	int Count = Inst->Params[2].Count;
	ml_unpacked_t Unpacked = ml_unpack(Result, Count);
	if (!Unpacked.Values) {
		Result = ml_error("TypeError", "Unable to unpack %s", Result->Type->Name);
		ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->LineNo});
		ERROR();
	}
	if (Unpacked.Count < Count) {
		Result = ml_error("ValueError", "Not enough values to unpack (%d < %d)", Unpacked.Count, Count);
		ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->LineNo});
		ERROR();
	}
	ml_value_t **Base = Top + Inst->Params[1].Index;
	for (int I = 0; I < Count; ++I) {
		Result = Unpacked.Values[I]->Type->deref(Unpacked.Values[I]);
		ERROR_CHECK(Result);
		ml_value_t *Uninitialized = Base[I];
		Base[I] = Result;
		if (Uninitialized) ml_uninitialized_set(Uninitialized, Result);
	}
	ADVANCE(0);
}

static void DO_FOR_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	Result = Result->Type->deref(Result);
	ERROR_CHECK(Result);
	Frame->Inst = Inst->Params[0].Inst;
	Frame->Top = Top;
	return ml_iterate((ml_state_t *)Frame, Result);
}

static void DO_NEXT_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	for (int I = Inst->Params[1].Count; --I >= 0;) *--Top = 0;
	Result = Top[-1];
	*--Top = 0;
	Frame->Inst = Inst->Params[0].Inst;
	Frame->Top = Top;
	return ml_iter_next((ml_state_t *)Frame, Result);
}

static void DO_VALUE_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	Result = Top[-1];
	Frame->Inst = Inst->Params[0].Inst;
	Frame->Top = Top;
	return ml_iter_value((ml_state_t *)Frame, Result);
}

static void DO_KEY_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	Result = Top[-2];
	Frame->Inst = Inst->Params[0].Inst;
	Frame->Top = Top;
	return ml_iter_key((ml_state_t *)Frame, Result);
}

static void DO_CALL_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	int Count = Inst->Params[1].Count;
	ml_value_t *Function = Top[~Count];
	Function = Function->Type->deref(Function);
	ERROR_CHECK(Function);
	ml_value_t **Args = Top - Count;
	ml_inst_t *Next = Inst->Params[0].Inst;
	if (Next->Opcode == MLI_RETURN) {
		return Function->Type->call(Frame->Base.Caller, Function, Count, Args);
	} else {
		Frame->Inst = Next;
		Frame->Top = Top - (Count + 1);
		return Function->Type->call((ml_state_t *)Frame, Function, Count, Args);
	}
}

static void DO_CONST_CALL_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	int Count = Inst->Params[1].Count;
	ml_value_t *Function = Inst->Params[2].Value;
	ml_value_t **Args = Top - Count;
	ml_inst_t *Next = Inst->Params[0].Inst;
	if (Next->Opcode == MLI_RETURN) {
		return Function->Type->call(Frame->Base.Caller, Function, Count, Args);
	} else {
		Frame->Inst = Inst->Params[0].Inst;
		Frame->Top = Top - Count;
		return Function->Type->call((ml_state_t *)Frame, Function, Count, Args);
	}
}

static void DO_ASSIGN_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	Result = Result->Type->deref(Result);
	ERROR_CHECK(Result);
	ml_value_t *Ref = Top[-1];
	*--Top = 0;
	Result = Ref->Type->assign(Ref, Result);
	ERROR_CHECK(Result);
	ADVANCE(0);
}

static void DO_LOCAL_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	int Index = Inst->Params[1].Index;
	Result = (Index < 0) ? Frame->UpValues[~Index] : Frame->Stack[Index];
	ADVANCE(0);
}

static void DO_LOCALX_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	int Index = Inst->Params[1].Index;
	ml_value_t **Slot = &Frame->Stack[Index];
	Result = Slot[0];
	if (!Result) Result = Slot[0] = ml_uninitialized();
	ADVANCE(0);
}

static void DO_TUPLE_NEW_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	int Size = Inst->Params[1].Count;
	ml_tuple_t *Tuple = xnew(ml_tuple_t, Size, ml_value_t *);
	Tuple->Type = MLTupleT;
	Tuple->Size = Size;
	*Top++ = (ml_value_t *)Tuple;
	ADVANCE(0);
}

static void DO_TUPLE_SET_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	int Index = Inst->Params[1].Index;
	ml_tuple_t *Tuple = (ml_tuple_t *)Top[-1];
	Tuple->Values[Index] = Result;
	ADVANCE(0);
}

static void DO_LIST_NEW_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	*Top++ = ml_list();
	ADVANCE(0);
}

static void DO_LIST_APPEND_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	Result = Result->Type->deref(Result);
	ERROR_CHECK(Result);
	ml_list_put(Top[-1], Result);
	ADVANCE(0);
}

static void DO_MAP_NEW_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	*Top++ = ml_map();
	ADVANCE(0);
}

static void DO_MAP_INSERT_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	ml_value_t *Key = Top[-1]->Type->deref(Top[-1]);
	ERROR_CHECK(Key);
	Result = Result->Type->deref(Result);
	ERROR_CHECK(Result);
	ml_map_insert(Top[-2], Key, Result);
	*--Top = 0;
	ADVANCE(0);
}

static void DO_CLOSURE_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	// closure <entry> <frame_size> <num_params> <num_upvalues> <upvalue_1> ...
	ml_closure_info_t *Info = Inst->Params[1].ClosureInfo;
	ml_closure_t *Closure = xnew(ml_closure_t, Info->NumUpValues, ml_value_t *);
	Closure->Type = MLClosureT;
	Closure->Info = Info;
	for (int I = 0; I < Info->NumUpValues; ++I) {
		int Index = Inst->Params[2 + I].Index;
		ml_value_t **Slot = (Index < 0) ? &Frame->UpValues[~Index] : &Frame->Stack[Index];
		ml_value_t *Value = Slot[0];
		if (!Value) Value = Slot[0] = ml_uninitialized();
		if (Value->Type == MLUninitializedT) {
			ml_uninitialized_use(Value, &Closure->UpValues[I]);
		}
		Closure->UpValues[I] = Value;
	}
	Result = (ml_value_t *)Closure;
	ADVANCE(0);
}

static void DO_PARTIAL_NEW_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	Result = Result->Type->deref(Result);
	ERROR_CHECK(Result);
	*Top++ = ml_partial_function_new(Result, Inst->Params[1].Count);
	ADVANCE(0);
}

static void DO_PARTIAL_SET_FN(ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top, ml_inst_t *Inst) {
	Result = Result->Type->deref(Result);
	ERROR_CHECK(Result);
	ml_partial_function_set(Top[-1], Inst->Params[1].Index, Result);
	ADVANCE(0);
}

ml_inst_fn_t MLInstFns[] = {
	[MLI_RETURN] = DO_RETURN_FN,
	[MLI_SUSPEND] = DO_SUSPEND_FN,
	[MLI_RESUME] = DO_RESUME_FN,
	[MLI_NIL] = DO_NIL_FN,
	[MLI_SOME] = DO_SOME_FN,
	[MLI_IF] = DO_IF_FN,
	[MLI_ELSE] = DO_ELSE_FN,
	[MLI_PUSH] = DO_PUSH_FN,
	[MLI_WITH] = DO_WITH_FN,
	[MLI_WITH_VAR] = DO_WITH_VAR_FN,
	[MLI_WITHX] = DO_WITHX_FN,
	[MLI_POP] = DO_POP_FN,
	[MLI_ENTER] = DO_ENTER_FN,
	[MLI_EXIT] = DO_EXIT_FN,
	[MLI_LOOP] = DO_LOOP_FN,
	[MLI_TRY] = DO_TRY_FN,
	[MLI_CATCH] = DO_CATCH_FN,
	[MLI_LOAD] = DO_LOAD_FN,
	[MLI_VAR] = DO_VAR_FN,
	[MLI_VARX] = DO_VARX_FN,
	[MLI_LET] = DO_LET_FN,
	[MLI_LETI] = DO_LETI_FN,
	[MLI_LETX] = DO_LETX_FN,
	[MLI_FOR] = DO_FOR_FN,
	[MLI_NEXT] = DO_NEXT_FN,
	[MLI_VALUE] = DO_VALUE_FN,
	[MLI_KEY] = DO_KEY_FN,
	[MLI_CALL] = DO_CALL_FN,
	[MLI_CONST_CALL] = DO_CONST_CALL_FN,
	[MLI_ASSIGN] = DO_ASSIGN_FN,
	[MLI_LOCAL] = DO_LOCAL_FN,
	[MLI_LOCALX] = DO_LOCALX_FN,
	[MLI_TUPLE_NEW] = DO_TUPLE_NEW_FN,
	[MLI_TUPLE_SET] = DO_TUPLE_SET_FN,
	[MLI_LIST_NEW] = DO_LIST_NEW_FN,
	[MLI_LIST_APPEND] = DO_LIST_APPEND_FN,
	[MLI_MAP_NEW] = DO_MAP_NEW_FN,
	[MLI_MAP_INSERT] = DO_MAP_INSERT_FN,
	[MLI_CLOSURE] = DO_CLOSURE_FN,
	[MLI_PARTIAL_NEW] = DO_PARTIAL_NEW_FN,
	[MLI_PARTIAL_SET] = DO_PARTIAL_SET_FN
};
