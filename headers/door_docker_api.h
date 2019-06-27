#ifndef MANAGEMENTHEADER_H
#define MANAGEMENTHEADER_H

#include <inttypes.h>
#include <time.h>
#include </usr/local/include/hs/hs.h>
#include "circular_buffer.h"
#include "shared_memory_support.h"

#define MMAP_BUF_SIZE 10240000
#define MAX_MMAP_BUFS 20
#define HS_DATABASE_NO 2
#define HS_SCRATCH_NO 2


typedef struct hs_shared_db_control_t{
  int init_database; 
  int newest_database;
  int running[HS_DATABASE_NO];
  int scatch_allocated[HS_DATABASE_NO];
  int change_db;
}hs_shared_db_control_t;

typedef struct bridge_door_control_t{  
  int no_of_bufs; 
  int max_buf_used;
  //other details
}bridge_door_control_t;

extern int mmap_buf_ids[MAX_MMAP_BUFS];
extern int mmap_buf_idx;

extern hs_database_t *databaseHS_array[HS_DATABASE_NO];
extern hs_shared_db_control_t *hs_db_controller;
extern bridge_door_control_t *bridge_controller;
extern char *worker_mmap_buf;
extern circular_buffer **cb_master;

#endif