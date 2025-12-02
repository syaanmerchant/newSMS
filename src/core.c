/* Contains core data handling functions for the MedMate application
 * Includes functionality for initializing the medication list, adding and removing
 * medications, searching for medications given their ID, and retrieving medications
 * that are due based on the current time
 * Manages MedicationList structure updates, handles time comparisons, 
 * and ensures recurrence (how often a medication is to be taken)settings are validated properly
 *
 * GROUP NUMBER: Team 27
 * DATE: December 3, 2025
 * VERSION: v1.1.0
 */

#include "core.h"
#include <time.h>
#include <stdlib.h>


/* init_med_list()
 * list: pointer to a MedicationList struct that will be initialized
 *
 * Initializes an empty medication list. Sets the count to zero and 
 * prepares the list so that newly added medications begin with ID = 1.
 * 
 * return: void
 */
void init_med_list(MedicationList *list) {
    // if list is null/invalid nothing happens
    if (!list) return;

    // list starts empty with nothing in it (zero medications stored)
    list->count = 0;

    // first medication added will be assigned ID 1
    list->next_id = 1;
}

/* add_medication()
 * list: pointer to a MedicationList
 * m: medication to be added by value
 *
 * Adds a new medication to the list. 
 * Assigns a unique ID, validates recurrence settings, resets one-time flags if needed, 
 * and appends the medication to the array.
 *
 * NOTE: If the maximum allowed number of medications is reached, the function 
 * returns without adding.
 *
 * return: void
 */
void add_medication(MedicationList *list, Medication m) {
    if (!list) return;

    // if the user reaches the max medications amount no insertion occurs 
    if (list->count >= MAX_MEDICATIONS) return;

    // sets unique ID for this medication entry
    m.id = list->next_id++;

    // logic for validating recurrences 
    // defaults to daily if invalid to simplify errors
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

/* remove_medication()
 * list: pointer to a MedicationList
 * id: type int, ID number of medication to remove
 *
 * Finds a medication by ID and removes it from the list by shifting 
 * subsequent entries left. If the ID is not found, the list is unchanged.
 *
 * return: void
 */
void remove_medication(MedicationList *list, int id) {
    if (!list) return;

    // initialize index to -1
    // set it to this instead of 0 to account for the left shift
    int idx = -1;

    // find index of medication matching the ID given
    for (int i = 0; i < list->count; i++) {
        if (list->meds[i].id == id) {
            idx = i;
            break;
        }
    }

    // if the medication if not in the list (index still = -1) nothing changes
    if (idx < 0) return;

    // shift the list over to account for removing the medicine
    for (int j = idx; j < list->count - 1; j++) {
        list->meds[j] = list->meds[j + 1];
    }

    // reduce the count for medications in the list after removing
    list->count--;
}

/* find_med_by_id()
 * list: pointer to MedicationList
 * id: type int, medication ID to search for
 *
 * Searches the medication list for a matching ID. Returns a pointer to the 
 * medication inside the list's internal array (not a copy).
 *
 * return: pointer to Medication if found, NULL otherwise
 */
Medication *find_med_by_id(MedicationList *list, int id) {
    if (!list) return NULL;

    // goes through every medication in list
    for (int i = 0; i < list->count; i++) {
        if (list->meds[i].id == id) {
            // if the IDs match, return a pointer to that medication inside the list's array.
            return &list->meds[i];
        }
    }

    // if after going through the list we cant find it, return null
    return NULL;
}

/* get_current_time()
 * out: pointer to a TimeOfDay struct that will be filled
 *
 * Reads the system clock and extracts the current hour and minute 
 * in the user's local timezone.
 *
 * return: void
 */
void get_current_time(TimeOfDay *out) {
    if (!out) return;

    // gets current time
    time_t t = time(NULL);

    // converts time t into (hour, minute, second, etc.) based on the user's local timezone
    struct tm *tm_info = localtime(&t);

    out->hour = tm_info->tm_hour;
    out->minute = tm_info->tm_min;
}

/* iabs_int()
 * x: integer value to convert
 *
 * Helper function that returns the absolute value of an integer.
 * Used when comparing time differences so negative values are
 * treated the same as positive ones.
 *
 * return: the absolute value of x
 */
static int iabs_int(int x) {
    return x < 0 ? -x : x;
}

/* get_due_medications()
 * list: pointer to MedicationList
 * now: current time (hour + minute)
 * out: output array to store medications that are due
 * max_out: maximum number of entries 'out' can hold
 *
 * A medication is considered “due” if its scheduled time is within
 * +5 or -5 minutes of the current time. 
 * One-time medications are included only if they have not been used yet.
 *
 * return: number of medications written into out[]
 */
int get_due_medications(MedicationList *list, TimeOfDay now, Medication *out, int max_out) {
    if (!list || !out || max_out <= 0) {
        return 0;
    }

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

        // check every dose time for this medication 
        for (int j = 0; j < m->num_doses; j++) {
            int dose_total = m->doses[j].hour * 60 + m->doses[j].minute;
            int diff = dose_total - now_total;

            // medication is considered due if within +5 or -5 minutes
            if (iabs_int(diff) <= 5) { 
                out[found++] = *m;
                break; // we only need to add it once
            }
        }
    }

    return found;
}

