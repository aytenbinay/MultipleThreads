#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#define MAXCHAR 500

int numberOfLines = 0;

struct isAllowed {
    int isChange;
    int isUpper;
    int isDone;
    int isWrite;
    int numOfLine;
    char *line;
};

typedef struct isAllowed isAllowed;

void createStructArray(int size, isAllowed *arr) {
    for (int i = 0; i < size; i++) {
        arr[i] = *(isAllowed *) malloc(sizeof(isAllowed));
    }
    for (int i = 0; i < size; i++) {
        arr[i].isChange = 0;
        arr[i].isUpper = 0;
        arr[i].isDone = 0;
        arr[i].isWrite = 0;
        arr[i].line = (char *) calloc(sizeof(char), MAXCHAR);
    }

}

char *filename;
int readThreadNum;
int upperThreadNum;
int replaceThreadNum;
int writeThreadNum;
sem_t semaphore_queue_read;
sem_t semaphore_queue_upper;
sem_t semaphore_queue_replace;
sem_t line_read;
sem_t line_replace;

pthread_mutex_t readMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fileReadMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t changeMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t writeMutex = PTHREAD_MUTEX_INITIALIZER;
int line_read_index = -1;
int line_upper_index = 0;
int line_replace_index = 0;
int line_write_index = 0;


int findNumberOfLines() {
    FILE *fp;
    int c;
    int numOfLine = 0;
    fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "cannot open target file \n");
        exit(1);
    }
    for (c = getc(fp); c != EOF; c = getc(fp))
        if (c == '\n') // Increment count if this character is newline
            numOfLine = numOfLine + 1;

    fclose(fp);
    return numOfLine;

}

int cnt = 0;
FILE *ft;
char STR[MAXCHAR];

int file_line_num = 0;

void *readIssue(isAllowed *arr) { //queue tutulmayacak //sem_wait ile sem_post arasında critical section olur

    while (!feof(ft)) {

        pthread_mutex_lock(&readMutex);
        int in = ++line_read_index;
        pthread_mutex_unlock(&readMutex);

        pthread_mutex_lock(&fileReadMutex);
        int infile = ++file_line_num;
        pthread_mutex_unlock(&fileReadMutex);

        char str[MAXCHAR];
        if (fgets(str, MAXCHAR, ft) != NULL) {
            strcpy(arr[in].line, str);
            arr[in].numOfLine = infile;
            pid_t tid = pthread_self();
            printf("Read_%d read the line %d which is %s", tid, arr[in].numOfLine,
                   arr[in].line);
        } else {
            break;
        }


    }
    // pthread_exit(0);
}


void replaceMethod(int whichLineReplace, isAllowed *arr) {
    for (int i = 0; i < MAXCHAR; i++) {
        if (arr[whichLineReplace].line[i] == ' ') {
            arr[whichLineReplace].line[i] = '_';
        }
    }

}

void *replaceIssue(isAllowed *arr) { //read yapıldı mı??
    while (line_read_index > 0) {
        pthread_mutex_lock(&changeMutex);
        sem_post(&semaphore_queue_replace);
        int in = line_replace_index++;
        sem_wait(&semaphore_queue_replace);

        pthread_mutex_unlock(&changeMutex);
        if (in >= numberOfLines) {
            break;
        }
        if (in < line_read_index && arr[in].isDone == 0) {
            pid_t tid = pthread_self();
            char *line = (char *) calloc(sizeof(char), MAXCHAR);
            strcpy(line, arr[in].line);
            replaceMethod(in, arr);
            printf("Replace_%d read index %d and converted %s to %s ", tid, arr[in].numOfLine, line,
                   arr[in].line);
            arr[in].isChange = 1;
            if (arr[in].isUpper == 1) {
                arr[in].isDone = 1;
            }
        } else {
            pthread_mutex_lock(&changeMutex);
            line_replace_index--;
            pthread_mutex_unlock(&changeMutex);
        }

    }
}

void upperMethod(int whichLineReplace, isAllowed *arr) {
    for (int i = 0; i < MAXCHAR; i++) {
        if (arr[whichLineReplace].line[i] >= 'a' && arr[whichLineReplace].line[i] <= 'z') {
            arr[whichLineReplace].line[i] = arr[whichLineReplace].line[i] - 32;
        }
    }

}

void *upperIssue(isAllowed *arr) {
    while (line_read_index > 0) {
        pthread_mutex_lock(&changeMutex);
        sem_post(&semaphore_queue_upper);
        int in = line_upper_index++;
        sem_wait(&semaphore_queue_upper);
        pthread_mutex_unlock(&changeMutex);
        if (in >= numberOfLines) {
            break;
        }
        if (in < line_read_index && arr[in].isDone == 0) {
            pid_t tid = pthread_self();
            char *line = (char *) calloc(sizeof(char), MAXCHAR);
            strcpy(line, arr[in].line);
            upperMethod(in, arr);
            printf("Upper_%d read index %d and converted %s to %s ", tid, arr[in].numOfLine,
                   line, arr[in].line);
            arr[in].isUpper = 1;
            if (arr[in].isChange == 1) {
                arr[in].isDone = 1;
            }
        } else {
            pthread_mutex_lock(&changeMutex);
            line_upper_index--;
            pthread_mutex_unlock(&changeMutex);
        }

    }
}

void writeMethod(int whichLineReplace, isAllowed *arr) {
    int count = 0;
    int ch;
    int numOfLine = 1;
    FILE *fa;
    fa = fopen(filename, "r+b");
    if (fa == NULL) {
        fprintf(stderr, "cannot open target file %s\n", "test.txt");
        exit(1);
    }
    if (whichLineReplace != numOfLine) {
        while ((ch = fgetc(fa)) != EOF) {
            if (ch == '\n') {
                numOfLine++;
                if (whichLineReplace == numOfLine) {
                    break;
                }
            }
        }

    }

    while ((ch = fgetc(fa)) != EOF) {
        if (arr[whichLineReplace].line[count] == '\n') {
            fseek(fa, -1, SEEK_CUR);
            fputc('\n', fa);
            fseek(fa, 0, SEEK_CUR);
            break;
        }

        fseek(fa, -1, SEEK_CUR);
        fputc(arr[whichLineReplace].line[count], fa);
        fseek(fa, 0, SEEK_CUR);
        // printf("end off %s", arr[whichLineReplace].line[count]);
        count++;

    }
    fclose(fa);

}

void *writeIssue(isAllowed *arr) {
    while (line_read_index > 0) {
        pthread_mutex_lock(&writeMutex);
        int in = line_write_index++;
        pthread_mutex_unlock(&writeMutex);
        if (in >= numberOfLines) {
            break;
        }
        if (in < line_write_index && arr[in].isDone == 1 && arr[in].isWrite == 0) {
            pid_t tid = pthread_self();
            char *line = (char *) calloc(sizeof(char), MAXCHAR);
            strcpy(line, arr[in].line);
            writeMethod(in, arr);
            printf("Writer_%d write line %d back which is %s", tid, arr[in].numOfLine, line, arr[in].line);
            arr[in].isWrite = 1;
        } else {
            pthread_mutex_lock(&writeMutex);
            line_write_index--;
            pthread_mutex_unlock(&writeMutex);
        }

    }


}

int main(int argc, char *argv[]) {

    numberOfLines = findNumberOfLines();
    filename = argv[2];
    readThreadNum = atoi(argv[4]);
    upperThreadNum = atoi(argv[5]);
    replaceThreadNum = atoi(argv[6]);
    writeThreadNum = atoi(argv[7]);
    ft = fopen(filename, "r+b");
    if (ft == NULL) {
        fprintf(stderr, "cannot open target file %s \n");
        exit(1);
    }
    isAllowed arr[numberOfLines];
    createStructArray(numberOfLines, arr);
    sem_init(&line_read, 0, 1);
    sem_init(&line_replace, 0, 1);

    pthread_t read_threads[readThreadNum]; //threadleri oluşturdum.
    sem_init(&semaphore_queue_replace, 0, 0);
    sem_init(&line_replace, 0, 1);
    pthread_t replace_threads[replaceThreadNum];
    sem_init(&semaphore_queue_upper, 0, 0);

    pthread_t upper_threads[upperThreadNum];
    pthread_t writer_threads[writeThreadNum];


    for (int i = 0; i < readThreadNum; i++) {
        pthread_create(&read_threads[i], NULL, &readIssue, (void *) &arr); //create ettim

    }
    for (int i = 0; i < replaceThreadNum; i++) {
        pthread_create(&replace_threads[i], NULL, &replaceIssue, (void *) &arr);
    }

    for (int i = 0; i < upperThreadNum; i++) {
        pthread_create(&upper_threads[i], NULL, &upperIssue, (void *) &arr);
    }
    for (int i = 0; i < writeThreadNum; i++) {
        pthread_create(&writer_threads[i], NULL, &writeIssue, (void *) &arr);

    }
    for (int i = 0; i < readThreadNum; i++)
        pthread_join(read_threads[i], NULL); //wait for the termination of the threads
    for (int i = 0; i < replaceThreadNum; i++)
        pthread_join(replace_threads[i], NULL); //wait for the termination of the threads
    for (int i = 0; i < upperThreadNum; i++)
        pthread_join(upper_threads[i], NULL); //wait for the termination of the threads
    for (int i = 0; i < writeThreadNum; i++)
        pthread_join(writer_threads[i], NULL);

    for (int i = 0; i < numberOfLines; i++) {
        printf(" %d \t %d: %s", i, arr[i].numOfLine, arr[i].line);
    }
    fclose(ft);

    return 0;
}