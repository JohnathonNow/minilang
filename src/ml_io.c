#include "ml_io.h"
#include "ml_macros.h"
#include <gc/gc.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

ML_TYPE(MLStreamT, (MLAnyT), "stream");
ML_METHOD_DECL(MLIORead, "io::read");
ML_METHOD_DECL(MLIOWrite, "io::write");

void ml_io_read(ml_state_t *Caller, ml_value_t *Value, void *Address, int Count) {
	typeof(ml_io_read) *function = ml_typed_fn_get(ml_typeof(Value), ml_io_read);
	if (function) return function(Caller, Value, Address, Count);
	ml_address_t *Buffer = new(ml_address_t);
	Buffer->Type = MLAddressT;
	Buffer->Value = Address;
	Buffer->Length = Count;
	ml_value_t **Args = ml_alloc_args(2);
	Args[0] = Value;
	Args[1] = (ml_value_t *)Buffer;
	return ml_call(Caller, MLIORead, 2, Args);
}

void ml_io_write(ml_state_t *Caller, ml_value_t *Value, const void *Address, int Count) {
	typeof(ml_io_write) *function = ml_typed_fn_get(ml_typeof(Value), ml_io_write);
	if (function) return function(Caller, Value, Address, Count);
	ml_address_t *Buffer = new(ml_address_t);
	Buffer->Type = MLAddressT;
	Buffer->Value = (void *)Address;
	Buffer->Length = Count;
	ml_value_t **Args = ml_alloc_args(3);
	Args[0] = Value;
	Args[1] = (ml_value_t *)Buffer;
	return ml_call(Caller, MLIOWrite, 2, Args);
}

ML_METHODX("write", MLStreamT, MLStringT) {
	return ml_io_write(Caller, Args[0], ml_string_value(Args[1]), ml_string_length(Args[1]));
}

typedef struct ml_fd_t {
	const ml_type_t *Type;
	int Fd;
} ml_fd_t;

ML_TYPE(MLFdT, (MLStreamT), "fd");

ml_value_t *ml_fd_new(int Fd) {
	ml_fd_t *Stream = new(ml_fd_t);
	Stream->Type = MLFdT;
	Stream->Fd = Fd;
	return (ml_value_t *)Stream;
}

static void ML_TYPED_FN(ml_io_read, MLFdT, ml_state_t *Caller, ml_fd_t *Stream, void *Address, int Count) {
	ssize_t Actual = read(Stream->Fd, Address, Count);
	ml_value_t *Result;
	if (Actual < 0) {
		Result = ml_error("ReadError", strerror(errno));
	} else {
		Result = ml_integer(Actual);
	}
	ML_CONTINUE(Caller, Result);
}

ML_METHOD(MLIORead, MLFdT, MLAddressT) {
//@io::read
	ml_fd_t *Stream = (ml_fd_t *)Args[0];
	ml_address_t *Buffer = (ml_address_t *)Args[1];
	ssize_t Actual = read(Stream->Fd, Buffer->Value, Buffer->Length);
	if (Actual < 0) {
		return ml_error("ReadError", strerror(errno));
	} else {
		return ml_integer(Actual);
	}
}

static void ML_TYPED_FN(ml_io_write, MLFdT, ml_state_t *Caller, ml_fd_t *Stream, void *Address, int Count) {
	ssize_t Actual = write(Stream->Fd, Address, Count);
	ml_value_t *Result;
	if (Actual < 0) {
		Result = ml_error("WriteError", strerror(errno));
	} else {
		Result = ml_integer(Actual);
	}
	ML_RETURN(Result);
}

ML_METHOD(MLIOWrite, MLFdT, MLAddressT) {
//@io::write
	ml_fd_t *Stream = (ml_fd_t *)Args[0];
	ml_address_t *Buffer = (ml_address_t *)Args[1];
	ssize_t Actual = write(Stream->Fd, Buffer->Value, Buffer->Length);
	if (Actual < 0) {
		return ml_error("WriteError", strerror(errno));
	} else {
		return ml_integer(Actual);
	}
}

void ml_io_init(stringmap_t *Globals) {
#include "ml_io_init.c"
	if (Globals) {
		stringmap_insert(Globals, "io", ml_module("io",
			"stream", MLStreamT,
			"fd", MLFdT,
			"stdin", ml_fd_new(STDIN_FILENO),
			"stdout", ml_fd_new(STDOUT_FILENO),
			"stderr", ml_fd_new(STDERR_FILENO),
			"read", MLIORead,
			"write", MLIOWrite,
		NULL));
	}
}
