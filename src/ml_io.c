#include "ml_io.h"
#include "ml_macros.h"
#include <gc/gc.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

ml_type_t *MLStreamT;
ML_METHOD_DECL(Read, NULL);
ML_METHOD_DECL(Write, NULL);

ml_value_t *ml_io_read(ml_state_t *Caller, ml_value_t *Value, void *Address, int Count) {
	typeof(ml_io_read) *function = ml_typed_fn_get(Value->Type, ml_io_read);
	if (function) return function(Caller, Value, Address, Count);
	ml_buffer_t *Buffer = new(ml_buffer_t);
	Buffer->Type = MLBufferT;
	Buffer->Address = Address;
	Buffer->Size = Count;
	ml_value_t **Args = anew(ml_value_t *, 2);
	Args[0] = Value;
	Args[1] = (ml_value_t *)Buffer;
	return ReadMethod->Type->call(Caller, ReadMethod, 2, Args);
}

ml_value_t *ml_io_write(ml_state_t *Caller, ml_value_t *Value, void *Address, int Count) {
	typeof(ml_io_write) *function = ml_typed_fn_get(Value->Type, ml_io_write);
	if (function) return function(Caller, Value, Address, Count);
	ml_buffer_t *Buffer = new(ml_buffer_t);
	Buffer->Type = MLBufferT;
	Buffer->Address = Address;
	Buffer->Size = Count;
	ml_value_t **Args = anew(ml_value_t *, 3);
	Args[0] = Value;
	Args[1] = (ml_value_t *)Buffer;
	return WriteMethod->Type->call(Caller, WriteMethod, 2, Args);
}

ML_METHODX("write", MLStreamT, MLStringT) {
	return ml_io_write(Caller, Args[0], ml_string_value(Args[1]), ml_string_length(Args[1]));
}

typedef struct ml_fd_t {
	const ml_type_t *Type;
	int Fd;
} ml_fd_t;

ml_type_t *MLFdT;

ml_value_t *ml_fd_new(int Fd) {
	ml_fd_t *Stream = new(ml_fd_t);
	Stream->Type = MLFdT;
	Stream->Fd = Fd;
	return (ml_value_t *)Stream;
}

static ml_value_t *ML_TYPED_FN(ml_io_read, MLFdT, ml_state_t *Caller, ml_fd_t *Stream, void *Address, int Count) {
	ssize_t Actual = read(Stream->Fd, Address, Count);
	ml_value_t *Result;
	if (Actual < 0) {
		Result = ml_error("ReadError", strerror(errno));
	} else {
		Result = ml_integer(Actual);
	}
	ML_CONTINUE(Caller, Result);
}

ML_METHOD(ReadMethod, MLFdT, MLBufferT) {
	ml_fd_t *Stream = (ml_fd_t *)Args[0];
	ml_buffer_t *Buffer = (ml_buffer_t *)Args[1];
	ssize_t Actual = read(Stream->Fd, Buffer->Address, Buffer->Size);
	if (Actual < 0) {
		return ml_error("ReadError", strerror(errno));
	} else {
		return ml_integer(Actual);
	}
}

static ml_value_t *ML_TYPED_FN(ml_io_write, MLFdT, ml_state_t *Caller, ml_fd_t *Stream, void *Address, int Count) {
	ssize_t Actual = write(Stream->Fd, Address, Count);
	ml_value_t *Result;
	if (Actual < 0) {
		Result = ml_error("WriteError", strerror(errno));
	} else {
		Result = ml_integer(Actual);
	}
	ML_CONTINUE(Caller, Result);
}

ML_METHOD(WriteMethod, MLFdT, MLBufferT) {
	ml_fd_t *Stream = (ml_fd_t *)Args[0];
	ml_buffer_t *Buffer = (ml_buffer_t *)Args[1];
	ssize_t Actual = write(Stream->Fd, Buffer->Address, Buffer->Size);
	if (Actual < 0) {
		return ml_error("WriteError", strerror(errno));
	} else {
		return ml_integer(Actual);
	}
}

void ml_io_init(stringmap_t *Globals) {
	MLStreamT = ml_type(MLAnyT, "stream");
	MLFdT = ml_type(MLStreamT, "fd");
#include "ml_io_init.c"
	if (Globals) {
		stringmap_insert(Globals, "io", ml_module("io",
			"buffer", ml_function(NULL, ml_buffer),
			"stdin", ml_fd_new(STDIN_FILENO),
			"stdout", ml_fd_new(STDOUT_FILENO),
			"stderr", ml_fd_new(STDERR_FILENO),
			"read", ReadMethod,
			"write", WriteMethod,
		NULL));
	}
}
