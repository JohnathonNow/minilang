#include "ml_list.h"
#include "minilang.h"
#include "ml_macros.h"
#include <string.h>

ML_METHOD_DECL(MLListOf, "list::of");

static ml_list_node_t *ml_list_index(ml_list_t *List, int Index) {
	int Length = List->Length;
	if (Index <= 0) Index += Length + 1;
	if (Index > Length) return NULL;
	if (Index == Length) return List->Tail;
	if (Index < 1) return NULL;
	if (Index == 1) return List->Head;
	int CachedIndex = List->CachedIndex;
	if (CachedIndex < 0) {
		CachedIndex = 0;
		List->CachedNode = List->Head;
	} else if (CachedIndex > Length) {

	}
	switch (Index - CachedIndex) {
	case -1: {
		List->CachedIndex = Index;
		return (List->CachedNode = List->CachedNode->Prev);
	}
	case 0: return List->CachedNode;
	case 1: {
		List->CachedIndex = Index;
		return (List->CachedNode = List->CachedNode->Next);
	}
	}
	List->CachedIndex = Index;
	ml_list_node_t *Node;
	if (2 * Index < CachedIndex) {
		Node = List->Head;
		int Steps = Index - 1;
		do Node = Node->Next; while (--Steps);
	} else if (Index < CachedIndex) {
		Node = List->CachedNode;
		int Steps = CachedIndex - Index;
		do Node = Node->Prev; while (--Steps);
	} else if (2 * Index < CachedIndex + Length) {
		Node = List->CachedNode;
		int Steps = Index - CachedIndex;
		do Node = Node->Next; while (--Steps);
	} else {
		Node = List->Tail;
		int Steps = Length - Index;
		do Node = Node->Prev; while (--Steps);
	}
	return (List->CachedNode = Node);
}

static void ml_list_call(ml_state_t *Caller, ml_list_t *List, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Arg = ml_deref(Args[0]);
	//if (ml_is_error(Arg)) ML_RETURN(Arg);
	if (ml_typeof(Arg) != MLIntegerT) ML_RETURN(ml_error("TypeError", "List index must be an integer"));
	int Index = ml_integer_value(Args[0]);
	ml_list_node_t *Node = ml_list_index(List, Index);
	ML_RETURN(Node ? Node->Value : MLNil);
}

ML_TYPE(MLListT, (MLFunctionT, MLIteratableT), "list",
//!list
// A list of elements.
	.call = (void *)ml_list_call
);

static ml_value_t *ml_list_node_deref(ml_list_node_t *Node) {
	return Node->Value;
}

static ml_value_t *ml_list_node_assign(ml_list_node_t *Node, ml_value_t *Value) {
	return (Node->Value = Value);
}

ML_TYPE(MLListNodeT, (), "list-node",
//!list
// A node in a :mini:`list`.
// Dereferencing a :mini:`listnode` returns the corresponding value from the :mini:`list`.
// Assigning to a :mini:`listnode` updates the corresponding value in the :mini:`list`.
	.deref = (void *)ml_list_node_deref,
	.assign = (void *)ml_list_node_assign
);

static void ML_TYPED_FN(ml_iter_next, MLListNodeT, ml_state_t *Caller, ml_list_node_t *Node) {
	ML_RETURN((ml_value_t *)Node->Next ?: MLNil);
}

static void ML_TYPED_FN(ml_iter_key, MLListNodeT, ml_state_t *Caller, ml_list_node_t *Node) {
	ML_RETURN(ml_integer(Node->Index));
}

static void ML_TYPED_FN(ml_iter_value, MLListNodeT, ml_state_t *Caller, ml_list_node_t *Node) {
	ML_RETURN(Node);
}

ml_value_t *ml_list() {
	ml_list_t *List = new(ml_list_t);
	List->Type = MLListT;
	List->Head = List->Tail = NULL;
	List->Length = 0;
	return (ml_value_t *)List;
}

ML_METHOD(MLListOfMethod) {
	return ml_list();
}

ML_METHOD(MLListOfMethod, MLTupleT) {
	ml_value_t *List = ml_list();
	ml_tuple_t *Tuple = (ml_tuple_t *)Args[0];
	for (int I = 0; I < Tuple->Size; ++I) ml_list_put(List, Tuple->Values[I]);
	return List;
}

ml_value_t *ml_list_from_array(ml_value_t **Values, int Length) {
	ml_value_t *List = ml_list();
	for (int I = 0; I < Length; ++I) ml_list_put(List, Values[I]);
	return List;
}

void ml_list_to_array(ml_value_t *List, ml_value_t **Values) {
	int I = 0;
	for (ml_list_node_t *Node = ((ml_list_t *)List)->Head; Node; Node = Node->Next, ++I) {
		Values[I] = Node->Value;
	}
}

void ml_list_grow(ml_value_t *List0, int Count) {
	ml_list_t *List = (ml_list_t *)List0;
	for (int I = 0; I < Count; ++I) ml_list_put(List0, MLNil);
	List->CachedIndex = 1;
	List->CachedNode = List->Head;
}

void ml_list_push(ml_value_t *List0, ml_value_t *Value) {
	ml_list_t *List = (ml_list_t *)List0;
	ml_list_node_t *Node = new(ml_list_node_t);
	Node->Type = MLListNodeT;
	Node->Value = Value;
	List->ValidIndices = 0;
	if ((Node->Next = List->Head)) {
		List->Head->Prev = Node;
#ifdef USE_GENERICS
		if (List->Type->Args) {
			const ml_type_t *Type = List->Type->Args[1];
			if (Type != ml_typeof(Value)) {
				const ml_type_t *Type2 = ml_type_max(Type, Value->Type);
				if (Type != Type2) {
					List->Type = ml_type_generic(MLListT, 1, &Type2);
				}
			}
		}
#endif
	} else {
		List->Tail = Node;
#ifdef USE_GENERICS
		const ml_type_t *Type = ml_typeof(Value);
		List->Type = ml_type_generic(MLListT, 1, &Type);
#endif
	}
	List->CachedNode = List->Head = Node;
	List->CachedIndex = 1;
	++List->Length;
}

void ml_list_put(ml_value_t *List0, ml_value_t *Value) {
	ml_list_t *List = (ml_list_t *)List0;
	ml_list_node_t *Node = new(ml_list_node_t);
	Node->Type = MLListNodeT;
	Node->Value = Value;
	List->ValidIndices = 0;
	if ((Node->Prev = List->Tail)) {
		List->Tail->Next = Node;
#ifdef USE_GENERICS
		if (List->Type->Args) {
			const ml_type_t *Type = List->Type->Args[1];
			if (Type != ml_typeof(Value)) {
				const ml_type_t *Type2 = ml_type_max(Type, Value->Type);
				if (Type != Type2) {
					List->Type = ml_type_generic(MLListT, 1, &Type2);
				}
			}
		}
#endif
	} else {
		List->Head = Node;
#ifdef USE_GENERICS
		const ml_type_t *Type = ml_typeof(Value);
		List->Type = ml_type_generic(MLListT, 1, &Type);
#endif
	}
	List->CachedNode = List->Tail = Node;
	List->CachedIndex = ++List->Length;
}

ml_value_t *ml_list_pop(ml_value_t *List0) {
	ml_list_t *List = (ml_list_t *)List0;
	ml_list_node_t *Node = List->Head;
	List->ValidIndices = 0;
	if (Node) {
		if ((List->Head = Node->Next)) {
			List->Head->Prev = NULL;
		} else {
			List->Tail = NULL;
		}
		List->CachedNode = List->Head;
		List->CachedIndex = 1;
		--List->Length;
		return Node->Value;
	} else {
		return MLNil;
	}
}

ml_value_t *ml_list_pull(ml_value_t *List0) {
	ml_list_t *List = (ml_list_t *)List0;
	ml_list_node_t *Node = List->Tail;
	List->ValidIndices = 0;
	if (Node) {
		if ((List->Tail = Node->Prev)) {
			List->Tail->Next = NULL;
		} else {
			List->Head = NULL;
		}
		List->CachedNode = List->Tail;
		List->CachedIndex = -List->Length;
		return Node->Value;
	} else {
		return MLNil;
	}
}

ml_value_t *ml_list_get(ml_value_t *List0, int Index) {
	ml_list_node_t *Node = ml_list_index((ml_list_t *)List0, Index);
	return Node ? Node->Value : NULL;
}

ml_value_t *ml_list_set(ml_value_t *List0, int Index, ml_value_t *Value) {
	ml_list_node_t *Node = ml_list_index((ml_list_t *)List0, Index);
	if (Node) {
		ml_value_t *Old = Node->Value;
		Node->Value = Value;
		return Old;
	} else {
		return NULL;
	}
}

int ml_list_foreach(ml_value_t *Value, void *Data, int (*callback)(ml_value_t *, void *)) {
	ML_LIST_FOREACH(Value, Node) if (callback(Node->Value, Data)) return 1;
	return 0;
}

ML_METHOD("count", MLListT) {
//!list
//<List
//>integer
// Returns the length of :mini:`List`
	ml_list_t *List = (ml_list_t *)Args[0];
	return ml_integer(List->Length);
}

ML_METHOD("length", MLListT) {
//!list
//<List
//>integer
// Returns the length of :mini:`List`
	ml_list_t *List = (ml_list_t *)Args[0];
	return ml_integer(List->Length);
}

ML_METHOD("filter", MLListT, MLFunctionT) {
//!list
//<List
//<Filter
//>list
// Removes every :mini:`Value` from :mini:`List` for which :mini:`Function(Value)` returns :mini:`nil` and returns those values in a new list.
	ml_list_t *List = (ml_list_t *)Args[0];
	ml_list_t *Drop = new(ml_list_t);
	Drop->Type = MLListT;
	ml_value_t *Filter = Args[1];
	ml_list_node_t *Node = List->Head;
	ml_list_node_t **KeepSlot = &List->Head;
	ml_list_node_t *KeepTail = NULL;
	ml_list_node_t **DropSlot = &Drop->Head;
	ml_list_node_t *DropTail = NULL;
	List->Head = NULL;
	int Length = 0;
	while (Node) {
		ml_value_t *Result = ml_simple_inline(Filter, 1, Node->Value);
		if (ml_is_error(Result)) {
			List->Head = List->Tail = NULL;
			List->Length = 0;
			return Result;
		}
		if (Result == MLNil) {
			Node->Prev = DropSlot[0];
			DropSlot[0] = Node;
			DropSlot = &Node->Next;
			DropTail = Node;
		} else {
			Node->Prev = KeepSlot[0];
			KeepSlot[0] = Node;
			KeepSlot = &Node->Next;
			++Length;
			KeepTail = Node;
		}
		Node = Node->Next;
	}
	Drop->Tail = DropTail;
	if (DropTail) DropTail->Next = NULL;
	Drop->Length = List->Length - Length;
	Drop->CachedIndex = Drop->Length;
	Drop->CachedNode = DropTail;
	List->Tail = KeepTail;
	if (KeepTail) KeepTail->Next = NULL;
	List->Length = Length;
	List->CachedIndex = Length;
	List->CachedNode = KeepTail;
	List->ValidIndices = 0;
	return (ml_value_t *)Drop;
}

ML_METHOD("[]", MLListT, MLIntegerT) {
//!list
//<List
//<Index
//>listnode | nil
// Returns the :mini:`Index`-th node in :mini:`List` or :mini:`nil` if :mini:`Index` is outside the range of :mini:`List`.
// Indexing starts at :mini:`1`. Negative indices are counted from the end of the list, with :mini:`-1` returning the last node.
	ml_list_t *List = (ml_list_t *)Args[0];
	int Index = ml_integer_value(Args[1]);
	return (ml_value_t *)ml_list_index(List, Index) ?: MLNil;
}

typedef struct {
	const ml_type_t *Type;
	ml_list_node_t *Head;
	int Length;
} ml_list_slice_t;

static ml_value_t *ml_list_slice_deref(ml_list_slice_t *Slice) {
	ml_value_t *List = ml_list();
	ml_list_node_t *Node = Slice->Head;
	int Length = Slice->Length;
	while (Node && Length) {
		ml_list_put(List, Node->Value);
		Node = Node->Next;
		--Length;
	}
	return List;
}

static ml_value_t *ml_list_slice_assign(ml_list_slice_t *Slice, ml_value_t *Packed) {
	ml_list_node_t *Node = Slice->Head;
	int Length = Slice->Length;
	int Index = 0;
	while (Node && Length) {
		ml_value_t *Value = ml_unpack(Packed, Index + 1);
		++Index;
		Node->Value = Value;
		Node = Node->Next;
		--Length;
	}
	return Packed;
}

ML_TYPE(MLListSliceT, (), "list-slice",
//!list
// A slice of a list.
	.deref = (void *)ml_list_slice_deref,
	.assign = (void *)ml_list_slice_assign
);

ML_METHOD("[]", MLListT, MLIntegerT, MLIntegerT) {
//!list
//<List
//<From
//<To
//>listslice
// Returns a slice of :mini:`List` starting at :mini:`From` (inclusive) and ending at :mini:`To` (exclusive).
// Indexing starts at :mini:`1`. Negative indices are counted from the end of the list, with :mini:`-1` returning the last node.
	ml_list_t *List = (ml_list_t *)Args[0];
	int Start = ml_integer_value(Args[1]);
	int End = ml_integer_value(Args[2]);
	if (Start <= 0) Start += List->Length + 1;
	if (End <= 0) End += List->Length + 1;
	if (Start <= 0 || End < Start || End > List->Length + 1) return MLNil;
	ml_list_slice_t *Slice = new(ml_list_slice_t);
	Slice->Type = MLListSliceT;
	Slice->Head = ml_list_index(List, Start);
	Slice->Length = End - Start;
	return (ml_value_t *)Slice;
}

static ml_value_t *ML_TYPED_FN(ml_stringbuffer_append, MLListT, ml_stringbuffer_t *Buffer, ml_list_t *List) {
	ml_stringbuffer_add(Buffer, "[", 1);
	ml_list_node_t *Node = List->Head;
	if (Node) {
		ml_stringbuffer_append(Buffer, Node->Value);
		while ((Node = Node->Next)) {
			ml_stringbuffer_add(Buffer, ", ", 2);
			ml_stringbuffer_append(Buffer, Node->Value);
		}
	}
	ml_stringbuffer_add(Buffer, "]", 1);
	return (ml_value_t *)Buffer;
}

ML_METHOD("write", MLStringBufferT, MLListT) {
//!list
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_add(Buffer, "[", 1);
	ml_list_t *List = (ml_list_t *)Args[1];
	ml_list_node_t *Node = List->Head;
	if (Node) {
		ml_stringbuffer_append(Buffer, Node->Value);
		while ((Node = Node->Next)) {
			ml_stringbuffer_add(Buffer, ", ", 2);
			ml_stringbuffer_append(Buffer, Node->Value);
		}
	}
	ml_stringbuffer_add(Buffer, "]", 1);
	return (ml_value_t *)Buffer;
}

ml_value_t *ML_TYPED_FN(ml_unpack, MLListT, ml_list_t *List, int Index) {
	ml_list_node_t *Node = ml_list_index(List, Index);
	return Node ? Node->Value : MLNil;
}

/*typedef struct ml_list_iterator_t {
	const ml_type_t *Type;
	ml_list_node_t *Node;
	long Index;
} ml_list_iterator_t;

static void ML_TYPED_FN(ml_iter_value, MLListIterT, ml_state_t *Caller, ml_list_iterator_t *Iter) {
	ML_RETURN(Iter->Node);
}

static void ML_TYPED_FN(ml_iter_next, MLListIterT, ml_state_t *Caller, ml_list_iterator_t *Iter) {
	if ((Iter->Node = Iter->Node->Next)) {
		++Iter->Index;
		ML_RETURN(Iter);
	} else {
		ML_RETURN(MLNil);
	}
}

static void ML_TYPED_FN(ml_iter_key, MLListIterT, ml_state_t *Caller, ml_list_iterator_t *Iter) {
	ML_RETURN(ml_integer(Iter->Index));
}

ML_TYPE(MLListIterT, (), "list-iterator");
//!internal
*/

static void ML_TYPED_FN(ml_iterate, MLListT, ml_state_t *Caller, ml_list_t *List) {
	if (List->Length) {
		if (!List->ValidIndices) {
			int I = 0;
			for (ml_list_node_t *Node = List->Head; Node; Node = Node->Next) {
				Node->Index = ++I;
			}
			List->ValidIndices = 1;
		}
		ML_RETURN(List->Head);
		/*ml_list_iterator_t *Iter = new(ml_list_iterator_t);
		Iter->Type = MLListIterT;
		Iter->Node = List->Head;
		Iter->Index = 1;
		ML_RETURN(Iter);*/
	} else {
		ML_RETURN(MLNil);
	}
}

ML_METHODV("push", MLListT) {
//!list
//<List
//<Values...: any
//>list
// Pushes :mini:`Values` onto the start of :mini:`List` and returns :mini:`List`.
	ml_value_t *List = Args[0];
	for (int I = 1; I < Count; ++I) ml_list_push(List, Args[I]);
	return Args[0];
}

ML_METHODV("put", MLListT) {
//!list
//<List
//<Values...: MLAnyT
//>list
// Pushes :mini:`Values` onto the end of :mini:`List` and returns :mini:`List`.
	ml_value_t *List = Args[0];
	for (int I = 1; I < Count; ++I) ml_list_put(List, Args[I]);
	return Args[0];
}

ML_METHOD("pop", MLListT) {
//!list
//<List
//>any | nil
// Removes and returns the first element of :mini:`List` or :mini:`nil` if the :mini:`List` is empty.
	return ml_list_pop(Args[0]) ?: MLNil;
}

ML_METHOD("pull", MLListT) {
//!list
//<List
//>any | nil
// Removes and returns the last element of :mini:`List` or :mini:`nil` if the :mini:`List` is empty.
	return ml_list_pull(Args[0]) ?: MLNil;
}

ML_METHOD("copy", MLListT) {
//!list
//<List
//>list
// Returns a (shallow) copy of :mini:`List`.
	ml_value_t *List = ml_list();
	ML_LIST_FOREACH(Args[0], Iter) ml_list_put(List, Iter->Value);
	return List;
}

ML_METHOD("+", MLListT, MLListT) {
//!list
//<List/1
//<List/2
//>list
// Returns a new list with the elements of :mini:`List/1` followed by the elements of :mini:`List/2`.
	ml_value_t *List = ml_list();
	ML_LIST_FOREACH(Args[0], Iter) ml_list_put(List, Iter->Value);
	ML_LIST_FOREACH(Args[1], Iter) ml_list_put(List, Iter->Value);
	return List;
}

ML_METHOD(MLStringOfMethod, MLListT) {
//!list
//<List
//>string
// Returns a string containing the elements of :mini:`List` surrounded by :mini:`[`, :mini:`]` and seperated by :mini:`,`.
	ml_list_t *List = (ml_list_t *)Args[0];
	if (!List->Length) return ml_cstring("[]");
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	const char *Seperator = "[";
	int SeperatorLength = 1;
	ML_LIST_FOREACH(List, Node) {
		ml_stringbuffer_add(Buffer, Seperator, SeperatorLength);
		ml_value_t *Result = ml_stringbuffer_append(Buffer, Node->Value);
		if (ml_is_error(Result)) return Result;
		Seperator = ", ";
		SeperatorLength = 2;
	}
	ml_stringbuffer_add(Buffer, "]", 1);
	return ml_stringbuffer_value(Buffer);
}

ML_METHOD(MLStringOfMethod, MLListT, MLStringT) {
//!list
//<List
//<Seperator
//>string
// Returns a string containing the elements of :mini:`List` seperated by :mini:`Seperator`.
	ml_list_t *List = (ml_list_t *)Args[0];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	const char *Seperator = ml_string_value(Args[1]);
	size_t SeperatorLength = ml_string_length(Args[1]);
	ml_list_node_t *Node = List->Head;
	if (Node) {
		ml_value_t *Result = ml_stringbuffer_append(Buffer, Node->Value);
		if (ml_is_error(Result)) return Result;
		while ((Node = Node->Next)) {
			ml_stringbuffer_add(Buffer, Seperator, SeperatorLength);
			ml_value_t *Result = ml_stringbuffer_append(Buffer, Node->Value);
			if (ml_is_error(Result)) return Result;
		}
	}
	return ml_stringbuffer_value(Buffer);
}

static ml_value_t *ml_list_sort(ml_list_t *List, ml_value_t *Compare) {
	ml_list_node_t *Head = List->Head;
	int InSize = 1;
	for (;;) {
		ml_list_node_t *P = Head;
		ml_list_node_t *Tail = Head = 0;
		int NMerges = 0;
		while (P) {
			NMerges++;
			ml_list_node_t *Q = P;
			int PSize = 0;
			for (int I = 0; I < InSize; I++) {
				PSize++;
				Q = Q->Next;
				if (!Q) break;
			}
			int QSize = InSize;
			ml_list_node_t *E;
			while (PSize > 0 || (QSize > 0 && Q)) {
				if (PSize == 0) {
					E = Q; Q = Q->Next; QSize--;
				} else if (QSize == 0 || !Q) {
					E = P; P = P->Next; PSize--;
				} else {
					ml_value_t *Result = ml_simple_inline(Compare, 2, P->Value, Q->Value);
					if (ml_is_error(Result)) return Result;
					if (Result == MLNil) {
						E = Q; Q = Q->Next; QSize--;
					} else {
						E = P; P = P->Next; PSize--;
					}
				}
				if (Tail) {
					Tail->Next = E;
				} else {
					Head = E;
				}
				E->Prev = Tail;
				Tail = E;
			}
			P = Q;
		}
		Tail->Next = 0;
		if (NMerges <= 1) {
			List->Head = Head;
			List->Tail = Tail;
			List->CachedIndex = 1;
			List->CachedNode = Head;
			break;
		}
		InSize *= 2;
	}
	List->ValidIndices = 0;
	return (ml_value_t *)List;
}

extern ml_value_t *LessMethod;

ML_METHOD("sort", MLListT) {
//!list
//<List
//>List
	return ml_list_sort((ml_list_t *)Args[0], LessMethod);
}

ML_METHOD("sort", MLListT, MLFunctionT) {
//!list
//<List
//<Compare
//>List
	return ml_list_sort((ml_list_t *)Args[0], Args[1]);
}

ML_TYPE(MLNamesT, (), "names",
//!internal
	.call = (void *)ml_list_call
);


void ml_list_init(stringmap_t *Globals) {
#include "ml_list_init.c"
	MLListT->Constructor = MLListOfMethod;
	stringmap_insert(MLListT->Exports, "of", MLListOfMethod);
	stringmap_insert(Globals, "list", MLListT);
	stringmap_insert(Globals, "names", MLNamesT);
}
