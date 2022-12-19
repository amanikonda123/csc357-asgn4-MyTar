#include "mytar.h"

int main(int argc, char *argv[])
{
    int is_c = 0, is_t = 0, is_x = 0, is_v = 0, is_S = 0;
    int i;

    /* Must take in prog name, prog call, flags, and tar file */
    if (argc < 3)
    {
        usage(argv[0]);
    }

    /* Tracks which flags were entered and checks if they are valid flags */
    for (i = 0; i < strlen(argv[1]); i++)
    {
        switch (argv[1][i])
        {
        case 'c':
            is_c = 1;
            break;
        case 't':
            is_t = 1;
            break;
        case 'x':
            is_x = 1;
            break;
        case 'v':
            is_v = 1;
            break;
        case 'S':
            is_S = 1;
            break;
        case 'f':
            continue;
        default:
            usage(argv[0]);
            break;
        }
    }

    /* Last character in flags argument must be 'f' */
    if (strcmp(&argv[1][i - 1], "f") != 0)
    {
        usage(argv[0]);
    }

    /* Can only entered one of 'c', 't', or 'x' */
    if (is_c == 1 && is_x == 1)
    {
        usage(argv[0]);
    }
    if (is_c == 1 && is_t == 1)
    {
        usage(argv[0]);
    }
    if (is_t == 1 && is_x == 1)
    {
        usage(argv[0]);
    }
    if (is_c == 0 && is_t == 0 && is_x == 0)
    {
        usage(argv[0]);
    }

    /* Call respective functions */
    if (is_c)
    {
        create(argc, argv, is_v, is_S);
    }
    else if (is_t)
    {
        list(argc, argv, is_v, is_S);
    }
    else if (is_x)
    {
        extract(argc, argv, is_v, is_S);
    }

    return 0;
}

void usage(const char *prog)
{
    fprintf(stderr,
            "usage: %s mytar [ctxvS]f tarfile [ path [ ... ] ] \n", prog);
    exit(EXIT_FAILURE);
}
