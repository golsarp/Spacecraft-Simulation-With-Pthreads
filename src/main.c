#include "queue.c"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <stdbool.h>  

int simulationTime = 120;     // simulation time
int seed = 10;               // seed for randomness
int emergencyFrequency = 40; // frequency of emergency
float p = 0.2;               // probability of a ground job (launch & assembly)
int t = 2;
time_t start_time;
time_t curr_time;
int emergency = 0;
time_t current;
bool launch_b = false;
bool assembly_b = false;
int eventCount = 0;
int logStartTime = 10;



Queue *landing_queue;
Queue *launch_queue;
Queue *assembly_queue;
Queue *emergency_queue;
Queue *pad_A;
Queue *pad_B;
//mutexes
pthread_mutex_t event_count_mutex;

pthread_mutex_t launch_mutex;
pthread_mutex_t landing_mutex;
pthread_mutex_t assembly_mutex;
pthread_mutex_t emergency_mutex;

pthread_mutex_t pad_a_mutex;
pthread_mutex_t pad_b_mutex;

pthread_cond_t launch_cv;
pthread_cond_t landing_cv;
pthread_cond_t assembly_cv;
pthread_cond_t emergency_cv;

FILE * fp;

void* LandingJob(void *arg); 
void* LaunchJob(void *arg);
void* EmergencyJob(void *arg); 
void* AssemblyJob(void *arg); 
void* ControlTower(void *arg); 
void* ControlTower2(void *arg); 

void* handleEmergency(Job* arg, int simStartTime);

// pthread sleeper function
int pthread_sleep (int seconds)
{
    pthread_mutex_t mutex;
    pthread_cond_t conditionvar;
    struct timespec timetoexpire;
    if(pthread_mutex_init(&mutex,NULL))
    {
        return -1;
    }
    if(pthread_cond_init(&conditionvar,NULL))
    {
        return -1;
    }
    struct timeval tp;
    //When to expire is an absolute time, so get the current time and add it to our delay time
    gettimeofday(&tp, NULL);
    timetoexpire.tv_sec = tp.tv_sec + seconds; timetoexpire.tv_nsec = tp.tv_usec * 1000;
    
    pthread_mutex_lock (&mutex);
    int res =  pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
    pthread_mutex_unlock (&mutex);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&conditionvar);
    
    //Upon successful completion, a value of zero shall be returned
    return res;
}

int main(int argc,char **argv){
    // Get nth second to start printing to command line as command line argument
    // If logStartTime is given as command line argument it is used if not default value is 10
    if (argv[1] != NULL) {
        char *commandLineArgument = argv[1];
        logStartTime = atoi(commandLineArgument);
    }
    printf("logStartTime: %d\n", logStartTime);

    // -p (float) => sets p
    // -t (int) => simulation time in seconds
    // -s (int) => change the random seed
    for(int i=1; i<argc; i++){
        if(!strcmp(argv[i], "-p")) {p = atof(argv[++i]);}
        else if(!strcmp(argv[i], "-t")) {simulationTime = atoi(argv[++i]);}
        else if(!strcmp(argv[i], "-s"))  {seed = atoi(argv[++i]);}
    }
    printf("sim time %d\n",simulationTime);
    srand(seed); // feed the seed
   
    //queue initiliazation
    landing_queue = ConstructQueue(1000);
    launch_queue = ConstructQueue(1000);
    assembly_queue = ConstructQueue(1000);
    emergency_queue = ConstructQueue(1000);

    // launch or land 
    pad_A = ConstructQueue(1000);
    // assembly or land
    pad_B = ConstructQueue(1000);

    //mutex and condiiton variable initiliazation
    if (pthread_mutex_init(&event_count_mutex, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }
    if (pthread_mutex_init(&launch_mutex, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }
    if (pthread_mutex_init(&landing_mutex, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }
    if (pthread_mutex_init(&assembly_mutex, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }
    if (pthread_mutex_init(&pad_a_mutex, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }
    if (pthread_mutex_init(&pad_b_mutex, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }
    if (pthread_cond_init (&launch_cv, NULL)) {
        printf("\n cv init has failed\n");
    }
    if (pthread_cond_init (&landing_cv, NULL)) {
        printf("\n cv init has failed\n");
    }
    if (pthread_cond_init (&assembly_cv, NULL)) {
        printf("\n cv init has failed\n");
    }
    if (pthread_mutex_init(&emergency_mutex, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }
    if (pthread_cond_init (&emergency_cv, NULL)) {
        printf("\n cv init has failed\n");
    }


    
    // your code goes here

    // Open the log file
    fp = fopen("./log.txt", "w+");
    fputs("Log of the Simulation\n", fp);
    fputs("---------------------\n", fp);

    curr_time = time(NULL);
    start_time = time(NULL);
    double prob;

    pthread_t tower;
    //change ControlTower2 to ControlTower to se part 2 
    pthread_create(&tower, NULL, ControlTower2, NULL);
    //simulation loop for job creation
    while(curr_time < start_time + simulationTime){
        prob = (double)rand()/RAND_MAX;
        //launch and assembly job creation
        if(prob >= p/2){
            //printf("prob: %f\n",prob);
           // printf("create********** rocket\n");
            pthread_t rocket;
            pthread_create(&rocket, NULL, LaunchJob, &start_time);
           // printf("create********** assembly\n");
            pthread_t assembly;
            pthread_create(&assembly, NULL, AssemblyJob, &start_time);
        }
        //landing job creation
        if(prob >= 1-p){
           // printf("prob: %f\n",prob);
           // printf("land------------- rocket\n");
            pthread_t land;
            pthread_create(&land, NULL, LandingJob, &start_time);
        }
        //emergency job creation
        if(((curr_time - start_time) % 40*t) == 0 && curr_time != start_time ){
            //printf("Emergency Occured and 2 Jobs Created\n");
            //2 tane rocket parçası oldugu için 2 emergency job create ettim
            pthread_t emergency;
            pthread_create(&emergency, NULL, EmergencyJob, &start_time);
            pthread_t emergency2;
            pthread_create(&emergency2, NULL, EmergencyJob, &start_time);
        }
        // printf("Time : %ld\n",curr_time);
        pthread_sleep(t);
        curr_time = time(NULL);
    }
    // Close the file 
    fclose(fp);
    return 0;
}

// Rather than doing this we could simply assaing corresponding letters ascıı int to the type
// Ex: For Assembly we have 'A' so job type is 65
char mapJobTypeIntToChar(int type) {
    if (type == 1) {
        return 'L';
    }else if (type == 2) {
        return 'D';
    } else if (type == 3) {
        return 'A';
    } else if (type == 4) {
        return 'E';
    } else {
        // Indicating error
        return 'Z'; 
    }
}
//writing log files
void *writeLog(Job finishedJob, char padType) {
    fprintf(fp, "job id: %d\n", finishedJob.ID);
    fprintf(fp, "job type: %c\n", mapJobTypeIntToChar(finishedJob.type));
    fprintf(fp, "jobs request time: %d\n", finishedJob.enter_time);
    fprintf(fp, "jobs end time: %d\n", finishedJob.exit_time);
    fprintf(fp, "jobs turnaround time: %d\n", finishedJob.exit_time - finishedJob.enter_time);
    fprintf(fp, "jobs performed on pad %c\n", padType);
    fputs("\n", fp);
}

 void printQueue(Queue *pQueue) {
    
     //printf("size: %d\n",pQueue->size);
     int i;
     NODE *temp = pQueue->head;
     
     for ( i = 0; i < pQueue->size; i++) {
         
         printf(" %d", temp->data.ID);
         temp = temp->prev;
     }
     printf("\n");
     
     
 }

void* moveLandingOutOfQueue() {
    pthread_mutex_lock(&landing_mutex);
    Job landingJob = Dequeue(landing_queue);
    pthread_mutex_unlock(&landing_mutex);
}

void* moveLaunchOutOfQueue() {
    pthread_mutex_lock(&launch_mutex);
    Job launchJob = Dequeue(launch_queue);
    pthread_mutex_unlock(&launch_mutex);
}

void* moveAssemblyOutOfQueue() {
    pthread_mutex_lock(&assembly_mutex);
    Job assemblyJob = Dequeue(assembly_queue);
    pthread_mutex_unlock(&assembly_mutex);
}

void* moveEmergencyOutOfQueue() {
    pthread_mutex_lock(&emergency_mutex);
    Job emer =  Dequeue(emergency_queue);
    pthread_mutex_unlock(&emergency_mutex);
}




void* moveLandingToPadAAndPerform(Job* landing, int simStartTime) {
    //lock mutex to prevent race conditions
    pthread_mutex_lock(&pad_a_mutex);
    moveLandingOutOfQueue();
    Enqueue(pad_A, *landing);
    
    printf("Thread %ld started to land on Pad A \n", pthread_self());
    pthread_sleep(t);
    printf("Thread %ld landed on Pad A \n", pthread_self());
    
   
    Job landingJob = Dequeue(pad_A);
    int endTime = time(NULL);
    int endTimeInSim = endTime - simStartTime;
    landingJob.exit_time = endTimeInSim;
    writeLog(landingJob, 'A');
    pthread_mutex_unlock(&pad_a_mutex);
    //unlock mutex after job is completed
}

void* moveLandingToPadBAndPerform(Job* landing, int simStartTime) {
    pthread_mutex_lock(&pad_b_mutex);
    moveLandingOutOfQueue();
    //enqueue job after it gets the mutex
    Enqueue(pad_B, *landing);
    
    
    printf("Thread %ld started to land on Pad B \n", pthread_self());
    pthread_sleep(t);
    printf("Thread %ld landed on Pad B \n", pthread_self());
    
     //dequeue job after it is completed
    Job landingJob = Dequeue(pad_B);
    int endTime = time(NULL);
    int endTimeInSim = endTime - simStartTime;
    landingJob.exit_time = endTimeInSim;
    writeLog(landingJob, 'B');
    pthread_mutex_unlock(&pad_b_mutex);
}

void* moveLaunchToPadAndPerform(Job* launch, int simStartTime) {
    pthread_mutex_lock(&pad_a_mutex);
    moveLaunchOutOfQueue();
     //enqueue job after it gets the mutex
    Enqueue(pad_A, *launch);
    
    printf("Thread %ld started to launch from Pad A\n", pthread_self());
    //sleep to simulate job 
    pthread_sleep(2*t);
    printf("Thread %ld launched from Pad A\n", pthread_self());

   //dequeue job after it is completed
    Job launchJob = Dequeue(pad_A);
    int endTime = time(NULL);
    int endTimeInSim = endTime - simStartTime;
    launchJob.exit_time = endTimeInSim;
    writeLog(launchJob, 'A');
    pthread_mutex_unlock(&pad_a_mutex);
}

void* moveAssemblyToPadAndPerform(Job* assembly, int simStartTime) {
    pthread_mutex_lock(&pad_b_mutex);
    moveAssemblyOutOfQueue();
    Enqueue(pad_B, *assembly);
   
    printf("Thread %ld started to assemble from pad B\n", pthread_self());
    pthread_sleep(2*t);
    printf("Thread %ld assembled from pad B\n", pthread_self());
    
    
    Job assemblyJob = Dequeue(pad_B);
    int endTime = time(NULL);
    int endTimeInSim = endTime - simStartTime;
    assemblyJob.exit_time = endTimeInSim;
    writeLog(assemblyJob, 'B');
    // printf("pad B size = %d\n",pad_B->size);
    pthread_mutex_unlock(&pad_b_mutex);
}

// the function that creates plane threads for landing
// ID = 1
void* LandingJob(void *arg){
    // Could have made the simStartTime a global variable and just read from it...
    pthread_mutex_lock(&event_count_mutex);
    int eventId = ++eventCount;
    pthread_mutex_unlock(&event_count_mutex);
    int simStartTime = *(int*)arg;
    
    pthread_mutex_lock(&landing_mutex);
    int entryTime = time(NULL);
    int entryTimeInSim = entryTime - simStartTime;
    //printf("enqueue landing job\n");
    Job land;
    land.ID = eventId;
    land.type = 1;
    land.enter_time = entryTimeInSim;
    Enqueue(landing_queue, land);
    pthread_cond_wait(&landing_cv, &landing_mutex);
    pthread_mutex_unlock(&landing_mutex);

    //check which pad is empty
    if (pad_A->size == 0) {
        moveLandingToPadAAndPerform(&land, simStartTime);
    } else if (pad_B->size == 0) {
        moveLandingToPadBAndPerform(&land, simStartTime);
    }
    pthread_exit(NULL);
}

// the function that creates plane threads for departure
// ID = 2
void* LaunchJob(void *arg){
    pthread_mutex_lock(&event_count_mutex);
    int eventId = ++eventCount;
    pthread_mutex_unlock(&event_count_mutex);
    int simStartTime = *(int*)arg;
   // printf("sim start time by launch job: %d\n", simStartTime);
    //printf("current time: %ld\n", time(NULL));
    // added to launching jobs que 
    // when it has the chance to launch tower will dequeue the job
    // from the launching jobs queue
    pthread_mutex_lock(&launch_mutex);
    int entryTime = time(NULL);
    int entryTimeInSim = entryTime - simStartTime;
    //printf("enqueue launch job\n");
    Job launch;

    launch.ID = eventId;
    launch.type = 2;
    launch.enter_time = entryTimeInSim;
    Enqueue(launch_queue, launch);
    pthread_cond_wait(&launch_cv, &launch_mutex);
    pthread_mutex_unlock(&launch_mutex);
    moveLaunchToPadAndPerform(&launch, simStartTime);
    pthread_exit(NULL);
}

// the function that creates plane threads for emergency landing

void* EmergencyJob(void *arg){
    pthread_mutex_lock(&event_count_mutex);
    int eventId = ++eventCount;
    pthread_mutex_unlock(&event_count_mutex);
    int simStartTime = *(int*)arg;
    pthread_mutex_lock(&emergency_mutex);
    int entryTime = time(NULL);
    int entryTimeInSim = entryTime - simStartTime;
    //printf("We have an emergency !!! emergency job created\n");
    
    Job emer;
    emer.ID = eventId;
    emer.type = 4;
    emer.enter_time = entryTimeInSim;
    Enqueue(emergency_queue, emer);
    // wait for tower
    pthread_cond_wait(&emergency_cv, &emergency_mutex);
    pthread_mutex_unlock(&emergency_mutex);
    handleEmergency(&emer, simStartTime);
    pthread_exit(NULL);
}

void* handleEmergency(Job* emergency, int simStartTime) {
    //check for empty pad and do the job
    if(pad_A->size == 0){
        //do job here 
        pthread_mutex_lock(&pad_a_mutex);
        moveEmergencyOutOfQueue();
        Enqueue(pad_A, *emergency);
         
        printf("Performing Emergency on Pad A\n");
        
        pthread_sleep(t);
         printf("Emergency completed on  Pad A\n");
        Job emer_comp = Dequeue(pad_A);
        int endTime = time(NULL);
        int endTimeInSim = endTime - simStartTime;
        emer_comp.exit_time = endTimeInSim;
        writeLog(emer_comp, 'A');
        pthread_mutex_unlock(&pad_a_mutex);
        
    } else if(pad_B->size == 0){
        //do job here
        pthread_mutex_lock(&pad_b_mutex);
        moveEmergencyOutOfQueue();
        Enqueue(pad_B, *emergency);
        printf("Performing Emergency on Pad B\n");
        pthread_sleep(t);
        printf("Emergency completed on  Pad B\n");
        

        Job emer_comp = Dequeue(pad_B);
        int endTime = time(NULL);
        int endTimeInSim = endTime - simStartTime;
        emer_comp.exit_time = endTimeInSim;
        writeLog(emer_comp, 'B');
        pthread_mutex_unlock(&pad_b_mutex);
    }
}

// the function that creates plane threads for emergency landing
// ID = 3
void* AssemblyJob(void *arg){
    pthread_mutex_lock(&event_count_mutex);
    int eventId = ++eventCount;
    pthread_mutex_unlock(&event_count_mutex);
    int simStartTime = *(int*)arg;
    //printf("sim start time by assembly job: %d\n", simStartTime);
    //printf("current time: %ld\n", time(NULL));
    pthread_mutex_lock(&assembly_mutex);
    int entryTime = time(NULL);
    int entryTimeInSim = entryTime - simStartTime;
    //printf("enqueue assembly job\n");
    Job assembly;
    assembly.ID = eventId;
    assembly.type = 3;
    assembly.enter_time = entryTimeInSim;
    Enqueue(assembly_queue, assembly);
    pthread_cond_wait(&assembly_cv, &assembly_mutex);
    pthread_mutex_unlock(&assembly_mutex);
    moveAssemblyToPadAndPerform(&assembly, simStartTime);
    pthread_exit(NULL);
}


// the function that controls the air traffic
void* ControlTower(void *arg){
    // while loop here for simulation 
    current = time(NULL); 
    while(current < start_time + simulationTime){
        printf("Tower started to control\n");
        printf("Landing queue size: %d\n", landing_queue->size);
        printf("Assembly queue size: %d\n", assembly_queue->size);
        printf("Launch queue size: %d\n", launch_queue->size);
        printf("Emergency queue size: %d\n", emergency_queue->size);
        printf("Pad_A size %d\n", pad_A->size);
        printf("Pad_B size %d\n", pad_B->size);
       
        
        //check for emergency jobs 
        if(emergency_queue->size > 0 && (pad_A->size == 0 || pad_B->size == 0) ){
            if (pad_A->size == 0 && pad_B->size == 0 && emergency_queue->size == 2) {
                printf("Both emergency jobs goes into pads\n");
                pthread_cond_signal(&emergency_cv);
                pthread_cond_signal(&emergency_cv);
            } else {
                printf("One emergency job goes into a pad\n");
                pthread_cond_signal(&emergency_cv);
            }
        }
        //check for wait time
        if(!isEmpty(launch_queue)){
            Job launch_first = peek(launch_queue);
            launch_b = ((time(NULL) - launch_first.enter_time) > 10*t);
        }
         //check for pad queue and emergency size 
        if (pad_A->size == 0 && emergency_queue->size ==  0) {
            //give priority to launch job if wait time is long or size is greater than 3 
            if (landing_queue->size > 0 && (launch_queue->size < 3 || launch_b) ) {
                pthread_cond_signal(&landing_cv);
            } else if (launch_queue->size > 0) {
                pthread_cond_signal(&launch_cv);
            }
        } else {
            printf("Pad A is full cannot add a job\n");
        }
       //check for wait time
        if(!isEmpty(assembly_queue)){
            Job assembly_first = peek(launch_queue);
            assembly_b = ((time(NULL) - assembly_first.enter_time) > 10*t);
        }

        if (pad_B->size == 0 && emergency_queue->size ==  0) {
            //give priority to assembly job if wait time is long or size is greater than 3 
            if (landing_queue->size > 0 && (assembly_queue->size < 3 || assembly_b) ) {
                pthread_cond_signal(&landing_cv);
            } else if (assembly_queue->size > 0) {
                pthread_cond_signal(&assembly_cv);
            }
        } else {
            printf("Pad B is full cannot add a job\n");
        }
        
        //queue size printing 
        printf("Launching queue ID: ");
        printQueue(launch_queue);
        printf("Landing queue ID: ");
        printQueue(landing_queue);
        printf("Assembly queue ID: ");
        printQueue(assembly_queue);
        printf("Emergency queue ID: ");
        printQueue(emergency_queue);

        
        
        pthread_sleep(t);
        current = time(NULL);
        printf("Tower finished its control\n");
    }
}
//function to get wait time
int getWaitTime(int currentTime, Queue* jobQueue) {
    if(!isEmpty(jobQueue)){
        Job firstJob = peek(jobQueue);
        printf("job id: %d\n", firstJob.ID);
        return currentTime- firstJob.enter_time;
    }
    return -1;
}

// the function that controls the air traffic
void* ControlTower2(void *arg){
    // while loop here for simulation 
    current = time(NULL); 
    while(current < start_time + simulationTime) {
        printf("Tower started to control\n");
        printf("Landing queue size: %d\n", landing_queue->size);
        printf("Assembly queue size: %d\n", assembly_queue->size);
        printf("Launch queue size: %d\n", launch_queue->size);
        printf("Emergency queue size: %d\n", emergency_queue->size);
        printf("Pad_A size %d\n", pad_A->size);
        printf("Pad_B size %d\n", pad_B->size);
        //check for emergency jobs
        if (emergency_queue->size > 0 && (pad_A->size == 0 || pad_B->size == 0) ) {
            if (pad_A->size == 0 && pad_B->size == 0 && emergency_queue->size == 2) {
                printf("Both emergency jobs goes into pads\n");
                pthread_cond_signal(&emergency_cv);
                pthread_cond_signal(&emergency_cv);
            } else {
                printf("One emergency job goes into a pad\n");
                pthread_cond_signal(&emergency_cv);
            }
        }

        int landingWaitTime = getWaitTime(current, landing_queue);
        int launchWaitTime = getWaitTime(current, launch_queue);
        int assemblyWaitTime = getWaitTime(current, assembly_queue);

        printf("Landing wait time: %d\n", landingWaitTime);
        printf("Launch wait time: %d\n", launchWaitTime);
        printf("Assembly wait time: %d\n", assemblyWaitTime);

        // Skip the loop if emergency jobs exists in the queue
        if (emergency_queue->size == 0) {
            if (pad_A->size == 0) {
                if (landing_queue->size > 0 && launch_queue->size > 0) {
                    //check for waiting time
                    if (landingWaitTime >= launchWaitTime) {
                        pthread_cond_signal(&landing_cv);
                    } else {
                        pthread_cond_signal(&launch_cv);
                    }
                } else if (landing_queue->size > 0) {
                    pthread_cond_signal(&landing_cv);
                } else if (launch_queue->size > 0) {
                    pthread_cond_signal(&launch_cv);
                } else {
                    printf("Pad A is empty but there are no jobs to add\n");
                }
            } else {
                printf("Pad A is full cannot add a job\n");
            }

            if (pad_B->size == 0) {
                if (landing_queue->size > 0 && assembly_queue->size > 0) {
                    if (landingWaitTime >= launchWaitTime) {
                        pthread_cond_signal(&landing_cv);
                    } else {
                        pthread_cond_signal(&assembly_cv);
                    }
                } else if (landing_queue->size > 0) {
                    pthread_cond_signal(&landing_cv);
                } else if (assembly_queue->size > 0) {
                    pthread_cond_signal(&assembly_cv);
                } else {
                    printf("Pad B is empty but there are no jobs to add\n");
                }
            } else {
                printf("Pad B is full cannot add a job\n");
            }
        }
        printf("Launching queue ID: ");
        printQueue(launch_queue);
        printf("Landing queue ID: ");
        printQueue(landing_queue);
        printf("Assembly queue ID: ");
        printQueue(assembly_queue);
        printf("Emergency queue ID: ");
        printQueue(emergency_queue);
        
        pthread_sleep(t);
        current = time(NULL);
        printf("Tower finished its control\n");
    }
}
