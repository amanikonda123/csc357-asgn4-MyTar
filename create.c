#include "create.h"

/* Responsible for driving all the create functionality */
int create(int argc, char *argv[], int is_v, int is_S)
{
    struct stat sb;
    int i;

    char header_buf[BLOCK_SIZE];
    FILE *archive_fp = fopen(argv[2], "wb");
    /* Check if fopen failed */
    if (!(archive_fp))
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    /* Clear the block and set everything to null character */
    for (i = 0; i < BLOCK_SIZE; i++)
    {
        header_buf[i] = '\0';
    }

    /* Starting at the 4th argument which is the files to be archived
     * iterate over and decide what to do based on type of argument */
    for (i = 3; i < argc; i++)
    {
        /* Proceed only if lstat doesnt fail */
        if (lstat(argv[i], &sb) != -1)
        {
            /* Call verbosity method, will be verbose if flag present */
            verbosity(sb, is_v, argv[i]);
            /* Conditons to handle different modes */
            if (S_ISDIR(sb.st_mode))
            {
                dir_handler(argv[i], archive_fp, sb, is_v);
            }
            else if (S_ISLNK(sb.st_mode))
            {
                sym_handler(argv[i], archive_fp, sb, is_v);
            }
            else if (S_ISREG(sb.st_mode))
            {
                reg_handler(argv[i], archive_fp, sb, is_v);
            }
        }
    }

    /* Use helper method to attach null blocks at the end */
    write_null_blocks(header_buf, archive_fp, 2);
    fclose(archive_fp);
    return 1;
}

/* Responsible for generating the whole header for a file */
unsigned char *genHeader(char *filename, int ft, int is_v)
{
    struct stat sb;
    Header *header;
    unsigned char *header_buf;
    unsigned int chksum = 0;

    /* Error checking to make sure calloc is successful */
    if (!(header_buf = calloc(BLOCK_SIZE, sizeof(char))))
    {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    if (!(header = calloc(1, sizeof(Header))))
    {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    /* Error handling to make sure lstat worked */
    if (lstat(filename, &sb) == -1)
    {
        perror("lstat");
        exit(EXIT_FAILURE);
    }
    /* Call genName in order to retrieve the file name and store in header */
    if (genName(header, filename, is_v) == -1)
    {
        free(header);
        return header_buf;
    }

    /* Store the mode within the header struct from stat buffer,
     * it is stored as an octal string */
    sprintf(header->mode, "%07o", sb.st_mode & 07777);

    /* Check for size of the uid, if greater than call insert int method
     * in order to insert it correctly */
    if (sb.st_uid > 07777777)
    {
        insertSpecialInt(header->uid, 8, sb.st_uid);
    }
    else
    {
        sprintf(header->uid, "%07o", sb.st_uid);
    }

    /* Store the gid as an octal string within the header struct */
    sprintf(header->gid, "%07o", sb.st_gid);

    /* if the file is a regular file then store size as octal string,
     * otherwise the size is empty */
    if (ft == REGFILE)
    {
        sprintf(header->size, "%011lo", (long)sb.st_size);
    }
    else
    {
        strcpy(header->size, "00000000000");
    }

    /* Store mtime as an octal string in the header struct */
    sprintf(header->mtime, "%011lo", sb.st_mtime);

    /* Based on the file type, store that as the typeflag within
     * the header struct */
    if (ft == 5)
    {
        header->typeflag[0] = '5';
    }
    else if (ft == 2)
    {
        header->typeflag[0] = '2';
    }
    else if (ft == 0)
    {
        header->typeflag[0] = '0';
    }
    else
    {
        fprintf(stderr, "Unknown file type");
    }

    /* If the argument given is a symlink and its length is too long
     * then provide appropriate error message and exit
     * otherwise read the link */
    if (ft == SYMLINK)
    {
        if (strlen(filename) > LINKNAME_SIZE)
        {
            if (is_v)
            {
                fprintf(stderr, ": Linkname too long");
            }

            free(header);
            return header_buf;
        }
        readlink(filename, header->linkname, LINKNAME_SIZE);
    }

    /* Copy the strings of different attributes into the header struct */
    strncpy(header->magic, MAGIC_VAL, MAGIC_SIZE);
    strncpy(header->version, VERSION_VAL, VERSION_SIZE);
    strcpy(header->uname, getpwuid(sb.st_uid)->pw_name);
    strcpy(header->gname, getgrgid(sb.st_gid)->gr_name);

    /* Calls pack header which finishes copying header struct data
     * into a buffer */
    packHeader(header_buf, header);

    /* Use utility method to calculate the check sum */
    chksum = calcChksum(header_buf);
    /* Store it as a octal string within the header struct */
    sprintf(header->chksum, "%07o", chksum);
    /* Move it from the struct into the headerbuf */
    memcpy(header_buf + CHKSUM, header->chksum, CHKSUM_SIZE);
    free(header);
    return header_buf;
}

/* Resposnible for moving data from header to header buffer */
void packHeader(unsigned char *header_buf, Header *header)
{
    memcpy(header_buf + NAME, header->name, NAME_SIZE);
    memcpy(header_buf + MODE, header->mode, MODE_SIZE);
    memcpy(header_buf + UID, header->uid, UID_SIZE);
    memcpy(header_buf + GID, header->gid, GID_SIZE);
    memcpy(header_buf + SIZE, header->size, SIZE_SIZE);
    memcpy(header_buf + MTIME, header->mtime, MTIME_SIZE);
    memcpy(header_buf + CHKSUM, header->chksum, CHKSUM_SIZE);
    memcpy(header_buf + TYPEFLAG, header->typeflag, TYPEFLAG_SIZE);
    memcpy(header_buf + LINKNAME, header->linkname, LINKNAME_SIZE);
    memcpy(header_buf + MAGIC, header->magic, MAGIC_SIZE);
    memcpy(header_buf + VERSION, header->version, VERSION_SIZE);
    memcpy(header_buf + UNAME, header->uname, UNAME_SIZE);
    memcpy(header_buf + GNAME, header->gname, GNAME_SIZE);
    memcpy(header_buf + DEVMAJOR, header->devmajor, DEVMAJOR_SIZE);
    memcpy(header_buf + DEVMINOR, header->devminor, DEVMINOR_SIZE);
    memcpy(header_buf + PREFIX, header->prefix, PREFIX_SIZE);
}

/* Responsible for generating the name given the path and header */
int genName(Header *header, char *path, int is_v)
{
    int i;
    /* If path is greater than max name size then needs to be split
     * otherwise just strncpy it into header struct */
    if (strlen(path) > NAME_SIZE)
    {
        i = strlen(path) - (NAME_SIZE + 1);
        while (i < strlen(path))
        {
            /* if the length of the substring starting at i + 1
             * (plus one since not counting the slash) is  less than or equal
             * to 100 bytes then strncpy that into header struct */
            if (path[i] == '/')
            {
                if (NAME_SIZE >= strlen(&path[i + 1]))
                {
                    strncpy(header->name, &path[i + 1], NAME_SIZE);
                    break;
                }
                return 1;
            }
            i++;
        }
        /* Check if prefix fits the size requirment, if so then memcpy */
        if (i <= PREFIX_SIZE)
        {
            memcpy(header->prefix, path, i);
            return 0;
        }
    }
    else
    {
        strncpy(header->name, path, NAME_SIZE);
        return 0;
    }
    /* If both those conditions fail and verbosity is set then
     * then show error message */
    if (is_v)
    {
        fprintf(stderr, "Cannot split file into prefix and name");
    }
    return -1;
}

/* Provided method for handling non conforming integers */
int insertSpecialInt(char *where, size_t size, int32_t val)
{
    int err = 0;
    if (val < 0 || (size < sizeof(val)))
    {
        err++;
    }
    else
    {
        memset(where, 0, size);
        *(int32_t *)(where + size - sizeof(val)) = htonl(val);
        *where |= 0x80;
    }
    return err;
}

/* Responsible for adding the file contents into archive */
void addFile(char *path, FILE *archive_fp)
{
    FILE *new_file;
    int i;
    char block_buf[BLOCK_SIZE];
    /* Error check to make sure fopen works */
    if (!(new_file = fopen(path, "rb")))
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    /* Set each location in buffer to null */
    for (i = 0; i < BLOCK_SIZE; i++)
    {
        block_buf[i] = '\0';
    }
    /*  Read into buffer from the file */
    while (fread(block_buf, BLOCK_SIZE, 1, new_file))
    {
        /* if new file exists and not end of file */
        if (new_file && !feof(new_file))
        {
            /* then write block into archive */
            fwrite(block_buf, BLOCK_SIZE, 1, archive_fp);
            memset(block_buf, 0, BLOCK_SIZE);
        }
    }
    fwrite(block_buf, BLOCK_SIZE, 1, archive_fp);
    memset(block_buf, 0, BLOCK_SIZE);
    fclose(new_file);
}

/* Responsible for handling directories given as arguments */
void addDir(char *pathname, FILE *output, int is_v)
{
    DIR *dir;
    char *filename;
    struct stat sb;
    struct stat dir_sb;
    struct dirent *ent;

    /* error checking to make sure opendir and stat work */
    if (!(dir = opendir(pathname)))
    {
        perror("opendir");
        exit(EXIT_FAILURE);
    }
    if (stat(pathname, &dir_sb) == -1)
    {
        perror("stat failed");
        exit(EXIT_FAILURE);
    }
    /* For while there is an entry in the directory */
    while ((ent = readdir(dir)))
    {
        /* Check to make sure it isnt the parent directory */
        if (ent->d_ino != dir_sb.st_ino)
        {
            /* Make sure it isnt equal to itself or directory above */
            if (strcmp(ent->d_name, ".") && strcmp(ent->d_name, ".."))
            {
                /* Malloc memory for filename */
                filename = malloc(strlen(pathname) + strlen(ent->d_name) + 2);
                /* error check the malloc */
                if (!(filename))
                {
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }
                /* Copy the pathname given into file name */
                strcpy(filename, pathname);
                /* Add the slash */
                strcat(filename, "/");
                /* Then add the current dirent's name */
                strcat(filename, ent->d_name);
                /* Call verbosity method  */
                verbosity(sb, is_v, filename);
                /* Now stat the full name since we have the path for
                 * current directory as well */
                if (lstat(filename, &sb) == -1)
                {
                    perror("lstat");
                    exit(EXIT_FAILURE);
                }

                /* Based on what type the entry is, determine what to do
                 * with it accordingly.
                 * Ends up being recursive for directories as dir_handler calls
                 * addDir as well*/
                if (S_ISDIR(sb.st_mode))
                {
                    dir_handler(filename, output, sb, is_v);
                }
                else if (S_ISLNK(sb.st_mode))
                {
                    sym_handler(filename, output, sb, is_v);
                }
                else if (S_ISREG(sb.st_mode))
                {
                    reg_handler(filename, output, sb, is_v);
                }
                /* free since it gets allocated again at start of the loop */
                free(filename);
            }
        }
    }
    /* close directory*/
    closedir(dir);
}

/* Helper function for when verbosity is needed */
void verbosity(struct stat sb, int is_v, char *name)
{
    if (is_v)
    {
        /* if its a directory print with slash
         * otherwise normally */
        if (S_ISDIR(sb.st_mode))
        {
            printf("%s/", name);
        }
        else
        {
            printf("%s", name);
        }
    }
}

/* Method to handle when given path is a directory */
void dir_handler(char *filename, FILE *archive_fp, struct stat sb, int is_v)
{
    char *name;
    unsigned char *header;
    /* Calloc memory for the name and error check it */
    if (!(name = calloc(1, strlen(filename) + 2)))
    {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    /* Copy the filename/directory given into name */
    /* Add the slash to specificy its a directory */
    /* Generate header based on that new name */
    strcpy(name, filename);
    strcat(name, "/");
    header = genHeader(name, DIRECTORY, is_v);

    /* Write the header into the archive */
    fwrite(header, BLOCK_SIZE, 1, archive_fp);
    /* Free header */
    free(header);
    /* If verbose print new line to set up for future lines */
    if (is_v)
    {
        printf("\n");
    }
    /* Free name */
    free(name);
    /* Call add dir since this was a directory and needs to be traversed */
    addDir(filename, archive_fp, is_v);
}

/* Responsible for handling symbolic links */
void sym_handler(char *filename, FILE *archive_fp, struct stat sb, int is_v)
{
    unsigned char *header;
    /* Get the header given the filename and pass in symlink as filetype */
    header = genHeader(filename, SYMLINK, is_v);
    /* Write it into the achive */
    fwrite(header, BLOCK_SIZE, 1, archive_fp);
    /* If verbose print new line to set up for future lines */
    if (is_v)
    {
        printf("\n");
    }
    /* Free header for memory management */
    free(header);
}

/* Responsible for handling regular files */
void reg_handler(char *filename, FILE *archive_fp, struct stat sb, int is_v)
{
    unsigned char *header;
    /*  Generate the header given the filename and file type */
    header = genHeader(filename, REGFILE, is_v);
    /*  write it into the archive */
    fwrite(header, BLOCK_SIZE, 1, archive_fp);
    /* Since its not a symbolic link, write the actual data into archive */
    addFile(filename, archive_fp);
    /* If verbose print new line to set up for future lines */
    if (is_v)
    {
        printf("\n");
    }
    /* free header for memory management */
    free(header);
}
