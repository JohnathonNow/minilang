#ifndef ML_EXPR_TYPES_H
#define ML_EXPR_TYPES_H

typedef enum {
	ML_EXPR_AND,
	ML_EXPR_ASSIGN,
	ML_EXPR_BLANK,
	ML_EXPR_BLOCK,
	ML_EXPR_CALL,
	ML_EXPR_CONDITION,
	ML_EXPR_CONST_CALL,
	ML_EXPR_DEBUG,
	ML_EXPR_DEF,
	ML_EXPR_DEF_IN,
	ML_EXPR_DEF_UNPACK,
	ML_EXPR_DEFAULT,
	ML_EXPR_DEFINE,
	ML_EXPR_DELEGATE,
	ML_EXPR_EACH,
	ML_EXPR_EXIT,
	ML_EXPR_FOR,
	ML_EXPR_FUN,
	ML_EXPR_GUARD,
	ML_EXPR_IDENT,
	ML_EXPR_IF,
	ML_EXPR_INLINE,
	ML_EXPR_IT,
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
	ML_EXPR_UNKNOWN,
	ML_EXPR_VALUE,
	ML_EXPR_VAR,
	ML_EXPR_VAR_IN,
	ML_EXPR_VAR_TYPE,
	ML_EXPR_VAR_UNPACK,
	ML_EXPR_WITH,
} ml_expr_type_t;

#endif
