#ifndef ML_MODULE_H
#define ML_MODULE_H

#include "minilang.h"

#define MLMF_USE_GLOBALS 1

#define ML_MODULE_PATH_INDEX 4

void ml_module_init(stringmap_t *Globals);

void ml_module_compile(ml_state_t *Caller, ml_parser_t *Parser, ml_compiler_t *Compiler, ml_value_t **Slot);
void ml_module_compile2(ml_state_t *Caller, ml_parser_t *Parser, ml_compiler_t *Compiler, ml_value_t **Slot, int Flags);
void ml_module_load_file(ml_state_t *Caller, const char *FileName, ml_getter_t GlobalGet, void *Globals, ml_value_t **Slot);

#endif
