#ifndef ML_COMPILER2_H
#define ML_COMPILER2_H

#include "ml_compiler.h"
#include "ml_bytecode.h"

typedef enum ml_token_t {
	MLT_NONE,
	MLT_EOL,
	MLT_EOI,
	MLT_AND,
	MLT_CASE,
	MLT_DEBUG,
	MLT_DEF,
	MLT_DO,
	MLT_EACH,
	MLT_ELSE,
	MLT_ELSEIF,
	MLT_END,
	MLT_EXIT,
	MLT_FOR,
	MLT_FUN,
	MLT_IF,
	MLT_IN,
	MLT_IS,
	MLT_LET,
	MLT_LOOP,
	MLT_METH,
	MLT_MUST,
	MLT_NEXT,
	MLT_NIL,
	MLT_NOT,
	MLT_OLD,
	MLT_ON,
	MLT_OR,
	MLT_REF,
	MLT_RET,
	MLT_SUSP,
	MLT_SWITCH,
	MLT_THEN,
	MLT_TO,
	MLT_UNTIL,
	MLT_VAR,
	MLT_WHEN,
	MLT_WHILE,
	MLT_WITH,
	MLT_IDENT,
	MLT_BLANK,
	MLT_LEFT_PAREN,
	MLT_RIGHT_PAREN,
	MLT_LEFT_SQUARE,
	MLT_RIGHT_SQUARE,
	MLT_LEFT_BRACE,
	MLT_RIGHT_BRACE,
	MLT_SEMICOLON,
	MLT_COLON,
	MLT_COMMA,
	MLT_ASSIGN,
	MLT_IMPORT,
	MLT_VALUE,
	MLT_EXPR,
	MLT_INLINE,
	MLT_EXPAND,
	MLT_EXPR_VALUE,
	MLT_OPERATOR,
	MLT_METHOD
} ml_token_t;

typedef struct mlc_function_t mlc_function_t;
typedef struct mlc_frame_t mlc_frame_t;
typedef struct mlc_loop_t mlc_loop_t;
typedef struct mlc_block_t mlc_block_t;
typedef struct mlc_try_t mlc_try_t;
typedef struct mlc_upvalue_t mlc_upvalue_t;
typedef struct mlc_define_t mlc_define_t;

struct mlc_expr_t {
	void (*compile)(mlc_function_t *, mlc_expr_t *, int);
	mlc_expr_t *Next;
	const char *Source;
	int StartLine, EndLine;
};

struct mlc_function_t {
	ml_state_t Base;
	ml_compiler_t *Compiler;
	mlc_frame_t *Frame;
	void *Limit;
	const char *Source;
	mlc_function_t *Up;
	mlc_block_t *Block;
	ml_decl_t *Decls;
	mlc_define_t *Defines;
	mlc_loop_t *Loop;
	mlc_try_t *Try;
	mlc_upvalue_t *UpValues;
	ml_inst_t *Next, *Returns;
	int Top, Size, Self, Space;
};

struct mlc_define_t {
	mlc_define_t *Next;
	const char *Ident;
	mlc_expr_t *Expr;
	long Hash;
};

typedef enum {
	ML_EXPR_UNKNOWN,
	ML_EXPR_AND,
	ML_EXPR_ASSIGN,
	ML_EXPR_BLANK,
	ML_EXPR_BLOCK,
	ML_EXPR_CALL,
	ML_EXPR_CONST_CALL,
	ML_EXPR_DEBUG,
	ML_EXPR_DEF,
	ML_EXPR_DEFAULT,
	ML_EXPR_DEFINE,
	ML_EXPR_DEF_IN,
	ML_EXPR_DEF_UNPACK,
	ML_EXPR_DELEGATE,
	ML_EXPR_EACH,
	ML_EXPR_EXIT,
	ML_EXPR_FOR,
	ML_EXPR_FUN,
	ML_EXPR_IDENT,
	ML_EXPR_IF,
	ML_EXPR_INLINE,
	ML_EXPR_LET,
	ML_EXPR_LET_IN,
	ML_EXPR_LET_UNPACK,
	ML_EXPR_LIST,
	ML_EXPR_LOOP,
	ML_EXPR_MAP,
	ML_EXPR_NEXT,
	ML_EXPR_NIL,
	ML_EXPR_NOT,
	ML_EXPR_OLD,
	ML_EXPR_OR,
	ML_EXPR_REF,
	ML_EXPR_REF_IN,
	ML_EXPR_REF_UNPACK,
	ML_EXPR_REGISTER,
	ML_EXPR_RESOLVE,
	ML_EXPR_RETURN,
	ML_EXPR_SCOPED,
	ML_EXPR_STRING,
	ML_EXPR_SUBST,
	ML_EXPR_SUSPEND,
	ML_EXPR_SWITCH,
	ML_EXPR_TUPLE,
	ML_EXPR_VALUE,
	ML_EXPR_VAR,
	ML_EXPR_VAR_IN,
	ML_EXPR_VAR_TYPE,
	ML_EXPR_VAR_UNPACK,
	ML_EXPR_WITH
} ml_expr_type_t;

ml_expr_type_t mlc_expr_type(mlc_expr_t *Expr);

#define MLC_EXPR_FIELDS(name) \
	void (*compile)(mlc_function_t *, mlc_## name ## _expr_t *, int); \
	mlc_expr_t *Next; \
	const char *Source; \
	int StartLine, EndLine;

typedef struct mlc_value_expr_t mlc_value_expr_t;

struct mlc_value_expr_t {
	MLC_EXPR_FIELDS(value);
	ml_value_t *Value;
};

typedef struct mlc_local_t mlc_local_t;

struct mlc_local_t {
	mlc_local_t *Next;
	const char *Ident;
	int Line, Index;
};

typedef struct mlc_if_expr_t mlc_if_expr_t;
typedef struct mlc_if_case_t mlc_if_case_t;

struct mlc_if_case_t {
	mlc_if_case_t *Next;
	mlc_expr_t *Condition;
	mlc_expr_t *Body;
	mlc_local_t Local[1];
	ml_token_t Token;
};

struct mlc_if_expr_t {
	MLC_EXPR_FIELDS(if);
	mlc_if_case_t *Cases;
	mlc_expr_t *Else;
};

typedef struct mlc_parent_expr_t mlc_parent_expr_t;

struct mlc_parent_expr_t {
	MLC_EXPR_FIELDS(parent);
	mlc_expr_t *Child;
};

typedef struct mlc_local_expr_t mlc_local_expr_t;

struct mlc_local_expr_t {
	MLC_EXPR_FIELDS(local);
	mlc_local_t *Local;
	mlc_expr_t *Child;
	int Count;
};

typedef struct mlc_for_expr_t mlc_for_expr_t;

struct mlc_for_expr_t {
	MLC_EXPR_FIELDS(for);
	const char *Key;
	mlc_local_t *Local;
	mlc_expr_t *Child;
	int Unpack;
};

typedef struct mlc_must_expr_t mlc_must_expr_t;

struct mlc_must_expr_t {
	MLC_EXPR_FIELDS(must);
	int Index;
};

typedef struct mlc_block_expr_t mlc_block_expr_t;

struct mlc_block_expr_t {
	MLC_EXPR_FIELDS(block);
	mlc_local_t *Vars, *Lets, *Defs;
	mlc_expr_t *Child, *CatchBody;
	const char *CatchIdent;
	int NumVars, NumLets, NumDefs;
};

typedef struct mlc_parent_value_expr_t mlc_parent_value_expr_t;

struct mlc_parent_value_expr_t {
	MLC_EXPR_FIELDS(parent_value);
	mlc_expr_t *Child;
	ml_value_t *Value;
};

typedef struct mlc_string_expr_t mlc_string_expr_t;
typedef struct mlc_string_part_t mlc_string_part_t;

struct mlc_string_expr_t {
	MLC_EXPR_FIELDS(string);
	mlc_string_part_t *Parts;
};

struct mlc_string_part_t {
	mlc_string_part_t *Next;
	union {
		mlc_expr_t *Child;
		const char *Chars;
	};
	int Length, Line;
};

typedef struct mlc_fun_expr_t mlc_fun_expr_t;
typedef struct mlc_default_expr_t mlc_default_expr_t;
typedef struct mlc_param_t mlc_param_t;

typedef enum {
	ML_PARAM_DEFAULT,
	ML_PARAM_EXTRA,
	ML_PARAM_NAMED,
	ML_PARAM_BYREF,
	ML_PARAM_ASVAR
} ml_param_kind_t;

struct mlc_param_t {
	mlc_param_t *Next;
	const char *Ident;
	mlc_expr_t *Type;
	int Line, Kind;
};

struct mlc_default_expr_t {
	MLC_EXPR_FIELDS(default);
	mlc_expr_t *Child;
	int Index, Flags;
};

struct mlc_fun_expr_t {
	MLC_EXPR_FIELDS(fun);
	const char *Name;
	mlc_param_t *Params;
	mlc_expr_t *Body;
	mlc_expr_t *ReturnType;
};

typedef struct mlc_ident_expr_t mlc_ident_expr_t;

struct mlc_ident_expr_t {
	MLC_EXPR_FIELDS(ident);
	const char *Ident;
};

typedef struct ml_macro_t ml_macro_t;

struct ml_macro_t {
	ml_type_t *Type;
	ml_value_t *Function;
};

#define MLCF_PUSH 1
#define MLCF_LOCAL 2
#define MLCF_CONSTANT 4
#define MLCF_RETURN 8

typedef void (*mlc_frame_fn)(mlc_function_t *Function, ml_value_t *Value, void *Frame);

struct mlc_frame_t {
	mlc_frame_t *Next;
	mlc_frame_fn run;
	int AllowErrors, Line;
	void *Data[];
};

typedef struct {
	ml_type_t *Type;
	mlc_expr_t *Expr;
	mlc_function_t *Function;
} ml_expr_value_t;

void mlc_expr_error(mlc_function_t *Function, mlc_expr_t *Expr, ml_value_t *Error);

#define MLC_EXPR_ERROR(EXPR, ERROR) return mlc_expr_error(Function, (mlc_expr_t *)(EXPR), ERROR)

#define MLC_FRAME(TYPE, RUN) TYPE *Frame = (TYPE *)mlc_frame_alloc(Function, sizeof(TYPE), (mlc_frame_fn)RUN)

#define MLC_XFRAME(TYPE, COUNT, TYPE2, RUN) TYPE *Frame = (TYPE *)mlc_frame_alloc(Function, sizeof(TYPE) + (COUNT) * sizeof(TYPE2), (mlc_frame_fn)RUN)

void mlc_pop(mlc_function_t *Function);
#define MLC_POP() mlc_pop(Function)

void mlc_return(mlc_function_t *Function, ml_value_t *Value);
#define MLC_RETURN(VALUE) return mlc_return(Function, VALUE)

#define MLC_EMIT(LINE, OPCODE, N) ml_inst_alloc(Function, LINE, OPCODE, N)
#define MLC_LINK(START, TARGET) mlc_fix_links(START, TARGET)

ml_inst_t *ml_inst_alloc(mlc_function_t *Function, int Line, ml_opcode_t Opcode, int N);
void mlc_fix_links(ml_inst_t *Start, ml_inst_t *Target);

void mlc_inc_top(mlc_function_t *Function);
void mlc_compile(mlc_function_t *Function, mlc_expr_t *Expr, int Flags);

#endif
