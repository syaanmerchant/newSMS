#ifndef CORE_H
#define CORE_H

#define MAX_NAME_LEN 64
#define MAX_NOTES_LEN 256
#define MAX_DOSES_PER_MED 4
#define MAX_MEDICATIONS 100

typedef enum {
    RECURRENCE_DAILY = 0,
    RECURRENCE_ONCE  = 1
} RecurrenceType;

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

typedef struct {
    Medication meds[MAX_MEDICATIONS];
    int count;
    int next_id;
} MedicationList;

void init_med_list(MedicationList *list);
void add_medication(MedicationList *list, Medication m);
Medication *find_med_by_id(MedicationList *list, int id);
void remove_medication(MedicationList *list, int id);

void get_current_time(TimeOfDay *out);

// Returns how many meds were written into out[] (up to max_out)
// “Due” means within ±5 minutes of now and not already used (for one-time).
int get_due_medications(MedicationList *list,
                        TimeOfDay now,
                        Medication *out,
                        int max_out);

#endif