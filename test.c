#include "mark_and_sweep.h"

#define N (100)

int main(void) {
  gc_manager_t manager;
  initGCManager(&manager);
  
  gc_object_t* root = newGCArrayN(&manager, N);
  manager.root_object = root;
  gc_array_t* array = root->value.as_array;
  
  for (int i = 0; i < N/2; i++) {
    array->values[i*2]   = newGCInt(&manager, i + 1);
    array->values[i*2+1] = newGCString(&manager, "Hello");
  }
  
  for (int i = 0; i < N; i += 3) {
    array->values[i] = NULL;
  }
  
  runGC(&manager);
  
  freeGCManager(&manager);
  
  return 0;
}

