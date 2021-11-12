#include "ml_library.h"
#include "ml_macros.h"
#include "ml_module.h"
#include <string.h>
#ifdef Mingw
#else
#include <dlfcn.h>
#endif

#undef ML_CATEGORY
#define ML_CATEGORY "library"

static ml_value_t *LibraryPath;
static int MaxLibraryPathLength = 0;
static stringmap_t Modules[1] = {STRINGMAP_INIT};
static stringmap_t *Globals;

typedef struct ml_library_loader_t ml_library_loader_t;

struct ml_library_loader_t {
	ml_library_loader_t *Next;
	const char *Extension;
	int (*test)(const char *FileName);
	void (*load)(ml_state_t *Caller, const char *FileName, ml_value_t **Slot);
	ml_value_t *(*load0)(const char *FileName, ml_value_t **Slot);
};

static ml_library_loader_t *Loaders = NULL;
static int MaxLibraryExtensionLength = 2;

typedef struct {
	ml_library_loader_t *Loader;
	const char *FileName;
} ml_library_info_t;

static ml_library_info_t ml_library_find(const char *Path, const char *Name) {
	char *FileName;
	ml_library_loader_t *Loader = NULL;
	if (Path) {
		const char *BasePath = realpath(Path, NULL);
		if (!BasePath) return (ml_library_info_t){NULL, NULL};
		FileName = snew(strlen(BasePath) + strlen(Name) + MaxLibraryExtensionLength);
		char *End = stpcpy(FileName, BasePath);
		*End++ = '/';
		End = stpcpy(End, Name);
		Loader = Loaders;
		while (Loader) {
			strcpy(End, Loader->Extension);
			if (Loader->test(FileName)) return (ml_library_info_t){Loader, FileName};
			Loader = Loader->Next;
		}
	} else {
		FileName = snew(MaxLibraryPathLength + strlen(Name) + MaxLibraryExtensionLength);
		ML_LIST_FOREACH(LibraryPath, Iter) {
			char *End = stpcpy(FileName, ml_string_value(Iter->Value));
			*End++ = '/';
			End = stpcpy(End, Name);
			Loader = Loaders;
			while (Loader) {
				strcpy(End, Loader->Extension);
				if (Loader->test(FileName)) return (ml_library_info_t){Loader, FileName};
				Loader = Loader->Next;
			}
		}
	}
	return (ml_library_info_t){NULL, NULL};
}

void ml_library_load(ml_state_t *Caller, const char *Path, const char *Name) {
	ml_library_info_t Info = ml_library_find(Path, Name);
	if (!Info.Loader) ML_ERROR("ModuleError", "Module %s not found in %s", Name, Path ?: "<library>");
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(Modules, Info.FileName);
	if (!Slot[0]) return Info.Loader->load(Caller, Info.FileName, Slot);
	ML_RETURN(Slot[0]);
}

ml_value_t *ml_library_load0(const char *Path, const char *Name) {
	ml_library_info_t Info = ml_library_find(Path, Name);
	if (!Info.Loader) return ml_error("ModuleError", "Module %s not found in %s", Name, Path ?: "<library>");
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(Modules, Info.FileName);
	if (!Slot[0]) return Info.Loader->load0(Info.FileName, Slot);
	return Slot[0];
}

#include <sys/stat.h>

static int ml_library_test_file(const char *FileName) {
	struct stat Stat[1];
	return !lstat(FileName, Stat);
}

static ml_value_t *ml_library_default_load0(const char *FileName, ml_value_t **Slot) {
	return ml_error("ModuleError", "Module %s loaded incorrectly", FileName);
}

static void ml_library_import(ml_state_t *Caller, ml_module_t *Module, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLStringT);
	// TODO: Allow relative library loading
	return ml_library_load(Caller, NULL, ml_string_value(Args[0]));
}

static ml_value_t *ml_library_mini_global_get(ml_module_t **Slot, const char *Name) {
	if (!strcmp(Name, "import")) return ml_cfunctionx(Slot[0], (ml_callbackx_t)ml_library_import);
	return (ml_value_t *)stringmap_search(Globals, Name);
}

static void ml_library_mini_load(ml_state_t *Caller, const char *FileName, ml_value_t **Slot) {
	return ml_module_load_file(Caller, FileName, (ml_getter_t)ml_library_mini_global_get, Slot, Slot);
}

static void ml_library_so_load(ml_state_t *Caller, const char *FileName, ml_value_t **Slot) {
	void *Handle = dlopen(FileName, RTLD_GLOBAL | RTLD_LAZY);
	if (Handle) {
		typeof(ml_library_entry0) *init0 = dlsym(Handle, "ml_library_entry0");
		if (init0) {
			ml_value_t *Library = Slot[0] = ml_module(FileName, NULL);
			init0(Library);
			ML_RETURN(Library);
		}
		typeof(ml_library_entry) *init = dlsym(Handle, "ml_library_entry");
		if (init) {
			ml_value_t *Library = Slot[0] = ml_module(FileName, NULL);
			init(Caller, Library);
			ML_RETURN(Library);
		}
		dlclose(Handle);
		ML_ERROR("LibraryError", "init function missing from %s", FileName);
	} else {
		ML_ERROR("LibraryError", "Failed to load %s: %s", FileName, dlerror());
	}
}

static ml_value_t *ml_library_so_load0(const char *FileName, ml_value_t **Slot) {
	void *Handle = dlopen(FileName, RTLD_GLOBAL | RTLD_LAZY);
	if (Handle) {
		typeof(ml_library_entry0) *init0 = dlsym(Handle, "ml_library_entry0");
		if (init0) {
			ml_value_t *Library = Slot[0] = ml_module(FileName, NULL);
			init0(Library);
			return Library;
		}
		typeof(ml_library_entry) *init = dlsym(Handle, "ml_library_entry");
		if (init) return ml_library_default_load0(FileName, Slot);
		dlclose(Handle);
		return ml_error("LibraryError", "init function missing from %s", FileName);
	} else {
		return ml_error("LibraryError", "Failed to load %s: %s", FileName, dlerror());
	}
}

#include "whereami.h"

void ml_library_path_add(const char *Path) {
	const char *RealPath = realpath(Path, NULL);
	if (!RealPath) {
		fprintf(stderr, "Error: library path %s not found\n", Path);
		exit(-1);
	}
	int PathLength = strlen(RealPath);
	ml_list_push(LibraryPath, ml_string(RealPath, PathLength));
	if (MaxLibraryPathLength < PathLength) MaxLibraryPathLength = PathLength;
}

static void ml_library_path_add_default(void) {
	int ExecutablePathLength = wai_getExecutablePath(NULL, 0, NULL);
	char *ExecutablePath = snew(ExecutablePathLength + 1);
	wai_getExecutablePath(ExecutablePath, ExecutablePathLength + 1, &ExecutablePathLength);
	ExecutablePath[ExecutablePathLength] = 0;
	for (int I = ExecutablePathLength - 1; I > 0; --I) {
		if (ExecutablePath[I] == '/') {
			ExecutablePath[I] = 0;
			ExecutablePathLength = I;
			break;
		}
	}
	int LibPathLength = ExecutablePathLength + strlen("/lib/minilang");
	char *LibPath = snew(LibPathLength + 1);
	memcpy(LibPath, ExecutablePath, ExecutablePathLength);
	strcpy(LibPath + ExecutablePathLength, "/lib/minilang");
	//printf("Looking for library path at %s\n", LibPath);
	struct stat Stat[1];
	if (!lstat(LibPath, Stat) && S_ISDIR(Stat->st_mode)) {
		ml_library_path_add(LibPath);
	}
}

void ml_library_loader_add(
	const char *Extension, int (*Test)(const char *),
	void (*Load)(ml_state_t *, const char *, ml_value_t **),
	ml_value_t *(*Load0)(const char *, ml_value_t **)
) {
	ml_library_loader_t *Loader = new(ml_library_loader_t);
	Loader->Next = Loaders;
	Loaders = Loader;
	Loader->Extension = Extension;
	Loader->test = Test ?: ml_library_test_file;
	Loader->load = Load;
	Loader->load0 = Load0 ?: ml_library_default_load0;
}

ML_FUNCTION(Unload) {
//<Path
//>nil
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	const char *FileName = realpath(ml_string_value(Args[0]), NULL);
	if (!FileName) return ml_error("ModuleError", "File %s not found", ml_string_value(Args[0]));
	stringmap_remove(Modules, FileName);
	return MLNil;
}

void ml_library_init(stringmap_t *_Globals) {
	Globals = _Globals;
	LibraryPath = ml_list();
	ml_library_path_add_default();
	ml_library_loader_add(".mini", NULL, ml_library_mini_load, NULL);
	ml_library_loader_add(".so", NULL, ml_library_so_load, ml_library_so_load0);
#include "ml_library_init.c"
	stringmap_insert(Globals, "unload", Unload);
}
