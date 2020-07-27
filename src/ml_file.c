#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <gc.h>
#include <errno.h>
#include <dirent.h>
#include "ml_file.h"
#include "ml_macros.h"

#define new(T) ((T *)GC_MALLOC(sizeof(T)))
#define anew(T, N) ((T *)GC_MALLOC((N) * sizeof(T)))
#define snew(N) ((char *)GC_MALLOC_ATOMIC(N))
#define xnew(T, N, U) ((T *)GC_MALLOC(sizeof(T) + (N) * sizeof(U)))

typedef struct ml_file_t {
	const ml_type_t *Type;
	FILE *Handle;
} ml_file_t;

ML_TYPE(MLFileT, (), "file");

FILE *ml_file_handle(ml_value_t *Value) {
	return ((ml_file_t *)Value)->Handle;
}

#ifdef __MINGW32__
static ssize_t ml_read_line(FILE *File, ssize_t Offset, char **Result) {
	char Buffer[129];
	if (fgets(Buffer, 129, File) == NULL) return -1;
	int Length = strlen(Buffer);
	if (Length == 128) {
		ssize_t Total = ml_read_line(File, Offset + 128, Result);
		memcpy(*Result + Offset, Buffer, 128);
		return Total;
	} else {
		*Result = GC_MALLOC_ATOMIC(Offset + Length + 1);
		strcpy(*Result + Offset, Buffer);
		return Offset + Length;
	}
}
#endif

ML_METHOD("read", MLFileT) {
	ml_file_t *File = (ml_file_t *)Args[0];
	if (!File->Handle) return ml_error("FileError", "file closed");
	char *Line = 0;
	size_t Length = 0;
#ifdef __MINGW32__
	ssize_t Read = ml_read_line(File->Handle, 0, &Line);
#else
	ssize_t Read = getline(&Line, &Length, File->Handle);
#endif
	if (Read < 0) return feof(File->Handle) ? MLNil : ml_error("FileError", "error reading from file: %s", strerror(errno));
	return ml_string(Line, Read);
}

ML_METHOD("read", MLFileT, MLIntegerT) {
	ml_file_t *File = (ml_file_t *)Args[0];
	if (!File->Handle) return ml_error("FileError", "file closed");
	if (feof(File->Handle)) return MLNil;
	ssize_t Requested = ml_integer_value(Args[1]);
	ml_stringbuffer_t Final[1] = {ML_STRINGBUFFER_INIT};
	char Buffer[ML_STRINGBUFFER_NODE_SIZE];
	while (Requested >= ML_STRINGBUFFER_NODE_SIZE) {
		ssize_t Actual = fread(Buffer, 1, ML_STRINGBUFFER_NODE_SIZE, File->Handle);
		if (Actual < 0) return ml_error("FileError", "error reading from file: %s", strerror(errno));
		if (Actual == 0) return ml_stringbuffer_get_string(Final);
		ml_stringbuffer_add(Final, Buffer, Actual);
		Requested -= Actual;
	}
	while (Requested > 0) {
		ssize_t Actual = fread(Buffer, 1, Requested, File->Handle);
		if (Actual < 0) return ml_error("FileError", "error reading from file: %s", strerror(errno));
		if (Actual == 0) return ml_stringbuffer_get_string(Final);
		ml_stringbuffer_add(Final, Buffer, Actual);
		Requested -= Actual;
	}
	return ml_stringbuffer_get_string(Final);
}

ML_METHODV("write", MLFileT, MLStringT) {
	ml_file_t *File = (ml_file_t *)Args[0];
	if (!File->Handle) return ml_error("FileError", "file closed");
	for (int I = 1; I < Count; ++I) {
		const char *Chars = ml_string_value(Args[I]);
		ssize_t Remaining = ml_string_length(Args[I]);
		while (Remaining > 0) {
			ssize_t Actual = fwrite(Chars, 1, Remaining, File->Handle);
			if (Actual < 0) return ml_error("FileError", "error writing to file: %s", strerror(errno));
			Chars += Actual;
			Remaining -= Actual;
		}
	}
	return Args[0];
}

static int ml_file_write_buffer_chars(ml_file_t *File, const char *Chars, size_t Remaining) {
	while (Remaining > 0) {
		ssize_t Actual = fwrite(Chars, 1, Remaining, File->Handle);
		if (Actual < 0) return 1;
		Chars += Actual;
		Remaining -= Actual;
	}
	return 0;
}

ML_METHOD("write", MLFileT, MLStringBufferT) {
	ml_file_t *File = (ml_file_t *)Args[0];
	if (!File->Handle) return ml_error("FileError", "file closed");
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[1];
	if (ml_stringbuffer_foreach(Buffer, File, (void *)ml_file_write_buffer_chars)) return ml_error("FileError", "error writing to file: %s", strerror(errno));
	return Args[0];
}

ML_METHOD("eof", MLFileT) {
	ml_file_t *File = (ml_file_t *)Args[0];
	if (!File->Handle) return ml_error("FileError", "file closed");
	if (feof(File->Handle)) return Args[0];
	return MLNil;
}

ML_METHOD("close", MLFileT) {
	ml_file_t *File = (ml_file_t *)Args[0];
	if (File->Handle) {
		fclose(File->Handle);
		File->Handle = 0;
	}
	return MLNil;
}

static void ml_file_finalize(ml_file_t *File, void *Data) {
	printf("ml_file_finalize!\n");
	if (File->Handle) {
		fclose(File->Handle);
		File->Handle = NULL;
	}
}

ml_value_t *ml_file_new(FILE *Handle) {
	ml_file_t *File = new(ml_file_t);
	File->Type = MLFileT;
	File->Handle = Handle;
	GC_register_finalizer(File, (void *)ml_file_finalize, 0, 0, 0);
	return (ml_value_t *)File;
}

ML_FUNCTION(MLFileOpen) {
//!file
//@file::open
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ML_CHECK_ARG_TYPE(1, MLStringT);
	const char *Path = ml_string_value(Args[0]);
	const char *Mode = ml_string_value(Args[1]);
	FILE *Handle = fopen(Path, Mode);
	if (!Handle) return ml_error("FileError", "failed to open %s in mode %s: %s", Path, Mode, strerror(errno));
	ml_file_t *File = new(ml_file_t);
	File->Type = MLFileT;
	File->Handle = Handle;
	GC_register_finalizer(File, (void *)ml_file_finalize, 0, 0, 0);
	return (ml_value_t *)File;
}

typedef struct {
	const ml_type_t *Type;
	DIR *Handle;
	ml_value_t *Entry;
	int Index;
} ml_dir_t;

ML_TYPE(MLDirT, (MLIteratableT), "directory");

ML_METHOD("read", MLDirT) {
	ml_dir_t *Dir = (ml_dir_t *)Args[0];
	struct dirent *Entry = readdir(Dir->Handle);
	if (!Entry) return MLNil;
	return ml_string(GC_strdup(Entry->d_name), -1);
}

static void ML_TYPED_FN(ml_iter_key, MLDirT, ml_state_t *Caller, ml_dir_t *Dir) {
	ML_RETURN(ml_integer(Dir->Index));
}

static void ML_TYPED_FN(ml_iter_value, MLDirT, ml_state_t *Caller, ml_dir_t *Dir) {
	ML_RETURN(Dir->Entry);
}

static void ML_TYPED_FN(ml_iter_next, MLDirT, ml_state_t *Caller, ml_dir_t *Dir) {
	struct dirent *Entry = readdir(Dir->Handle);
	if (!Entry) {
		closedir(Dir->Handle);
		Dir->Handle = NULL;
		ML_RETURN(MLNil);
	}
	++Dir->Index;
	Dir->Entry = ml_string(GC_strdup(Entry->d_name), -1);
	ML_RETURN(Dir);
}

static void ML_TYPED_FN(ml_iterate, MLDirT, ml_state_t *Caller, ml_dir_t *Dir) {
	struct dirent *Entry = readdir(Dir->Handle);
	if (!Entry) {
		closedir(Dir->Handle);
		Dir->Handle = NULL;
		ML_RETURN(MLNil);
	}
	Dir->Index = 1;
	Dir->Entry = ml_string(GC_strdup(Entry->d_name), -1);
	ML_RETURN(Dir);
}

static void ml_dir_finalize(ml_dir_t *Dir, void *Data) {
	if (Dir->Handle) {
		closedir(Dir->Handle);
		Dir->Handle = NULL;
	}
}

ML_FUNCTION(MLDirOpen) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	const char *Path = ml_string_value(Args[0]);
	DIR *Handle = opendir(Path);
	if (!Handle) return ml_error("FileError", "failed to open %s: %s", Path, strerror(errno));
	ml_dir_t *Dir = new(ml_dir_t);
	Dir->Type = MLDirT;
	Dir->Handle = Handle;
	GC_register_finalizer(Dir, (void *)ml_dir_finalize, 0, 0, 0);
	return (ml_value_t *)Dir;
}

void ml_file_init(stringmap_t *Globals) {
#include "ml_file_init.c"
	if (Globals) {
		stringmap_insert(Globals, "file", MLFileT);
		stringmap_insert(MLFileT->Exports, "of", MLFileOpen);
		stringmap_insert(Globals, "dir", MLDirT);
		stringmap_insert(MLDirT->Exports, "of", MLDirOpen);
	}
}
