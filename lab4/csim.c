// 20160443 Dongmin Lee
#include "cachelab.h"
# include <stdio.h>
# include <getopt.h>
# include <strings.h>
# include <stdlib.h>

// structure for the cache size information
typedef struct{

    int s;
    int b;
    int E;
    int S;
    int B;
}cache_size;

// variable for finding whether there is -
int verbose;

int main(int argc, char **argv)
{
    cache_size _cache;
    char *file_name;
    char c;                 // argument for parsing

    // parameter for counting hit_count, miss_count, eviction_count
    int hit_count = 0;
    int miss_count = 0;
    int eviction_count = 0;

    // parameter which means for space i.e L, M, S
    char space;

    // size read from file
    int size;

    // value for Least Recently Used (LRU)
    int used = 0;

    // index of empty space
    int empty = -1;

    // check if there is hit or eviction
    int isHit = 0;
    int isEvict = 0;

    // address memomry
    unsigned long long address;

    // parsing the "command-line argument"
    while (( c = getopt( argc, argv, "s:E:b:t:vh" )) != -1)
       {
          switch ( c )
          {
             case 's':
                 _cache.s = atoi( optarg );
                break;
             case 'E':
                _cache.E = atoi( optarg );
                break;
             case 'b':
                _cache.b = atoi( optarg );
                break;
             case 't':
                file_name = optarg;
                break;
             case 'v':
                verbose = 1;
                break;
             case 'h':
                exit( 0 );
             default:
                exit( 1 );
          }
       }





    // make a notice if the cache size is zero or the file is wrong
    if (_cache.s == 0 || _cache.E == 0 || _cache.b == 0 || file_name == NULL){

        printf("Not a right cache size or file name!\n");
        exit(1);
    }

    // setting the cache size
    _cache.S = 1 << _cache.s;
    _cache.B = 1 << _cache.b;


    // structure for cache line
    typedef struct{

        int valid;
        unsigned long long tag;
        int data_bit;

    }cache_line;

    // structure for cach_set
    typedef struct{

        cache_line *line;

    }cache_set;

    // making cache array 
    typedef struct{

        cache_set *set;
    }cache_array;


    cache_array cache;

    // allocate space for set and line
    cache.set = malloc(_cache.S * sizeof(cache_set));

    for (int i = 0; i < _cache.S; i++){

        cache.set[i].line = malloc(sizeof(cache_line) * _cache.E);

    }

    // open file and read it
    FILE *_file_name = fopen(file_name, "r");

    if (_file_name != NULL){

        // read the format
        while (fscanf (_file_name, " %c %llx,%d", &space, &address, &size) ==3){

            // keep track of what to evict
            int E = 0;

            // do not have to take care of "I" instruction
            if (space != 'I'){

                // setting the tag and index of the address
                unsigned long long address_tag = address >> (_cache.s + _cache.b);
                unsigned long long temp = address << (64 - (_cache.s +  _cache.b));
                unsigned long long set_index = temp >> (64 - _cache.s);
                cache_set set = cache.set[set_index];
                int max = 0x7fffffff;

                for (int e = 0; e < _cache.E; e++){

                    // check if the valid of the line is 1 or not
                    if (set.line[e].valid == 1){

                        // check if the part of the tag is same with the address_tag
                        // if same, increase hit_count, set isHit and update data_bit
                        if (set.line[e].tag == address_tag){

                            hit_count += 1;
                            isHit = 1;
                            set.line[e].data_bit = used;
                            used += 1;

                        }

                        // if tag is not same with address tag, update max
                        else if (set.line[e].data_bit < max){

                            max = set.line[e].data_bit;
                            E = e;

                        }
                    }

                    // if there is no empty, mark with it e
                    else if (empty == -1){
                        empty = e;
                    }

                }

                // cahce miss
                if (isHit != 1){

                    miss_count += 1;

                    // if there is an empty line
                    if (empty > -1){
                        set.line[empty].valid = 1;
                        set.line[empty].tag = address_tag;
                        set.line[empty].data_bit = used;
                        used += 1;
                    }

                    // if the set is full, do eviction
                    else if (empty < 0){

                        isEvict = 1;
                        set.line[E].tag = address_tag;
                        set.line[E].data_bit = used;
                        used += 1;
                        eviction_count += 1;

                    }

                }

                // if the space is 'M' we write on it which is always cache hit
                if (space == 'M'){

                    hit_count += 1;

                }

                // if the command line argument has -v
                if (verbose == 1){

                    printf("%c ", space);
                    printf("%llx,%d", address, size);
                    if ( isHit == 1){

                        printf("Hit ");
                    }

                    else if (isHit != 1){

                        printf("Miss ");

                    }
                    if (isEvict == 1){

                        printf("Eviction ");


                    }

                    if (space == 'M'){

                        printf("Hit");
                    }

                    printf("\n");
                }
                    // reset the parameter to initial value since the for-loop has to be gone
                    empty = -1;
                    isHit = 0;
                    isEvict = 0;
            }


        }

    }


    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
