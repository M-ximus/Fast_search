// Test sample for debugging hash table

#include "Hash.h"
#include "stdio.h"

int main()
{
    hash_table* table = hash_table_ctr(2);
    assert(table != NULL);

    node* test_rat_0 = add_node(table, "Hamlet.txt", 10, 0, 0);
    assert(test_rat_0 != NULL);
    printf("%s\n", table->family_arr->head->file_name);

    node* test_rat_1 = add_node(table, "Hamlet.txt", 15, 10, 1);
    assert(test_rat_1 != NULL);
    printf("%s\n", (table->family_arr + 1)->head->file_name);

    node* test_rat_2 = add_node(table, "Text.txt", 10, 0, 2);
    assert(test_rat_2 != NULL);
    printf("%s\n", table->family_arr->head->next->file_name);

    int ret = hash_table_dstr(table);
    assert(ret == 0);

    return 0;
}
