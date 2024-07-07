#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <inttypes.h>
#include <time.h>

int main()
{   int config[5] = {0};
    //0 = size of cache
    //1 = block size
    //2 = associativity 
    //3= replacement policy (0 = fifo , 1= lru , 2 = random)
    //4 = write back policy(0=wb, 1 = wt)

    //Reading config file
    srand(time(NULL));
    int count = 0;
    FILE *file;
    char filename[100];
    strcpy(filename,"cache.config");
    file = fopen(filename,"r");
    if (file == NULL) 
    {   perror("Error opening file");
        return 1;
    }
    char* temp = (char*)malloc(20 * sizeof(char));
    while (count <= 4)
    {   fgets(temp, 20, file);
        temp[strcspn(temp, "\n")] = '\0';
        if(count == 3)
        {   if(!strcmp(temp,"FIFO"))       { config[count] = 0; }
            else if(!strcmp(temp,"LRU"))   { config[count] = 1; }
            else if(!strcmp(temp,"RANDOM")){ config[count] = 2; }
            else {config[count] = -1;}
        }
        else if(count == 4)
        {   if(!strcmp(temp,"WB"))         { config[count] = 0; }
            else if(!strcmp(temp,"WT"))    { config[count] = 1; }
            else if(!strcmp(temp,"RANDOM")){ config[count] = 2; }
            else {config[count] = -1;}
        }    
        else
        {   config[count] = atoi(temp); 
        }    
        count++;
    }


    int n_sets;
    int n_ways;
    if(config[2] != 0)
    {   n_sets = (config[0])/(config[1] * config[2]);
        n_ways = config[2];
    }
    else
    {   n_sets = 1;
        n_ways = (config[0])/(config[1]);
    }



    uint32_t cache[n_sets][n_ways][2];
    int occupied[n_sets];
    for (int i = 0; i < n_sets; i++) {
        occupied[i] = 0;
    }
    for(int i = 0;i< n_sets;i++)
    {   for(int j = 0;j<n_ways;j++)
        {   cache[i][j][0] = 0;
            cache[i][j][1] = 0;  //this is time, if time is zero that means that location is empty
        }
    }

    int hit = 0;
    int miss = 0;
    int r = 0;
    int w = 0;
    int setindex;
    int setindexbits = log(n_sets)/log(2);
    int offsetbits = log(config[1])/log(2);
    uint32_t tag;
    uint32_t address;
    strcpy(filename,"cache.access");
    file = fopen(filename,"r");
    if (file == NULL) 
    {   perror("Error opening file");
        return 1;
    }
    int time = 0;
     count = 0;
     char *add;

    while (fgets(temp, 20, file)) {
        count++;
        time = time +1;
        temp[strcspn(temp, "\n")] = '\0';
        if(temp[0] == 'R'){r = 1; w = 0;}
        else if(temp[0] == 'W'){r = 0; w = 1;}
        add = temp + 3;


        address = (uint32_t)strtoul(add, NULL, 16);
        tag = address >> (setindexbits+offsetbits);
        setindex =  (address >> offsetbits) & (uint32_t)(pow(2,setindexbits) -1);
        
        //search for tag
        int boola = 0;
        for(int i = 0;i<occupied[setindex];i++)
        {   if(cache[setindex][i][0] == tag)
            {   boola = 1;
                cache[setindex][i][1] = time;
                break;
            }
        }

        if(boola == 1)
        {   printf("Address: 0x%08x, Set: 0x%x, Hit, Tag: 0x%x\n",address,setindex,tag);
            hit++;
        }
        else
        {
            printf("Address: 0x%08x, Set: 0x%x, Miss, Tag: 0x%x\n",address,setindex,tag);

            miss++;

            if(w == 1 && config[4] == 1){}
            else
            {   if(occupied[setindex] < n_ways)
                {   cache[setindex][occupied[setindex]][0] = tag;
                    cache[setindex][occupied[setindex]][1] = time;
                    occupied[setindex] ++;
                }
                else if(config[3] == 0) //fifo
                {   for(int i = 0;i<n_ways-1;i++)
                    {cache[setindex][i][0] = cache[setindex][i+1][0];
                    }
                    cache[setindex][n_ways-1][0] = tag;
                }
                else if(config[3] == 1) //lru
                {   int minvalue = cache[setindex][0][1] ;
                    int minindex = 0;
                    for(int i = 0;i< n_ways;i++)
                    {   if(cache[setindex][i][1] < minvalue)
                        {   minvalue = cache[setindex][i][1];
                            minindex = i;
                        }
                    }
                    cache[setindex][minindex][0] = tag;
                    cache[setindex][minindex][1] = time;
                }
                else if(config[3] == 2)//random
                {   int index = rand() % (n_ways);
                    cache[setindex][index][0] = tag;
                    cache[setindex][index][1] = time;
                }

            }

        }
        }

    printf("%d hits\n%d misses\n",hit,miss);
}