#ifndef ML_CONSOLE_H
#define ML_CONSOLE_H

#include "minilang.h"

#ifdef	__cplusplus
extern "C" {
#endif

void ml_console(ml_getter_t GlobalGet, void *Globals);

#ifdef	__cplusplus
}
#endif

#endif