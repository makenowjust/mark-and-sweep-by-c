#include <stdlib.h>
#include <string.h>

#include "mark_and_sweep.h"

#ifdef DEBUG
  #include <stdio.h>
  #define LOG0(msg) printf("%s[%d]: " msg "\n", __FILE__, __LINE__)
  #define LOG(msg, ...) printf("%s[%d]: " msg "\n", __FILE__, __LINE__, __VA_ARGS__)
#else
  #define LOG0(msg)
  #define LOG(msg, ...) 
#endif

#define DEFAULT_NEXT_GC (64)

void freeGCObject(gc_object_t* object, int mode);

void initGCManager(gc_manager_t* manager) {
  LOG("init manager[%p]", manager);
  
  memset(manager, 0, sizeof(gc_manager_t));
  manager->next_gc = DEFAULT_NEXT_GC;
}

void freeGCManager(gc_manager_t* manager) {
  LOG("free manager[%p]", manager);
  
  LOG0("start heap free");
  gc_list_t* current = manager->heap.head;
  while (current) {
    freeGCObject(current->value, 2);
    
    gc_list_t* next = current->next;
    free(current);
    current = next;
  }
  LOG0("end heap free");
  
  manager->heap.head = manager->heap.tail = NULL;
  manager->heap.size = 0;
  
  manager->next_gc = DEFAULT_NEXT_GC;
  manager->root_object = NULL;
}

void markGC(gc_object_t* target) {
  // targetがNULLの場合は当然、
  // targetが既にmarkされているときも循環を防ぐために処理を終了する。
  if (!target || target->mark) return;
  
  // とりあえずtargetをmarkしておく。
  target->mark = 1;
  
  // targetの型毎に処理を分岐する。
  switch (target->type) {
  // 整数、浮動小数、文字列のときは何もしない。
  case GC_INT:    LOG("mark int[%d]",    target->value.as_int);    break;
  case GC_FLOAT:  LOG("mark float[%lf]", target->value.as_float);  break;
  case GC_STRING: LOG("mark string[%p]", target->value.as_string); break;
    
  // リストの場合は、全ての要素をマークする。
  case GC_ARRAY:
    LOG("mark array[%p, %d]", target->value.as_array, target->value.as_array->size);
    {
      gc_array_t* array = target->value.as_array;
      for (int i = 0; i < array->size; i++) {
        LOG("mark array loop[%d, %p]", i, array->values[i]);
        markGC(array->values[i]);
      }
    }
    LOG("mark array finish[%p, %d]", target->value.as_array, target->value.as_array->size);
    break;
  }
}

void sweepGC(gc_manager_t* manager) {
  LOG("start sweep[%p]", manager);
  
  gc_list_t* current = manager->heap.head;
  gc_list_t* tail = NULL;
  
  while (current) {
    gc_list_t* next;
    
    if (current->value->mark) {
      // マークされていたら、マークを戻しておく。
      current->value->mark = 0;
      tail = current;
      next = current->next;
      
    } else {
      // マークされていない場合は、まずオブジェクトを解放する。
      freeGCObject(current->value, 1);
      
      // ヒープの前のオブジェクトと次のオブジェクトを連結させる。
      if (current->prev) {
        next = current->prev->next = current->next;
      } else {
        next = manager->heap.head = current->next;
      }
      
      //要素も解放する。
      free(current);
      manager->heap.size -= 1;
    }
    
    current = next;
  }
  
  //tailも忘れずに直す。
  LOG("tail change[%p -> %p]", manager->heap.tail, tail);
  manager->heap.tail = tail;
}

void runGC(gc_manager_t* manager) {
  LOG("start GC[%p]", manager);
  LOG("start mark[%p]", manager);
  markGC(manager->root_object);
  sweepGC(manager);
}

//mode = 1 //再帰的に解放
//mode = 2 //再帰せずに解放
void freeGCObject(gc_object_t* object, int mode) {
  if (!object) return;
  
  LOG("free object strat[%p]", object);
  
  switch (object->type) {
  // 整数と浮動小数の場合は、特別にすることは無い。
  case GC_INT:   LOG("free int[%d]",    object->value.as_int);   break;
  case GC_FLOAT: LOG("free float[%lf]", object->value.as_float); break;
  
  // 文字列の場合、文字列を解放する。
  case GC_STRING:
    LOG("free string[%p]", object->value.as_string);
    {
      gc_string_t* string = object->value.as_string;
      free(string->text);
      free(string);
    }
    break;
  // 配列の場合、各要素を解放する。
  case GC_ARRAY:
    LOG("free array[%p, %d]", object->value.as_array, object->value.as_array->size);
    if (mode == 1) {
      gc_array_t* array = object->value.as_array;
      for (int i = 0; i < array->size; i++) {
        freeGCObject(array->values[i], 1);
      }
      free(array->values);
      free(array);
    }
    break;
  }
  
  free(object);
}

void* mallocOnGCManager(gc_manager_t* manager, size_t size) {
  void* memory = malloc(size);
  
  if (!memory) {
    runGC(manager);
    memory = malloc(size);
    if (!memory) return NULL;
  }
  
  memset(memory, 0, size);
  return memory;
}

#define MALLOC_ON_GC_MANAGER(manager__, size__)             \
  ({                                                        \
    void* memory = mallocOnGCManager(manager__, size__);    \
    if (!memory) return NULL;                               \
    memory;                                                 \
  })

void* appendGCHeap(gc_manager_t* manager, gc_object_t* object) {
  if (manager->heap.size > manager->next_gc) {
    runGC(manager);
    manager->next_gc = manager->next_gc * 2;
  }
  
  gc_list_t* new_tail = (gc_list_t*)
    MALLOC_ON_GC_MANAGER(manager, sizeof(gc_list_t));
  new_tail->value = object;
  
  if (!manager->heap.head) {
    manager->heap.head = new_tail;
  }
  
  if (manager->heap.tail) {
    gc_list_t* tail = manager->heap.tail;
    tail->next = new_tail;
    new_tail->prev = tail;
  }
  manager->heap.tail = new_tail;
  manager->heap.size += 1;
  return manager;
}

#define NEW_GC_OBJECT(manager__, type__)                    \
  ({                                                        \
    gc_object_t* object = (gc_object_t*)                    \
      MALLOC_ON_GC_MANAGER(manager__, sizeof(gc_object_t)); \
    object->type = type__;                                  \
    if (!appendGCHeap(manager__, object)) return NULL;      \
    object;                                                 \
  })

gc_object_t* newGCInt(gc_manager_t* manager, int value) {
  gc_object_t* object = NEW_GC_OBJECT(manager, GC_INT);
  object->value.as_int = value;
  LOG("new int[%d, %p]", value, object);
  return object;
}

gc_object_t* newGCFloat(gc_manager_t* manager, double value) {
  gc_object_t* object = NEW_GC_OBJECT(manager, GC_FLOAT);
  object->value.as_float = value;
  LOG("new float[%lf, %p]", value, object);
  return object;
}

gc_object_t* newGCString(gc_manager_t* manager, char* value) {
  return newGCStringEx(manager, &(gc_string_t) {
    .text = value, .size = strlen(value),
  });
}

gc_object_t* newGCStringEx(gc_manager_t* manager, gc_string_t* value) {
  gc_object_t* object = NEW_GC_OBJECT(manager, GC_STRING);
  
  char* copy_text = (char*)
    MALLOC_ON_GC_MANAGER(manager, sizeof(char) * (value->size + 1));
  memcpy(copy_text, value->text, 
    sizeof(char) * value->size + 1);
  copy_text[value->size] = '\0';
  
  gc_string_t* string = (gc_string_t*)
    MALLOC_ON_GC_MANAGER(manager, sizeof(gc_string_t));
  string->text = copy_text;
  string->size = value->size;
  
  object->value.as_string = string;
  LOG("new string[%p, %d, %p]", string, string->size, object);
  return object;
}

gc_object_t* newGCArray(gc_manager_t* manager, gc_object_t** values, int size) {
  return newGCArrayEx(manager, &(gc_array_t) {
    .values = values, .size = size,
  });
}

gc_object_t* newGCArrayN(gc_manager_t* manager, int size) {
  gc_object_t* values[size];
  memset(values, 0, sizeof(gc_object_t*) * size);
  return newGCArrayEx(manager, &(gc_array_t) {
    .values = values, .size = size,
  });
}

gc_object_t* newGCArrayEx(gc_manager_t* manager, gc_array_t* value) {
  LOG("new array start[%p, %d]", value, value->size);
  gc_object_t* object = NEW_GC_OBJECT(manager, GC_ARRAY);
  
  gc_object_t** copy_values = (gc_object_t**)
    MALLOC_ON_GC_MANAGER(manager, sizeof(gc_object_t*) * value->size);
  memcpy(copy_values, value->values, sizeof(gc_object_t*) * value->size);
  
  gc_array_t* array = (gc_array_t*)
    MALLOC_ON_GC_MANAGER(manager, sizeof(gc_array_t));
  array->values = copy_values;
  array->size = value->size;
  
  object->value.as_array = array;
  LOG("new array[%p, %d, %p]", array, array->size, object);
  return object;
}

