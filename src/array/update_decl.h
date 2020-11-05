#define UPDATE_ROW_DECL(NAME, OP, TARGET, SOURCE) \
\
extern void NAME ## _row_ ## TARGET ## _ ## SOURCE(ml_array_dimension_t *TargetDimension, char *TargetData, ml_array_dimension_t *SourceDimension, char *SourceData);

#define UPDATE_ROW_VALUE_DECL(NAME, OP, SOURCE) \
\
extern void NAME ## _row_value_ ## SOURCE(ml_array_dimension_t *TargetDimension, char *TargetData, ml_array_dimension_t *SourceDimension, char *SourceData);

#define UPDATE_ROW_DECL_VALUE(NAME, OP, TARGET) \
\
extern void NAME ## _row_ ## TARGET ## _value(ml_array_dimension_t *TargetDimension, char *TargetData, ml_array_dimension_t *SourceDimension, char *SourceData);

#define UPDATE_ROW_VALUE_DECL_VALUE(NAME, OP) \
\
extern void NAME ## _row_value_value(ml_array_dimension_t *TargetDimension, char *TargetData, ml_array_dimension_t *SourceDimension, char *SourceData);

#define UPDATE_ROW_TARGET_DECL(NAME, OP, TARGET) \
UPDATE_ROW_DECL(NAME, OP, TARGET, int8_t) \
UPDATE_ROW_DECL(NAME, OP, TARGET, uint8_t) \
UPDATE_ROW_DECL(NAME, OP, TARGET, int16_t) \
UPDATE_ROW_DECL(NAME, OP, TARGET, uint16_t) \
UPDATE_ROW_DECL(NAME, OP, TARGET, int32_t) \
UPDATE_ROW_DECL(NAME, OP, TARGET, uint32_t) \
UPDATE_ROW_DECL(NAME, OP, TARGET, int64_t) \
UPDATE_ROW_DECL(NAME, OP, TARGET, uint64_t) \
UPDATE_ROW_DECL(NAME, OP, TARGET, float) \
UPDATE_ROW_DECL(NAME, OP, TARGET, double) \
UPDATE_ROW_DECL_VALUE(NAME, OP, TARGET)

#define UPDATE_ROW_TARGET_VALUE_DECL(NAME, OP) \
UPDATE_ROW_VALUE_DECL(NAME, OP, int8_t) \
UPDATE_ROW_VALUE_DECL(NAME, OP, uint8_t) \
UPDATE_ROW_VALUE_DECL(NAME, OP, int16_t) \
UPDATE_ROW_VALUE_DECL(NAME, OP, uint16_t) \
UPDATE_ROW_VALUE_DECL(NAME, OP, int32_t) \
UPDATE_ROW_VALUE_DECL(NAME, OP, uint32_t) \
UPDATE_ROW_VALUE_DECL(NAME, OP, int64_t) \
UPDATE_ROW_VALUE_DECL(NAME, OP, uint64_t) \
UPDATE_ROW_VALUE_DECL(NAME, OP, float) \
UPDATE_ROW_VALUE_DECL(NAME, OP, double) \
UPDATE_ROW_VALUE_DECL_VALUE(NAME, OP)

#define UPDATE_ROW_OPS_DECL(NAME, OP) \
UPDATE_ROW_TARGET_DECL(NAME, OP, int8_t) \
UPDATE_ROW_TARGET_DECL(NAME, OP, uint8_t) \
UPDATE_ROW_TARGET_DECL(NAME, OP, int16_t) \
UPDATE_ROW_TARGET_DECL(NAME, OP, uint16_t) \
UPDATE_ROW_TARGET_DECL(NAME, OP, int32_t) \
UPDATE_ROW_TARGET_DECL(NAME, OP, uint32_t) \
UPDATE_ROW_TARGET_DECL(NAME, OP, int64_t) \
UPDATE_ROW_TARGET_DECL(NAME, OP, uint64_t) \
UPDATE_ROW_TARGET_DECL(NAME, OP, float) \
UPDATE_ROW_TARGET_DECL(NAME, OP, double) \
UPDATE_ROW_TARGET_VALUE_DECL(NAME, OP)


UPDATE_ROW_OPS_DECL(set, =)
UPDATE_ROW_OPS_DECL(add, +)
UPDATE_ROW_OPS_DECL(sub, -)
UPDATE_ROW_OPS_DECL(mul, *)
UPDATE_ROW_OPS_DECL(div, /)
