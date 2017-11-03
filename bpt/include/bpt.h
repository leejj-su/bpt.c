#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

extern int fd;

typedef struct {
    int64_t key;
    char value[120];
} record;

void plus_number_of_pages();
int open_db(char * pathname);
void usage(void);
int isleaf(int offset);
int number_of_keys(int64_t page_start_offset);
void set_number_of_keys(int64_t page_start_offset, int number);
int64_t find_next_offset_of_internal(int offset_,int64_t key);
int64_t find_leafpage_offset(int start_offset,int64_t key);
int64_t root_page_offset();
void set_rootpage_offset(int64_t rootoffset);
char* find(int64_t key);
int64_t find_place_to_make_preepage();
void decision_make_preepage();
int64_t return_preepage_offset();
int64_t pree_to_leaf();
int64_t pree_to_internal();
void buildtree(int64_t key, char* value);
int64_t get_parent_offset(int64_t leaf_offset);
void set_parent_offset(int64_t page_offset, int64_t value_of_offset);
int64_t split_internal(int64_t leftpage_offset,int64_t rightpage_offset,int64_t key);
int insert_internal(int64_t leftpage_offset,int64_t rightpage_offset,int64_t key);
int insert_leaf(int64_t leafpage_offset,int64_t key,char* value);
int64_t split_leaf(int64_t leafoffset,int64_t key,char* value);
int insert(int64_t key,char* value);
void print_leaf();



