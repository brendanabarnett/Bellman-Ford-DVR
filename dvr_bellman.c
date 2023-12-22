#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_NODES 10        //max nodes in graph
#define INF 1000000         //infinity
#define MAX_COST 10000      //max cost (weight)
#define S_USER_INPUT 0      //for status
#define S_CONSUMERS_DONE 1  //for status

enum {S_CONS_DONE, S_NEXT, S_EXIT};       //statuses
int num_nodes;                          //num nodes in graph
int distances[MAX_NODES][MAX_NODES];    //[from][to] - best distances
int adjMatrix[MAX_NODES][MAX_NODES];    //[from][to] - adjacency matrix to map nodes of graph

pthread_mutex_t mutex;      //control printing/editing
pthread_barrier_t barrier;  //maintains sync

//shared among all threads
typedef struct shared_data{
    int status;
    pthread_cond_t cond_userInput;      //comsumers wait for
    pthread_cond_t cond_consumersDone;  //master waits for
} thread_shared_t;

//each thread is passed its own
typedef struct thread_data{
    int id;                       //id of each thread
    int *shortest;                //pointer to neighbor info
    int *nbrs;                    //pointer to adjMtrx
    thread_shared_t* shared;
} thread_arg_t;

//inits adjmtrx and distances
void initialize(int nodes) {
    num_nodes = nodes;
    for (int i = 0; i < num_nodes; ++i) {
        for (int j = 0; j < num_nodes; ++j) {
            if (i == j) adjMatrix[i][j] = distances[i][j] = 0;
            else adjMatrix[i][j] = distances[i][j] = INF;
        }
    }
}

//adds edge to adjmtrx and updates dists if necessary
void addEdge(int from, int to, int weight){
    adjMatrix[from][to] = weight;       //also updates nbrs!
    if (weight < distances[from][to]){
        distances[from][to] = weight;
    }
}

//inits edge
void initEdge(int from, int to, int weight) {
    distances[from][to] = adjMatrix[from][to] = weight;
}

//prints vector table to console
void print_vec_tbl(int num_relax, int id, int *shortest){
    printf("Relaxation %d, lowest cost from Node %d:\n", num_relax, id);
    for (int i = 0; i < num_nodes; i++) {
        if (shortest[i] == INF) printf("Node %d to Node %d: Cost = INF\n", id, i);
        else printf("Node %d to Node %d: Cost = %d\n", id, i, shortest[i]);
    }
}

//Producer. UI- menu options: next ('n'), edit ('e'), quit ('q')
void* Menu_Interface(void* arg_in) {
    thread_arg_t *arg = arg_in;
    thread_shared_t *shared = arg->shared;
    shared->status = S_NEXT;

    while (1) {
        pthread_mutex_lock(&mutex);
        while (shared->status != S_CONS_DONE){    //wait for consumer threads to finish 1st relaxation
            pthread_cond_wait(&shared->cond_consumersDone, &mutex);
        }

        char input;
        int from, to, weight, flag;
        flag = 0;

        //menu
        while (!flag){
            printf("\nEnter 'N' for next relaxation, 'E' to edit edges, or 'Q' to quit: ");
            
            //scan for input & check for failure
            if (scanf(" %c", &input) != 1){
                fprintf(stderr, "Input failure <char>\n");
                pthread_exit(NULL);
            }

            //next relaxation
            if (input == 'N' || input == 'n'){
                break;
            }

            //edit edge
            else if (input == 'E' || input == 'e'){
                int flag2 = 0;
                while (!flag2){
                    printf("Enter new edge (from to weight): ");

                    //get edge change info, check if failed
                    if (scanf(" %d %d %d", &from, &to, &weight) != 3){
                        fprintf(stderr, "Input failure <from to weight>\n");
                        pthread_exit(NULL);
                    }

                    //check if valid edge
                    if (from < num_nodes && from >= 0 && to < num_nodes && to >= 0 && weight > 0 && to != from && weight < MAX_COST){
                        addEdge(from, to, weight);
                        printf("New edge from Node %d to Node %d with Cost %d\n", from, to, weight);    //confirmation msg
                        flag2 = 1;
                        break;
                    }

                    else{
                        printf("Oops! Invalid value(s)\n");     //if wt/node out of bounds
                    }
                }
                continue;
            }

            //quit
            else if (input == 'Q' || input == 'q'){
                printf("Goodbye!\n");   //goodbye message
                pthread_mutex_unlock(&mutex);
                shared->status = S_EXIT;
                pthread_cond_broadcast(&shared->cond_userInput); //signal end (status = 2)
                pthread_exit(NULL);         //exit thread
            }

            else{
                //invalid input: try again
                printf("Oops! Invalid input\n");
                continue;
            }
        }
        
        //signaling consumers
        pthread_mutex_unlock(&mutex);
        shared->status = S_NEXT;     //next relaxation status = 1
        pthread_cond_broadcast(&shared->cond_userInput);    //broadcast condition to all consumers
    }
    return NULL;
}

void* BellmanFord(void* arg_in){
    thread_arg_t *arg = arg_in;
    thread_shared_t *shared = arg->shared;
    int id = arg->id;
    int *shortest = arg->shortest;      //distance vector
    int *nbrs = arg->nbrs;              //adjmtrx
    int num_relax = 0; 

    //first relaxation
    pthread_mutex_lock(&mutex); //lock to prevent overlap in printing
    print_vec_tbl(++num_relax, id, shortest);
    pthread_mutex_unlock(&mutex);

    pthread_barrier_wait(&barrier); //all here
    if (id == 0){
        shared->status = S_CONS_DONE;
        pthread_cond_broadcast(&shared->cond_consumersDone);     //only 1 need to signal done
    }
    pthread_barrier_wait(&barrier); //all here

    //init thread-local info for bellman ford
    int best[num_nodes];
    for (int i = 0; i< num_nodes; i++){
        best[i] = shortest[i];      //copy of shortest
    }

    int isDone = 0;
    while (!isDone){
        pthread_mutex_lock(&mutex);
        while (shared->status == S_CONS_DONE){      //waits for producer
            pthread_cond_wait(&shared->cond_userInput, &mutex);
        }
        pthread_mutex_unlock(&mutex);

        if (shared->status == S_EXIT){      //check for exit status
            
            isDone = 1;
            pthread_exit(NULL);     //exit thread
        }

        //bellman-ford alg
        int new_dist;
        for (int i = 0; i < num_nodes; i++){        //i = dest
            for (int j = 0; j < num_nodes; j++){    //j = nbr (1st step)
                if (nbrs[j] != INF){                //check if nbr
                    new_dist = distances[j][i] + nbrs[j];             //step plus total cost
                    if (new_dist < best[i] && new_dist < (INF - MAX_COST)){     //negative weight logic consistancy: INF - x = INF
                        best[i] = new_dist;     //if smaller and not INF, update
                    }
                }
            }
        }
        
        pthread_barrier_wait(&barrier); //all here
        for (int i = 0; i < num_nodes; i++){        //upate dists w new bests
            distances[id][i] = best[i];
        }

        pthread_mutex_lock(&mutex);     //lock to print
        print_vec_tbl(++num_relax, id, shortest);
        pthread_mutex_unlock(&mutex);

        pthread_barrier_wait(&barrier); //all here
        if (id == 0){
            shared->status = S_CONS_DONE;
            pthread_cond_broadcast(&shared->cond_consumersDone);       //broadcast done
        }
        pthread_barrier_wait(&barrier); //all here
    }
    return NULL;
}


int main() {
    int total_nodes = 6;        // Change this according to network size (must be < MAX_NODES)
    initialize(total_nodes);        //init mtrxes
  
    //init edges
    initEdge(0, 1, 5);      //from,to,wt
    initEdge(1, 2, 1);
    initEdge(1, 3, 2);
    initEdge(2, 4, 1);
    initEdge(3, 5, 2);
    initEdge(4, 3, -1);
    initEdge(5, 4, -3);

    thread_shared_t shared;     //init shared struct

    pthread_mutex_init(&mutex, NULL);
    pthread_barrier_init(&barrier, NULL, total_nodes);
    pthread_cond_init(&shared.cond_userInput, NULL);
    pthread_cond_init(&shared.cond_consumersDone, NULL);

    pthread_t threads[total_nodes + 1];     // +1 for the menu (master) thread
    thread_arg_t thread_args[total_nodes+1];
    
    int rc;
    //init master thread
    thread_args[0].id = total_nodes;
    thread_args[0].shared = &shared;
    rc = pthread_create(&threads[0], NULL, Menu_Interface, &thread_args[0]);

    //check for failure
    if (rc) {
            fprintf(stderr, "ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
    }
    
    //init thread for each node
    for (int i = 0; i < total_nodes; i++) {
        thread_args[i+1].id = i;
        thread_args[i+1].shared = &shared;          //points to shared struct
        thread_args[i+1].shortest = distances[i];   //points to col of min distances
        thread_args[i+1].nbrs = adjMatrix[i];       //points to col of adjmtrx
        
        rc = pthread_create(&threads[i+1], NULL, BellmanFord, &thread_args[i+1]);
        
        //check for failure
        if (rc) {
            fprintf(stderr, "ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }
    
    //join threads
    for (int i = 0; i < total_nodes + 1; ++i) {
        rc = pthread_join(threads[i], NULL);

        //check for failure
        if (rc) {
            fprintf(stderr, "ERROR; return code from pthread_join() is %d\n", rc);
            exit(-1);
        }
    }

    //clean up
    pthread_mutex_destroy(&mutex);
    pthread_barrier_destroy(&barrier);
    pthread_cond_destroy(&shared.cond_userInput);
    pthread_cond_destroy(&shared.cond_consumersDone);
    
    return 0;
}
