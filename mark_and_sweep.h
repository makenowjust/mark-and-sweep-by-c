#pragma once
#ifndef MARK_AND_SWEEP_H_
#define MARK_AND_SWEEP_H_

// gc_object_tの型を表わす列挙型
typedef enum gc_object_type_t_ {
  // scalar type
  GC_INT = 1, // int
  GC_FLOAT,   // double
  GC_STRING,  // gc_string_t
  
  // object type
  GC_ARRAY,    //gc_array_t
} gc_object_type_t;

struct gc_string_t_;
struct gc_array_t_;

// gc_manager_tによって管理されるオブジェクトです。
typedef struct gc_object_t_ {
  gc_object_type_t type;
  char mark;
  union {
    int                  as_int;
    double               as_float;
    struct gc_string_t_* as_string;
    struct gc_array_t_*  as_array;
  } value;
} gc_object_t;

typedef struct gc_string_t_ {
  char* text;
  int size;
} gc_string_t;

typedef struct gc_array_t_ {
  gc_object_t** values;
  int size;
} gc_array_t;

// ヒープの表現に使うリストです。
typedef struct gc_list_t_ {
  struct gc_list_t_* prev;
  struct gc_list_t_* next;
  gc_object_t* value;
} gc_list_t;

//gc_object_tを管理する構造体です。
typedef struct gc_manager_t_ {
  struct {
    gc_list_t* head;
    gc_list_t* tail;
    int size;
  } heap;
  int next_gc;
  gc_object_t* root_object; // 恐らくGC_ARRAYとなる。
} gc_manager_t;

// gc_manager_tを初期化する。
void initGCManager(gc_manager_t* manager);

// gc_manager_tのheapと管理しているgc_object_tを解放する。
void freeGCManager(gc_manager_t* manager);

// GCを強制的に動かす。
void runGC(gc_manager_t* manager);

// gc_object_tを新たに作って、managerの管理下に置き返す。
// heap.sizeがnext_gcに逹したら、GCを動かし、next_gcを2倍にする。
// また、確保に失敗した場合もGCを動かし、再度メモリを確保しようとする。（それでも失敗した場合はNULLを返す）
gc_object_t* newGCInt     (gc_manager_t* manager, int value);
gc_object_t* newGCFloat   (gc_manager_t* manager, double value);
// valueはコピーされて、GCによって解放される。
gc_object_t* newGCString  (gc_manager_t* manager, char* value);
gc_object_t* newGCStringEx(gc_manager_t* manager, gc_string_t* value);
gc_object_t* newGCArray   (gc_manager_t* manager, gc_object_t** values, int size);
gc_object_t* newGCArrayN  (gc_manager_t* manager, int size);
gc_object_t* newGCArrayEx (gc_manager_t* manager, gc_array_t* array);

#endif
