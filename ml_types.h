#ifndef ML_TYPES_H
#define ML_TYPES_H

typedef struct ml_type_t ml_type_t;
typedef struct ml_value_t ml_value_t;
typedef struct ml_function_t ml_function_t;

typedef struct ml_reference_t ml_reference_t;
typedef struct ml_integer_t ml_integer_t;
typedef struct ml_real_t ml_real_t;
typedef struct ml_string_t ml_string_t;
typedef struct ml_regex_t ml_regex_t;
typedef struct ml_list_t ml_list_t;
typedef struct ml_tree_t ml_tree_t;
typedef struct ml_object_t ml_object_t;
typedef struct ml_property_t ml_property_t;
typedef struct ml_closure_t ml_closure_t;
typedef struct ml_method_t ml_method_t;
typedef struct ml_error_t ml_error_t;

typedef struct ml_closure_info_t ml_closure_info_t;

typedef struct ml_list_node_t ml_list_node_t;
typedef struct ml_tree_node_t ml_tree_node_t;

typedef ml_value_t *(*ml_callback_t)(void *Data, int Count, ml_value_t **Args);
typedef ml_value_t *(*ml_getter_t)(void *Data, const char *Name);
typedef ml_value_t *(*ml_setter_t)(void *Data, const char *Name, ml_value_t *Value);

ml_value_t *ml_string_new(void *Data, int Count, ml_value_t **Args);
ml_value_t *ml_list_new(void *Data, int Count, ml_value_t **Args);
ml_value_t *ml_tree_new(void *Data, int Count, ml_value_t **Args);

struct ml_reference_t {
	const ml_type_t *Type;
	ml_value_t **Address;
	ml_value_t *Value[];
};

struct ml_closure_t {
	const ml_type_t *Type;
	ml_closure_info_t *Info;
	ml_value_t *UpValues[];
};

long ml_hash(ml_value_t *Value);
ml_type_t *ml_class(ml_type_t *Parent, const char *Name);

void ml_method_by_name(const char *Method, void *Data, ml_callback_t Function, ...) __attribute__ ((sentinel));
void ml_method_by_value(ml_value_t *Method, void *Data, ml_callback_t Function, ...) __attribute__ ((sentinel));

ml_value_t *ml_string(const char *Value, int Length);
ml_value_t *ml_regex(const char *Value);
ml_value_t *ml_integer(long Value);
ml_value_t *ml_real(double Value);
ml_value_t *ml_list();
ml_value_t *ml_tree();
ml_value_t *ml_function(void *Data, ml_callback_t Function);
ml_value_t *ml_property(void *Data, const char *Name, ml_getter_t Get, ml_setter_t Set, ml_getter_t Next, ml_getter_t Key);
ml_value_t *ml_error(const char *Error, const char *Format, ...) __attribute__ ((format(printf, 2, 3)));
ml_value_t *ml_reference(ml_value_t **Address);
ml_value_t *ml_method(const char *Name);

long ml_integer_value(ml_value_t *Value);
double ml_real_value(ml_value_t *Value);
const char *ml_string_value(ml_value_t *Value);
int ml_string_length(ml_value_t *Value);

const char *ml_error_type(ml_value_t *Value);
const char *ml_error_message(ml_value_t *Value);
int ml_error_trace(ml_value_t *Value, int Level, const char **Source, int *Line);

void ml_closure_hash(ml_value_t *Closure, unsigned char Hash[SHA256_BLOCK_SIZE]);

void ml_list_append(ml_value_t *List, ml_value_t *Value);
int ml_list_length(ml_value_t *List);
void ml_list_to_array(ml_value_t *List, ml_value_t **Array);
int ml_list_foreach(ml_value_t *List, void *Data, int (*callback)(ml_value_t *, void *));
int ml_tree_foreach(ml_value_t *Tree, void *Data, int (*callback)(ml_value_t *, ml_value_t *, void *));

struct ml_type_t {
	const ml_type_t *Parent;
	const char *Name;
	long (*hash)(ml_value_t *);
	ml_value_t *(*call)(ml_value_t *, int, ml_value_t **);
	ml_value_t *(*deref)(ml_value_t *);
	ml_value_t *(*assign)(ml_value_t *, ml_value_t *);
	ml_value_t *(*next)(ml_value_t *);
	ml_value_t *(*key)(ml_value_t *);
};

long ml_default_hash(ml_value_t *Value);
ml_value_t *ml_default_call(ml_value_t *Value, int Count, ml_value_t **Args);
ml_value_t *ml_default_deref(ml_value_t *Ref);
ml_value_t *ml_default_assign(ml_value_t *Ref, ml_value_t *Value);
ml_value_t *ml_default_next(ml_value_t *Iter);
ml_value_t *ml_default_key(ml_value_t *Iter);

extern ml_type_t MLAnyT[];
extern ml_type_t MLNilT[];
extern ml_type_t MLFunctionT[];
extern ml_type_t MLNumberT[];
extern ml_type_t MLIntegerT[];
extern ml_type_t MLRealT[];
extern ml_type_t MLStringT[];
extern ml_type_t MLRegexT[];
extern ml_type_t MLMethodT[];
extern ml_type_t MLReferenceT[];
extern ml_type_t MLListT[];
extern ml_type_t MLTreeT[];
extern ml_type_t MLPropertyT[];
extern ml_type_t MLClosureT[];
extern ml_type_t MLErrorT[];
extern ml_type_t MLErrorValueT[];

struct ml_value_t {
	const ml_type_t *Type;
};

extern ml_value_t MLNil[];
extern ml_value_t MLSome[];

int ml_is(ml_value_t *Value, ml_type_t *Type);

struct ml_function_t {
	const ml_type_t *Type;
	ml_callback_t Callback;
	void *Data;
};

#define ML_STRINGBUFFER_NODE_SIZE 248

typedef struct ml_stringbuffer_t ml_stringbuffer_t;
typedef struct ml_stringbuffer_node_t ml_stringbuffer_node_t;

struct ml_stringbuffer_t {
	const ml_type_t *Type;
	ml_stringbuffer_node_t *Nodes;
	size_t Space, Length;
};

extern ml_type_t MLStringBufferT[1];

#define ML_STRINGBUFFER_INIT (ml_stringbuffer_t){MLStringBufferT, 0,}

ssize_t ml_stringbuffer_add(ml_stringbuffer_t *Buffer, const char *String, size_t Length);
ssize_t ml_stringbuffer_addf(ml_stringbuffer_t *Buffer, const char *Format, ...) __attribute__ ((format(printf, 2, 3)));
char *ml_stringbuffer_get(ml_stringbuffer_t *Buffer);
char *ml_stringbuffer_get_uncollectable(ml_stringbuffer_t *Buffer);
int ml_stringbuffer_foreach(ml_stringbuffer_t *Buffer, void *Data, int (*callback)(const char *, size_t, void *));

struct ml_list_t {
	const ml_type_t *Type;
	ml_list_node_t *Head, *Tail;
	int Length;
};

struct ml_list_node_t {
	ml_list_node_t *Next, *Prev;
	ml_value_t *Value;
};

#endif
