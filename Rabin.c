#include <math.h>
#include <sys/mman.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <dirent.h>
#include "Hash.h"

const size_t window             = 48;
const size_t base               = 2;
const size_t mod                = 337;
const size_t hash_family_number = 119;
const size_t error_size         = 4096;
const size_t min_hit_num        = 4096;

typedef struct hit_position
{
    size_t file_offset;
    size_t buff_offset;
    size_t anomaly_size;
    char file_name[256];
} hit_position;

//------------------------------------------------------------------------------

static uint64_t start_Rabin_hash(size_t x, size_t q, const unsigned char* buff, size_t* counter, size_t buff_size);
size_t Rabin_hash(size_t x, size_t q, const unsigned char* buff, size_t buff_size);
long int give_num(const char* str_num);
size_t file_size(int fd);
int file_func(const char* file_name, hash_table* info_table);
int collect_dir_info(const char* dir_name, hash_table* info_table);
int temple_func(const char* file_name, int dir_fd, hash_table* info_table);
static uint64_t gnu_hash(const char* buff, size_t size);

//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        perror("incorrect num of params");
        exit(EXIT_FAILURE);
    }

    hash_table* info_table = hash_table_ctr(hash_family_number);
    if (info_table == NULL)
    {
        perror("[main] Error creating of hash table\n");
        exit(EXIT_FAILURE);
    }
    int ret = collect_dir_info(argv[2], info_table);
    assert(ret == 0);


    ret = file_func(argv[1], info_table);
    assert(ret == 0);


    ret = hash_table_dstr(info_table);
    assert(ret == 0);

    return 0;
}


int collect_dir_info(const char* dir_name, hash_table* info_table)
{
    if(dir_name == NULL || info_table == NULL)
        return E_BADARGS;

    errno = 0;
    DIR* dir_point = opendir(dir_name);
    if (dir_point == NULL)
    {
        perror("[collect_dir_info] Opendir error");
        return E_ERROR;
    }

    errno = 0;
    int dir_fd = open(dir_name, O_RDONLY);
    if (dir_fd < 0)
    {
        perror("[collect_dir_info] Opendir error");
        return E_ERROR;
    }

    struct dirent* temple_info;
    errno = 0;
    int ret = 0;
    while((temple_info = readdir(dir_point)) != NULL)
    {
        if (temple_info->d_type == DT_REG)
        {
            ret = temple_func(temple_info->d_name, dir_fd, info_table);
            assert(ret == 0);
            //printf("%s\n", temple_info->d_name);
        }
    }
    if (errno != 0)
    {
        perror("[collect_dir_info] error in reading temple files");
        return E_ERROR;
    }

    closedir(dir_point);
    close(dir_fd);

    return 0;
}

char* mmap_readable_file(const char* file_name, size_t* return_size, int* fd)
{
    if (file_name == NULL || return_size == NULL || fd == NULL)
        return NULL;

    errno = 0;
    int in_fd = open(file_name, O_RDONLY); // O_BINARY?
    if (in_fd < 0)
    {
        perror("[readable_file] Can't open file");
        printf("[readable_file] file == %s\n", file_name);
        return NULL;
    }

    size_t size = file_size(in_fd);
    if (size == -1)
    {
        perror("[readable_file] file size error");
        return NULL;
    }

    errno = 0;
    char* buff = (char*) mmap(NULL, size, PROT_READ, MAP_PRIVATE, in_fd, 0);
    if (buff == NULL)
        perror("[readable_file] Can't map file int RAM");

    *return_size = size;
    *fd = in_fd;

    return buff;
}

char* mmap_readable_file_in_dir(const char* file_name, int dir_fd, size_t* return_size, int* fd)
{
    if (file_name == NULL || return_size == NULL || fd == NULL || dir_fd < 0)
        return NULL;

    errno = 0;
    int in_fd = openat(dir_fd, file_name, O_RDONLY); // O_BINARY?
    if (in_fd < 0)
    {
        perror("[readable_file] Can't open file");
        printf("[readable_file] file == %s\n", file_name);
        return NULL;
    }

    size_t size = file_size(in_fd);
    if (size == -1)
    {
        perror("[readable_file] file size error");
        return NULL;
    }

    errno = 0;
    char* buff = (char*) mmap(NULL, size, PROT_READ, MAP_PRIVATE, in_fd, 0);
    if (buff == NULL)
        perror("[readable_file] Can't map file int RAM");

    *return_size = size;
    *fd = in_fd;

    return buff;
}

int temple_func(const char* file_name, int dir_fd, hash_table* info_table)
{
    if (file_name == NULL || info_table == NULL || dir_fd < 0)
        return E_BADARGS;

    size_t size = 0;
    int fd = 0;
    char* buff = mmap_readable_file_in_dir(file_name, dir_fd, &size, &fd);
    if (buff == NULL || fd < 0)
    {
        perror("[temple_func] mmap file error");
        return E_MEM;
    }

    size_t curr_zero = 0;
    size_t max_size = 0;
    do{
        size_t ret = Rabin_hash(base, mod, buff + curr_zero, size - curr_zero);
        if (ret == E_BADARGS)
        {
            perror("[temple_func] Rabin hash bad args");
            return E_ERROR;
        }

        uint64_t string_hash = gnu_hash(buff + curr_zero, ret);
        node* new_node = add_node(info_table, file_name, ret, curr_zero, string_hash);
        if (new_node == NULL)
        {
            perror("[temple_func] Error adding new node");
            return E_ERROR;
        }

        curr_zero += ret;
        if (ret > max_size)
            max_size = ret;

        if (ret > error_size)
            perror("[temple_func] size of anomaly > error size");

    }while(curr_zero < size && (size - curr_zero) > window);

    printf("%lu\n", max_size);

    errno = 0;
    int ret = munmap(buff, size);
    if (ret < 0)
    {
        perror("[file_func] unmap error");
        return E_MEM;
    }
    close(fd);

    return 0;
}

long int check_sim(const char* file_name, size_t file_offset, char* buff_text, size_t anom_size, size_t buff_offset, size_t buff_size, hit_position* hit_info)
{
    if (file_name == NULL || buff_text == NULL || hit_info == NULL || anom_size == 0)
    {
        perror("[check_sim] Bad args");
        return E_BADARGS;
    }

    size_t file_size = 0;
    int fd = 0;
    char* file_buff = mmap_readable_file(file_name, &file_size, &fd);
    if (file_buff == NULL || fd < 0)
    {
        perror("[check_sim] Mmap error");
        return E_MEM;
    }

    char* cur_buff    = buff_text + buff_offset;
    char* cur_file    = file_buff + file_offset;
    char* buff_border = buff_text + buff_size;
    char* file_border = file_buff + file_size;

    long int hit_size = 0;
    while (cur_buff != buff_border && cur_file != file_border)
    {
        hit_size++;
        cur_buff++;
        cur_file++;
    }

    cur_buff = buff_text + buff_offset;
    cur_file = file_buff + file_offset;

    while (cur_buff != buff_text && cur_file != file_buff)
    {
        hit_size++;
        cur_buff--;
        cur_file--;
    }

    if (hit_size >= min_hit_num)
    {
        hit_info->file_offset = cur_file - file_buff;
        hit_info->buff_offset = cur_buff - buff_text;
        hit_info->anomaly_size = hit_size;

        size_t len = strlen(file_name);
        if (len > 255)
            len = 255;
        memcpy(hit_info->file_name, file_name, len);
        hit_info->file_name[255] = '\0';
    }

    errno = 0;
    int ret = munmap(file_buff, file_size);
    if (ret < 0)
    {
        perror("[check_sim] unmap error");
        return E_MEM;
    }
    close(fd);

    return hit_size;
}

long int find_anomaly(hash_table* info_table, char* buff_text, size_t anom_size, size_t buff_offset, size_t buff_size, hit_position* hit_info)
{
    if (info_table == NULL || buff_text == NULL || hit_info == NULL)
        return E_BADARGS;

    uint64_t text_hash = gnu_hash(buff_text + buff_offset, anom_size);
    size_t cur_family = text_hash % (info_table->num_family);
    size_t num_nodes = (info_table->family_arr + cur_family)->size;

    node* cur_node = (info_table->family_arr + cur_family)->head;
    for(size_t i = 0; i < num_nodes; i++)
    {
        if (cur_node == NULL)
        {
            perror("[find_anomaly] Bad list - curr node == NULL");
            return E_ERROR;
        }
        if (cur_node->hash == text_hash && cur_node->size == anom_size)
        {
            long int hit_num = check_sim(cur_node->file_name, cur_node->offset, buff_text, anom_size, buff_offset, buff_size, hit_info);
            if (hit_num >= min_hit_num)
                return hit_num;
            if (hit_num < 0)
            {
                perror("[find_anomaly] compare error");
                exit(EXIT_FAILURE);
            }
        }
        cur_node = cur_node->next;
    }

    return 0;
}

int file_func(const char* file_name, hash_table* info_table)
{
    if (file_name == NULL || info_table == NULL)
        return E_BADARGS;

    size_t size = 0;
    int fd = 0;
    char* buff = mmap_readable_file(file_name, &size, &fd);
    if (buff == NULL || fd < 0)
    {
        perror("[file_func] mmap file error");
        return E_MEM;
    }

    errno = 0;
    hit_position* hit_info = (hit_position*) calloc(1, sizeof(*hit_info));
    if (hit_info == NULL)
    {
        perror("[file_func] hil info alloc error");
        return E_MEM;
    }
    size_t curr_zero = 0;
    size_t max_size = 0;
    size_t hit_rate = 0;
    do{
        size_t ret = Rabin_hash(base, mod, buff + curr_zero, size - curr_zero);
        if (ret == E_BADARGS)
        {
            perror("[file_func] Rabin hash bad args");
            return E_ERROR;
        }

        long int hits = find_anomaly(info_table, buff, ret, curr_zero, size, hit_info);
        if (hits > 0)
            hit_rate++;
            //printf("%s/n", hit_info->file_name);
        if (hits < 0)
        {
            perror("[file_func] find anomaly error");
            exit(EXIT_FAILURE);
        }

        curr_zero += ret;
        if (ret > max_size)
            max_size = ret;

        if (ret > error_size)
            perror("[file_func] size of anomaly > error size");

    }while(curr_zero < size && (size - curr_zero) > window);

    printf("%lu\n", max_size);
    printf("hit rate = %lu\n", hit_rate);

    free(hit_info);

    errno = 0;
    int ret = munmap(buff, size);
    if (ret < 0)
    {
        perror("[file_func] unmap error");
        return E_MEM;
    }
    close(fd);

    return 0;
}

size_t file_size(int fd)
{
    if (fd < 0)
        return E_BADARGS;

    struct stat file_stat;

    errno = 0;
    int ret = fstat(fd, &file_stat);
    if (ret < 0)
        return ret;

    return file_stat.st_size;
}

static uint64_t start_Rabin_hash(size_t x, size_t q, const unsigned char* buff, size_t* counter, size_t buff_size)
{
    assert(buff != NULL); // dev_time check
    assert(counter != NULL); // dev_time check
    if (x == 0 || q == 0)
    {
        perror("[start_Rabin_hash] Bad input args");
        return E_BADARGS;
    }
    if (buff_size < window)
    {
        perror("[start_Rabin_hash] Too small buffer for this window!");
        return E_BADARGS;
    }

    uint64_t curr_hash = 0;
    size_t i = 0;

    for(; i < window - 1; i++)
    {
        curr_hash = ((curr_hash + buff[i]) * x) % q;
    }
    curr_hash = (curr_hash + buff[i]) % q;

    *counter += window;

    return curr_hash;
}

size_t Rabin_hash(size_t x, size_t q, const unsigned char* buff, size_t buff_size)
{
    assert(buff != NULL); // dev_time check
    if (x == 0 || q == 0)
    {
        perror("[Rabin_hash] Bad input args");
        return E_BADARGS;
    }
    if (buff_size < window)
    {
        perror("[Rabin_hash] Too small buffer for this window!");
        return E_BADARGS;
    }

    size_t counter = 0;
    uint64_t curr_hash = start_Rabin_hash(x, q, buff, &counter, buff_size);

    uint64_t x_poly = 1;
    for(int i = 1; i < window; i++) // O(n) -> O(logn)
        x_poly *= x;

    size_t i = counter;

    for(; i < buff_size; i++)
    {
        curr_hash = (((curr_hash - buff[i - window] * x_poly % q) % q) * x % q + buff[i]) % q;
        if (curr_hash == 0)
            break;
    }

    return i;
}

static uint64_t gnu_hash(const char* buff, size_t size)
{
    assert(buff != NULL);

    uint64_t hash = 0;
    for(size_t i = 0; i < size; i++)
        hash = hash * 33 + buff[i];

    return hash;
}
/*
long int give_num(const char* str_num)
{
    if (str_num == NULL)
        return

    long int in_num = 0;
    char *end_string;

    errno = 0;
    in_num = strtoll(str_num, &end_string, 10);
    if ((errno != 0 && in_num == 0) || (errno == ERANGE && (in_num == LLONG_MAX || in_num == LLONG_MIN))) {
        perror("Bad string");
        return E_MAX;
    }

    if (str_num == end_string) {
        perror("No number");
        return E_COPY;
    }

    if (*end_string != '\0') {
        perror("Garbage after number");
        return E_RIGHT;
    }

    if (in_num < 0) {
        perror("i want unsigned num");
        return E_ERROR;
    }

    return in_num;
}*/
