#include "extract.h"
#define STRMIN(x, y) ((strlen(x) < strlen(y)) ? strlen(x) : strlen(y))

/*This function processes the 'x' flag from the command line. If entered,
 * the program will extract the contents from the archive. */
int extract(int argc, char *argv[], int is_v, int is_S)
{
    FILE *archive_fp;
    char *block_buf, *name;
    Header *header = NULL;

    /* Opens file and allocates space */
    if (!(archive_fp = fopen(argv[2], "rb")))
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    block_buf = (char *)calloc(BLOCK_SIZE, sizeof(char));
    if (!block_buf)
    {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    /* Traverses each 512 byte block in archive */
    while (fread(block_buf, BLOCK_SIZE, 1, archive_fp))
    {
        /* Reads in each header block into header struct */
        header = unpack_header(block_buf, archive_fp, is_S);
        if (!header)
        {
            free(block_buf);
            fclose(archive_fp);
            return 1;
        }
        /* Empty block check - if the header's checksum is 0,
         * it is an empty block */
        if (strtol(header->chksum, NULL, BASE8) == 0)
        {
            free(block_buf);
            free(header);
            fclose(archive_fp);
            return 1;
        }

        /* If additional files are passed through, handle them */
        name = getHeaderPath(header);
        if (argc > 3)
        {
            /* if this evaluates it means the block of data wasn't of use
             * so we skip over it -> moving the pointer */
            if (!multi_file_handler(argc, argv, archive_fp, header, name, is_v))
            {
                ignoreBlock(block_buf, archive_fp);
            }
        }
        else
        {

            check_file_type(header, archive_fp, name, is_v);
            free(name);
        }

        free(header);
    }
    fclose(archive_fp);
    free(block_buf);
    return 0;
}

/* This function checks the case where the file passed through */
int multi_file_handler(int argc, char *argv[], FILE *archive, Header *header,
                       char *name, int is_v)
{
    int i, ret_val = 0;
    /* since we are parsing multiple file args, we use a for loop to iterate */
    for (i = 3; i < argc; i++)
    {
        /* check if file specificed is a dirctory or contained within
         * specificed argument */
        if (!strncmp(header->typeflag, "5", TYPEFLAG_SIZE) ||
            strstr(name, argv[i]))
        {
            /* Check if the argument is equal to the name up until the
             * nth index <- determined by the MIN macro */
            if (!strncmp(argv[i], name, STRMIN(name, argv[i])))
            {
                /* get the path */
                name = getHeaderPath(header);
                /* pass it into check file type */
                check_file_type(header, archive, name, is_v);
                /* free name for memory management */
                free(name);
                /* set ret val to one indicating we hit a block data */
                ret_val = 1;
            }
        }
    }
    return ret_val;
}

/* Responsible for deciding what to do for different file types */
void check_file_type(Header *header, FILE *archive_fp, char *name, int is_v)
{
    FILE *ret_file;

    /* if vebosity then print name */
    if (is_v)
    {
        printf("%s\n", name);
    }

    /* if the typeflag of the current header is regular file */
    if (!strncmp(header->typeflag, "0", TYPEFLAG_SIZE) ||
        !strncmp(header->typeflag, "\0", TYPEFLAG_SIZE))
    {
        /* we open the file using the name */
        if (!(ret_file = fopen(name, "wb")))
        {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        /* Call helper functions to extract mode & time, and write data */
        make_mode(header, archive_fp, name, ret_file);
        make_utime(header, archive_fp, name, ret_file);
        /* close file */
        fclose(ret_file);
        return;
    }
    /* if the file specificed it a directory
     * then we first try to mkdir it and then chmod with all perms */
    else if (!strncmp(header->typeflag, "5", TYPEFLAG_SIZE))
    {
        if ((mkdir(name, 0777) == -1) && ((chmod(name, 0777)) == -1))
        {
            perror("mkdir");
            exit(EXIT_FAILURE);
        }
        return;
    }
    /* if the file specified was a symbolic link then create a
     * symlink to the path(aka name) and check for failure as well */
    else if (!strncmp(header->typeflag, "2", TYPEFLAG_SIZE))
    {
        if (symlink(header->linkname, name) == -1)
        {
            perror("symlink");
        }
        return;
    }
    /* if none of those conditions are hit then file type is unsupported */
    fprintf(stderr, "File type not supported");
    exit(EXIT_FAILURE);
}

/* Responsible for getting the mode of the file and setting the new one to it */
void make_mode(Header *header, FILE *archive_fp, char *name, FILE *ret_file)
{

    /* get the mode from header struct */
    int mode = strtol(header->mode, NULL, BASE8);
    /* check if any user has execute perms, if so then
     * chmod it to 0777 so everyone has it */
    if (mode & S_IXUSR || mode & S_IXGRP || mode & S_IXOTH)
    {
        if ((chmod(name, 0777)) < 0)
        {
            perror("chmod");
            exit(EXIT_FAILURE);
        }
        return;
    }

    /* If no one has execute then we chmod it with 0666,
     * so everyone does have at least read and write */
    if ((chmod(name, 0666)) < 0)
    {
        perror("chmod");
        exit(EXIT_FAILURE);
    }
}

/* Responsible for getting utime as well as writing file content */
void make_utime(Header *header, FILE *archive_fp, char *name, FILE *ret_file)
{

    int i, c;
    long head_size;
    char temp_char;
    struct utimbuf *utime_buf;

    /* get the size of the file */
    head_size = strtol(header->size, NULL, BASE8);
    /* malloc space for the buffer */
    utime_buf = malloc(sizeof(struct utimbuf));

    /* error check malloc */
    if (!(utime_buf))
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    /* fdr the size of the file we iterate */
    for (i = 0; i < head_size; i++)
    {
        /* using getc to get the contents char by char */
        temp_char = getc(archive_fp);
        /* error check getc */
        if (temp_char == -1)
        {
            perror("getc");
            exit(EXIT_FAILURE);
        }
        /* put the character from getc into the extracted file
         * created earlier */
        putc(temp_char, ret_file);
    }

    /* calculate the the space we need to go back to continue parsing
     * header */
    c = BLOCK_SIZE * (head_size / BLOCK_SIZE + 1) - head_size;
    /* set utime buffers mod time to the mtime present in the header */
    utime_buf->modtime = strtol(header->mtime, NULL, BASE8);
    /* change the utime of the file named to the time from the header */
    if (utime(name, utime_buf) == -1)
    {
        perror("utime");
        exit(EXIT_FAILURE);
    }

    /* using the calculation from above, fseek back to the end of header
     * in order to continue parsing header by header */
    fseek(archive_fp, c, SEEK_CUR);
    /* free utime buffer for memory management */
    free(utime_buf);
}
