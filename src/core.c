#include "core.h"
#include <time.h>
#include <stdlib.h>


void init_med_list(MedicationList *list) {
    // if list is null nothing happens
    if (!list) return;
    // list starts empty with nothing in it
    list->count = 0;
    // list sets the ID number that will be assigned to the next medication added
    list->next_id = 1;
}