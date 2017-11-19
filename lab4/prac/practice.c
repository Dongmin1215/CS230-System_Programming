// 20160443 Dongmin Lee
# include "cachelab.h"
# include <stdio.h>
# include <stdlib.h>
# include <getopt.h>
# include <limits.h>

typedef struct{

    int s;
    int S;
    int b;
    int B;
    int E;

}cache_data;

int verbose = 0;

int main(int argc, char **argv){

    char *file_name;
    char c;
    cache_data _cache;
    int hit_count = 0;
    int miss_count = 0;
    int eviction_count = 0;

    while (( c = getopt(argc, argv, "hvs:E:b:t:")) != -1){
        switch (c){

            case('h'):
                exit(0);
            case('v'):
                verbose = 1;
                break;
            case('s'):
                _cache.s = atoi(optarg);
                break;
            case('E'):
                _cache.E = atoi(optarg);
                break;
            case('b'):
                _cache.b = atoi(optarg);
                break;
            case('t'):
                file_name = optarg;
                break;
            default:
                exit(1);

        }
    }


    if ( _cache.s == 0 || _cache.E == 0 || _cache.b == 0 || file_name == NULL){
        printf("Not enough arguments!\n");
        exit(1);
    }


    _cache.S = 1 << _cache.s;
    _cache.B = 1 << _cache.b;

    typedef struct{

        int valid;
        long tag;
        int data_bit;

    }cache_line;


    typedef struct{

        cache_line *line;

    }cache_set;

    typedef struct{

        cache_set *set;
    }cache_array;

    cache_array cache;

    cache.set = malloc(_cache.S * sizeof(cache_set));
    for (int i = 0; i < _cache.S; i++){

        cache.set[i].line = malloc(sizeof(cache_line) * _cache.E);

    }

    char space;
    unsigned long long address;
    int size;
    int used = 0;
    int isHit = 0;
    int isEvict = 0;
    int empty = -1;
    int is_empty = 0;

    FILE *filename = fopen(file_name, "r");
    if (filename != NULL){

        while (fscanf(filename, " %c %llx,%d", &space, &address, &size) == 3){
            int pos_evict = 0;

            if (pos_evict == 0){
                break;
            }
            if (space != 'I'){

                unsigned long long addr_tag = address >> (_cache.s + _cache.b);
                int addr_tag_size = (64 - (_cache.s + _cache.b));
                unsigned long long temp = address << addr_tag_size;
                unsigned long long index_of_set = temp >> (addr_tag_size + _cache.b);

                cache_set set = cache.set[index_of_set];
                int low = INT_MAX;

                for (int e = 0; e < _cache.E; e++){
                    if (set.line[e].tag == addr_tag){
                        hit_count += 1;
                        isHit = 1;
                        set.line[e].data_bit = used;
                        used += 1;
                    }
                    else if (set.line[e].data_bit < low){
                        low = set.line[e].data_bit;
                        pos_evict = e;
                    }

                    else if (empty == -1){
                        empty = e;
                }


                }

                if ( isHit != 1){

                    miss_count += 1;

                    if (empty > -1){

                        set.line[empty].valid = 1;
                        set.line[empty].tag = addr_tag;
                        set.line[empty].data_bit = used;
                        used += 1;
                    }
                    else if (empty < 0){
                        isEvict = 1;
                        set.line[is_empty].tag = addr_tag;
                        set.line[is_empty].data_bit = used;
                        used += 1;
                        eviction_count += 1;

                    }

                }

                if (space == 'M'){

                    hit_count += 1;
                }

                if (verbose == 1){

                    printf("%c", space);
                    printf("%llx,%d", address, size);
                    if (isHit == 1){
                        printf("Hit ");

                    }
                    else if (isHit != 1){
                        printf("Miss");
                    }
                    if (isEvict == 1){
                        printf("Eviction ");

                    }
                    if (space == 'M'){

                        printf("Hit");
                    }
                    printf("\n");
                }
                empty = -1;
                isHit = 0;
                isEvict =0;



            }

        }
    }


    printSummary(hit_count,miss_count,eviction_count);
    return 0;

