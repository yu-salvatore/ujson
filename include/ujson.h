
#ifndef UJSON_H
#define UJSON_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Token types */
#define UJ_OBJ  0  /* Object: {...} */
#define UJ_ARR  1  /* Array:  [...] */
#define UJ_STR  2  /* String: "..." */
#define UJ_PRI  3  /* Primitive: number, true, false, null */

/* Error codes */
#define UJ_OK       0   /* Success */
#define UJ_ENOMEM  -1   /* Not enough tokens */
#define UJ_EINVAL  -2   /* Invalid JSON */
#define UJ_EPART   -3   /* Incomplete JSON (need more data) */

/* Check if character is a hex digit (for \uXXXX validation) */
#define UJ_IS_HEX(c) (((c)>='0'&&(c)<='9')||((c)>='A'&&(c)<='F')||((c)>='a'&&(c)<='f'))

#ifdef __cplusplus
}
#endif

#endif /* UJSON_H */
