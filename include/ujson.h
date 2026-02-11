
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

/* Parser state - 6 bytes */
typedef struct
{
    unsigned short pos;     /* Current position in JSON string */
    unsigned short next;    /* Next token index to allocate */
    short          super;   /* Parent token index, -1 = none */
} uj_t;

/* Initialize parser */
#define uj_init(p) do { (p)->pos = 0; (p)->next = 0; (p)->super = -1; } while(0)

/* Parse JSON string into token array, return token count or error */
static int uj_parse(uj_t *p, const char *js, unsigned short len,
                    uj_tok *tk, unsigned short ntk)
{
    unsigned short cnt = p->next;
    char c;

    while (p->pos < len)
    {
        c = js[p->pos];

        /* Skip whitespace */
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
        {
            p->pos++;
            continue;
        }

        /* Object or array open */
        if (c == '{' || c == '[')
        {
            if (tk)
            {
                if (p->next >= ntk)
                {
                    return UJ_ENOMEM;
                }
                uj_tok *t = &tk[p->next];
                UJ_SET(t, (c == '{') ? UJ_OBJ : UJ_ARR, 0);
                t->start = p->pos;
                t->size = 0;
#ifdef UJ_PARENT_LINKS
                t->parent = p->super;
#endif
                if (p->super >= 0)
                {
                    tk[p->super].size++;
                }
                p->super = p->next;
            }
            p->next++;
            cnt++;
            p->pos++;
        }
        /* Object or array close */
        else if (c == '}' || c == ']')
        {
            if (tk)
            {
                unsigned char ty = (c == '}') ? UJ_OBJ : UJ_ARR;
                short i = p->next - 1;
                while (i >= 0)
                {
                    uj_tok *t = &tk[i];
                    if (UJ_LEN(t) == 0 && UJ_TYPE(t) == ty)
                    {
                        UJ_SET(t, ty, p->pos - t->start + 1);
                        p->super = i - 1;
                        while (p->super >= 0 && UJ_LEN(&tk[p->super]) != 0)
                        {
                            p->super--;
                        }
                        goto brk_ok;
                    }
                    i--;
                }
                return UJ_EINVAL;
            }
brk_ok:
            p->pos++;
        }
        /* String */
        else if (c == '"')
        {
            unsigned short st = ++p->pos;
            while (p->pos < len)
            {
                c = js[p->pos];
                if (c == '"')
                {
                    if (tk)
                    {
                        if (p->next >= ntk)
                        {
                            return UJ_ENOMEM;
                        }
                        uj_tok *t = &tk[p->next];
                        UJ_SET(t, UJ_STR, p->pos - st);
                        t->start = st;
                        t->size = 0;
#ifdef UJ_PARENT_LINKS
                        t->parent = p->super;
#endif
                        if (p->super >= 0)
                        {
                            tk[p->super].size++;
                        }
                    }
                    p->next++;
                    cnt++;
                    p->pos++;
                    goto str_ok;
                }
                /* Reject unescaped control characters (0x00-0x1F) */
                if ((unsigned char)c < 0x20)
                {
                    return UJ_EINVAL;
                }
                if (c == '\\' && p->pos + 1 < len)
                {
                    p->pos++;
                    c = js[p->pos];
                    switch (c)
                    {
                        case '"': case '\\': case '/':
                        case 'b': case 'f': case 'n': case 'r': case 't':
                            break;
                        case 'u':
                            /* Validate \uXXXX - must have 4 hex digits */
                            if (p->pos + 4 >= len)
                            {
                                return UJ_EPART;
                            }
                            if (!UJ_IS_HEX(js[p->pos + 1]) || !UJ_IS_HEX(js[p->pos + 2]) || !UJ_IS_HEX(js[p->pos + 3]) || !UJ_IS_HEX(js[p->pos + 4]))
                            {
                                return UJ_EINVAL;
                            }
                            p->pos += 4;
                            break;
                        default:
                            return UJ_EINVAL;
                    }
                }
                p->pos++;
            }
            return UJ_EPART;
str_ok:
            ;
        }
        /* Colon: key-value separator */
        else if (c == ':')
        {
            if (tk && p->next > 0)
            {
                p->super = p->next - 1;
            }
            p->pos++;
        }
        /* Comma: element separator */
        else if (c == ',')
        {
            if (tk && p->super >= 0)
            {
                unsigned char ty = UJ_TYPE(&tk[p->super]);
                if (ty != UJ_OBJ && ty != UJ_ARR)
                {
                    short i = p->super - 1;
                    while (i >= 0 && UJ_LEN(&tk[i]) != 0)
                    {
                        i--;
                    }
                    if (i >= 0)
                    {
                        p->super = i;
                    }
                }
            }
            p->pos++;
        }
        /* Primitive: number, true, false, null */
        else
        {
            unsigned short st = p->pos;
            if (!((c >= '0' && c <= '9') || c == '-' || c == 't' || c == 'f' || c == 'n'))
            {
                return UJ_EINVAL;
            }
            while (p->pos < len)
            {
                c = js[p->pos];
                if (c == ',' || c == '}' || c == ']' || c == ' ' || c == '\t' || c == '\n' || c == '\r')
                {
                    break;
                }
                if ((unsigned char)c < 0x20)
                {
                    return UJ_EINVAL;
                }
                p->pos++;
            }
            if (tk)
            {
                if (p->next >= ntk)
                {
                    return UJ_ENOMEM;
                }
                uj_tok *t = &tk[p->next];
                UJ_SET(t, UJ_PRI, p->pos - st);
                t->start = st;
                t->size = 0;
#ifdef UJ_PARENT_LINKS
                t->parent = p->super;
#endif
                if (p->super >= 0)
                {
                    tk[p->super].size++;
                }
            }
            p->next++;
            cnt++;
        }
    }

    /* Check for unclosed containers */
    if (tk)
    {
        short i = p->next - 1;
        while (i >= 0)
        {
            unsigned char ty = UJ_TYPE(&tk[i]);
            if ((ty == UJ_OBJ || ty == UJ_ARR) && UJ_LEN(&tk[i]) == 0)
            {
                return UJ_EPART;
            }
            i--;
        }
    }
    return cnt;
}

#ifdef __cplusplus
}
#endif

#endif /* UJSON_H */
