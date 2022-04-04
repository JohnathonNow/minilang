#include "ml_module.h"
#include "ml_macros.h"
#include <gc/gc.h>
#include <string.h>
#include <stdio.h>
#include "ml_runtime.h"

#undef ML_CATEGORY
#define ML_CATEGORY "module"
//!internal

typedef struct {
	ml_module_t Base;
	int Flags;
} ml_mini_module_t;

ML_TYPE(MLMiniModuleT, (MLModuleT), "module");

ML_METHODX("::", MLMiniModuleT, MLStringT) {
	ml_mini_module_t *Module = (ml_mini_module_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(Module->Base.Exports, Name);
	if (!Slot[0]) Slot[0] = ml_uninitialized(Name);
	ML_RETURN(Slot[0]);
}

static void ml_export(ml_state_t *Caller, ml_mini_module_t *Module, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLNamesT);
	int Index = 0;
	ml_value_t *Value = MLNil;
	ML_NAMES_FOREACH(Args[0], Iter) {
		const char *Name = ml_string_value(Iter->Value);
		Value = Args[++Index];
		ml_value_t **Slot = (ml_value_t **)stringmap_slot(Module->Base.Exports, Name);
		if (Module->Flags & MLMF_USE_GLOBALS) {
			if (Slot[0]) {
				if (ml_typeof(Slot[0]) == MLGlobalT) {
				} else if (ml_typeof(Slot[0]) == MLUninitializedT) {
					ml_value_t *Global = ml_global(Name);
					ml_uninitialized_set(Slot[0], Global);
					Slot[0] = Global;
				} else {
					ML_ERROR("ExportError", "Duplicate export %s", Name);
				}
			} else {
				Slot[0] = ml_global(Name);
			}
			ml_global_set(Slot[0], Value);
		} else {
			if (Slot[0]) {
				if (ml_typeof(Slot[0]) != MLUninitializedT) {
					ML_ERROR("ExportError", "Duplicate export %s", Name);
				}
				ml_uninitialized_set(Slot[0], Value);
			}
			Slot[0] = Value;
		}
	}
	ML_RETURN(Value);
}

typedef struct ml_module_state_t {
	ml_state_t Base;
	ml_value_t *Module;
	ml_value_t *Import;
	ml_getter_t GlobalGet;
	void *Globals;
	ml_value_t *Args[1];
} ml_module_state_t;

ML_TYPE(MLModuleStateT, (), "module-state");

static void ml_module_done_run(ml_module_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	ML_RETURN(State->Module);
}

static void ml_module_init_run(ml_module_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (ml_state_fn)ml_module_done_run;
	return ml_call(State, Value, 1, State->Args);
}

void ml_module_compile2(ml_state_t *Caller, ml_parser_t *Parser, ml_compiler_t *Compiler, ml_value_t **Slot, int Flags) {
	static const char *Parameters[] = {"export", NULL};
	ml_mini_module_t *Module;
	if ((Flags & MLMF_USE_GLOBALS) && Slot[0] && ml_typeof(Slot[0]) == MLMiniModuleT) {
		Module = (ml_mini_module_t *)Slot[0];
	} else {
		Module = new(ml_mini_module_t);
		Module->Base.Type = MLMiniModuleT;
		Module->Base.Path = ml_parser_name(Parser);
		Module->Flags = Flags;
		Slot[0] = (ml_value_t *)Module;
	}
	ml_module_state_t *State = new(ml_module_state_t);
	State->Base.Type = MLModuleStateT;
	State->Base.run = (void *)ml_module_init_run;
	State->Base.Context = Caller->Context;
	State->Base.Caller = Caller;
	State->Module = (ml_value_t *)Module;
	State->Args[0] = ml_cfunctionz(Module, (ml_callbackx_t)ml_export);
	mlc_expr_t *Expr = ml_accept_file(Parser);
	if (!Expr) ML_RETURN(ml_parser_value(Parser));
	ml_function_compile((ml_state_t *)State, Expr, Compiler, Parameters);
}


void ml_module_compile(ml_state_t *Caller, ml_parser_t *Parser, ml_compiler_t *Compiler, ml_value_t **Slot) {
	return ml_module_compile2(Caller, Parser, Compiler, Slot, 0);
}

void ml_module_load_file(ml_state_t *Caller, const char *FileName, ml_getter_t GlobalGet, void *Globals, ml_value_t **Slot) {
	static const char *Parameters[] = {"export", NULL};
	ml_mini_module_t *Module = new(ml_mini_module_t);
	Module->Base.Type = MLMiniModuleT;
	Module->Base.Path = FileName;
	Slot[0] = (ml_value_t *)Module;
	ml_module_state_t *State = new(ml_module_state_t);
	State->Base.Type = MLModuleStateT;
	State->Base.run = (void *)ml_module_init_run;
	State->Base.Context = Caller->Context;
	State->Base.Caller = Caller;
	State->Module = (ml_value_t *)Module;
	State->Args[0] = ml_cfunctionz(Module, (ml_callbackx_t)ml_export);
	return ml_load_file((ml_state_t *)State, GlobalGet, Globals, FileName, Parameters);
}

typedef struct {
	ml_module_t Base;
	ml_value_t *Lookup;
} ml_fn_module_t;

ML_TYPE(MLFnModuleT, (), "module");

typedef struct {
	ml_state_t Base;
	ml_fn_module_t *Module;
	const char *Name;
} ml_module_lookup_state_t;

static void ml_module_lookup_run(ml_module_lookup_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (!ml_is_error(Value)) {
		stringmap_insert(State->Module->Base.Exports, State->Name, Value);
	}
	ML_RETURN(Value);
}

ML_METHODX("::", MLFnModuleT, MLStringT) {
//<Module
//<Name
//>MLAnyT
// Imports a symbol from a module.
	ml_fn_module_t *Module = (ml_fn_module_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_value_t *Value = stringmap_search(Module->Base.Exports, Name);
	if (!Value) {
		ml_module_lookup_state_t *State = new(ml_module_lookup_state_t);
		State->Base.Caller = Caller;
		State->Base.Context = Caller->Context;
		State->Base.run = (ml_state_fn)ml_module_lookup_run;
		State->Module = Module;
		State->Name = Name;
		return ml_call(State, Module->Lookup, 1, Args + 1);
	}
	ML_RETURN(Value);
}

ML_METHOD(MLModuleT, MLStringT, MLFunctionT) {
//@module
//<Path:string
//<Lookup:function
//>module
// Returns a generic module which calls resolves :mini:`Module::Import` by calling :mini:`Lookup(Import)`, caching results for future use.
	ml_fn_module_t *Module = new(ml_fn_module_t);
	Module->Base.Type = MLFnModuleT;
	Module->Base.Path = ml_string_value(Args[0]);
	Module->Lookup = Args[1];
	return (ml_value_t *)Module;
}

void ml_module_init(stringmap_t *_Globals) {
#include "ml_module_init.c"
}
