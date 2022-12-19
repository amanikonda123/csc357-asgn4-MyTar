#ifndef UTIL
#define UTIL

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <libgen.h>
#include <wchar.h>
#include <arpa/inet.h>
#include <utime.h>

#define NAME 0
#define MODE 100
#define UID 108
#define GID 116
#define SIZE 124
#define MTIME 136
#define CHKSUM 148
#define TYPEFLAG 156
#define LINKNAME 157
#define MAGIC 257
#define VERSION 263
#define UNAME 265
#define GNAME 297
#define DEVMAJOR 329
#define DEVMINOR 337
#define PREFIX 345
#define BASE8 8

#define BLOCK_SIZE 512

#define NAME_SIZE 100
#define MODE_SIZE 8
#define UID_SIZE 8
#define GID_SIZE 8
#define SIZE_SIZE 12
#define MTIME_SIZE 12
#define CHKSUM_SIZE 8
#define TYPEFLAG_SIZE 1
#define LINKNAME_SIZE 100
#define MAGIC_SIZE 6
#define VERSION_SIZE 2
#define UNAME_SIZE 32
#define GNAME_SIZE 32
#define DEVMAJOR_SIZE 8
#define DEVMINOR_SIZE 8
#define PREFIX_SIZE 155
#define PADDING_SIZE 12

#define SPACE 32
#define MAGIC_VAL "ustar\0"
#define VERSION_VAL "00"

#define REGFILE 0
#define SYMLINK 2
#define DIRECTORY 5

typedef struct Header
{
    char name[NAME_SIZE];
    char mode[MODE_SIZE];
    char uid[UID_SIZE];
    char gid[GID_SIZE];
    char size[SIZE_SIZE];
    char mtime[MTIME_SIZE];
    char chksum[CHKSUM_SIZE];
    char typeflag[TYPEFLAG_SIZE];
    char linkname[LINKNAME_SIZE];
    char magic[MAGIC_SIZE];
    char version[VERSION_SIZE];
    char uname[UNAME_SIZE];
    char gname[GNAME_SIZE];
    char devmajor[DEVMAJOR_SIZE];
    char devminor[DEVMINOR_SIZE];
    char prefix[PREFIX_SIZE];
    char padding[PADDING_SIZE];
} Header;

char *getHeaderPath(Header *header);

void ignoreBlock(char *header_buf, FILE *archive_fp);

unsigned int calcChksum(unsigned char *output);

Header *unpack_header(char *header_b, FILE *archive_fp, int is_S);

void write_null_blocks(char header_buf[], FILE *output, int num);

#endif
