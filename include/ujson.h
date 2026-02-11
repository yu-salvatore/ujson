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

/* Compare token with null-terminated string */
static int uj_eq(const char *js, const uj_tok *t, const char *s)
{
    const char *p = js + t->start;
    unsigned short n = UJ_LEN(t);
    while (n && *s)
    {
        if (*p++ != *s++)
        {
            return 0;
        }
        n--;
    }
    return (n == 0 && *s == 0);
}

/* Token to int */
static int uj_int(const char *js, const uj_tok *t)
{
    const char *p = js + t->start;
    int v = 0, neg = 0;
    unsigned short n = UJ_LEN(t);
    if (*p == '-')
    {
        neg = 1;
        p++;
        n--;
    }
    while (n--)
    {
        v = v * 10 + (*p++ - '0');
    }
    return neg ? -v : v;
}

/* Token to bool (1=true, 0=false, -1=other) */
static int uj_bool(const char *js, const uj_tok *t)
{
    if (UJ_TYPE(t) != UJ_PRI)
    {
        return -1;
    }
    char c = js[t->start];
    if (c == 't')
    {
        return 1;
    }
    if (c == 'f')
    {
        return 0;
    }
    return -1;
}

/* Check if token is null */
static int uj_null(const char *js, const uj_tok *t)
{
    return (UJ_TYPE(t) == UJ_PRI && js[t->start] == 'n');
}

/* Query result */
typedef struct
{
    const char    *js;
    const uj_tok  *tk;
    unsigned char  ok;      /* 1=found, 0=not found */
} uj_val;

/* Skip token and all its children, return next token index */
static int uj_skip(const uj_tok *tk, int idx, int ntk)
{
    int i = idx;
    unsigned short end;
    if (i < 0 || i >= ntk)
    {
        return ntk;
    }

    unsigned char ty = UJ_TYPE(&tk[i]);
    if (ty == UJ_OBJ || ty == UJ_ARR)
    {
        end = tk[i].start + UJ_LEN(&tk[i]);
        i++;
        while (i < ntk && tk[i].start < end)
        {
            i++;
        }
        return i;
    }
    return i + 1;
}

/* Get value from object by key, return token index or -1 */
static int uj_obj_get(const char *js, const uj_tok *tk, int obj, int ntk,
                      const char *key, int keylen)
{
    if (obj < 0 || obj >= ntk)
    {
        return -1;
    }
    if (UJ_TYPE(&tk[obj]) != UJ_OBJ)
    {
        return -1;
    }

    int i = obj + 1;
    int end = tk[obj].start + UJ_LEN(&tk[obj]);

    while (i < ntk && tk[i].start < end)
    {
        if (UJ_TYPE(&tk[i]) == UJ_STR)
        {
            const char *p = js + tk[i].start;
            int n = UJ_LEN(&tk[i]);
            int match = (n == keylen);
            if (match)
            {
                for (int j = 0; j < n; j++)
                {
                    if (p[j] != key[j])
                    {
                        match = 0;
                        break;
                    }
                }
            }
            if (match && i + 1 < ntk)
            {
                return i + 1;
            }
        }
        i++;
        if (i < ntk)
        {
            i = uj_skip(tk, i, ntk);
        }
    }
    return -1;
}

/* Get value from array by index, return token index or -1 */
static int uj_arr_get(const uj_tok *tk, int arr, int ntk, int index)
{
    if (arr < 0 || arr >= ntk)
    {
        return -1;
    }
    if (UJ_TYPE(&tk[arr]) != UJ_ARR)
    {
        return -1;
    }

    int i = arr + 1;
    int end = tk[arr].start + UJ_LEN(&tk[arr]);
    int count = 0;

    while (i < ntk && tk[i].start < end)
    {
        if (count == index)
        {
            return i;
        }
        i = uj_skip(tk, i, ntk);
        count++;
    }
    return -1;
}

/* Parse integer from string (internal helper) */
static int uj_atoi(const char **p)
{
    int v = 0;
    while (**p >= '0' && **p <= '9')
    {
        v = v * 10 + (**p - '0');
        (*p)++;
    }
    return v;
}

/*
 * Path query: "key.nested[0].field[1][2]"
 * Returns uj_val with ok=1 if found
 */
static uj_val uj_get(const char *js, const uj_tok *tk, int ntk, const char *path)
{
    uj_val v = {js, 0, 0};
    int idx = 0;

    while (*path && idx >= 0 && idx < ntk)
    {
        if (*path == '.')
        {
            path++;
        }

        if (*path == '[')
        {
            path++;
            int index = uj_atoi(&path);
            if (*path == ']')
            {
                path++;
            }
            idx = uj_arr_get(tk, idx, ntk, index);
        }
        else if (*path && *path != '[')
        {
            const char *start = path;
            while (*path && *path != '.' && *path != '[')
            {
                path++;
            }
            int keylen = path - start;
            idx = uj_obj_get(js, tk, idx, ntk, start, keylen);
        }
        else
        {
            break;
        }
    }

    if (idx >= 0 && idx < ntk)
    {
        v.tk = &tk[idx];
        v.ok = 1;
    }
    return v;
}

/* Get type: UJ_OBJ, UJ_ARR, UJ_STR, UJ_PRI, or -1 if not found */
#define uj_v_type(v)  ((v)->ok ? UJ_TYPE((v)->tk) : -1)

/* Get as int */
static int uj_v_int(const uj_val *v)
{
    if (!v->ok)
    {
        return 0;
    }
    return uj_int(v->js, v->tk);
}

/* Get as bool (1=true, 0=false, -1=error) */
static int uj_v_bool(const uj_val *v)
{
    if (!v->ok)
    {
        return -1;
    }
    return uj_bool(v->js, v->tk);
}

/* Check if null */
static int uj_v_null(const uj_val *v)
{
    if (!v->ok)
    {
        return 0;
    }
    return uj_null(v->js, v->tk);
}

/* Copy string to buffer, return actual length */
static int uj_v_str(const uj_val *v, char *buf, int bufsz)
{
    if (!v->ok || !buf || bufsz < 1)
    {
        return 0;
    }
    int len = UJ_LEN(v->tk);
    if (len >= bufsz)
    {
        len = bufsz - 1;
    }
    const char *src = v->js + v->tk->start;
    for (int i = 0; i < len; i++)
    {
        buf[i] = src[i];
    }
    buf[len] = '\0';
    return len;
}

/* Get string length */
static int uj_v_len(const uj_val *v)
{
    if (!v->ok)
    {
        return 0;
    }
    return UJ_LEN(v->tk);
}

/* Get array length or object key count */
static int uj_v_size(const uj_val *v)
{
    if (!v->ok)
    {
        return 0;
    }
    unsigned char ty = UJ_TYPE(v->tk);
    if (ty == UJ_ARR)
    {
        return v->tk->size;
    }
    if (ty == UJ_OBJ)
    {
        return v->tk->size / 2;
    }
    return 0;
}

/* Direct pointer to string (not null-terminated!) */
static const char *uj_v_ptr(const uj_val *v)
{
    if (!v->ok)
    {
        return 0;
    }
    return v->js + v->tk->start;
}

/* Compare value with string */
static int uj_v_eq(const uj_val *v, const char *s)
{
    if (!v->ok)
    {
        return 0;
    }
    return uj_eq(v->js, v->tk, s);
}

#ifdef UJ_PARENT_LINKS
/* Get parent token index (-1 if root) */
#define uj_parent(t) ((t)->parent)

/* Get parent value */
static uj_val uj_v_parent(const uj_val *v, const uj_tok *tk)
{
    uj_val r = {v->js, 0, 0};
    if (v->ok && v->tk->parent >= 0)
    {
        r.tk = &tk[v->tk->parent];
        r.ok = 1;
    }
    return r;
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* UJSON_H */
