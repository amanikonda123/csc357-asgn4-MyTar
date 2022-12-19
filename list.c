#include "list.h"

/*This function processes the 't' flag from the command line. If entered,
 * the program will list out the files within the tar file's header. */
int list(int argc, char *argv[], int is_v, int is_S)
{
    char *header_buf;
    FILE *archive_fp;
    Header *header;
    char *perms = NULL;
    char *owner_group = NULL;
    char *mtime = NULL;
    char *name = NULL;
    u_int32_t size = 0;
    int i;

    /* Allocates space */
    archive_fp = fopen(argv[2], "rb");
    if (!archive_fp)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    header_buf = (char *)calloc(BLOCK_SIZE, sizeof(char));
    if (!header_buf)
    {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    /* Traverses the tar file in 512 byte blocks */
    while (fread(header_buf, sizeof(char), BLOCK_SIZE, archive_fp))
    {
        /* Reads each header block into a header struct */
        header = unpack_header(header_buf, archive_fp, is_S);
        if (!header)
        {
            free(header_buf);
            fclose(archive_fp);
            return 1;
        }
        /* Empty block check - if the header's checksum is 0,
         * it is an empty block */
        if (strtol(header->chksum, NULL, BASE8) == 0)
        {
            free(header_buf);
            free(header);
            fclose(archive_fp);
            return 1;
        }

        /* If the header is a regular file, ignore its data blocks */
        if (!strncmp(header->typeflag, "0", TYPEFLAG_SIZE) ||
            !strncmp(header->typeflag, "\0", TYPEFLAG_SIZE))
        {
            ignoreBlock(header_buf, archive_fp);
        }

        /* if there aren't any extra files passed in the command line,
         * print out the header's record */
        if (argc == 3)
        {
            print_archive_info(header, perms, owner_group,
                               size, mtime, name, is_v);
        }
        /* if they do pass in extra files, then check if each header block
         * matches the header in the command line. If they match,
         * print the record, and if they don't, move onto the next. */
        else
        {
            for (i = 3; i < argc; i++)
            {
                name = getHeaderPath(header);
                if (argv[i] && (strncmp(argv[i], name, strlen(argv[i])) != 0))
                {
                    free(name);
                    continue;
                }
                else
                {
                    print_archive_info(header, perms, owner_group,
                                       size, mtime, name, is_v);
                    free(name);
                }
            }
        }
        free(header);
    }
    /* Frees allocated space */
    fclose(archive_fp);
    free(header_buf);
    free(header);
    return 0;
}

/* This functions prints out a header block's record */
void print_archive_info(Header *header, char *perms, char *owner_group,
                        long size, char *mtime, char *name, int is_v)
{
    /* If they pass in the verbose flag, then print out all the
     * necessary fields. Otherwise, only print out the name. */
    if (is_v == 1)
    {
        perms = getPerms(header);
        owner_group = getOwnerGroup(header);
        size = getSize(header);
        mtime = getMtime(header);
        name = getHeaderPath(header);
        /*print out verbose info*/
        printf("%-10s %-17s %8lu %-16s %s\n", perms, owner_group,
               size, mtime, name);
        free(perms);
        free(owner_group);
        free(mtime);
        free(name);
    }
    else
    {
        name = getHeaderPath(header);
        /*print out name only*/
        printf("%s\n", name);
        free(name);
    }
}

/* This function retrieves the permissions from a header block */
char *getPerms(Header *header)
{
    char *perms;
    long mode;
    int i;
    perms = (char *)calloc(PERM_NUM_CHARS + 1, sizeof(char));
    /* Allocates necessary space */

    if (!perms)
    {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    mode = strtol(header->mode, NULL, BASE8);
    /* If header is a sym link, give it all perms */
    if (strncmp(header->typeflag, "2", 1) == 0)
    {
        strcpy(perms, "lrwxrwxrwx");
        return perms;
    }
    /* Initializes perms with no permissions */
    for (i = 0; i < PERM_NUM_CHARS; i++)
    {
        perms[i] = '-';
    }

    /* Sets first byte of perms to header's type */
    if (strncmp(header->typeflag, "0", 1) == 0 ||
        strncmp(header->typeflag, "\0", 1) == 0)
    {
        perms[0] = '-';
    }
    else if (strncmp(header->typeflag, "2", 1) == 0)
    {
        perms[0] = 'l';
    }
    else if (strncmp(header->typeflag, "5", 1) == 0)
    {
        perms[0] = 'd';
    }

    /* Reads mode and check which perms are applied */
    if (mode & S_IRUSR)
    {
        perms[1] = 'r';
    }
    if (mode & S_IWUSR)
    {
        perms[2] = 'w';
    }
    if (mode & S_IXUSR)
    {
        perms[3] = 'x';
    }
    if (mode & S_IRGRP)
    {
        perms[4] = 'r';
    }
    if (mode & S_IWGRP)
    {
        perms[5] = 'w';
    }
    if (mode & S_IXGRP)
    {
        perms[6] = 'x';
    }
    if (mode & S_IROTH)
    {
        perms[7] = 'r';
    }
    if (mode & S_IWOTH)
    {
        perms[8] = 'w';
    }
    if (mode & S_IXOTH)
    {
        perms[9] = 'x';
    }
    return perms;
}

/* This function builds the 'owner/group' string from a header block */
char *getOwnerGroup(Header *header)
{
    char *owner_group, *temp;
    /* Allocates necessary space */
    owner_group = (char *)calloc(OWNER_GROUP_CHARS + 1, sizeof(char));
    if (!owner_group)
    {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    temp = (char *)calloc(OWNER_GROUP_CHARS + 1, sizeof(char));
    if (!temp)
    {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    /* Copies the header's uname into the return string */
    strncpy(owner_group, header->uname, strlen(header->uname));
    /* If the uname is longer than 17 chars, copy the first 17 chars into
     * return string */
    if (strlen(header->uname) <= OWNER_GROUP_CHARS)
    {
        strncpy(owner_group, header->uname, strlen(header->uname));
        /* If there is room to concatenate a '/,' do so */
        if (strlen(owner_group) + 1 <= OWNER_GROUP_CHARS)
        {
            strcat(owner_group, "/");
            /* If the return string isn't 17 chars, fill it with as many
             * chars from gname as possible */
            if (strlen(owner_group) != OWNER_GROUP_CHARS)
            {
                strncpy(temp, header->gname,
                        OWNER_GROUP_CHARS - strlen(owner_group));
                strcat(owner_group, temp);
            }
        }
    }
    free(temp);
    return owner_group;
}

/* This function retrieves the mtime from a header block */
char *getMtime(Header *header)
{
    char *mtime;
    time_t header_time;
    /* Allocates necessary space */
    mtime = (char *)calloc(MTIME_CHARS, sizeof(char));
    if (!mtime)
    {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    /* Retrieve mtime from header and cast it as time_t */
    header_time = (time_t)(strtol(header->mtime, NULL, BASE8));
    /* Store the localtime of mtime, properly format it, and return it */
    strftime(mtime, MTIME_CHARS, "%Y-%m-%d %H:%M ", localtime(&header_time));
    return mtime;
}

/* This function retrieves the size from a header block */
u_int32_t getSize(Header *header)
{
    return (u_int32_t)strtol(header->size, NULL, BASE8);
}
