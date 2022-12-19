#ifndef CREATE
#define CREATE

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <libgen.h>
#include <fcntl.h>
#include <wchar.h>
#include <arpa/inet.h>
#include "utility.h"

int create(int argc, char *argv[], int is_v, int is_S);

unsigned char *genHeader(char *filename, int ft, int is_v);

int genName(Header *header, char *path, int is_v);

int insertSpecialInt(char *where, size_t size, int32_t val);

void addFile(char *path, FILE *outfile);

void addDir(char *pathname, FILE *output, int);

void packHeader(unsigned char *output, Header *header);

unsigned int calcChksum(unsigned char *output);

void dir_handler(char *filename, FILE *archive_fp, struct stat sb, int is_v);

void sym_handler(char *filename, FILE *archive_fp, struct stat sb, int is_v);

void reg_handler(char *filename, FILE *archive_fp, struct stat sb, int is_v);

void verbosity(struct stat sb, int is_v, char *name);

#endif
