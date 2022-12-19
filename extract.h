#ifndef EXTRACT
#define EXTRACT

#include "utility.h"

int extract(int argc, char *argv[], int is_v, int is_S);

void check_file_type(Header *header, FILE *archive_fp, char *name, int is_v);

void make_mode(Header *header, FILE *archive_fp, char *name, FILE *ret_file);

void make_utime(Header *header, FILE *archive_fp, char *name, FILE *ret_file);

int multi_file_handler(int argc, char *argv[], FILE *archive, Header *header,
                       char *name, int is_v);

#endif
