#include "args.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

typedef struct args_option_t
{
    args_type_t type;
    const char* long_opt;
    char short_opt;
    void* dest;

    union
    {
        int int_val;
        bool bool_val;
        const char* str_val;
    } default_value;

    const char* help;
} args_option_t;

void args_parser_init(args_parser_t* parser, const int argc, char** argv)
{
    if (parser == NULL)
    {
        return;
    }
    parser->argc = argc;
    parser->argv = argv;
    parser->index = 1;
    parser->options = NULL;
    parser->option_count = 0;
    parser->option_capacity = 0;
}

void args_parser_destroy(args_parser_t* parser)
{
    if (parser == NULL)
    {
        return;
    }
    if (parser->options != NULL)
    {
        free(parser->options);
        parser->options = NULL;
    }
    parser->option_count = 0;
    parser->option_capacity = 0;
}

static void args_grow_options(args_parser_t* parser)
{
    if (parser->option_count >= parser->option_capacity)
    {
        int new_capacity = parser->option_capacity == 0 ? 8 : parser->option_capacity * 2;
        args_option_t* new_options = (args_option_t*)realloc(parser->options, new_capacity * sizeof(args_option_t));
        if (new_options != NULL)
        {
            parser->options = new_options;
            parser->option_capacity = new_capacity;
        }
    }
}

void args_register_int(args_parser_t* parser, const char* long_opt, char short_opt, int* dest, int default_value,
                       const char* help)
{
    if (parser == NULL || dest == NULL)
    {
        return;
    }

    args_grow_options(parser);
    args_option_t* opt = &parser->options[parser->option_count++];
    opt->type = ARGS_TYPE_INT;
    opt->long_opt = long_opt;
    opt->short_opt = short_opt;
    opt->dest = dest;
    opt->default_value.int_val = default_value;
    *dest = default_value;
    opt->help = help;
}

void args_register_bool(args_parser_t* parser, const char* long_opt, char short_opt, bool* dest, bool default_value,
                        const char* help)
{
    if (parser == NULL || dest == NULL)
    {
        return;
    }

    args_grow_options(parser);
    args_option_t* opt = &parser->options[parser->option_count++];
    opt->type = ARGS_TYPE_BOOL;
    opt->long_opt = long_opt;
    opt->short_opt = short_opt;
    opt->dest = dest;
    opt->default_value.bool_val = default_value;
    *dest = default_value;
    opt->help = help;
}

void args_register_string(args_parser_t* parser, const char* long_opt, char short_opt, const char** dest,
                          const char* default_value, const char* help)
{
    if (parser == NULL || dest == NULL)
    {
        return;
    }

    args_grow_options(parser);
    args_option_t* opt = &parser->options[parser->option_count++];
    opt->type = ARGS_TYPE_STRING;
    opt->long_opt = long_opt;
    opt->short_opt = short_opt;
    opt->dest = dest;
    opt->default_value.str_val = default_value;
    *dest = default_value;
    opt->help = help;
}

static bool args_parser_has_next(const args_parser_t* parser)
{
    return parser->index < parser->argc;
}

static char* args_parser_current(const args_parser_t* parser)
{
    return parser->argv[parser->index];
}

static int args_find_option_index(args_parser_t* parser, const char* long_opt, char short_opt)
{
    if (parser == NULL)
    {
        return -1;
    }

    int original_index = parser->index;
    parser->index = 1;

    int found_index = -1;
    while (args_parser_has_next(parser))
    {
        char* arg = args_parser_current(parser);

        if (long_opt != NULL)
        {
            size_t long_opt_len = strlen(long_opt);
            if (strncmp(arg, "--", 2) == 0)
            {
                if (strncmp(arg + 2, long_opt, long_opt_len) == 0)
                {
                    found_index = parser->index;
                    break;
                }
            }
        }

        if (short_opt != 0)
        {
            if (arg[0] == '-' && arg[1] == short_opt && arg[2] == '\0')
            {
                found_index = parser->index;
                break;
            }
        }

        parser->index++;
    }

    parser->index = original_index;
    return found_index;
}

static char* args_get_value_internal(args_parser_t* parser, const char* long_opt, char short_opt)
{
    int index = args_find_option_index(parser, long_opt, short_opt);
    if (index == -1)
    {
        return NULL;
    }

    char* arg = parser->argv[index];

    char* equals_sign = strchr(arg, '=');
    if (equals_sign != NULL)
    {
        return equals_sign + 1;
    }

    if (index + 1 < parser->argc)
    {
        char* next_arg = parser->argv[index + 1];
        if (next_arg[0] != '-')
        {
            return next_arg;
        }
    }

    return NULL;
}

bool args_parser_parse(args_parser_t* parser)
{
    if (parser == NULL)
    {
        return false;
    }

    for (int i = 0; i < parser->option_count; i++)
    {
        args_option_t* opt = &parser->options[i];

        switch (opt->type)
        {
        case ARGS_TYPE_INT:
            {
                int* dest = (int*)opt->dest;
                char* value_str = args_get_value_internal(parser, opt->long_opt, opt->short_opt);
                if (value_str != NULL)
                {
                    char* end_ptr;
                    long value = strtol(value_str, &end_ptr, 10);
                    if (end_ptr != value_str && *end_ptr == '\0' && value >= INT_MIN && value <= INT_MAX)
                    {
                        *dest = (int)value;
                    }
                }
                break;
            }
        case ARGS_TYPE_BOOL:
            {
                bool* dest = (bool*)opt->dest;
                if (args_find_option_index(parser, opt->long_opt, opt->short_opt) != -1)
                {
                    char* value_str = args_get_value_internal(parser, opt->long_opt, opt->short_opt);
                    if (value_str == NULL)
                    {
                        *dest = true;
                    }
                    else
                    {
                        if (strcmp(value_str, "false") == 0 || strcmp(value_str, "FALSE") == 0 ||
                            strcmp(value_str, "0") == 0 || strcmp(value_str, "no") == 0 ||
                            strcmp(value_str, "NO") == 0)
                        {
                            *dest = false;
                        }
                        else
                        {
                            *dest = true;
                        }
                    }
                }
                break;
            }
        case ARGS_TYPE_STRING:
            {
                const char** dest = (const char**)opt->dest;
                char* value_str = args_get_value_internal(parser, opt->long_opt, opt->short_opt);
                if (value_str != NULL)
                {
                    *dest = value_str;
                }
                break;
            }
        }
    }

    return true;
}

bool args_has_option(args_parser_t* parser, const char* long_opt, char short_opt)
{
    return args_find_option_index(parser, long_opt, short_opt) != -1;
}

void args_parser_print_help(args_parser_t* parser)
{
    if (parser == NULL)
    {
        return;
    }

    printf("Usage: %s [options]\n", parser->argv[0]);
    printf("Options:\n");

    for (int i = 0; i < parser->option_count; i++)
    {
        args_option_t* opt = &parser->options[i];
        printf("  ");
        if (opt->short_opt != 0)
        {
            printf("-%c, ", opt->short_opt);
        }
        else
        {
            printf("    ");
        }

        if (opt->long_opt != NULL)
        {
            switch (opt->type)
            {
            case ARGS_TYPE_INT:
                printf("--%s <int>", opt->long_opt);
                break;
            case ARGS_TYPE_BOOL:
                printf("--%s", opt->long_opt);
                break;
            case ARGS_TYPE_STRING:
                printf("--%s <string>", opt->long_opt);
                break;
            }
        }

        int padding = 25;
        int length = 0;
        if (opt->short_opt != 0) length += 3;
        if (opt->long_opt != NULL)
        {
            length += (int)strlen(opt->long_opt) + 2;
            switch (opt->type)
            {
            case ARGS_TYPE_INT:
                length += 6;
                break;
            case ARGS_TYPE_STRING:
                length += 9;
                break;
            case ARGS_TYPE_BOOL:
                break;
            }
        }

        for (int j = length; j < padding; j++)
        {
            printf(" ");
        }

        printf("%s", opt->help != NULL ? opt->help : "");

        switch (opt->type)
        {
        case ARGS_TYPE_INT:
            printf(" (default: %d)", opt->default_value.int_val);
            break;
        case ARGS_TYPE_BOOL:
            printf(" (default: %s)", opt->default_value.bool_val ? "true" : "false");
            break;
        case ARGS_TYPE_STRING:
            if (opt->default_value.str_val != NULL)
                printf(" (default: \"%s\")", opt->default_value.str_val);
            break;
        }

        printf("\n");
    }
}
