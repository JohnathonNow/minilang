#include <gc.h>
#include <string.h>
#include "minilang.h"
#include "ml_macros.h"
#include "ml_object.h"

#undef ML_CATEGORY
#define ML_CATEGORY "object"

typedef struct ml_class_t ml_class_t;
typedef struct ml_object_t ml_object_t;

struct ml_class_t {
	ml_type_t Base;
	ml_value_t *Initializer;
	ml_value_t *Call;
	stringmap_t Fields[1];
};

typedef struct {
	const ml_type_t *Type;
	ml_value_t *Value;
} ml_field_t;

static ml_value_t *ml_field_deref(ml_field_t *Field) {
	return Field->Value;
}

static void ml_field_assign(ml_state_t *Caller, ml_field_t *Field, ml_value_t *Value) {
	Field->Value = Value;
	ML_RETURN(Value);
}

static void ml_field_call(ml_state_t *Caller, ml_field_t *Field, int Count, ml_value_t **Args) {
	return ml_call(Caller, Field->Value, Count, Args);
}

ML_TYPE(MLFieldT, (), "field",
//!internal
	.deref = (void *)ml_field_deref,
	.assign = (void *)ml_field_assign,
	.call = (void *)ml_field_call
);

struct ml_object_t {
	ml_class_t *Type;
	ml_field_t Fields[];
};

ML_INTERFACE(MLObjectT, (), "object");
// Parent type of all object classes.

static void ML_TYPED_FN(ml_value_find_refs, MLObjectT, ml_object_t *Value, void *Data, ml_value_ref_fn RefFn) {
	if (!RefFn(Data, (ml_value_t *)Value)) return;
	int Size = Value->Type->Fields->Size;
	for (int I = 0; I < Size; ++I) ml_value_find_refs(Value->Fields[I].Value, Data, RefFn);
}

ML_METHOD("::", MLObjectT, MLStringT) {
//<Object
//<Field
//>field
// Retrieves the field :mini:`Field` from :mini:`Object`. Mainly intended for unpacking objects.
	ml_object_t *Object = (ml_object_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	uintptr_t Offset = (uintptr_t)stringmap_search(Object->Type->Fields, Name);
	if (!Offset) return ml_error("NameError", "Type %s has no field %s", Object->Type->Base.Name, Name);
	return (ml_value_t *)((char *)Object + Offset);
}

static void ml_class_call(ml_state_t *Caller, ml_type_t *Class, int Count, ml_value_t **Args) {
	ml_value_t *Constructor = Class->Constructor;
	return ml_call(Caller, Constructor, Count, Args);
}

extern ml_cfunctionx_t MLClass[];

ML_TYPE(MLClassT, (MLTypeT), "class",
//!object
// Type of all object classes.
	.call = (void *)ml_class_call,
	.Constructor = (ml_value_t *)MLClass
);

typedef struct {
	ml_object_t *Object;
	ml_stringbuffer_t *Buffer;
	int Comma;
} ml_object_stringer_t;

static int field_string(const char *Name, void *Offset, ml_object_stringer_t *Stringer) {
	if (Stringer->Comma++) ml_stringbuffer_write(Stringer->Buffer, ", ", 2);
	ml_stringbuffer_write(Stringer->Buffer, Name, strlen(Name));
	ml_stringbuffer_write(Stringer->Buffer, " is ", 4);
	ml_stringbuffer_simple_append(Stringer->Buffer, ((ml_field_t *)((char *)Stringer->Object + (uintptr_t)Offset))->Value);
	return 0;
}

ML_METHOD("append", MLStringBufferT, MLObjectT) {
	ml_object_t *Object = (ml_object_t *)Args[1];
	ml_object_stringer_t Stringer = {Object, (ml_stringbuffer_t *)Args[0], 0};
	ml_stringbuffer_printf(Stringer.Buffer, "%s(", Object->Type->Base.Name);
	stringmap_foreach(Object->Type->Fields, &Stringer, (void *)field_string);
	ml_stringbuffer_put(Stringer.Buffer, ')');
	return MLSome;
}

ml_value_t *ml_field_fn(void *Data, int Count, ml_value_t **Args) {
	ml_object_t *Object = (ml_object_t *)Args[0];
	return (ml_value_t *)((char *)Object + (uintptr_t)Data);
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Object;
	ml_value_t *Args[];
} ml_init_state_t;

static void ml_init_state_run(ml_init_state_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	ML_RETURN(State->Object);
}

static void ml_object_call(ml_state_t *Caller, ml_object_t *Object, int Count, ml_value_t **Args) {
	ml_value_t **Args2 = ml_alloc_args(Count + 1);
	memmove(Args2 + 1, Args, Count * sizeof(ml_value_t *));
	Args2[0] = (ml_value_t *)Object;
	return ml_call(Caller, Object->Type->Call, Count + 1, Args2);
}

static void ml_object_constructor_fn(ml_state_t *Caller, ml_class_t *Class, int Count, ml_value_t **Args) {
	int NumFields = Class->Fields->Size;
	ml_object_t *Object = xnew(ml_object_t, NumFields, ml_field_t);
	Object->Type = Class;
	ml_field_t *Slot = Object->Fields;
	for (int I = NumFields; --I >= 0; ++Slot) {
		Slot->Type = MLFieldT;
		Slot->Value = MLNil;
	}
	if (Class->Initializer) {
		ml_init_state_t *State = xnew(ml_init_state_t, Count + 1, ml_value_t *);
		State->Args[0] = (ml_value_t *)Object;
		for (int I = 0; I < Count; ++I) State->Args[I + 1] = Args[I];
		State->Base.run = (void *)ml_init_state_run;
		State->Base.Caller = Caller;
		State->Base.Context = Caller->Context;
		State->Object = (ml_value_t *)Object;
		return ml_call(State, Class->Initializer, Count + 1, State->Args);
	}
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Arg = ml_deref(Args[I]);
		if (ml_is_error(Arg)) ML_RETURN(Arg);
		if (ml_is(Arg, MLNamesT)) {
			ML_NAMES_CHECKX_ARG_COUNT(I);
			ML_NAMES_FOREACH(Args[I], Iter) {
				++I;
				const char *Name = ml_string_value(Iter->Value);
				void *Offset = stringmap_search(Class->Fields, Name);
				if (!Offset) {
					ML_ERROR("ValueError", "Class %s does not have field %s", Class->Base.Name, Name);
				}
				ml_field_t *Field = (ml_field_t *)((char *)Object + (uintptr_t)Offset);
				Field->Value = Arg;
			}
			break;
		} else if (I > NumFields) {
			break;
		} else {
			Object->Fields[I].Value = Arg;
		}
	}
	ML_RETURN(Object);
}

typedef struct {
	ml_type_t Base;
	ml_type_t *Native;
	ml_value_t *Constructor;
	ml_value_t *Initializer;
} ml_named_type_t;

ML_TYPE(MLNamedTypeT, (MLTypeT), "named-type",
//!internal
	.call = (void *)ml_class_call
);

typedef struct {
	ml_state_t Base;
	ml_value_t *Object;
	ml_type_t *Old, *New;
	ml_value_t *Init;
	int Count;
	ml_value_t *Args[];
} ml_named_init_state_t;

static void ml_named_init_state_run(ml_named_init_state_t *State, ml_value_t *Result) {
	ml_type_t *Old = ml_typeof(Result);
	if (Old == State->Old) {
		Result->Type = State->New;
#ifdef ML_GENERICS
	} else if (Old->Type == MLTypeGenericT && ml_generic_type_args(Old)[0] == State->Old) {
		Result->Type = State->New;
#endif
	} else {
		ML_CONTINUE(State->Base.Caller, Result);
	}
	if (State->Init) {
		State->Object = State->Args[0] = Result;
		State->Base.run = (void *)ml_init_state_run;
		return ml_call(State, State->Init, State->Count, State->Args);
	}
	ML_CONTINUE(State->Base.Caller, Result);
}

static void ml_named_constructor_fn(ml_state_t *Caller, ml_named_type_t *Class, int Count, ml_value_t **Args) {
	ml_named_init_state_t *State = new(ml_named_init_state_t);
	State->Base.run = (void *)ml_named_init_state_run;
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Old = Class->Native;
	State->New = (ml_type_t *)Class;
	return ml_call(State, Class->Native->Constructor, Count, Args);
}

static void ml_named_initializer_fn(ml_state_t *Caller, ml_named_type_t *Class, int Count, ml_value_t **Args) {
	ml_named_init_state_t *State = xnew(ml_named_init_state_t, Count + 1, ml_value_t *);
	for (int I = 0; I < Count; ++I) State->Args[I + 1] = Args[I];
	State->Base.run = (void *)ml_named_init_state_run;
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Old = Class->Native;
	State->New = (ml_type_t *)Class;
	State->Init = Class->Initializer;
	State->Count = Count + 1;
	return ml_call(State, Class->Native->Constructor, 0, NULL);
}

static int add_field(const char *Name, void *Value, ml_class_t *Class) {
	//printf("Adding field %s to class %s\n", Name, Class->Base.Name);
	void **Slot = stringmap_slot(Class->Fields, Name);
	if (!Slot[0]) Slot[0] = &((ml_object_t *)0)->Fields[Class->Fields->Size - 1];
	return 0;
}

typedef struct {
	ml_methods_t *Methods;
	ml_value_t **FieldFns;
	ml_class_t *Class;
} class_setup_t;


static int setup_field(const char *Name, char *Offset, class_setup_t *Setup) {
	//printf("Adding method %s:%s\n", Setup->Class->Base.Name, Name);
	int Index = (Offset - (char *)&((ml_object_t *)0)->Fields) / sizeof(ml_field_t);
	ml_type_t *Types[1] = {(ml_type_t *)Setup->Class};
	ml_method_t *Method = (ml_method_t *)ml_method(Name);
	ml_method_insert(Setup->Methods, Method, Setup->FieldFns[Index], 1, 1, Types);
	return 0;
}

static void setup_fields(ml_state_t *Caller, ml_class_t *Class) {
	static ml_value_t **FieldFns = NULL;
	static int NumFieldFns = 0;
	int NumFields = Class->Fields->Size;
	if (NumFields > NumFieldFns) {
		ml_value_t **NewFieldFns = anew(ml_value_t *, NumFields);
		memcpy(NewFieldFns, FieldFns, NumFieldFns * sizeof(ml_value_t *));
		for (int I = NumFieldFns; I < NumFields; ++I) {
			void *Offset = &((ml_object_t *)0)->Fields[I];
			NewFieldFns[I] = ml_cfunction(Offset, ml_field_fn);
		}
		FieldFns = NewFieldFns;
		NumFieldFns = NumFields;
	}
	class_setup_t Setup = {Caller->Context->Values[ML_METHODS_INDEX], FieldFns, Class};
	stringmap_foreach(Class->Fields, &Setup, (void *)setup_field);
}

extern ml_value_t *CallMethod;

ML_FUNCTIONX(MLClass) {
//!object
//@class
//<Parents...:class
//<Fields...:method
//<Exports...:names
//>class
// Returns a new class inheriting from :mini:`Parents`, with fields :mini:`Fields` and exports :mini:`Exports`. The special exports :mini:`::of` and :mini:`::init` can be set to override the default conversion and initialization behaviour. The :mini:`::new` export will *always* be set to the original constructor for this class.
	int Rank = 0;
	ml_type_t *NativeType = NULL;
	for (int I = 0; I < Count; ++I) {
		if (ml_typeof(Args[I]) == MLMethodT) {
		} else if (ml_is(Args[I], MLClassT)) {
			ml_class_t *Parent = (ml_class_t *)Args[I];
			if (Rank < Parent->Base.Rank) Rank = Parent->Base.Rank;
		} else if (ml_is(Args[I], MLNamedTypeT)) {
			ml_named_type_t *Parent = (ml_named_type_t *)Args[I];
			if (NativeType && NativeType != Parent->Native) {
				ML_ERROR("TypeError", "Classes can not inherit from multiple native types");
			}
			NativeType = Parent->Native;
		} else if (ml_is(Args[I], MLTypeT)) {
			ml_type_t *Parent = (ml_type_t *)Args[I];
			if (Parent->NoInherit) {
				ML_ERROR("TypeError", "Classes can not inherit from <%s>", Parent->Name);
			}
			if (!Parent->Interface) {
				if (NativeType && NativeType != Parent) {
					ML_ERROR("TypeError", "Classes can not inherit from multiple native types");
				}
				NativeType = Parent;
			}
			if (Rank < Parent->Rank) Rank = Parent->Rank;
		} else if (ml_is(Args[I], MLNamesT)) {
			break;
		} else {
			ML_ERROR("TypeError", "Unexpected argument type: <%s>", ml_typeof(Args[I])->Name);
		}
	}
	if (NativeType) {
		ml_named_type_t *Class = new(ml_named_type_t);
		Class->Base.Type = MLNamedTypeT;
		asprintf((char **)&Class->Base.Name, "named-%s:%lx", NativeType->Name, (uintptr_t)Class);
		Class->Base.hash = NativeType->hash;
		Class->Base.call = NativeType->call;
		Class->Base.deref = NativeType->deref;
		Class->Base.assign = NativeType->assign;
		Class->Base.Rank = Rank + 1;
		Class->Native = NativeType;
		ml_value_t *Constructor = ml_cfunctionx(Class, (void *)ml_named_constructor_fn);
		Class->Base.Constructor = Constructor;
		for (int I = 0; I < Count; ++I) {
			if (ml_is(Args[I], MLTypeT)) {
				ml_type_t *Parent = (ml_type_t *)Args[I];
				ml_type_add_parent((ml_type_t *)Class, Parent);
			} else if (ml_is(Args[I], MLNamesT)) {
				ML_NAMES_CHECKX_ARG_COUNT(I);
				ML_NAMES_FOREACH(Args[I], Iter) {
					ml_value_t *Key = Iter->Value;
					const char *Name = ml_string_value(Key);
					ml_value_t *Value = Args[++I];
					stringmap_insert(Class->Base.Exports, Name, Value);
					ml_value_set_name(Value, Name);
					if (!strcmp(Name, "of")) {
						Class->Base.Constructor = Value;
					} else if (!strcmp(Name, "init")) {
						Class->Initializer = Value;
						Class->Base.Constructor = ml_cfunctionx(Class, (void *)ml_named_initializer_fn);
					}
				}
				break;
			}
		}
		stringmap_insert(Class->Base.Exports, "new", Constructor);
		ML_RETURN(Class);
	} else {
		ml_class_t *Class = new(ml_class_t);
		Class->Base.Type = MLClassT;
		asprintf((char **)&Class->Base.Name, "object:%lx", (uintptr_t)Class);
		Class->Base.hash = ml_default_hash;
		Class->Base.call = (void *)ml_object_call;
		Class->Base.deref = ml_default_deref;
		Class->Base.assign = ml_default_assign;
		Class->Base.Rank = Rank + 1;
		ml_value_t *Constructor = ml_cfunctionx(Class, (void *)ml_object_constructor_fn);
		Class->Base.Constructor = Constructor;
		Class->Call = CallMethod;
		for (int I = 0; I < Count; ++I) {
			if (ml_typeof(Args[I]) == MLMethodT) {
				add_field(ml_method_name(Args[I]), NULL, Class);
			} else if (ml_is(Args[I], MLClassT)) {
				ml_class_t *Parent = (ml_class_t *)Args[I];
				stringmap_foreach(Parent->Fields, Class, (void *)add_field);
				ml_type_add_parent((ml_type_t *)Class, (ml_type_t *)Parent);
			} else if (ml_is(Args[I], MLTypeT)) {
				ml_type_t *Parent = (ml_type_t *)Args[I];
				ml_type_add_parent((ml_type_t *)Class, Parent);
			} else if (ml_is(Args[I], MLNamesT)) {
				ML_NAMES_CHECKX_ARG_COUNT(I);
				ML_NAMES_FOREACH(Args[I], Iter) {
					ml_value_t *Key = Iter->Value;
					const char *Name = ml_string_value(Key);
					ml_value_t *Value = Args[++I];
					stringmap_insert(Class->Base.Exports, Name, Value);
					ml_value_set_name(Value, Name);
					if (!strcmp(Name, "of")) {
						Class->Base.Constructor = Value;
					} else if (!strcmp(Name, "init")) {
						Class->Initializer = Value;
					} else if (!strcmp(Name, "call")) {
						Class->Call = Value;
					}
				}
				break;
			}
		}
		ml_type_add_parent((ml_type_t *)Class, MLObjectT);
		setup_fields(Caller, Class);
		stringmap_insert(Class->Base.Exports, "new", Constructor);
		ML_RETURN(Class);
	}
}

static void ML_TYPED_FN(ml_value_set_name, MLClassT, ml_class_t *Class, const char *Name) {
	Class->Base.Name = Name;
}

typedef struct ml_property_t {
	const ml_type_t *Type;
	ml_value_t *Value, *Setter;
} ml_property_t;

static ml_value_t *ml_property_deref(ml_property_t *Property) {
	return Property->Value;
}

static void ml_property_assign(ml_state_t *Caller, ml_property_t *Property, ml_value_t *Value) {
	return ml_call(Caller, Property->Setter, 1, &Value);
}

static void ml_property_call(ml_state_t *Caller, ml_property_t *Property, int Count, ml_value_t **Args) {
	return ml_call(Caller, Property->Value, Count, Args);
}

extern ml_cfunctionx_t MLProperty[];

ML_TYPE(MLPropertyT, (), "property",
// A value with an associated setter function.
	.deref = (void *)ml_property_deref,
	.assign = (void *)ml_property_assign,
	.call = (void *)ml_property_call,
	.Constructor = (ml_value_t *)MLProperty
);

ML_FUNCTIONX(MLProperty) {
//@property
//<Value
//<Set:function
//>property
// Returns a new property which dereferences to :mini:`Value`. Assigning to the property will call :mini:`Set(NewValue)`.
	ML_CHECKX_ARG_COUNT(2);
	ml_property_t *Property = new(ml_property_t);
	Property->Type = MLPropertyT;
	Property->Value = Args[0];
	Property->Setter = Args[1];
	ML_RETURN(Property);
}

size_t ml_class_size(const ml_value_t *Value) {
	return ((ml_class_t *)Value)->Fields->Size;
}

static int ml_class_field_fn(const char *Name, void *Index, const void **Data) {
	if (*Data == Index) {
		*Data = Name;
		return 1;
	}
	return 0;
}

const char *ml_class_field_name(const ml_value_t *Value, size_t Field) {
	void *Name = &((ml_object_t *)0)->Fields[Field];
	if (stringmap_foreach(((ml_class_t *)Value)->Fields, &Name, (void *)ml_class_field_fn)) {
		return (const char *)Name;
	} else {
		return NULL;
	}
}

size_t ml_object_size(const ml_value_t *Value) {
	return ((ml_class_t *)ml_typeof(Value))->Fields->Size;
}

ml_value_t *ml_object_field(const ml_value_t *Value, size_t Field) {
	return ((ml_object_t *)Value)->Fields[Field].Value;
}

typedef struct {
	ml_type_t Base;
	ml_value_t *Values[];
} ml_enum_t;

typedef struct {
#ifdef ML_NANBOXING
	ml_int64_t Base;
#else
	ml_integer_t Base;
#endif
	ml_value_t *Name;
} ml_enum_value_t;

static long ml_enum_value_hash(ml_enum_value_t *Value, ml_hash_chain_t *Chain) {
	return (long)Value->Base.Type + Value->Base.Value;
}

#ifdef ML_NANBOXING
#define MLIntegerT MLInt64T
#endif

ML_TYPE(MLEnumValueT, (MLIntegerT), "enum-value");
//@enum::value
// An instance of an enumeration type.

#ifdef ML_NANBOXING
#undef MLIntegerT
#endif

ML_METHOD("append", MLStringBufferT, MLEnumValueT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_enum_value_t *Value = (ml_enum_value_t *)Args[1];
	ml_stringbuffer_write(Buffer, ml_string_value(Value->Name), ml_string_length(Value->Name));
	return Args[0];
}

ML_FUNCTION(MLEnum) {
//@enum
//<Values...:string
//>enum
// Returns a new enumeration type.
//$= let day := enum("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun")
//$= day::Wed
//$= day::Fri + 0
	for (int I = 0; I < Count; ++I) ML_CHECK_ARG_TYPE(I, MLStringT);
	ml_enum_t *Enum = xnew(ml_enum_t, Count, ml_value_t *);
	Enum->Base.Type = MLEnumT;
	asprintf((char **)&Enum->Base.Name, "enum:%lx", (uintptr_t)Enum);
	Enum->Base.deref = ml_default_deref;
	Enum->Base.assign = ml_default_assign;
	Enum->Base.hash = (void *)ml_enum_value_hash;
	Enum->Base.call = ml_default_call;
	Enum->Base.Rank = 1;
	ml_type_init((ml_type_t *)Enum, MLEnumValueT, NULL);
	Enum->Base.Exports[0] = (stringmap_t)STRINGMAP_INIT;
	for (int I = 0; I < Count; ++I) {
		ml_enum_value_t *Value = new(ml_enum_value_t);
		Value->Base.Type = (ml_type_t *)Enum;
		Value->Name = Args[I];
		Enum->Values[I] = (ml_value_t *)Value;
		Value->Base.Value = I + 1;
		stringmap_insert(Enum->Base.Exports, ml_string_value(Args[I]), Value);
	}
	return (ml_value_t *)Enum;
}

ml_type_t *ml_enum(const char *TypeName, ...) {
	va_list Args;
	int Size = 0;
	va_start(Args, TypeName);
	while (va_arg(Args, const char *)) ++Size;
	va_end(Args);
	ml_enum_t *Enum = xnew(ml_enum_t, Size, ml_value_t *);
	Enum->Base.Type = MLEnumT;
	Enum->Base.Name = TypeName;
	Enum->Base.deref = ml_default_deref;
	Enum->Base.assign = ml_default_assign;
	Enum->Base.hash = (void *)ml_enum_value_hash;
	Enum->Base.call = ml_default_call;
	Enum->Base.Rank = 1;
	ml_type_init((ml_type_t *)Enum, MLEnumValueT, NULL);
	Enum->Base.Exports[0] = (stringmap_t)STRINGMAP_INIT;
	int Index = 0;
	va_start(Args, TypeName);
	const char *String;
	while ((String = va_arg(Args, const char *))) {
		ml_value_t *Name = ml_string(String, -1);
		ml_enum_value_t *Value = new(ml_enum_value_t);
		Value->Base.Type = (ml_type_t *)Enum;
		Value->Name = Name;
		Enum->Values[Index++] = (ml_value_t *)Value;
		Value->Base.Value = Index;
		stringmap_insert(Enum->Base.Exports, String, Value);
	}
	va_end(Args);
	return (ml_type_t *)Enum;
}

ml_type_t *ml_enum2(const char *TypeName, ...) {
	va_list Args;
	int Size = 0;
	va_start(Args, TypeName);
	while (va_arg(Args, const char *)) {
		++Size;
		va_arg(Args, int);
	}
	va_end(Args);
	ml_enum_t *Enum = xnew(ml_enum_t, Size, ml_value_t *);
	Enum->Base.Type = MLEnumT;
	Enum->Base.Name = TypeName;
	Enum->Base.deref = ml_default_deref;
	Enum->Base.assign = ml_default_assign;
	Enum->Base.hash = (void *)ml_enum_value_hash;
	Enum->Base.call = ml_default_call;
	Enum->Base.Rank = 1;
	ml_type_init((ml_type_t *)Enum, MLEnumValueT, NULL);
	Enum->Base.Exports[0] = (stringmap_t)STRINGMAP_INIT;
	int Index = 0;
	va_start(Args, TypeName);
	const char *String;
	while ((String = va_arg(Args, const char *))) {
		ml_value_t *Name = ml_string(String, -1);
		ml_enum_value_t *Value = new(ml_enum_value_t);
		Value->Base.Type = (ml_type_t *)Enum;
		Value->Name = Name;
		Enum->Values[Index++] = (ml_value_t *)Value;
		Value->Base.Value = va_arg(Args, int);
		stringmap_insert(Enum->Base.Exports, String, Value);
	}
	va_end(Args);
	return (ml_type_t *)Enum;
}

static void ML_TYPED_FN(ml_value_set_name, MLEnumT, ml_enum_t *Enum, const char *Name) {
	Enum->Base.Name = Name;
}

ml_value_t *ml_enum_value(ml_type_t *Type, uint64_t Index) {
	const ml_enum_t *Enum = (ml_enum_t *)Type;
	ml_enum_value_t **Values = (ml_enum_value_t **)Enum->Values;
	for (int I = 0; I < Enum->Base.Exports->Size; ++I) {
		if (Values[I]->Base.Value == Index) return (ml_value_t *)Values[I];
	}
	return ml_error("EnumError", "Invalid enum index");
}

uint64_t ml_enum_value_value(ml_value_t *Value) {
	return (uint64_t)((ml_enum_value_t *)Value)->Base.Value;
}

const char *ml_enum_value_name(ml_value_t *Value) {
	return ml_string_value(((ml_enum_value_t *)Value)->Name);
}

static void ml_enum_call(ml_state_t *Caller, ml_enum_t *Enum, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Arg = ml_deref(Args[0]);
	if (ml_is(Arg, MLStringT)) {
		ml_value_t *Value = stringmap_search(Enum->Base.Exports, ml_string_value(Arg));
		if (!Value) ML_ERROR("EnumError", "Invalid enum name");
		ML_RETURN(Value);
	} else if (ml_is(Arg, MLIntegerT)) {
		int Index = ml_integer_value_fast(Arg);
		if (Index <= 0 || Index > Enum->Base.Exports->Size) ML_ERROR("EnumError", "Invalid enum index");
		ML_RETURN(Enum->Values[Index - 1]);
	} else {
		ML_ERROR("TypeError", "Expected <integer> or <string> not <%s>", ml_typeof(Arg)->Name);
	}
}

ML_TYPE(MLEnumT, (MLTypeT, MLSequenceT), "enum",
// The base type of enumeration types.
	.call = (void *)ml_enum_call,
	.Constructor = (void *)MLEnum
);

typedef struct {
	ml_value_t *Index;
	uint64_t Value;
} ml_enum_case_t;

typedef struct {
	ml_type_t *Type;
	ml_enum_t *Enum;
	ml_enum_case_t Cases[];
} ml_enum_switch_t;

static void ml_enum_switch(ml_state_t *Caller, ml_enum_switch_t *Switch, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Arg = ml_deref(Args[0]);
	if (!ml_is(Arg, (ml_type_t *)Switch->Enum)) {
		ML_ERROR("TypeError", "expected %s for argument 1", Switch->Enum->Base.Name);
	}
	uint64_t Value = ml_enum_value_value(Arg);
	for (ml_enum_case_t *Case = Switch->Cases;; ++Case) {
		if (Case->Value == Value) ML_RETURN(Case->Index);
		if (Case->Value == UINT64_MAX) ML_RETURN(Case->Index);
	}
	ML_RETURN(MLNil);
}

ML_TYPE(MLEnumSwitchT, (MLFunctionT), "enum-switch",
//!internal
	.call = (void *)ml_enum_switch
);

static ml_value_t *ml_enum_switch_fn(ml_enum_t *Enum, int Count, ml_value_t **Args) {
	int Total = 1;
	for (int I = 0; I < Count; ++I) {
		ML_CHECK_ARG_TYPE(I, MLListT);
		Total += ml_list_length(Args[I]);
	}
	ml_enum_switch_t *Switch = xnew(ml_enum_switch_t, Total, ml_enum_case_t);
	Switch->Type = MLEnumSwitchT;
	Switch->Enum = Enum;
	ml_enum_case_t *Case = Switch->Cases;
	for (int I = 0; I < Count; ++I) {
		ML_LIST_FOREACH(Args[I], Iter) {
			ml_value_t *Value = Iter->Value;
			if (ml_is(Value, (ml_type_t *)Enum)) {
				Case->Value = ml_enum_value_value(Value);
			} else if (ml_is(Value, MLStringT)) {
				ml_value_t *EnumValue = stringmap_search(Enum->Base.Exports, ml_string_value(Value));
				if (!EnumValue) return ml_error("EnumError", "Invalid enum name");
				Case->Value = ml_enum_value_value(EnumValue);
			} else {
				return ml_error("ValueError", "Unsupported value in enum case");
			}
			Case->Index = ml_integer(I);
			++Case;
		}
	}
	Case->Value = UINT64_MAX;
	Case->Index = ml_integer(Count);
	return (ml_value_t *)Switch;
}

ML_METHOD(MLCompilerSwitch, MLEnumT) {
//!internal
	return ml_inline_call_macro(ml_cfunction(Args[0], (ml_callback_t)ml_enum_switch_fn));
}

ML_METHOD("count", MLEnumT) {
//<Enum
//>integer
// Returns the size of the enumeration :mini:`Enum`.
//$= let day := enum("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun")
//$= day:count
	ml_enum_t *Enum = (ml_enum_t *)Args[0];
	return ml_integer(Enum->Base.Exports->Size);
}

typedef struct {
	ml_type_t *Type;
	ml_value_t **Values;
	int Index, Size;
} ml_enum_iter_t;

ML_TYPE(MLEnumIterT, (), "enum-iter");
//!internal

static void ML_TYPED_FN(ml_iterate, MLEnumT, ml_state_t *Caller, ml_enum_t *Enum) {
	int Size = Enum->Base.Exports->Size;
	if (!Size) ML_RETURN(MLNil);
	ml_enum_iter_t *Iter = new(ml_enum_iter_t);
	Iter->Type = MLEnumIterT;
	Iter->Values = Enum->Values;
	Iter->Index = 0;
	Iter->Size = Size;
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_next, MLEnumIterT, ml_state_t *Caller, ml_enum_iter_t *Iter) {
	if (++Iter->Index == Iter->Size) ML_RETURN(MLNil);
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLEnumIterT, ml_state_t *Caller, ml_enum_iter_t *Iter) {
	ML_RETURN(ml_integer(Iter->Index + 1));
}

static void ML_TYPED_FN(ml_iter_value, MLEnumIterT, ml_state_t *Caller, ml_enum_iter_t *Iter) {
	ML_RETURN(Iter->Values[Iter->Index]);
}

typedef struct {
	ml_type_t *Type;
	ml_value_t **Current, **Base, **Limit;
	int Index, Count;
} ml_enum_range_iter_t;

ML_TYPE(MLEnumRangeIterT, (), "enum-range-iter");
//!internal

typedef struct {
	ml_type_t *Type;
	ml_enum_t *Enum;
	int Min, Max;
} ml_enum_range_t;

ML_TYPE(MLEnumRangeT, (MLSequenceT), "enum-range");
// A range of enum values.

ML_METHOD("..", MLEnumValueT, MLEnumValueT) {
//<Min
//<Max
//>enum::range
// Returns a range of enum values. :mini:`Min` and :mini:`Max` must belong to the same enumeration.
//$= let day := enum("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun")
//$= day::Mon .. day::Fri
	ml_enum_value_t *ValueA = (ml_enum_value_t *)Args[0];
	ml_enum_value_t *ValueB = (ml_enum_value_t *)Args[1];
	if (ValueA->Base.Type != ValueB->Base.Type) {
		return ml_error("TypeError", "Enum types do not match");
	}
	ml_enum_range_t *Range = new(ml_enum_range_t);
	Range->Type = MLEnumRangeT;
	Range->Enum = (ml_enum_t *)ValueA->Base.Type;
	Range->Min = ValueA->Base.Value - 1;
	Range->Max = ValueB->Base.Value - 1;
	return (ml_value_t *)Range;
}

static void ML_TYPED_FN(ml_iterate, MLEnumRangeT, ml_state_t *Caller, ml_enum_range_t *Range) {
	int Count = Range->Max - Range->Min;
	if (Count == 0) ML_RETURN(MLNil);
	int Size = Range->Enum->Base.Exports->Size;
	if (Count < 0) Count += Size;
	ml_enum_range_iter_t *Iter = new(ml_enum_range_iter_t);
	Iter->Type = MLEnumRangeIterT;
	Iter->Index = 1;
	Iter->Count = Count + 1;
	ml_value_t **Base = Iter->Base = Range->Enum->Values;
	Iter->Current = Base + Range->Min;
	Iter->Limit = Base + Size;
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_next, MLEnumRangeIterT, ml_state_t *Caller, ml_enum_range_iter_t *Iter) {
	if (--Iter->Count) {
		++Iter->Current;
		if (Iter->Current == Iter->Limit) Iter->Current = Iter->Base;
		++Iter->Index;
		ML_RETURN(Iter);
	} else {
		ML_RETURN(MLNil);
	}
}

static void ML_TYPED_FN(ml_iter_key, MLEnumRangeIterT, ml_state_t *Caller, ml_enum_range_iter_t *Iter) {
	ML_RETURN(ml_integer(Iter->Index));
}

static void ML_TYPED_FN(ml_iter_value, MLEnumRangeIterT, ml_state_t *Caller, ml_enum_range_iter_t *Iter) {
	ML_RETURN(Iter->Current[0]);
}

typedef struct {
	ml_type_t Base;
	ml_value_t *Names[];
} ml_flags_t;

#ifdef ML_NANBOXING
typedef ml_int64_t ml_flags_value_t;
#else
typedef ml_integer_t ml_flags_value_t;
#endif

static void ml_flags_call(ml_state_t *Caller, ml_flags_t *Flags, int Count, ml_value_t **Args) {
	ml_flags_value_t *Value = new(ml_flags_value_t);
	Value->Type = (ml_type_t *)Flags;
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Arg = ml_deref(Args[I]);
		if (ml_is(Arg, MLStringT)) {
			ml_value_t *Flag = stringmap_search(Flags->Base.Exports, ml_string_value(Arg));
			if (!Flag) ML_ERROR("FlagError", "Invalid flag name");
			Value->Value |= ml_flags_value_value(Flag);
		} else if (ml_is(Arg, MLIntegerT)) {
			uint64_t Flag = ml_integer_value_fast(Arg);
			if (Flag >= (1L << Flags->Base.Exports->Size)) ML_ERROR("FlagError", "Invalid flags value");
			Value->Value |= Flag;
		} else {
			ML_ERROR("TypeError", "Expected <integer> or <string> not <%s>", ml_typeof(Arg)->Name);
		}
	}
	ML_RETURN(Value);
}

ML_TYPE(MLFlagsT, (MLTypeT), "flags",
// The base type of flag types.
	.call = (void *)ml_flags_call
);

static long ml_flag_value_hash(ml_flags_value_t *Value, ml_hash_chain_t *Chain) {
	return (long)Value->Type + Value->Value;
}

#ifdef ML_NANBOXING
#define MLIntegerT MLInt64T
#endif

ML_TYPE(MLFlagsValueT, (MLIntegerT), "flag-value");
//@flags::value
// An instance of a flags type.

#ifdef ML_NANBOXING
#undef MLIntegerT
#endif

typedef struct {
	ml_stringbuffer_t *Buffer;
	uint64_t Value;
	int Length;
} ml_flags_value_append_t;

static int ml_flags_value_append(const char *Name, ml_flags_value_t *Flags, ml_flags_value_append_t *Append) {
	if ((Append->Value & Flags->Value) == Flags->Value) {
		ml_stringbuffer_t *Buffer = Append->Buffer;
		if (Buffer->Length > Append->Length) ml_stringbuffer_put(Buffer, '|');
		ml_stringbuffer_write(Buffer, Name, strlen(Name));
	}
	return 0;
}

ML_METHOD("append", MLStringBufferT, MLFlagsValueT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_flags_value_t *Value = (ml_flags_value_t *)Args[1];
	ml_flags_value_append_t Append[1] = {{Buffer, Value->Value, Buffer->Length}};
	stringmap_foreach(Value->Type->Exports, Append, (void *)ml_flags_value_append);
	return Buffer->Length > Append->Length ? MLSome : MLNil;
}

ML_METHODV(MLFlagsT, MLStringT) {
//<Name/1
//>flags
// Returns a new flags type, where :mini:`Name/i` has value $2^{i-1}$.
//$= let mode := flags("Read", "Write", "Execute")
//$= mode::Read
//$= mode::Read + mode::Write
	for (int I = 1; I < Count; ++I) ML_CHECK_ARG_TYPE(I, MLStringT);
	ml_flags_t *Flags = xnew(ml_flags_t, Count, ml_value_t *);
	Flags->Base.Type = MLFlagsT;
	asprintf((char **)&Flags->Base.Name, "flags:%lx", (uintptr_t)Flags);
	Flags->Base.deref = ml_default_deref;
	Flags->Base.assign = ml_default_assign;
	Flags->Base.hash = (void *)ml_flag_value_hash;
	Flags->Base.call = ml_default_call;
	Flags->Base.Rank = 1;
	ml_type_init((ml_type_t *)Flags, MLFlagsValueT, NULL);
	Flags->Base.Exports[0] = (stringmap_t)STRINGMAP_INIT;
	uint64_t Flag = 1;
	for (int I = 0; I < Count; ++I) {
		ml_flags_value_t *Value = new(ml_flags_value_t);
		Value->Type = (ml_type_t *)Flags;
		Flags->Names[I] = Args[I];
		Value->Value = Flag;
		Flag <<= 1;
		stringmap_insert(Flags->Base.Exports, ml_string_value(Args[I]), Value);
	}
	return (ml_value_t *)Flags;
}

ML_METHODV(MLFlagsT, MLNamesT) {
//<Name,Value
//>flags
// Returns a new flags type
// Returns a new flags type, where :mini:`Name/i` has value :mini:`Value/i`.
//$= let mode := flags(Read is 1, Write is 4, Execute is 32)
//$= mode::Read
//$= mode::Read + mode::Write
	ML_NAMES_CHECK_ARG_COUNT(0);
	for (int I = 1; I < Count; ++I) ML_CHECK_ARG_TYPE(I, MLIntegerT);
	ml_flags_t *Flags = xnew(ml_flags_t, Count - 1, ml_value_t *);
	Flags->Base.Type = MLFlagsT;
	asprintf((char **)&Flags->Base.Name, "flags:%lx", (uintptr_t)Flags);
	Flags->Base.deref = ml_default_deref;
	Flags->Base.assign = ml_default_assign;
	Flags->Base.hash = (void *)ml_flag_value_hash;
	Flags->Base.call = ml_default_call;
	Flags->Base.Rank = 1;
	ml_type_init((ml_type_t *)Flags, MLFlagsValueT, NULL);
	Flags->Base.Exports[0] = (stringmap_t)STRINGMAP_INIT;
	int I = 0;
	ML_NAMES_FOREACH(Args[0], Iter) {
		ml_flags_value_t *Value = new(ml_flags_value_t);
		Value->Type = (ml_type_t *)Flags;
		Flags->Names[I] = Iter->Value;
		Value->Value = ml_integer_value(Args[++I]);
		stringmap_insert(Flags->Base.Exports, ml_string_value(Iter->Value), Value);
	}
	return (ml_value_t *)Flags;
}

ml_type_t *ml_flags(const char *TypeName, ...) {
	va_list Args;
	int Size = 0;
	va_start(Args, TypeName);
	while (va_arg(Args, const char *)) ++Size;
	va_end(Args);
	ml_flags_t *Flags = xnew(ml_flags_t, Size, ml_value_t *);
	Flags->Base.Type = MLFlagsT;
	Flags->Base.Name = TypeName;
	Flags->Base.deref = ml_default_deref;
	Flags->Base.assign = ml_default_assign;
	Flags->Base.hash = (void *)ml_flag_value_hash;
	Flags->Base.call = ml_default_call;
	Flags->Base.Rank = 1;
	ml_type_init((ml_type_t *)Flags, MLFlagsValueT, NULL);
	Flags->Base.Exports[0] = (stringmap_t)STRINGMAP_INIT;
	uint64_t Flag = 1;
	int Index = 0;
	va_start(Args, TypeName);
	const char *String;
	while ((String = va_arg(Args, const char *))) {
		ml_value_t *Name = ml_string(String, -1);
		ml_flags_value_t *Value = new(ml_flags_value_t);
		Value->Type = (ml_type_t *)Flags;
		Flags->Names[Index++] = Name;
		Value->Value = Flag;
		Flag <<= 1;
		stringmap_insert(Flags->Base.Exports, String, Value);
	}
	va_end(Args);
	return (ml_type_t *)Flags;
}

ml_type_t *ml_flags2(const char *TypeName, ...) {
	va_list Args;
	int Size = 0;
	va_start(Args, TypeName);
	while (va_arg(Args, const char *)) {
		++Size;
		va_arg(Args, int);
	}
	va_end(Args);
	ml_flags_t *Flags = xnew(ml_flags_t, Size, ml_value_t *);
	Flags->Base.Type = MLFlagsT;
	Flags->Base.Name = TypeName;
	Flags->Base.deref = ml_default_deref;
	Flags->Base.assign = ml_default_assign;
	Flags->Base.hash = (void *)ml_flag_value_hash;
	Flags->Base.call = ml_default_call;
	Flags->Base.Rank = 1;
	ml_type_init((ml_type_t *)Flags, MLFlagsValueT, NULL);
	Flags->Base.Exports[0] = (stringmap_t)STRINGMAP_INIT;
	int Index = 0;
	va_start(Args, TypeName);
	const char *String;
	while ((String = va_arg(Args, const char *))) {
		ml_value_t *Name = ml_string(String, -1);
		ml_flags_value_t *Value = new(ml_flags_value_t);
		Value->Type = (ml_type_t *)Flags;
		Flags->Names[Index++] = Name;
		Value->Value = va_arg(Args, int);
		stringmap_insert(Flags->Base.Exports, String, Value);
	}
	va_end(Args);
	return (ml_type_t *)Flags;
}

static void ML_TYPED_FN(ml_value_set_name, MLFlagsT, ml_flags_t *Flags, const char *Name) {
	Flags->Base.Name = Name;
}

ml_value_t *ml_flags_value(ml_type_t *Type, uint64_t Flags) {
	ml_flags_value_t *Value = new(ml_flags_value_t);
	Value->Type = Type;
	Value->Value = (int64_t)Flags;
	return (ml_value_t *)Value;
}

uint64_t ml_flags_value_value(ml_value_t *Value) {
	return (uint64_t)((ml_flags_value_t *)Value)->Value;
}

const char *ml_flags_value_name(ml_value_t *Value) {
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_flags_value_append_t Append[1] = {{Buffer, ml_flags_value_value(Value), Buffer->Length}};
	stringmap_foreach(Value->Type->Exports, Append, (void *)ml_flags_value_append);
	return ml_stringbuffer_get_string(Buffer);
}

typedef struct {
	ml_value_t *Index;
	uint64_t Value;
} ml_flags_case_t;

typedef struct {
	ml_type_t *Type;
	ml_flags_t *Flags;
	ml_flags_case_t Cases[];
} ml_flags_switch_t;

static void ml_flags_switch(ml_state_t *Caller, ml_flags_switch_t *Switch, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Arg = ml_deref(Args[0]);
	if (!ml_is(Arg, (ml_type_t *)Switch->Flags)) {
		ML_ERROR("TypeError", "expected %s for argument 1", Switch->Flags->Base.Name);
	}
	uint64_t Value = ml_enum_value_value(Arg);
	for (ml_flags_case_t *Case = Switch->Cases;; ++Case) {
		if ((Case->Value & Value) == Case->Value) ML_RETURN(Case->Index);
	}
	ML_RETURN(MLNil);
}

ML_TYPE(MLFlagsSwitchT, (MLFunctionT), "flags-switch",
//!internal
	.call = (void *)ml_flags_switch
);

static ml_value_t *ml_flags_switch_fn(ml_flags_t *Flags, int Count, ml_value_t **Args) {
	int Total = 1;
	for (int I = 0; I < Count; ++I) {
		ML_CHECK_ARG_TYPE(I, MLListT);
		Total += ml_list_length(Args[I]);
	}
	ml_flags_switch_t *Switch = xnew(ml_flags_switch_t, Total, ml_flags_case_t);
	Switch->Type = MLFlagsSwitchT;
	Switch->Flags = Flags;
	ml_flags_case_t *Case = Switch->Cases;
	for (int I = 0; I < Count; ++I) {
		ML_LIST_FOREACH(Args[I], Iter) {
			ml_value_t *Value = Iter->Value;
			if (ml_is(Value, (ml_type_t *)Flags)) {
				Case->Value = ml_flags_value_value(Value);
			} else if (ml_is(Value, MLStringT)) {
				ml_value_t *FlagsValue = stringmap_search(Flags->Base.Exports, ml_string_value(Value));
				if (!FlagsValue) return ml_error("FlagsError", "Invalid flags name");
				Case->Value = ml_flags_value_value(FlagsValue);
			} else if (ml_is(Value, MLTupleT)) {
				ml_tuple_t *Tuple = (ml_tuple_t *)Value;
				for (int J = 0; J < Tuple->Size; ++J) {
					ml_value_t *Value = Tuple->Values[J];
					if (!ml_is(Value, MLStringT)) return ml_error("ValueError", "Unsupported value in flags case");
					ml_value_t *FlagsValue = stringmap_search(Flags->Base.Exports, ml_string_value(Tuple->Values[J]));
					if (!FlagsValue) return ml_error("FlagsError", "Invalid flags name");
					Case->Value |= ml_flags_value_value(FlagsValue);
				}
			} else {
				return ml_error("ValueError", "Unsupported value in flags case");
			}
			Case->Index = ml_integer(I);
			++Case;
		}
	}
	Case->Value = 0;
	Case->Index = ml_integer(Count);
	return (ml_value_t *)Switch;
}

ML_METHOD(MLCompilerSwitch, MLFlagsT) {
//!internal
	return ml_inline_call_macro(ml_cfunction(Args[0], (ml_callback_t)ml_flags_switch_fn));
}

ML_METHOD("+", MLFlagsValueT, MLFlagsValueT) {
//<Flags/1
//<Flags/2
//>flags::value
// Returns the union of :mini:`Flags/1` and :mini:`Flags/2`. :mini:`Flags/1` and :mini:`Flags/2` must have the same flags type.
//$= let mode := flags("Read", "Write", "Execute")
//$= mode::Read + mode::Write
	ml_flags_value_t *A = (ml_flags_value_t *)Args[0];
	ML_CHECK_ARG_TYPE(1, A->Type);
	ml_flags_value_t *B = (ml_flags_value_t *)Args[1];
	ml_flags_value_t *C = new(ml_flags_value_t);
	C->Type = A->Type;
	C->Value = A->Value | B->Value;
	return (ml_value_t *)C;
}

ML_METHOD("-", MLFlagsValueT, MLFlagsValueT) {
//<Flags/1
//<Flags/2
//>flags::value
// Returns the difference of :mini:`Flags/1` and :mini:`Flags/2`. :mini:`Flags/1` and :mini:`Flags/2` must have the same flags type.
//$= let mode := flags("Read", "Write", "Execute")
//$= mode("Read", "Write") - mode::Write
	ml_flags_value_t *A = (ml_flags_value_t *)Args[0];
	ML_CHECK_ARG_TYPE(1, A->Type);
	ml_flags_value_t *B = (ml_flags_value_t *)Args[1];
	ml_flags_value_t *C = new(ml_flags_value_t);
	C->Type = A->Type;
	C->Value = A->Value & ~B->Value;
	return (ml_value_t *)C;
}

ML_METHOD("<", MLFlagsValueT, MLFlagsValueT) {
//<Flags/1
//<Flags/2
//>flags::value
// Returns the :mini:`Flags/2` if it contains all of :mini:`Flags/1`. :mini:`Flags/1` and :mini:`Flags/2` must have the same flags type.
//$= let mode := flags("Read", "Write", "Execute")
//$= mode("Read", "Write") < mode("Read", "Write", "Execute")
//$= mode("Read", "Write", "Execute") < mode("Read", "Write")
	ml_flags_value_t *A = (ml_flags_value_t *)Args[0];
	ML_CHECK_ARG_TYPE(1, A->Type);
	ml_flags_value_t *B = (ml_flags_value_t *)Args[1];
	if ((A->Value & B->Value) == A->Value) {
		return Args[1];
	} else {
		return MLNil;
	}
}

ML_METHOD("<=", MLFlagsValueT, MLFlagsValueT) {
//<Flags/1
//<Flags/2
//>flags::value
// Returns the :mini:`Flags/2` if it contains all of :mini:`Flags/1`. :mini:`Flags/1` and :mini:`Flags/2` must have the same flags type.
//$= let mode := flags("Read", "Write", "Execute")
//$= mode("Read", "Write") <= mode("Read", "Write", "Execute")
//$= mode("Read", "Write", "Execute") <= mode("Read", "Write")
	ml_flags_value_t *A = (ml_flags_value_t *)Args[0];
	ML_CHECK_ARG_TYPE(1, A->Type);
	ml_flags_value_t *B = (ml_flags_value_t *)Args[1];
	if ((A->Value & B->Value) == A->Value) {
		return Args[1];
	} else {
		return MLNil;
	}
}

ML_METHOD(">", MLFlagsValueT, MLFlagsValueT) {
//<Flags/1
//<Flags/2
//>flags::value
// Returns the :mini:`Flags/2` if it is contained in :mini:`Flags/1`. :mini:`Flags/1` and :mini:`Flags/2` must have the same flags type.
//$= let mode := flags("Read", "Write", "Execute")
//$= mode("Read", "Write") > mode("Read", "Write", "Execute")
//$= mode("Read", "Write", "Execute") > mode("Read", "Write")
	ml_flags_value_t *A = (ml_flags_value_t *)Args[0];
	ML_CHECK_ARG_TYPE(1, A->Type);
	ml_flags_value_t *B = (ml_flags_value_t *)Args[1];
	if ((A->Value & B->Value) == B->Value) {
		return Args[1];
	} else {
		return MLNil;
	}
}

ML_METHOD(">=", MLFlagsValueT, MLFlagsValueT) {
//<Flags/1
//<Flags/2
//>flags::value
// Returns the :mini:`Flags/2` if it is contained in :mini:`Flags/1`. :mini:`Flags/1` and :mini:`Flags/2` must have the same flags type.
//$= let mode := flags("Read", "Write", "Execute")
//$= mode("Read", "Write") >= mode("Read", "Write", "Execute")
//$= mode("Read", "Write", "Execute") >= mode("Read", "Write")
	ml_flags_value_t *A = (ml_flags_value_t *)Args[0];
	ML_CHECK_ARG_TYPE(1, A->Type);
	ml_flags_value_t *B = (ml_flags_value_t *)Args[1];
	if ((A->Value & B->Value) == B->Value) {
		return Args[1];
	} else {
		return MLNil;
	}
}

typedef struct {
	ml_value_t *Values;
	uint64_t Value;
} ml_flags_value_list_t;

static int ml_flags_value_list(const char *Name, ml_flags_value_t *Flags, ml_flags_value_list_t *List) {
	if ((List->Value & Flags->Value) == Flags->Value) ml_list_put(List->Values, (ml_value_t *)Flags);
	return 0;
}

ML_METHOD(MLListT, MLFlagsValueT) {
	ml_flags_value_list_t List[1] = {{ml_list(), ml_flags_value_value(Args[0])}};
	stringmap_foreach(Args[0]->Type->Exports, &List, (void *)ml_flags_value_list);
	return List->Values;
}

void ml_object_init(stringmap_t *Globals) {
#include "ml_object_init.c"
	if (Globals) {
		stringmap_insert(Globals, "property", MLPropertyT);
		stringmap_insert(Globals, "object", MLObjectT);
		stringmap_insert(Globals, "class", MLClassT);
		stringmap_insert(Globals, "enum", MLEnumT);
		stringmap_insert(Globals, "flags", MLFlagsT);
	}
}
