#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "bloomfiltertree.h"
#include "bloomfilter.h"
#include "dataitem.h"
#include "helper.h"
#include "key128bitGenerator.h"



int countTheNumberOfNodesAboveTheNode(unsigned int Numberofnodes){
    unsigned int sum = 0;
    unsigned int n = Numberofnodes;
    while( n != 2){
        if(n<2)
            break;
        sum += ((n+1)/2);
        n = (n+1)/2;
    }
    sum += 1;
    return sum;
}

short is_leaf(DOUBLENODEBLOOMFILTERTREE *bft,unsigned int i){
    unsigned int tmp = countTheNumberOfNodesAboveTheNode(bft->leavesSize);
    if(i > tmp)
        return 1;
    else
        return 0;
}

DOUBLENODEBLOOMFILTERTREE *init_fixed_bf_tree(unsigned int nodes, unsigned long bf_size_in_bytes){
    #ifdef LOGGING
        printf("---------------------------fixed tree initialization starts-------------------\n");
    #endif 
    DOUBLENODEBLOOMFILTERTREE *ret = malloc(sizeof(DOUBLENODEBLOOMFILTERTREE));


    ret->leavesSize = nodes;
    unsigned int tmp = countTheNumberOfNodesAboveTheNode(nodes);

    ret->size = nodes + tmp;
    ret->built = false;


    if((ret->data = malloc((ret->size + 1) * sizeof(DOUBLENODEBLOOMFILTER *))) == NULL)
        printf("Data allocation failed\n");
    


    //add leaf BFS in correct postions   0  :  nodes-1
    //add leaf BFS in correct postions   1  :  nodes

    for(unsigned int i = 1; i < nodes + 1; i++){
        ret->data[i+tmp] = init_empty_BF(bf_size_in_bytes);
        printf("bft->data[%d] is initiallized\n",i+tmp);

    }


    ret->type = FIX;

    #ifdef LOGGING
        printf("---------------------------fixed tree initialization end-----------------------\n");
    #endif 

    return ret;                    
}



void build_bf_tree(DOUBLENODEBLOOMFILTERTREE *bft){
    if(bft->type == FIX && !bft->built){
        unsigned int Currentnumberofnodes = bft->leavesSize;
        unsigned int Abovenumberofnodes = countTheNumberOfNodesAboveTheNode(Currentnumberofnodes);
        unsigned int totalnodes = Currentnumberofnodes + Abovenumberofnodes;
        unsigned int level = (unsigned int)log2(totalnodes);
        
        
        for(int i = level; i > 0; i--){
            #ifdef LOGGING
                printf("---------------------level: %d----------------------------\n",i);
            #endif 

            int k = 0;
            char state = 0;
            for(unsigned int j = totalnodes; j > Abovenumberofnodes; j -= 2){

                if(j != totalnodes){

                    if(state == 0){
                        bft->data[Abovenumberofnodes-k] = bf_union(bft->data[j-1], bft->data[j]);
                        bft->data[Abovenumberofnodes-k]->leftparentnode = j-1;
                        bft->data[Abovenumberofnodes-k]->rightparentnode = j;
                        #ifdef LOGGING
                            printf("bft->data[%d] is generated by leftparentnode:bft->data[%d] and rightparentnode bft->data[%d] \n",Abovenumberofnodes-k,j-1,j);
                        #endif 
                        k++;
                    }

                    else{
                        bft->data[Abovenumberofnodes-k] = bf_union(bft->data[j + 1], bft->data[j]);
                        bft->data[Abovenumberofnodes-k]->leftparentnode = j;
                        bft->data[Abovenumberofnodes-k]->rightparentnode = j + 1;
                        #ifdef LOGGING
                            printf("bft->data[%d] is generated by leftparentnode:bft->data[%d] and rightparentnode bft->data[%d] \n",Abovenumberofnodes-k,j,j+1);
                        #endif 
                        k++;
                    }
                }
                else{
                    if(((j-Abovenumberofnodes)%2) == 1){
                        bft->data[Abovenumberofnodes] = copy(bft->data[j]);
                        bft->data[Abovenumberofnodes]->leftparentnode = j;
                        #ifdef LOGGING
                            printf("bft->data[%d] is copyed from mother node : bft->data[%d] \n",Abovenumberofnodes-k,j);
                        #endif 
                        state = 1;
                        k++;
                    }
                    else{
                        bft->data[Abovenumberofnodes] = bf_union(bft->data[j - 1], bft->data[j]);
                        bft->data[Abovenumberofnodes]->leftparentnode = j-1;
                        bft->data[Abovenumberofnodes]->rightparentnode = j;
                        #ifdef LOGGING
                            printf("bft->data[%d] is generated by leftparentnode:bft->data[%d] and rightparentnode bft->data[%d] \n",Abovenumberofnodes-k,j-1,j);
                        #endif 
                        k++;
                        state = 0;
                    }
                }             
            }
            
            Currentnumberofnodes = (Currentnumberofnodes+1)/2;

            Abovenumberofnodes = countTheNumberOfNodesAboveTheNode(Currentnumberofnodes);

            totalnodes = Currentnumberofnodes + Abovenumberofnodes;

        }   

        bft->built = true;
    }
}


void destroy_bftree(DOUBLENODEBLOOMFILTERTREE *bft){
    unsigned int i = bft->type || bft->built ? 1 : countTheNumberOfNodesAboveTheNode(bft->leavesSize);
    while(i < (bft->size + 1))
        destroy_bf(bft->data[i++]);
    free(bft->data);
    free(bft);

}
void findv2(DOUBLENODEBLOOMFILTERTREE *bft, BYTE trapdoor[][SHA256_BLOCK_SIZE], int i){
    #ifdef LOGGING
        // printf("Visit %d\n",i);
    #endif

        if(is_trapdoor_in_bloomV2(bft->data[i],trapdoor)){
            // printf("trapdoor in contained in nodes : bft->data[%d]\n",i);
            if(is_leaf(bft,i)){
                printf("@@@@@@@@@@@@@@@@trapdoor : trapdoor is found in@@@@@@@@@@@ #####Leaf nodes : %d \n",i-countTheNumberOfNodesAboveTheNode(bft->leavesSize));
            }
            else{
                if(bft->data[i]->leftparentnode != 0)
                    findv2(bft,trapdoor,bft->data[i]->leftparentnode);
                if(bft->data[i]->rightparentnode != 0)
                    findv2(bft,trapdoor,bft->data[i]->rightparentnode);
            }
        }
}

void findv3(DOUBLENODEBLOOMFILTERTREE *bft, BYTE multitrapdoor[5][NUMBEROFKEYS][SHA256_BLOCK_SIZE], int i){
    #ifdef LOGGING
        // printf("Visit %d\n",i);
    #endif

        if(is_trapdoor_in_bloomV2(bft->data[i],multitrapdoor[0]) && is_trapdoor_in_bloomV2(bft->data[i],multitrapdoor[1])&&is_trapdoor_in_bloomV2(bft->data[i],multitrapdoor[2])\
        &&is_trapdoor_in_bloomV2(bft->data[i],multitrapdoor[3])&&is_trapdoor_in_bloomV2(bft->data[i],multitrapdoor[4]) ){
            // printf("multitrapdoor is contained in nodes : bft->data[%d]\n",i);
            if(is_leaf(bft,i)){
                printf("@@@@@@@@@@@@@@@@trapdoor : multitrapdoor is found in@@@@@@@@@@@ #####Leaf nodes : %d \n",i-countTheNumberOfNodesAboveTheNode(bft->leavesSize));
            }
            else{
                if(bft->data[i]->leftparentnode != 0)
                    findv3(bft,multitrapdoor,bft->data[i]->leftparentnode);
                if(bft->data[i]->rightparentnode != 0)
                    findv3(bft,multitrapdoor,bft->data[i]->rightparentnode);
            }
        }
}
void findv3_disjunctive(DOUBLENODEBLOOMFILTERTREE *bft, BYTE multitrapdoor[5][NUMBEROFKEYS][SHA256_BLOCK_SIZE], int i){
    #ifdef LOGGING
        // printf("Visit %d\n",i);
    #endif
    for(int i = 0; i < 5;i++)
        findv2(bft,multitrapdoor[i],1);
        // if(is_trapdoor_in_bloomV2(bft->data[i],multitrapdoor[0]) && is_trapdoor_in_bloomV2(bft->data[i],multitrapdoor[1])&&is_trapdoor_in_bloomV2(bft->data[i],multitrapdoor[2])\
        // &&is_trapdoor_in_bloomV2(bft->data[i],multitrapdoor[3])&&is_trapdoor_in_bloomV2(bft->data[i],multitrapdoor[4]) ){
        //     // printf("multitrapdoor is contained in nodes : bft->data[%d]\n",i);
        //     if(is_leaf(bft,i)){
        //         printf("@@@@@@@@@@@@@@@@trapdoor : multitrapdoor is found in@@@@@@@@@@@ #####Leaf nodes : %d \n",i-countTheNumberOfNodesAboveTheseNodes(bft->leavesSize));
        //     }
        //     else{
        //         if(bft->data[i]->leftparentnode != 0)
        //             findv3(bft,multitrapdoor,bft->data[i]->leftparentnode);
        //         if(bft->data[i]->rightparentnode != 0)
        //             findv3(bft,multitrapdoor,bft->data[i]->rightparentnode);
        //     }
        // }
}

void findv4(DOUBLENODEBLOOMFILTERTREE *bft, BYTE multitrapdoor[10][NUMBEROFKEYS][SHA256_BLOCK_SIZE], int i){
    #ifdef LOGGING
        // printf("Visit %d\n",i);
    #endif

        if(is_trapdoor_in_bloomV2(bft->data[i],multitrapdoor[0]) && is_trapdoor_in_bloomV2(bft->data[i],multitrapdoor[1])&&is_trapdoor_in_bloomV2(bft->data[i],multitrapdoor[2])\
        &&is_trapdoor_in_bloomV2(bft->data[i],multitrapdoor[3])&&is_trapdoor_in_bloomV2(bft->data[i],multitrapdoor[4])&&is_trapdoor_in_bloomV2(bft->data[i],multitrapdoor[5])\
        &&is_trapdoor_in_bloomV2(bft->data[i],multitrapdoor[6])&&is_trapdoor_in_bloomV2(bft->data[i],multitrapdoor[7])&&is_trapdoor_in_bloomV2(bft->data[i],multitrapdoor[8])\
        &&is_trapdoor_in_bloomV2(bft->data[i],multitrapdoor[9])  ){
            // printf("multitrapdoor is contained in nodes : bft->data[%d]\n",i);
            if(is_leaf(bft,i)){
                printf("@@@@@@@@@@@@@@@@trapdoor : multitrapdoor is found in@@@@@@@@@@@ #####Leaf nodes : %d \n",i-countTheNumberOfNodesAboveTheNode(bft->leavesSize));
            }
            else{
                if(bft->data[i]->leftparentnode != 0)
                    findv4(bft,multitrapdoor,bft->data[i]->leftparentnode);
                if(bft->data[i]->rightparentnode != 0)
                    findv4(bft,multitrapdoor,bft->data[i]->rightparentnode);
            }
        }
}

void findv4_disjunctive(DOUBLENODEBLOOMFILTERTREE *bft, BYTE multitrapdoor[10][NUMBEROFKEYS][SHA256_BLOCK_SIZE], int i){
    for(int i = 0; i < 10; i++)
        findv2(bft,multitrapdoor[i],1);
}
DOUBLENODEBLOOMFILTER *copy(DOUBLENODEBLOOMFILTER *bf1){

    DOUBLENODEBLOOMFILTER *ret = init_empty_BF(bf1->size);
    for(unsigned int i = 0; i < ret->size * 8; i++)
        if(ChosenCellLocationIsChosen(bf1,i/8,i))
            setChosenCellTo1(ret,i/8,i);

    return ret;
}

DOUBLENODEBLOOMFILTER *bf_union(DOUBLENODEBLOOMFILTER *bf1,DOUBLENODEBLOOMFILTER *bf2){
    if(bf1->size != bf2->size){
        fprintf(stderr, "Cannot perform union on bloom filters of different sizes.");
        exit(1);
    }
    DOUBLENODEBLOOMFILTER *ret = init_empty_BF(bf1->size);

    for(unsigned int i = 0; i < ret->size * 8; i++)
        if(ChosenCellLocationIsChosen(bf1,i/8,i) || ChosenCellLocationIsChosen(bf2,i/8,i))      
            setChosenCellTo1(ret,i/8,i);

    return ret;
}







void saveDoubleNodeBloomfilterTree(char *filename, DOUBLENODEBLOOMFILTERTREE *bft){
    FILE *f = fopen(filename, "w");
    if(f == NULL){
        fprintf(stderr,"error in initializing a file handle");
        exit(-1);
    }
    fprintf(f,"%c%lu:%u:",bft->type == VARIABLE ? 'v' : 'f',bft->data[1]->size,bft->leavesSize);
    for(int i = 1; i < bft->size + 1; i++){
        fprintf(f,"%u:",bft->data[i]->rb);
        fprintf(f,"%hu:",bft->data[i]->leftparentnode);
        fprintf(f,"%hu:",bft->data[i]->rightparentnode);
        for(int j = 0; j < bft->data[i]->size; j++){
            fprintf(f,"%hx",bft->data[i]->array[j]);
        }
    #ifdef LOGGING
        printf("bft->data[%d] is saved in file : %s\n",i,filename);
    #endif 
    }
    
    fclose(f);
}

DOUBLENODEBLOOMFILTERTREE *init_fixed_bf_treev2(unsigned int nodes, unsigned long bf_size_in_bytes){
    #ifdef LOGGING
        printf("---------------------------fixed tree initialization starts-------------------\n");
    #endif 
    DOUBLENODEBLOOMFILTERTREE *ret = malloc(sizeof(DOUBLENODEBLOOMFILTERTREE));


    ret->leavesSize = nodes;
    unsigned int tmp = countTheNumberOfNodesAboveTheNode(nodes);

    ret->size = nodes + tmp;
    ret->built = false;


    if((ret->data = malloc((ret->size + 1) * sizeof(DOUBLENODEBLOOMFILTER *))) == NULL)
        printf("Data allocation failed\n");
    


    //add leaf BFS in correct postions   0  :  nodes-1
    //add leaf BFS in correct postions   1  :  nodes

    for(unsigned int i = 1; i < ret->size + 1; i++){
        ret->data[i] = init_empty_BFv2(bf_size_in_bytes);
        printf("bft->data[%d] is initiallized\n",i+tmp);

    }


    ret->type = FIX;

    #ifdef LOGGING
        printf("---------------------------fixed tree initialization end-----------------------\n");
    #endif 

    return ret;                    
}

DOUBLENODEBLOOMFILTERTREE *load_bf_tree(char *filename){
    FILE *f = fopen(filename,"r");
    if (f == NULL) {
        fprintf(stderr, "File not found: %s\n", filename);
        exit(1);
    }

    char type = fgetc(f);
    int c;
    unsigned long bfdata1size;
    unsigned int bftleavessize;
    fscanf(f,"%lu:%u:",&bfdata1size,&bftleavessize);

    DOUBLENODEBLOOMFILTERTREE *bft;

    if (type == 'v')
        bft = init_fixed_bf_treev2(bftleavessize, bfdata1size);
    else {
        bft = init_fixed_bf_treev2(bftleavessize, bfdata1size);
    }

    for(int i = 1; i < bft->size + 1; i++){
        fscanf(f,"%u:",&(bft->data[i]->rb));
        fscanf(f,"%u:",&(bft->data[i]->leftparentnode));
        fscanf(f,"%u:",&(bft->data[i]->rightparentnode));
        for(int j = 0; j < bft->data[i]->size; j++){
            fscanf(f,"%4hx",&(bft->data[i]->array[j]));
        }
    #ifdef LOGGING
        printf("bft->data [%d] is loaded----------------------------\n",i);
    #endif 
    }
    fclose(f);
return bft;
}




// int main(){
//     clock_t start,finish;
//     start = clock();
//     printf("start build tree\n");
//     produceNPlusOnekeys(NPlusOnekey);
//     #ifdef LOGGING
//         for(int i = 0;i < NUMBEROFKEYS + 1;i++){

//                 printf("---------------------------keys[%d] : ",i);

//             for(int j = 0; j < strlen(NPlusOnekey[0]); j++){
//                 printf("%c",NPlusOnekey[i][j]);
//             }
//             printf("\n");
//         }
//     #endif 
//     save_NPlusOneKeys("NPlusOnekey.txt");


//     DOUBLENODEBLOOMFILTERTREE *bft = init_fixed_bf_tree(250,50000);
//     unsigned int tmp = countTheNumberOfNodesAboveTheNode(bft->leavesSize);

//     for(int i = 1; i < bft->leavesSize + 1; i++){
//         dataitem *di = init_dataitem(i);
//         add_a_dataitem_to_bloomfilter(di,bft->data[i+tmp]);
//         saveoneDataItems("dataitem.txt",di);
//         destroy_dataitem(di);
//     }
//         build_bf_tree(bft);
//         saveDoubleNodeBloomfilterTree("bft_tree.txt",bft);

//     trapdoor *keyword_trapdoor = get_a_trapdoor_of_a_keyword("wlrb",bft->data[1]->size);
//     findv2(bft,keyword_trapdoor,1);

//     destroy_bftree(bft);
//     destroy_tpd(keyword_trapdoor);
//     finish = clock();
//     double duration;
//     duration = (double)(finish-start)/CLOCKS_PER_SEC;
//     printf("the total build tree time is : %f seconds.\n",duration);

//     return 0;
// }


// int main(){
//     clock_t start,start2,finish;
//     printf("start\n");
//     load_NPlusOneKeys("NPlusOnekey.txt",NPlusOnekey);
//     #ifdef LOGGING
//         for(int i = 0;i < NUMBEROFKEYS + 1;i++){

//                 printf("---------------------------keys[%d] : ",i);

//             for(int j = 0; j < strlen(NPlusOnekey[0]); j++){
//                 printf("%c",NPlusOnekey[i][j]);
//             }
//             printf("\n");
//         }
//     #endif 
//     start = clock();
//     DOUBLENODEBLOOMFILTERTREE *bft = load_bf_tree("bft_tree.txt");
//     trapdoor *keyword_trapdoor = get_a_trapdoor_of_a_keyword("jwnnvbw",bft->data[1]->size);
//     start2 = clock();
//     findv2(bft,keyword_trapdoor,1);
//     destroy_bftree(bft);
//     destroy_tpd(keyword_trapdoor);
//     finish = clock();
//     double total,duration;
//     total = (double)(finish-start2)/CLOCKS_PER_SEC;
//     duration = (double)(finish-start)/CLOCKS_PER_SEC;
//     // FILE *f = fopen("record.txt","w");
//     printf("the total search time is : %f seconds.\n",duration);
//     printf("from build to search time is : %f seconds.\n",total);
//     // fprintf(f,"the total search time is : %f seconds.\n",duration);
//     // fprintf(f,"from build to search time is : %f seconds.\n",total);
//     // fclose(f);
//     return 0;
// }