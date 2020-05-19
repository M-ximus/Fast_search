#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

enum{
    E_BADARGS = -1,
    E_ERROR = -2,
    E_MEM = -3
};

typedef struct node{
    struct node* next;
    struct node* prev;
    uint64_t hash;
    size_t offset;
    size_t size;
    char file_name[256];
} node;

typedef struct List{
    node* head;
    node* tail;
    size_t size;
} List;

typedef struct hash_table{
    List* family_arr;
    size_t num_family;
} hash_table;

hash_table* hash_table_ctr(size_t anom_size);
int hash_table_dstr(hash_table* table);
node* add_node(hash_table* table, const char* file_name, size_t anom_size, size_t offset, uint64_t hash);

node* insert_node(List* parent, const char* buff, size_t size, size_t offset, uint64_t hash);
int delete_node(List* parent, node* del_node);
List* List_ctr();
int List_dstr(List* parent);
