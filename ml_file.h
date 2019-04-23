#ifndef ML_FILE_H
#define ML_FILE_H

#include <stdio.h>
#include "minilang.h"

#ifdef	__cplusplus
extern "C" {
#endif

void ml_file_init();

ml_value_t *ml_file_new(FILE *File);
ml_value_t *ml_file_open(void *Data, int Count, ml_value_t **Args);

#ifdef	__cplusplus
}
#endif

#endif
