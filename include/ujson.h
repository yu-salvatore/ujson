
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

/* Uncomment for Parent links*/
// #define UJ_PARENT_LINKS

/*
 * Token structure - 6 bytes per token
 * With parent links is 8 bytes
 */
typedef struct
{
    unsigned short start;   /* 2 bytes: position in JSON string */
    unsigned short len;     /* 2 bytes: 14-bit length + 2-bit type */
    unsigned char  size;    /* 1 byte: children count */
#ifdef UJ_PARENT_LINKS
    unsigned char  _pad;    /* 1 byte: alignment */
    short          parent;  /* 2 bytes: parent index, -1 = root */
#else
    unsigned char  _pad;    /* 1 byte: alignment */
#endif
} uj_tok;

/* Extract type from token (2 high bits of len) */
#define UJ_TYPE(t)  ((t)->len >> 14)

/* Extract length from token (14 low bits of len) */
#define UJ_LEN(t)   ((t)->len & 0x3FFF)

/* Set type and length into token */
#define UJ_SET(t, ty, l) ((t)->len = ((ty) << 14) | ((l) & 0x3FFF))

#ifdef __cplusplus
}
#endif

#endif /* UJSON_H */
