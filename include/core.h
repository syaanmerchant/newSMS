
/* core logic and data structures for medmate.
 * defines the medication model, time-of-day helpers,
 * and basic operations on the medication list.
 *
 * author: Mahnoor Naveed
 * date: 2025/12/03
 * version: v1.1.1
 */

#ifndef CORE_H
#define CORE_H

#define MAX_NAME_LEN 64
#define MAX_NOTES_LEN 256
#define MAX_DOSES_PER_MED 4
#define MAX_MEDICATIONS 100

/* recurrence type for a medication:
 * - daily: repeats every day
 * - once: one-time reminder, then marked as used
 */
typedef enum {
    RECURRENCE_DAILY = 0,
    RECURRENCE_ONCE  = 1
} RecurrenceType;

/* simple time-of-day structure (24h clock). */
typedef struct {
    int hour;
    int minute;
} TimeOfDay;

typedef struct {
    int id;
    char name[MAX_NAME_LEN];
    char dosage[MAX_NAME_LEN];
    int num_doses;
    TimeOfDay doses[MAX_DOSES_PER_MED];

    RecurrenceType recurrence;  // NEW: daily vs one-time
    int one_time_used;          // NEW: for one-time meds, has the reminder already fired?

    char notes[MAX_NOTES_LEN];
} Medication;

/* container for all medications tracked by the program. */
typedef struct {
    Medication meds[MAX_MEDICATIONS]; /* fixed-size array of meds */
    int count;                        /* how many meds are currently stored */
    int next_id;                      /* next id to assign when adding a med */
} MedicationList;

/* initializes an empty medication list.
 * sets count to 0 and next_id to 1 (or another starting value).
 */
void init_med_list(MedicationList *list);

/* adds a medication to the list.
 * assumes there is space left (count < MAX_MEDICATIONS).
 * the function assigns a new id to m and stores it in the array.
 */
void add_medication(MedicationList *list, Medication m);

/* finds a medication in the list by its id.
 * returns a pointer to the matching medication or null if not found.
 */
Medication *find_med_by_id(MedicationList *list, int id);

/* removes a medication from the list by its id.
 * shifts subsequent medications down to fill the gap.
 * does nothing if the id is not found.
 */
void remove_medication(MedicationList *list, int id);

void get_current_time(TimeOfDay *out);

/* fills out[] with medications that are "due" around the given time.
 * "due" means:
 *   - at least one dose time is within ±5 minutes of now, and
 *   - for one-time meds, the reminder has not already been used.
 *
 * params:
 *   list    - full medication list
 *   now     - current time of day
 *   out     - array where due medications will be copied
 *   max_out - max number of meds that fit into out
 *
 * returns: number of medications written into out (0 to max_out).
 */
int get_due_medications(MedicationList *list,
                        TimeOfDay now,
                        Medication *out,
                        int max_out);

#endif