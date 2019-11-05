#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cachelab.h"

typedef struct{
    bool valid;
    int tag, lru;
}Line;

typedef struct{
    int len;
    Line *lines;
}Set;

typedef struct{
    int set_num;
    int miss, hit, evic;
    Set* sets;
}Cache;
Cache cache;
int cnt = 0;

void print_document();
bool check_num(char* s);
void init_cache(int E, int b, int s);
void work(int E, int b, int s, bool v);
void load(unsigned int ad, int l, int E, int b, int s, bool v);
void save(unsigned int ad, int l, int E, int b, int s, bool v);
void findline(Set s, int tag, bool v);
int find_min_lru(Set s);
void free_cache();
void print_summary(int hit, int miss, int evic);

int main(int argc, char **argv)
{
    /* code */
    // E: number of lines per set 
    // s: Number of set index bits (S = 2^s is the number of sets)
    // b: Number of block bits (B = 2b is the block size)
    int E, b, s; 
    bool v = false;
    char* t;
    if(argc == 2){
        if(strcmp(argv[1], "-h")) print_document();
        else printf("Wrong arguments!");
    }else if(argc > 10) printf("Too many arguments!");
    else{
        bool f[11] = {false};
        for(int i = 1; i < argc; i++){
            if(strcmp(argv[i], "-E") == 0){
                if(!check_num(argv[i + 1])) return 0;
                E = atoi(argv[i + 1]);
                f[i] = f[i + 1] = true;
            }
            if(strcmp(argv[i], "-s") == 0){
                if(!check_num(argv[i + 1])) return 0;
                s = atoi(argv[i + 1]);
                f[i] = f[i + 1] = true;
            }
            if(strcmp(argv[i], "-b") == 0){
                if(!check_num(argv[i + 1])) return 0;
                b = atoi(argv[i + 1]);
                f[i] = f[i + 1] = true;
            }
            if(strcmp(argv[i], "-v") == 0) v = true, f[i] = true;
            if(strcmp(argv[i], "-t") == 0){
                t = argv[i + 1];
                f[i] = f[i + 1] = true;
            }
        }
        for(int i = 1; i < argc; i++)
            if(!f[i]){ 
                printf("Wrong arguments!");
                return 0;
        }
        if(E == 0 || b == 0 || s == 0){
            printf("Wrong arguments!");
            return 0;
        } 

        init_cache(E, b, s);
        
        freopen(t, "r", stdin);
        work(E, b, s, v);

        printSummary(cache.hit, cache.miss, cache.evic);

        free_cache();
    }

    return 0;
}

bool check_num(char *s){
    int len = strlen(s);
    for(int i = 0; i < len; i++)
      if(s[i] < '0' || s[i] > '9') return false;
    return true;
}

void print_document(){
    // Print the help inforation of this program
    printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\n");
    printf("Examples:\n");
    printf("  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

void init_cache(int E, int b, int s){
    Set *headSet = malloc(sizeof(Set) * (1 << s));
    for(int i = 0; i < (1 << s); i++){
        headSet[i].len = E;
        Line *lines = malloc(sizeof(Line) * E);
        headSet[i].lines = lines;
    }
    cache.sets = headSet;
    cache.set_num = (1 << s);
}

void free_cache(){
    for(int i = 0; i < cache.set_num; i++){
        Set set = cache.sets[i];
        free(set.lines);
    }
    free(cache.sets);
}

void work(int E, int b, int s, bool v){
    char c[2]; unsigned int ad, l;
    while(scanf("%s %x,%d", c, &ad, &l) != EOF){  //ad是十六进制数！
        if(c[0] == 'I') continue;
        if(v) printf("%c %x,%u ", c[0], ad, l);
        switch (c[0]){
            case 'L': 
                load(ad, l, E, b, s, v);
                break;
            case 'S':
                save(ad, l, E, b, s, v);
                break;
            case 'M':
                load(ad, l, E, b, s, v);
                save(ad, l, E, b, s, v);
                break;
        default:
            continue;
        }
        if(v) printf("\n");
    }
}

void load(unsigned int ad, int l, int E, int b, int s, bool v){
    int index = (ad >> b) % (1 << s);
    int tag = (ad >> (b + s));
    Set* sets = cache.sets;

    findline(sets[index], tag, v);
}

void save(unsigned int ad, int l, int E, int b, int s, bool v){
    load(ad, l, E, b, s, v);
}

void findline(Set s, int tag, bool v){
    Line* lines = s.lines;
    for(int i = 0; i < s.len; i++)
      if(lines[i].valid && lines[i].tag == tag){
          if(v) printf("hit ");
          cache.hit++;
          lines[i].lru = cnt++;
          return ;
      }
    cache.miss++;
    if(v) printf("miss ");
    for(int i = 0; i < s.len; i++){
        if(!lines[i].valid){
            lines[i].tag = tag;
            lines[i].lru = cnt++;
            lines[i].valid = true;
            return ;
        }
    }
    if(v) printf("evction ");
    cache.evic++;
    int num = find_min_lru(s);
    lines[num].tag = tag;
    lines[num].lru = cnt++;
    return ;
}

int find_min_lru(Set s){
    int min_line_num; 
    min_line_num = 0;
    for(int i = 1; i < s.len; i++)
        if(s.lines[i].lru < s.lines[min_line_num].lru) min_line_num = i;
    return min_line_num;
}

void print_summary(int hit, int miss, int evic){
    printf("hits:%d misses:%d evictions:%d\n", hit, miss, evic);
}