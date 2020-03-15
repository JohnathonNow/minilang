#include "minilang.h"
#include "ml_console.h"
#include "ml_compiler.h"
#include "ml_macros.h"
#include "ml_file.h"
#include "ml_object.h"
#include "ml_iterfns.h"
#include "stringmap.h"
#include <stdio.h>
#include <gc.h>

#ifdef USE_ML_MATH
#include "ml_math.h"
#include "ml_array.h"
#endif

#ifdef USE_ML_IO
#include "ml_io.h"
#endif

#ifdef USE_ML_GIR
#include "gtk_console.h"
#include "ml_gir.h"
#endif

#ifdef USE_ML_CBOR
#include "ml_cbor.h"
#endif

#ifdef USE_ML_MPC
#include "ml_mpc.h"
#endif

#ifdef USE_ML_RADB
#include "ml_radb.h"
#endif

#ifdef USE_ML_MODULES
#include "ml_module.h"
#include "ml_library.h"
#endif

static stringmap_t Globals[1] = {STRINGMAP_INIT};

static ml_value_t *global_get(void *Data, const char *Name) {
	return stringmap_search(Globals, Name);
}

static ml_value_t *ml_print(void *Data, int Count, ml_value_t **Args) {
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Result = Args[I];
		if (Result->Type != MLStringT) {
			Result = ml_call(MLStringOfMethod, 1, &Result);
			if (Result->Type == MLErrorT) return Result;
			if (Result->Type != MLStringT) return ml_error("ResultError", "string method did not return string");
		}
		fwrite(ml_string_value(Result), 1, ml_string_length(Result), stdout);
	}
	fflush(stdout);
	return MLNil;
}

static ml_value_t *ml_throw(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ML_CHECK_ARG_TYPE(1, MLStringT);
	return ml_error(ml_string_value(Args[0]), "%s", ml_string_value(Args[1]));
}

extern int MLDebugClosures;

static ml_value_t *ml_debug(void *Data, int Count, ml_value_t **Args) {
	if (Count > 0 && Args[0] == MLNil) {
		MLDebugClosures = 0;
	} else {
		MLDebugClosures = 1;
	}
	return MLNil;
}

static ml_value_t *ml_break(void *Data, int Count, ml_value_t **Args) {
#ifdef DEBUG
	asm("int3");
#endif
	return MLNil;
}

static ml_value_t *ml_halt(void *Data, int Count, ml_value_t **Args) {
	if (Count > 0) {
		ML_CHECK_ARG_TYPE(0, MLIntegerT);
		exit(ml_integer_value(Args[0]));
	} else {
		exit(0);
	}
}

static ml_value_t *ml_collect(void *Data, int Count, ml_value_t **Args) {
	GC_gcollect();
	return MLNil;
}

extern ml_value_t MLCallCC[];
extern ml_value_t MLSpawn[];

#ifdef USE_ML_MODULES
static stringmap_t Modules[1] = {STRINGMAP_INIT};

static ml_value_t *ml_import_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLStringT);
	const char *FileName = realpath(ml_string_value(Args[0]), NULL);
	ml_value_t **Slot = stringmap_slot(Modules, FileName);
	if (!Slot[0]) {
		printf("Loading %s\n", FileName);
		const char *Extension = strrchr(FileName, '.');
		if (!Extension) ML_CONTINUE(Caller, ml_error("ModuleError", "Unknown module type: %s", FileName));
		if (!strcmp(Extension, ".so")) {
			return ml_library_load_file(Caller, FileName, stringmap_search, Globals, Slot);
		} else if (!strcmp(Extension, ".mini")) {
			return ml_module_load_file(Caller, FileName, stringmap_search, Globals, Slot);
		} else {
			ML_CONTINUE(Caller, ml_error("ModuleError", "Unknown module type: %s", FileName));
		}
	}
	ML_CONTINUE(Caller, Slot[0]);
}
#endif

int main(int Argc, const char *Argv[]) {
	static const char *Parameters[] = {"Args", NULL};
	ml_init();
	ml_types_init(Globals);
	ml_file_init(Globals);
	ml_object_init(Globals);
	ml_iterfns_init(Globals);
	stringmap_insert(Globals, "print", ml_function(0, ml_print));
	stringmap_insert(Globals, "error", ml_function(0, ml_throw));
	stringmap_insert(Globals, "debug", ml_function(0, ml_debug));
	stringmap_insert(Globals, "break", ml_function(0, ml_break));
	stringmap_insert(Globals, "halt", ml_function(0, ml_halt));
	stringmap_insert(Globals, "collect", ml_function(0, ml_collect));
	stringmap_insert(Globals, "callcc", MLCallCC);
	stringmap_insert(Globals, "spawn", MLSpawn);
#ifdef USE_ML_CBOR
	ml_cbor_init(Globals);
#endif
#ifdef USE_ML_MATH
	ml_math_init(Globals);
	ml_array_init(Globals);
#endif
#ifdef USE_ML_IO
	ml_io_init(Globals);
#endif
#ifdef USE_ML_GIR
	ml_gir_init(Globals);
	int GtkConsole = 0;
#endif
#ifdef USE_ML_MPC
	ml_mpc_init(Globals);
#endif
#ifdef USE_ML_RADB
	ml_radb_init(Globals);
#endif
#ifdef USE_ML_MODULES
	ml_module_init(Globals);
	ml_library_init(Globals);
	stringmap_insert(Globals, "import", ml_functionx(0, ml_import_fnx));
#endif
	ml_value_t *Args = ml_list();
	const char *FileName = 0;
	const char *ModuleName = 0;
	for (int I = 1; I < Argc; ++I) {
		if (Argv[I][0] == '-') {
			switch (Argv[I][1]) {
#ifdef USE_ML_MODULES
			case 'm':
				if (++I >= Argc) {
					printf("Error: module name required\n");
					exit(-1);
				}
				ModuleName = Argv[I];
			break;
#endif
			case 'D': MLDebugClosures = 1; break;
			case 'z': GC_disable(); break;
#ifdef USE_ML_GIR
			case 'G': GtkConsole = 1; break;
#endif
			}
		} else if (!FileName) {
			FileName = Argv[I];
		} else {
			ml_list_append(Args, ml_string(Argv[I], -1));
		}
	}
	if (FileName) {
		ml_value_t *Closure = ml_load(global_get, 0, FileName, Parameters);
		if (Closure->Type == MLErrorT) {
			printf("Error: %s\n", ml_error_message(Closure));
			const char *Source;
			int Line;
			for (int I = 0; ml_error_trace(Closure, I, &Source, &Line); ++I) printf("\t%s:%d\n", Source, Line);
			return 1;
		}
		ml_value_t *Result = ml_inline(Closure, 1, Args);
		if (Result->Type == MLErrorT) {
			printf("Error: %s\n", ml_error_message(Result));
			const char *Source;
			int Line;
			for (int I = 0; ml_error_trace(Result, I, &Source, &Line); ++I) printf("\t%s:%d\n", Source, Line);
			return 1;
		}
	} else if (ModuleName) {
		ml_value_t *Args[] = {ml_string(ModuleName, -1)};
		ml_value_t *Result = ml_import_fnx(NULL, NULL, 1, Args);
		if (Result->Type == MLErrorT) {
			printf("Error: %s\n", ml_error_message(Result));
			const char *Source;
			int Line;
			for (int I = 0; ml_error_trace(Result, I, &Source, &Line); ++I) printf("\t%s:%d\n", Source, Line);
			return 1;
		}
#ifdef USE_ML_GIR
	} else if (GtkConsole) {
		console_t *Console = console_new(stringmap_search, Globals);
		stringmap_insert(Globals, "print", ml_function(Console, (void *)console_print));
		console_show(Console, NULL);
		gtk_main();
#endif
	} else {
		ml_console(stringmap_search, Globals);
	}
	return 0;
}
