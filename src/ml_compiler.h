#ifndef ML_COMPILER_H
#define ML_COMPILER_H

#include <setjmp.h>

#include "ml_runtime.h"
#include "stringmap.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct mlc_scanner_t mlc_scanner_t;

mlc_scanner_t *ml_scanner(const char *SourceName, void *Data, const char *(*read)(void *), ml_getter_t GlobalGet, void *Globals);
ml_source_t ml_scanner_source(mlc_scanner_t *Scanner, ml_source_t Source);
void ml_scanner_reset(mlc_scanner_t *Scanner);
const char *ml_scanner_clear(mlc_scanner_t *Scanner);

void ml_function_compile(ml_state_t *Caller, mlc_scanner_t *Scanner, const char **Parameters);
void ml_command_evaluate(ml_state_t *Caller, mlc_scanner_t *Scanner, stringmap_t *Vars);

#ifdef	__cplusplus
}
#endif

#endif
