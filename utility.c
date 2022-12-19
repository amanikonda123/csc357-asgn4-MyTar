#include "utility.h"

/* This function retrieves the name from the header block */
char *getHeaderPath(Header *header)
{
    char *path, *prefix, *name;
    prefix = header->prefix;
    name = header->name;

    /* If the full name fits in name field, return the name */
    if (strlen(prefix) == 0)
    {
        path = (char *)calloc(NAME_SIZE + 2, sizeof(char));
        if (!path)
        {
            perror("calloc");
            exit(EXIT_FAILURE);
        }
        strncpy(path, name, NAME_SIZE);
        return path;
    }

    /* Otherwise, concatenate prefix and name and separate it with a '/' */
    path = (char *)calloc(NAME_SIZE + strlen(prefix) + 2, sizeof(char));
    if (!path)
    {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    strcpy(path, prefix);
    strcat(path, "/");
    strncat(path, name, NAME_SIZE);
    return path;
}

/* This function passes over the data blocks if the header describes a
 * regular file */
void ignoreBlock(char *header_buf, FILE *archive_fp)
{
    unsigned char block_buf[BLOCK_SIZE];
    long size;
    int i, num_read;
    /* Retrives size field from header and sees how many data blocks need
     * to be read*/
    size = strtol((char *)&header_buf[SIZE], NULL, BASE8);
    num_read = size / BLOCK_SIZE + 1;

    /* Reads over that many blocks */
    for (i = num_read; i > 0; i--)
    {
        fread(block_buf, sizeof(char), BLOCK_SIZE, archive_fp);
    }
}

/* This function calculates the true checksum value of a header block */
unsigned int calcChksum(unsigned char *header_b)
{
    unsigned int chksum = 0;
    int i;
    /* Adds up every byte in header block except the blocks in the
     * checksum field */
    for (i = 0; i < BLOCK_SIZE; i++)
    {
        if (!(i >= CHKSUM && (i < CHKSUM + CHKSUM_SIZE)))
        {
            chksum += (unsigned char)header_b[i];
        }
    }
    /* Replaces the values of the checksum bytes with spaces */
    chksum += CHKSUM_SIZE * SPACE;
    return chksum;
}

/* This function reads in a header buffer and stores it inside
 * a header struct. Also checks if header is valid. */
Header *unpack_header(char *header_b, FILE *archive_fp, int is_S)
{
    /* assumes archive_fp is pointing at start of header block */
    Header *header;
    int i, nullCheck = 1, is_null_block = 0;
    int checksum_file = 0, checksum_header;

    /* Allocates necessary space */
    header = (Header *)calloc(1, sizeof(Header));
    if (!header)
    {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    /* Copies whatever is in header buffer into header struct */
    memcpy(header->name, header_b + NAME, NAME_SIZE * sizeof(char));
    memcpy(header->mode, header_b + MODE, MODE_SIZE * sizeof(char));
    memcpy(header->uid, header_b + UID, UID_SIZE * sizeof(char));
    memcpy(header->gid, header_b + GID, GID_SIZE * sizeof(char));
    memcpy(header->size, header_b + SIZE, SIZE_SIZE * sizeof(char));
    memcpy(header->mtime, header_b + MTIME, MTIME_SIZE * sizeof(char));
    memcpy(header->chksum, header_b + CHKSUM, CHKSUM_SIZE * sizeof(char));
    memcpy(header->typeflag, header_b + TYPEFLAG, TYPEFLAG_SIZE * sizeof(char));
    memcpy(header->linkname, header_b + LINKNAME, LINKNAME_SIZE * sizeof(char));
    memcpy(header->magic, header_b + MAGIC, MAGIC_SIZE * sizeof(char));
    memcpy(header->version, header_b + VERSION, VERSION_SIZE * sizeof(char));
    memcpy(header->uname, header_b + UNAME, UNAME_SIZE * sizeof(char));
    memcpy(header->gname, header_b + GNAME, GNAME_SIZE * sizeof(char));
    memcpy(header->devmajor, header_b + DEVMAJOR, DEVMAJOR_SIZE * sizeof(char));
    memcpy(header->devminor, header_b + DEVMINOR, DEVMINOR_SIZE * sizeof(char));
    memcpy(header->prefix, header_b + PREFIX, PREFIX_SIZE * sizeof(char));

    /* If 'S' flag was passed through, check if it complies with the
     * standard. */
    if (is_S == 1)
    {
        /* If the magic number isn't null-terminated and version
         * number isn't '00', then it's an invalid header*/
        if (!(!(header->magic[5]) && header->version[0] == '0' &&
              header->version[1] == '\0'))
        {
            fprintf(stderr, "Invalid Header");
            exit(EXIT_FAILURE);
        }
    }

    /* Checks if header's checksum and true checksum values match up */
    checksum_file = calcChksum((unsigned char *)header_b);
    checksum_header = strtol(header->chksum, NULL, BASE8);
    if ((checksum_header != checksum_file))
    {
        /* The two checksums can not match up if at terminating null blocks.
         * If the archive isn't terminated by 2 null blocks, it's an invalid
         * header */

        /* Checks if current block is a null block*/
        for (i = 0; i < BLOCK_SIZE; i++)
        {
            if (memcmp(&header_b[i], "\0", 1) == 0)
            {
                is_null_block = 1;
            }
        }

        /* Checks if next block is a null block*/
        if (is_null_block == 1)
        {
            if (fread(header_b, BLOCK_SIZE, 1, archive_fp))
            {
                for (i = 0; i < BLOCK_SIZE; i++)
                {
                    if (memcmp(&header_b[i], "\0", 1))
                    {
                        nullCheck = 0;
                        break;
                    }
                }
            }
        }
        /* If they are not both null blocks, it's an invalid header */
        if (nullCheck == 0)
        {
            fprintf(stderr, "Invalid Header");
            exit(EXIT_FAILURE);
        }
        free(header);
        return NULL;
    }
    return header;
}

void write_null_blocks(char buffer[], FILE *output, int num)
{
    int i;
    for (i = 0; i < num; i++)
    {
        fwrite(buffer, BLOCK_SIZE, 1, output);
    }
}
