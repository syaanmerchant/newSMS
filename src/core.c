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

void add_medication(MedicationList *list, Medication m) {
    if (!list) return;

    // if the user reaches the max medications amount then return the list 
    if (list->count >= MAX_MEDICATIONS) return;

    // sets medication id
    m.id = list->next_id++;

    // if the recurrence is invalid and doesn't match daily or once, then default it to daily
    // NOTE: maybe there should be a warning first
    if (m.recurrence != RECURRENCE_DAILY && m.recurrence != RECURRENCE_ONCE) {
        m.recurrence = RECURRENCE_DAILY;
    }

    // if the medicine is one time use, reset the flag to 0
    if (m.recurrence == RECURRENCE_ONCE && m.one_time_used != 0) {
        m.one_time_used = 0;
    }

    // stores medication in list
    list->meds[list->count++] = m;
}
