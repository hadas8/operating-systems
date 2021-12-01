#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <stdatomic.h>


//================== QUEUE STRUCTURE ===========================
//a queue node structure, implemented as a linked-list node
typedef struct queue_node {
	char dir[PATH_MAX];
	struct queue_node *next;
} queue_node_t;

// a FIFO queue structure
typedef struct queue {
	queue_node_t *head;
	queue_node_t *tail;
} queue_t;

//check if the queue is empty
int is_empty(queue_t *dir_queue) {
	if (dir_queue->head == NULL) {
		return 1;
	}
	return 0;
}

//================== VARIABLE DEFINITIONS ===========================
queue_t *dir_queue;
pthread_mutex_t qlock;
pthread_mutex_t startlock;
pthread_cond_t not_empty;
pthread_cond_t start;
atomic_int found;
int started_threads = 0;
int dead_threads = 0;
int queued_threads = 0;
int thread_count;


//===================== QUEUE FUNCTIONS =============================
//insert a directory to the queue
void enqueue(queue_node_t *node) {
	pthread_mutex_lock(&qlock);
	if (is_empty(dir_queue)) {
		dir_queue->head = node;
		dir_queue->tail = node;
		pthread_cond_signal(&not_empty);
	} else {
		dir_queue->tail->next = node;
		dir_queue->tail = node;
	}

	pthread_mutex_unlock(&qlock);
}

//dequeue the head directory from the queue
//if all alive threads are stuck in an empty queue, exit the program
queue_node_t* dequeue() {
	queue_node_t *node;

	pthread_mutex_lock(&qlock);
	queued_threads++;
	while (is_empty(dir_queue)) {
		if ((queued_threads + dead_threads) == thread_count) {
			//all threads are either dead or queued, time to finish
			pthread_cond_signal(&not_empty);
			pthread_mutex_unlock(&qlock);
			pthread_exit(NULL);
		}
		pthread_cond_wait(&not_empty, &qlock);
	}
	queued_threads--;
	node = dir_queue->head;
	dir_queue->head = dir_queue->head->next;
	pthread_mutex_unlock(&qlock);

	return node;
}


//allocate memory for a queue_node
queue_node_t* allocate_queue_node() {
	queue_node_t *node = malloc(sizeof(queue_node_t));
	if(!node) {
		return NULL;
	}
	node->next = NULL;

	return node;
}

//allocate memory for the queue
queue_t* allocate_queue() {
	queue_t *dir_queue = malloc(sizeof(queue_t));
	if(!dir_queue) {
		return NULL;
	}
	dir_queue->head = NULL;
	dir_queue->tail = NULL;

	return dir_queue;
}



//================== HELPER FUNCTIONS ===========================
// check that the arguments are correct
// if not, exit the program
void check_args(int argc, char *argv[]) {
	struct stat statbuf;

	if (argc != 4) {
		fprintf(stderr, "Wrong number of arguments\n");
		exit(1);
	}

	if (lstat(argv[1], &statbuf) != 0) {
		fprintf(stderr, "%s\n", strerror(errno));
		exit(1);
	} else {
		if (!S_ISDIR(statbuf.st_mode)) {
			fprintf(stderr, "%s is not a directory\n", argv[1]);
			exit(1);
		} else {
			if(!(statbuf.st_mode & S_IRUSR)) {
				printf("Directory %s: Permission denied.\n", argv[1]);
				exit(1);
			}
		}
	}
}

//iterate over the first directory in the queue
int iterate_directory(char* dir, char *substr) {
	struct stat statbuf;
	struct dirent *pdirent;
	DIR *pdir;
	queue_node_t *new_dir;
	char path[PATH_MAX];

	pdir = opendir(dir);
	if (pdir == NULL) {
		fprintf(stderr, "Cannot open directory %s\n", dir);
		closedir(pdir);
		return 1;
	}

	while ((pdirent = readdir(pdir)) != NULL) {
		sprintf(path, "%s/%s", dir, pdirent->d_name);

		//check read permissions
		if (lstat(path, &statbuf) != 0) {
			fprintf(stderr, "Directory %s: %s\n", path, strerror(errno));
			closedir(pdir);
			return 1;
		}

		//if the file contains the given string, print path and add to found counter
		else if(S_ISREG(statbuf.st_mode)) {
			if (strstr(pdirent->d_name, substr)) {
				printf("%s\n", path);
				found++;
			}
		}

		//if the file is a directory, adding it to queue
		else if(S_ISDIR(statbuf.st_mode)) {
			if (strcmp(pdirent->d_name, ".") && strcmp(pdirent->d_name, "..")) {
				if (!(statbuf.st_mode & S_IRUSR)) {
					printf("Directory %s: Permission denied.\n", path);
				}
				else {
					if ((new_dir = allocate_queue_node()) == NULL) {
						fprintf(stderr, "%s\n", strerror(ENOMEM));
					}

					strcpy(new_dir->dir, path);
					enqueue(new_dir);
				}
			}
		}
	}

	closedir(pdir);
	return 0;
}

//======================= THREADS ===============================
//flow of a search thread
//dequeue the first directory and iterate over it
void *thread_func(void * substr) {
	queue_node_t *head;
	char *dir;
	int last_thread = 1;  //signaling the last thread

	pthread_mutex_lock(&startlock);
	started_threads++;
	while (started_threads < thread_count) {
		last_thread = 0;
		pthread_cond_wait(&start, &startlock);
	}
	if (last_thread) {
		//signal threads to start searching
		pthread_cond_broadcast(&start);
	}
	pthread_mutex_unlock(&startlock);

	while(1) {
		head = dequeue();
		dir = head->dir;

		if (iterate_directory(dir, (char *)substr)) {
			//all threads are either dead or queued, time to finish
			pthread_mutex_lock(&qlock);
			dead_threads++;
			pthread_cond_signal(&not_empty);
			pthread_mutex_unlock(&qlock);
			pthread_exit(NULL);
		}

		free(head);
	}
}

//================== MAIN THREAD ===========================
int main(int argc, char *argv[]) {
	int i, rc;
	pthread_t *thread_id;
	queue_node_t *head;

	//check that the arguments are correct
	check_args(argc, argv);

	//create the FIFO directory queue and put the the search directory in the queue
	dir_queue = allocate_queue();
	if (dir_queue == NULL) {
		fprintf(stderr, "%s\n", strerror(ENOMEM));
		exit(1);
	}

	if ((head = allocate_queue_node()) == NULL) {
		fprintf(stderr, "%s\n", strerror(ENOMEM));
		exit(1);
	}

	strcpy(head->dir, argv[1]);
	enqueue(head);

	//initialize mutex and condition variables
	pthread_mutex_init(&qlock, NULL);
	pthread_mutex_init(&startlock, NULL);
	pthread_cond_init(&not_empty, NULL);
	pthread_cond_init(&start, NULL);

	//create search threads
	thread_count = atoi(argv[3]);
	thread_id = malloc(sizeof(pthread_t) * thread_count);
	if (!thread_id) {
		fprintf(stderr, "%s\n", strerror(ENOMEM));
		exit(1);
	}

	for (i = 0; i < thread_count; ++i) {
		rc = pthread_create(&thread_id[i], NULL, thread_func, (void *)argv[2]);
		if (rc) {
			fprintf(stderr, "Failed creating thread: %s\n", strerror(rc));
			exit(1);
		}
	}

	//wait for threads and exit condition
	for (i = 0; i < thread_count; ++i) {
		pthread_join(thread_id[i], NULL);
	}

	printf("Done searching, found %d files\n", found);

	pthread_mutex_destroy(&qlock);
	pthread_mutex_destroy(&startlock);
	pthread_cond_destroy(&not_empty);
	pthread_cond_destroy(&start);
	free(thread_id);

	if (dead_threads) {
		exit(1);
	}

	free(dir_queue);
	exit(0);
}
