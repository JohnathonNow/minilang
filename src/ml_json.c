#include "ml_json.h"
#include "ml_macros.h"
#include "ml_stream.h"
#include <yajl/yajl_common.h>
#include <yajl/yajl_parse.h>

#undef ML_CATEGORY
#define ML_CATEGORY "json"

// Overview
// JSON values are mapped to Minilang as follows:
//
// * :json:`null` |harr| :mini:`nil`
// * :json:`true` |harr| :mini:`true`
// * :json:`false` |harr| :mini:`false`
// * *integer* |harr| :mini:`integer`
// * *real* |harr| :mini:`real`
// * *string* |harr| :mini:`string`
// * *array* |harr| :mini:`list`
// * *object* |harr| :mini:`map`

#define ML_JSON_STACK_SIZE 10

typedef struct json_stack_t json_stack_t;

struct json_stack_t {
	ml_value_t *Values[ML_JSON_STACK_SIZE];
	json_stack_t *Prev;
	int Index;
};

typedef struct {
	void (*Callback)(void *Data, ml_value_t *Value);
	void *Data;
	ml_value_t *Key, *Value;
	json_stack_t *Stack;
	json_stack_t Stack0;
} json_decoder_t;

static int value_handler(json_decoder_t *Decoder, ml_value_t *Value) {
	if (Decoder->Value) {
		if (Decoder->Key) {
			ml_map_insert(Decoder->Value, Decoder->Key, Value);
			Decoder->Key = NULL;
		} else {
			ml_list_put(Decoder->Value, Value);
		}
	} else {
		Decoder->Callback(Decoder->Data, Value);
	}
	return 1;
}

static int null_handler(json_decoder_t *Decoder) {
	return value_handler(Decoder, MLNil);
}

static int boolean_handler(json_decoder_t *Decoder, int Value) {
	return value_handler(Decoder, ml_boolean(Value));
}

static int integer_handler(json_decoder_t *Decoder, long long Value) {
	return value_handler(Decoder, ml_integer(Value));
}

static int real_handler(json_decoder_t *Decoder, double Value) {
	return value_handler(Decoder, ml_real(Value));
}

static int string_handler(json_decoder_t *Decoder, const char *Value, size_t Length) {
	return value_handler(Decoder, ml_string_copy(Value, Length));
}

static int push_value(json_decoder_t *Decoder, ml_value_t *Value) {
	if (Decoder->Value) {
		if (Decoder->Key) {
			ml_map_insert(Decoder->Value, Decoder->Key, Value);
			Decoder->Key = NULL;
		} else {
			ml_list_put(Decoder->Value, Value);
		}
	}
	json_stack_t *Stack = Decoder->Stack;
	if (Stack->Index == ML_JSON_STACK_SIZE) {
		json_stack_t *NewStack = new(json_stack_t);
		NewStack->Prev = Stack;
		Stack = Decoder->Stack = NewStack;
	}
	Stack->Values[Stack->Index] = Decoder->Value;
	++Stack->Index;
	Decoder->Value = Value;
	return 1;
}

static int pop_value(json_decoder_t *Decoder) {
	json_stack_t *Stack = Decoder->Stack;
	if (Stack->Index == 0) {
		if (!Stack->Prev) return 1;
		Stack = Decoder->Stack = Stack->Prev;
	}
	ml_value_t *Value = Decoder->Value;
	--Stack->Index;
	Decoder->Value = Stack->Values[Stack->Index];
	Stack->Values[Stack->Index] = NULL;
	if (!Decoder->Value) {
		Decoder->Callback(Decoder->Data, Value);
	}
	return 1;
}

static int start_map_handler(json_decoder_t *Decoder) {
	return push_value(Decoder, ml_map());
}

static int map_key_handler(json_decoder_t *Decoder, const char *Key, size_t Length) {
	Decoder->Key = ml_string_copy(Key, Length);
	return 1;
}

static int end_map_handler(json_decoder_t *Decoder) {
	return pop_value(Decoder);
}

static int start_array_handler(json_decoder_t *Decoder) {
	return push_value(Decoder, ml_list());
}

static int end_array_handler(json_decoder_t *Decoder) {
	return pop_value(Decoder);
}

static yajl_callbacks Callbacks = {
	.yajl_null = (void *)null_handler,
	.yajl_boolean = (void *)boolean_handler,
	.yajl_integer = (void *)integer_handler,
	.yajl_double = (void *)real_handler,
	.yajl_number = (void *)NULL,
	.yajl_string = (void *)string_handler,
	.yajl_start_map = (void *)start_map_handler,
	.yajl_map_key = (void *)map_key_handler,
	.yajl_end_map = (void *)end_map_handler,
	.yajl_start_array = (void *)start_array_handler,
	.yajl_end_array = (void *)end_array_handler
};

static void *ml_alloc(void *Ctx, size_t Size) {
	return GC_MALLOC(Size);
}

static void *ml_realloc(void *Ctx, void *Ptr, size_t Size) {
	return GC_REALLOC(Ptr, Size);
}

static void ml_free(void *Ctx, void *Ptr) {
}

static yajl_alloc_funcs AllocFuncs = {ml_alloc, ml_realloc, ml_free, NULL};

static void json_decode_callback(ml_value_t **Result, ml_value_t *Value) {
	Result[0] = Value;
}

ML_FUNCTION(JsonDecode) {
//@json::decode
//<Json
//>any
// Decodes :mini:`Json` into a Minilang value.
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ml_value_t *Result = NULL;
	json_decoder_t Decoder = {0,};
	Decoder.Callback = (void *)json_decode_callback;
	Decoder.Data = &Result;
	Decoder.Stack = &Decoder.Stack0;
	yajl_handle Handle = yajl_alloc(&Callbacks, &AllocFuncs, &Decoder);
	const unsigned char *Text = (const unsigned char *)ml_string_value(Args[0]);
	size_t Length = ml_string_length(Args[0]);
	if (yajl_parse(Handle, Text, Length) == yajl_status_error) {
		const unsigned char *Error = yajl_get_error(Handle, 0, NULL, 0);
		size_t Position = yajl_get_bytes_consumed(Handle);
		return ml_error("JSONError", "@%ld: %s", Position, Error);
	}
	if (yajl_complete_parse(Handle) == yajl_status_error) {
		const unsigned char *Error = yajl_get_error(Handle, 0, NULL, 0);
		size_t Position = yajl_get_bytes_consumed(Handle);
		return ml_error("JSONError", "@%ld: %s", Position, Error);
	}
	return Result ?: ml_error("JSONError", "Incomplete JSON");
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Callback;
	ml_value_t *Args[1];
	yajl_handle Handle;
	json_decoder_t Decoder[1];
} ml_json_decoder_t;

extern ml_type_t MLJsonDecoderT[];

static void ml_json_decode_callback(ml_json_decoder_t *Decoder, ml_value_t *Value) {
	Decoder->Args[0] = Value;
	ml_call((ml_state_t *)Decoder, Decoder->Callback, 1, Decoder->Args);
}

static void ml_json_decoder_run(ml_state_t *State, ml_value_t *Value) {
}

ML_FUNCTIONX(JsonDecoder) {
//@json::decoder
//<Callback
//>json::decoder
// Returns a new JSON decoder that calls :mini:`Callback(Value)` whenever a complete JSON value is written to the decoder.
	ML_CHECKX_ARG_COUNT(1);
	ml_json_decoder_t *Decoder = new(ml_json_decoder_t);
	Decoder->Base.Type = MLJsonDecoderT;
	Decoder->Base.Context = Caller->Context;
	Decoder->Base.run = ml_json_decoder_run;
	Decoder->Callback = Args[0];
	Decoder->Decoder->Callback = (void *)ml_json_decode_callback;
	Decoder->Decoder->Data = Decoder;
	Decoder->Decoder->Stack = &Decoder->Decoder->Stack0;
	Decoder->Handle = yajl_alloc(&Callbacks, &AllocFuncs, &Decoder->Decoder);
	yajl_config(Decoder->Handle, yajl_allow_multiple_values, 1);
	ML_RETURN(Decoder);
}

ML_TYPE(MLJsonDecoderT, (MLStreamT), "json-decoder",
//@json::decoder
// A JSON decoder that can be written to as a stream and calls a user-supplied callback whenever a complete value is parsed.
	.Constructor = (ml_value_t *)JsonDecoder
);

static void ML_TYPED_FN(ml_stream_write, MLJsonDecoderT, ml_state_t *Caller, ml_json_decoder_t *Decoder, const void *Address, int Count) {
	if (yajl_parse(Decoder->Handle, Address, Count) == yajl_status_error) {
		const unsigned char *Error = yajl_get_error(Decoder->Handle, 0, NULL, 0);
		size_t Position = yajl_get_bytes_consumed(Decoder->Handle);
		ML_ERROR("JSONError", "@%ld: %s", Position, Error);
	}
	ML_RETURN(ml_integer(Count));
}

static void ML_TYPED_FN(ml_stream_flush, MLJsonDecoderT, ml_state_t *Caller, ml_json_decoder_t *Decoder) {
	if (yajl_complete_parse(Decoder->Handle) == yajl_status_error) {
		const unsigned char *Error = yajl_get_error(Decoder->Handle, 0, NULL, 0);
		size_t Position = yajl_get_bytes_consumed(Decoder->Handle);
		ML_ERROR("JSONError", "@%ld: %s", Position, Error);
	}
	ML_RETURN(Decoder);
}

static void ml_json_encode_string(ml_stringbuffer_t *Buffer, ml_value_t *Value) {
	ml_stringbuffer_write(Buffer, "\"", 1);
	const unsigned char *String = (const unsigned char *)ml_string_value(Value);
	const unsigned char *End = String + ml_string_length(Value);
	while (String < End) {
		unsigned char Char = *String++;
		switch (Char) {
		case '\r': ml_stringbuffer_write(Buffer, "\\r", 2); break;
		case '\n': ml_stringbuffer_write(Buffer, "\\n", 2); break;
		case '\\': ml_stringbuffer_write(Buffer, "\\\\", 2); break;
		case '\"': ml_stringbuffer_write(Buffer, "\\\"", 2); break;
		case '\f': ml_stringbuffer_write(Buffer, "\\f", 2); break;
		case '\b': ml_stringbuffer_write(Buffer, "\\b", 2); break;
		case '\t': ml_stringbuffer_write(Buffer, "\\t", 2); break;
		default:
			if (Char < 32) {
				ml_stringbuffer_printf(Buffer, "\\u%02x", Char);
			} else {
				ml_stringbuffer_put(Buffer, Char);
			}
			break;
		}
	}
	ml_stringbuffer_write(Buffer, "\"", 1);
}

static ml_value_t *ml_json_encode(ml_stringbuffer_t *Buffer, ml_value_t *Value) {
	if (Value == MLNil) {
		ml_stringbuffer_write(Buffer, "null", 4);
	} else if (ml_is(Value, MLBooleanT)) {
		if (ml_boolean_value(Value)) {
			ml_stringbuffer_write(Buffer, "true", 4);
		} else {
			ml_stringbuffer_write(Buffer, "false", 5);
		}
	} else if (ml_is(Value, MLIntegerT)) {
		ml_stringbuffer_printf(Buffer, "%ld", ml_integer_value(Value));
	} else if (ml_is(Value, MLDoubleT)) {
		ml_stringbuffer_printf(Buffer, "%.20g", ml_real_value(Value));
	} else if (ml_is(Value, MLStringT)) {
		ml_json_encode_string(Buffer, Value);
	} else if (ml_is(Value, MLListT)) {
		ml_list_node_t *Node = ((ml_list_t *)Value)->Head;
		if (Node) {
			ml_stringbuffer_write(Buffer, "[", 1);
			ml_value_t *Error = ml_json_encode(Buffer, Node->Value);
			if (Error) return Error;
			while ((Node = Node->Next)) {
				ml_stringbuffer_write(Buffer, ",", 1);
				ml_value_t *Error = ml_json_encode(Buffer, Node->Value);
				if (Error) return Error;
			}
			ml_stringbuffer_write(Buffer, "]", 1);
		} else {
			ml_stringbuffer_write(Buffer, "[]", 2);
		}
	} else if (ml_is(Value, MLMapT)) {
		ml_map_node_t *Node = ((ml_map_t *)Value)->Head;
		if (Node) {
			ml_stringbuffer_write(Buffer, "{", 1);
			if (!ml_is(Node->Key, MLStringT)) return ml_error("JSONError", "JSON keys must be strings");
			ml_json_encode_string(Buffer, Node->Key);
			ml_stringbuffer_write(Buffer, ":", 1);
			ml_value_t *Error = ml_json_encode(Buffer, Node->Value);
			if (Error) return Error;
			while ((Node = Node->Next)) {
				ml_stringbuffer_write(Buffer, ",", 1);
				if (!ml_is(Node->Key, MLStringT)) return ml_error("JSONError", "JSON keys must be strings");
				ml_json_encode_string(Buffer, Node->Key);
				ml_stringbuffer_write(Buffer, ":", 1);
				ml_value_t *Error = ml_json_encode(Buffer, Node->Value);
				if (Error) return Error;
			}
			ml_stringbuffer_write(Buffer, "}", 1);
		} else {
			ml_stringbuffer_write(Buffer, "{}", 2);
		}
	} else {
		return ml_error("JSONError", "Invalid type for JSON: %s", ml_typeof(Value)->Name);
	}
	return NULL;
}

ML_FUNCTION(JsonEncode) {
//@json::encode
//<Value
//>string|error
// Encodes :mini:`Value` into JSON, raising an error if :mini:`Value` cannot be represented as JSON.
	ML_CHECK_ARG_COUNT(1);
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	return ml_json_encode(Buffer, Args[0]) ?: ml_stringbuffer_get_value(Buffer);
}

extern ml_type_t MLJsonT[];

ML_FUNCTION(MLJson) {
//@json
//<Value:any
//>json
// Encodes :mini:`Value` into JSON.
	ML_CHECK_ARG_COUNT(1);
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_value_t *Error = ml_json_encode(Buffer, Args[0]);
	if (Error) return Error;
	ml_value_t *Json = ml_stringbuffer_to_string(Buffer);
	Json->Type = MLJsonT;
	return (ml_value_t *)Json;
}

ML_TYPE(MLJsonT, (MLStringT), "json",
// Contains a JSON encoded value. Primarily used to distinguish strings containing JSON from other strings (e.g. for CBOR encoding).
	.Constructor = (ml_value_t *)MLJson
);

ML_METHOD("decode", MLJsonT) {
//<Json
//>any|error
// Decodes the JSON string in :mini:`Json` into a Minilang value.
	ml_value_t *Result = NULL;
	json_decoder_t Decoder = {0,};
	Decoder.Callback = (void *)json_decode_callback;
	Decoder.Data = &Result;
	Decoder.Stack = &Decoder.Stack0;
	yajl_handle Handle = yajl_alloc(&Callbacks, &AllocFuncs, &Decoder);
	const unsigned char *Text = (const unsigned char *)ml_string_value(Args[0]);
	size_t Length = ml_string_length(Args[0]);
	if (yajl_parse(Handle, Text, Length) == yajl_status_error) {
		const unsigned char *Error = yajl_get_error(Handle, 0, NULL, 0);
		size_t Position = yajl_get_bytes_consumed(Handle);
		return ml_error("JSONError", "@%ld: %s", Position, Error);
	}
	if (yajl_complete_parse(Handle) == yajl_status_error) {
		const unsigned char *Error = yajl_get_error(Handle, 0, NULL, 0);
		size_t Position = yajl_get_bytes_consumed(Handle);
		return ml_error("JSONError", "@%ld: %s", Position, Error);
	}
	return Result ?: ml_error("JSONError", "Incomplete JSON");
}

#ifdef ML_CBOR

#include "ml_cbor.h"
#include "minicbor/minicbor.h"

static void ML_TYPED_FN(ml_cbor_write, MLJsonT, ml_cbor_writer_t *Writer, ml_string_t *Value) {
	ml_cbor_write_tag(Writer, 262);
	ml_cbor_write_bytes(Writer, Value->Length);
	ml_cbor_write_raw(Writer, Value->Value, Value->Length);
}

static ml_value_t *ml_cbor_read_json(ml_cbor_reader_t *Reader, ml_value_t *Value) {
	if (!ml_is(Value, MLAddressT)) return ml_error("TagError", "Json requires bytes or string");
	ml_value_t *Json = ml_string_copy(ml_address_value(Value), ml_address_length(Value));
	Json->Type = MLJsonT;
	return (ml_value_t *)Json;
}

#endif

void ml_json_init(stringmap_t *Globals) {
#include "ml_json_init.c"
	stringmap_insert(MLJsonT->Exports, "encode", JsonEncode);
	stringmap_insert(MLJsonT->Exports, "decode", JsonDecode);
	stringmap_insert(MLJsonT->Exports, "decoder", MLJsonDecoderT);
	if (Globals) {
		stringmap_insert(Globals, "json", MLJsonT);
	}
#ifdef ML_CBOR
	ml_cbor_default_tag(262, ml_cbor_read_json);
#endif
}
