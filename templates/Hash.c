#include "Hash.h"

node* insert_node(List* parent, const char* buff, size_t size, size_t offset, uint64_t hash)
{
    if (parent == NULL || buff == NULL)
        return NULL;

    errno = 0;
    node* new_node = (node*) calloc(1, sizeof(node));
    if (new_node == NULL)
    {
        perror("[insert_node] Alloc error in new node");
        return NULL;
    }

    (parent->size)++;

    if (parent->head == NULL)
    {
        parent->head = new_node;
        parent->tail = new_node;
        parent->head->prev = NULL;
    }
    else
    {
        parent->tail->next = new_node;
        new_node->prev = parent->tail;
        parent->tail = new_node;
    }

    new_node->next   = NULL;
    new_node->offset = offset;
    new_node->size   = size;
    new_node->hash   = hash;

    size_t file_name_size = strlen(buff);
    if (file_name_size > 255)
        file_name_size = 255;
    memcpy(new_node->file_name, buff, file_name_size);
    new_node->file_name[255] = '\0'; // I have a paronoia

    return new_node;
}

int delete_node(List* parent, node* del_node)
{
    if (parent == NULL || del_node == NULL)
        return E_BADARGS;

    del_node->offset = 0;
    del_node->size   = 0;
    del_node->hash   = 0;

    (parent->size)--;

    if (parent->head == del_node)
    {
        parent->head = del_node->next;
        if (del_node->next != NULL)
            del_node->next->prev = NULL;
    }
    else if (parent->tail == del_node)
    {
        parent->tail = del_node->prev;
        if (del_node->prev != NULL)
            del_node->prev->next = NULL;
    }
    else
    {
        assert(del_node->prev != NULL);
        assert(del_node->next != NULL);
        del_node->prev->next = del_node->next;
        del_node->next->prev = del_node->prev;
    }

    free(del_node);

    return 0;
}

List* List_ctr(List* new_List)
{
    if (new_List == NULL)
        return NULL;

    new_List->size = 0;
    new_List->head = NULL;
    new_List->tail = NULL;

    return new_List;
}

int List_dstr(List* parent)
{
    if (parent == NULL)
        return E_BADARGS;

    node* curr_node = parent->head;
    node* next_node = NULL;
    size_t num_nodes = parent->size;
    for(size_t i = 0; i < num_nodes; i++)
    {
        if (curr_node == NULL)
            return E_ERROR;

        next_node = curr_node->next;
        delete_node(parent, curr_node);
        curr_node = next_node;
    }

    parent->size = 0;
    parent->head = NULL;
    parent->tail = NULL;

    return 0;
}

node* add_node(hash_table* table, const char* file_name, size_t anom_size, size_t offset, uint64_t hash)
{
    if (table == NULL || file_name == NULL || anom_size == 0)
        return NULL;
    assert(table->family_arr != NULL);

    size_t cur_family = hash % (table->num_family);
    node* new_node = insert_node(table->family_arr + cur_family, file_name, anom_size, offset, hash);
    return new_node;
}

hash_table* hash_table_ctr(size_t anom_size)
{
    if (anom_size == 0)
        return NULL;

    errno = 0;
    hash_table* new_table = (hash_table*) calloc(1, sizeof(*new_table));
    if (new_table == NULL)
        return NULL;

    new_table->num_family = anom_size;

    errno = 0;
    new_table->family_arr = (List*) calloc(anom_size, sizeof(*(new_table->family_arr)));
    if (new_table == NULL)
    {
        free(new_table);
        return NULL;
    }

    for(size_t i = 0; i < anom_size; i++)
    {
        List* ret = List_ctr(new_table->family_arr + i);
        if (ret == NULL)
        {
            free(new_table->family_arr);
            free(new_table);
        }
    }

    return new_table;
}


int hash_table_dstr(hash_table* table)
{
    if (table == NULL)
        return E_BADARGS;

    for(size_t i = 0; i < table->num_family; i++)
    {
        List_dstr(table->family_arr + i);
    }

    free(table->family_arr);
    free(table);

    return 0;
}
