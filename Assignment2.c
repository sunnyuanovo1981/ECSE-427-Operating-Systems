#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

static sem_t mutex;             //Semaphore for readCount
static sem_t mutex_rw;          //Semaphore for reader/writer access
static int readCount = 0;       //Number of readers accessing target
static int target = 0;          //Target value to be read ad written
static int currentReader = 0;   //Index for current reader, used for timing
static int currentWriter = 0;   //Index for current wirter, used for timing

static float readerVal[100], writerVal[10]; //Holds reader and writer waiting times

float findMax(float array[], size_t size);  //defines required functions
float findMin(float array[], size_t size);
float findAvg(float array[], size_t size);

static void *reader(void * args)            //Reader function
{
  int loops = *((int *) args);              //args used to determine number of times to loop
  struct timeval tv;                        //Used to access current time
  time_t dTime, timeIn, timeOut;            //used to calculate timing

  dTime = 0;      //Initialize timing values
  timeIn = 0;
  timeOut = 0;

  int i;

  for(i = 0; i < loops; i++)
  {
    int r = rand();                           //create random variable
    gettimeofday(&tv, NULL);                  //get current time for analysis
    timeIn = tv.tv_sec*1000000 + tv.tv_usec;
    if(sem_wait(&mutex) == -1)                //Check to make sure no other reader
      exit(2);                                //is accessing the readCount var
    gettimeofday(&tv, NULL);
    timeOut = tv.tv_sec*1000000 + tv.tv_usec; //get time after waiting
    dTime = dTime + (timeOut - timeIn);       //updates waiting time
    timeOut = 0;                              //resets timing values
    timeIn = 0;
    readCount++;                              //update number of readers
    if(readCount == 1)                        //If this reader is the first reader
    {                                         //to enter its critical section
      gettimeofday(&tv, NULL);                //it must check to see if a writer
      timeIn = tv.tv_sec*1000000 + tv.tv_usec;//has already accessed the target value
      if(sem_wait(&mutex_rw)==-1)             //and wait. waiting time is also updated
      {
        exit(2);
      }
      gettimeofday(&tv, NULL);
      timeOut = tv.tv_sec*1000000 + tv.tv_usec;

      dTime = dTime + (timeOut - timeIn);
      timeOut = 0;
      timeIn = 0;
    }
    if(sem_post(&mutex) == -1)                //once readCount is updated, signal semaphore
      exit(2);
    //preform read of target value
    printf("Current target value %d\n There are %d readers currently\n", target, readCount);

    gettimeofday(&tv, NULL);                  //update timing and check that access to
    timeIn = tv.tv_sec*1000000 + tv.tv_usec;  //mutex to decrement readCount is allowed
    if(sem_wait(&mutex) == -1)
      exit(2);
    gettimeofday(&tv, NULL);
    timeOut = tv.tv_sec*1000000 + tv.tv_usec;
    dTime = dTime + (timeOut-timeIn);
    timeOut = 0;
    timeIn = 0;

    readCount--;                              //decrement readCount to original value
    if(readCount == 0)                        //If reader is last to exit, check to make sure no
    {                                         //signal to allow writer to access target
      if(sem_post(&mutex_rw) == -1)
      {
        exit(2);
      }

    }
    if(sem_post(&mutex) == -1)                //signal that the reader has finished
      exit(2);                                //editing the readCount

    usleep((float)(r%100));                   //sleep for random time between 0 - .1ms
  }
  readerVal[currentReader] = dTime;           //store timing information and update
  currentReader++;                            //current reader
}

static void *writer(void * args)              //writer function
{
  int loops = *((int *) args);                //args used to determine number of times to loop
  int temp;                                   //Used to temp store target
  struct timeval tv;                          //used to calculate timing, hold timing
  time_t dTime, timeIn, timeOut;

  dTime = 0;              //initiate timing values
  timeIn = 0;
  timeOut = 0;

  int r = rand();         //create random number

  int i;
  for (i = 0; i < loops; i++)
  {
    gettimeofday(&tv, NULL);                  //start timing anaylsis
    timeIn = tv.tv_sec*1000000 + tv.tv_usec;
    if (sem_wait(&mutex_rw) == -1)            //check access to critical section
      exit(2);
    gettimeofday(&tv, NULL);                  //capture wating time
    timeOut = tv.tv_sec*1000000 + tv.tv_usec;
    dTime = dTime + (timeOut - timeIn);       //update waiting time
    timeOut = 0;                              //reset timing values
    timeIn = 0;

    printf("writing to target\n");            //write to target value
    temp = target;
    temp = temp+10;
    target = temp;
    if(sem_post(&mutex_rw) == -1)             //signal that write is complete
      exit(2);
    usleep((float)(r%100));                   //sleep for random time between 0 and .1 ms
  }
  writerVal[currentWriter] = dTime;           //store timing information
  currentWriter++;                            //update current writer
}

int main(int argc, char *argv[])
{
  pthread_t readers[100],writers[10];         //create arrays of threads
  int s;
  int loops = 100;                            //args for writer and readers

  float  readMax, readMin, readAvg, writeMax, writeMin, writeAvg; //hold timing info

  srand(time(NULL));            //seed random number generator

  if(sem_init(&mutex,0,1) == -1)          //initiate semaphores
  {
    printf("Error initiating semaphore exiting...\n");
    exit(1);
  }

  if(sem_init(&mutex_rw,0,1) == -1)
  {
    printf("Error initiating semaphore exiting...\n");
    exit(1);
  }

  int i;
  for (i = 0; i < 10; i++)                  //create writer threads
  {
    printf("creating writer thread\n");
    s = pthread_create(&writers[i], NULL, &writer, &loops);
    if(s !=0)
    {
      printf("Error creating writer exiting...\n");
      exit(1);
    }

  }

  int j;
  for (j = 0; j < 100; j++)                 //create reader threads
  {
    printf("creating reader thread\n");
    s = pthread_create(&readers[j], NULL, &reader, &loops);
    if(s !=0)
    {
      printf("Error creating reader exiting...\n");
      exit(1);
    }

  }

  int k;
  for (k = 0; k < 10; k++)                //detach writer threads
  {
    s = pthread_join(writers[k], NULL);
    if (s != 0) {
      printf("Error, creating threads\n");
      exit(1);
    }

  }

  int p;
  for (p = 0; p < 100; p++)               //detach reader threads
  {
    s = pthread_join(readers[p], NULL);
    if (s != 0) {
      printf("Error, creating threads\n");
      exit(1);
    }

  }

  readMax = findMax(readerVal, 100);      //calculate timing analysis
  readMin = findMin(readerVal, 100);
  readAvg = findAvg(readerVal, 100);

  writeMax = findMax(writerVal, 10);
  writeMin = findMin(writerVal, 10);
  writeAvg = findAvg(writerVal, 10);
  //print timing analysis
  printf("The maximum waiting time for the readers is: %f microseconds\n", readMax);
  printf("The minimum waiting time for the readers is: %f microseconds\n", readMin);
  printf("The average waiting time for the readers is: %f microseconds\n", readAvg);

  printf("The maximum waiting time for the writers is: %f microseconds\n", writeMax);
  printf("The minimum waiting time for the writers is: %f microseconds\n", writeMin);
  printf("The average waiting time for the writers is: %f microseconds\n", writeAvg);

}

float findMax(float array[], size_t size) //calculate max value of an array of floats
{
  float max = array[0];
  int i;
  for(i = 1; i < size; i++)
  {
    if(max < array[i])
      max = array[i];
  }

  return max;
}

float findMin(float array[], size_t size) //calculates min value of an array of floats
{
  float min = array[0];
  int i;
  for(i = 1; i < size; i++)
  {
    if(min > array[i])
      min = array[i];
  }

  return min;
}

float findAvg(float array[], size_t size) //calculates average of an array of floats
{
  float avg = 0;
  float sum = 0;
  int i;
  for(i = 0; i < size; i++)
  {
    sum = sum + array[i];
  }
  avg = sum/((float) size);
  return avg;
}
