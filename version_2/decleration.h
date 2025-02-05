#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>

#define MAX_WORDS 1000
#define MAX_TEXT 100
#define FOLDER_PATH "./dictionary"
#define MSG_KEY 1234
#define SHM_KEY 5678

typedef struct {
    char english[50];
    char french[50];
} WordPair;

// Message structure for message queue
typedef struct {
    long mtype;
    char english[50];
    char french[50];
    int direction; // 1: English-to-French, 2: French-to-English
} Message;

#endif