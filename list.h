#ifndef LIST
#define LIST

#include "utility.h"
#define PERM_NUM_CHARS 10
#define OWNER_GROUP_CHARS 17
#define SIZE_CHARS 8
#define MTIME_CHARS 17

int list(int argc, char *argv[], int is_v, int is_S);
void print_archive_info(Header *header, char *perms,
                        char *owner_group, long size,
                        char *mtime, char *name, int is_v);
Header *unpack_header(char *header_b, FILE *archive_fp, int is_S);
char *getHeaderPath(Header *header);
char *getPerms(Header *header);
char *getOwnerGroup(Header *header);
char *getMtime(Header *header);
u_int32_t getSize(Header *header);
void ignoreBlock(char *header_buf, FILE *archive_fp);

#endif
