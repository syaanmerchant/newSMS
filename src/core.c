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


void remove_medication(MedicationList *list, int id) {
    if (!list) return;
    int idx = -1;

    // locate the index of the medication to be removed in the list
    for (int i = 0; i < list->count; i++) {
        if (list->meds[i].id == id) {
            idx = i;
            break;
        }
    }

    // if the medication if not in the list (index still = -1) then return the list as it is
    if (idx < 0) return;

    // shift the list over to account for removing the medicine
    for (int j = idx; j < list->count - 1; j++) {
        list->meds[j] = list->meds[j + 1];
    }

    // reduce the count for medications in the list after removing
    list->count--;
}

Medication *find_med_by_id(MedicationList *list, int id) {
    if (!list) return NULL;

    // goes through every medication in list
    for (int i = 0; i < list->count; i++) {
        // checks if medicine at index i has the same id as the one we are searching for
        if (list->meds[i].id == id) {
            // if the IDs match, return a pointer to that medication inside the list's array.
            return &list->meds[i];
        }
    }

    // if after going through the list we cant find it, return null
    return NULL;
}


void get_current_time(TimeOfDay *out) {
    if (!out) return;

    // gets current time
    time_t t = time(NULL);

    // converts time t into (hour, minute, second, etc.) based on the user's local timezon
    struct tm *tm_info = localtime(&t);


    out->hour = tm_info->tm_hour;
    out->minute = tm_info->tm_min;
}

// Helper: absolute value of int
static int iabs_int(int x) {
    return x < 0 ? -x : x;
}

/*
static int iabs_int(int x) {
    if (x < 0) {
        return -x;
    }
    return x;
}
*/


int get_due_medications(MedicationList *list, TimeOfDay now, Medication *out, int max_out) {
    if (!list || !out || max_out <= 0) return 0;

    // converts current time into total minutes since midnight
    int now_total = now.hour * 60 + now.minute;

    // counter for the medications that are due
    int found = 0;

    // go through every medication in the list
    for (int i = 0; i < list->count && found < max_out; i++) {
        Medication *m = &list->meds[i];

        // skip one-time meds that already fired
        if (m->recurrence == RECURRENCE_ONCE && m->one_time_used) {
            continue;
        }

        for (int j = 0; j < m->num_doses; j++) {
            int dose_total = m->doses[j].hour * 60 + m->doses[j].minute;
            int diff = dose_total - now_total;

            if (iabs_int(diff) <= 5) { // within +5 or -5 minutes
                out[found++] = *m;
                break; // we only need to add it once
            }
        }
    }

    return found;
}

