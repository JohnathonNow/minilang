#ifndef ML_OPCODES_H
#define ML_OPCODES_H

#define ML_BYTECODE_VERSION 4

typedef enum {
	MLI_AND = 0,
	MLI_ASSIGN = 1,
	MLI_ASSIGN_LOCAL = 2,
	MLI_CALL = 3,
	MLI_CALL_CONST = 4,
	MLI_CALL_METHOD = 5,
	MLI_CATCH = 6,
	MLI_CATCHX = 7,
	MLI_CLOSURE = 8,
	MLI_CLOSURE_TYPED = 9,
	MLI_ENTER = 10,
	MLI_EXIT = 11,
	MLI_FOR = 12,
	MLI_GOTO = 13,
	MLI_IF_DEBUG = 14,
	MLI_ITER = 15,
	MLI_KEY = 16,
	MLI_LET = 17,
	MLI_LETI = 18,
	MLI_LETX = 19,
	MLI_LINK = 20,
	MLI_LIST_APPEND = 21,
	MLI_LIST_NEW = 22,
	MLI_LOAD = 23,
	MLI_LOAD_PUSH = 24,
	MLI_LOAD_VAR = 25,
	MLI_LOCAL = 26,
	MLI_LOCALI = 27,
	MLI_LOCAL_PUSH = 28,
	MLI_MAP_INSERT = 29,
	MLI_MAP_NEW = 30,
	MLI_NEXT = 31,
	MLI_NIL = 32,
	MLI_NIL_PUSH = 33,
	MLI_NOT = 34,
	MLI_OR = 35,
	MLI_PARAM_TYPE = 36,
	MLI_PARTIAL_NEW = 37,
	MLI_PARTIAL_SET = 38,
	MLI_POP = 39,
	MLI_PUSH = 40,
	MLI_REF = 41,
	MLI_REFI = 42,
	MLI_REFX = 43,
	MLI_RESOLVE = 44,
	MLI_RESUME = 45,
	MLI_RETRY = 46,
	MLI_RETURN = 47,
	MLI_STRING_ADD = 48,
	MLI_STRING_ADDS = 49,
	MLI_STRING_ADD_1 = 50,
	MLI_STRING_END = 51,
	MLI_STRING_NEW = 52,
	MLI_STRING_POP = 53,
	MLI_SUSPEND = 54,
	MLI_SWITCH = 55,
	MLI_TAIL_CALL = 56,
	MLI_TAIL_CALL_CONST = 57,
	MLI_TAIL_CALL_METHOD = 58,
	MLI_TRY = 59,
	MLI_TUPLE_NEW = 60,
	MLI_UPVALUE = 61,
	MLI_VALUE_1 = 62,
	MLI_VALUE_2 = 63,
	MLI_VAR = 64,
	MLI_VARX = 65,
	MLI_VAR_TYPE = 66,
	MLI_WITH = 67,
	MLI_WITHX = 68,
} ml_opcode_t;

typedef enum {
	MLIT_CLOSURE,
	MLIT_COUNT,
	MLIT_COUNT_CHARS,
	MLIT_COUNT_COUNT,
	MLIT_COUNT_COUNT_DECL,
	MLIT_COUNT_DECL,
	MLIT_DECL,
	MLIT_INST,
	MLIT_INST_COUNT_DECL,
	MLIT_NONE,
	MLIT_SWITCH,
	MLIT_VALUE,
	MLIT_VALUE_COUNT,
	MLIT_VALUE_COUNT_DATA,
} ml_inst_type_t;

extern const char *MLInstNames[];
extern const ml_inst_type_t MLInstTypes[];

#endif
