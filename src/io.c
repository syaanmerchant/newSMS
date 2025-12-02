/* 
 * io.c implements loading, saving, and CSV export for the MedicationList.
 * Medications are stored in a plain-text format for the main app to reload, and can also be exported in a spreadsheet-friendly CSV format.
 *
 * AUTHOR: Shifa Zaman
 * DATE: December 2nd, 2025
 * VERSION: 1.1.1
 */

#include "io.h"
#include <stdio.h>
#include <string.h>

/*
 * Loads medications from a plain-text file into a MedicationList.
 *
 * Parameters:
 * - filename: path to the input file (must not be NULL)
 * - list: pointer to a MedicationList that will be initialized and filled
 *
 * Returns 1 on success, 0 on error (invalid arguments, file missing, or bad format).
 * If the file does not exist, this returns 0 but does not treat it as a hard error for the rest of the program.
 *
 * Expects the same format that save_medications() writes below.
 */
int load_medications(const char *filename, MedicationList *list) {
    if (!filename || !list) return 0;  // basic argument check
 
    FILE *f = fopen(filename, "r");
    if (!f) {
     // No file yet is not a hard error
        return 0;  
    }
    init_med_list(list);  // start from an empty list

    int count = 0;
    // first line: number of meds stored in the file
    if (fscanf(f, "%d\n", &count) != 1) {
        fclose(f);
        return 0;
    }

    for (int i = 0; i < count; i++) {
        Medication m;
        memset(&m, 0, sizeof(Medication));  // clear everything to safe defaults

        // read ID; if we can't, stop loading
        if (fscanf(f, "%d\n", &m.id) != 1) break;

        // Read name
        if (!fgets(m.name, MAX_NAME_LEN, f)) break;
        m.name[strcspn(m.name, "\r\n")] = '\0';

        // Read dosage
        if (!fgets(m.dosage, MAX_NAME_LEN, f)) break;
        m.dosage[strcspn(m.dosage, "\r\n")] = '\0';

        // Number of doses per day
        if (fscanf(f, "%d\n", &m.num_doses) != 1) break;

        // ensure num_doses stays within valid range
        if (m.num_doses < 0) m.num_doses = 0;
        if (m.num_doses > MAX_DOSES_PER_MED) m.num_doses = MAX_DOSES_PER_MED;

        // read each dose time (hour, minute)
        for (int j = 0; j < m.num_doses; j++) {
            if (fscanf(f, "%d %d\n",
                       &m.doses[j].hour,
                       &m.doses[j].minute) != 2) {
                // if reading fails, default this time to 00:00
                m.doses[j].hour = 0;
                m.doses[j].minute = 0;
            }
        }

        int rec_int = 0;
        int used_int = 0;

        // read recurrence integer; fall back to 0 if missing
        if (fscanf(f, "%d\n", &rec_int) != 1) {
            rec_int = 0;
        }

        // read one_time_used flag; fall back to 0 if missing
        if (fscanf(f, "%d\n", &used_int) != 1) {
            used_int = 0;
        }

        // convert stored integer back into the recurrence
        m.recurrence = (rec_int == (int)RECURRENCE_ONCE)
                       ? RECURRENCE_ONCE
                       : RECURRENCE_DAILY;

        // ensure one_time_used is normalized to 0 or 1
        m.one_time_used = used_int ? 1 : 0;


        // read notes line; if missing, set to empty string
        if (!fgets(m.notes, MAX_NOTES_LEN, f)) {
            m.notes[0] = '\0';
        } else {
            m.notes[strcspn(m.notes, "\r\n")] = '\0';
        }

        // add into list, preserving id and updating next_id
        if (list->count < MAX_MEDICATIONS) {
            list->meds[list->count++] = m;
            // keep next_id always higher than any existing id
            if (m.id >= list->next_id) {
                list->next_id = m.id + 1;
            }
        }
    }

    fclose(f);
    return 1;
}


/*
 * Saves the current MedicationList to a plain-text file.
 *
 * Parameters:
 * - filename: path to the output file (must not be NULL)
 * - list: pointer to an initialized MedicationList (must not be NULL)
 *
 * Returns 1 on success, 0 on error (invalid arguments or file open failure).
 *
 * File format:
 *  - Line 1: total number of medications (int)
 *  - Per med:
 *     id (int)
 *     name (string, one line)
 *     dosage (string, one line)
 *     num_doses (int)
 *     num_doses lines of: hour minute
 *     recurrence (int)
 *     one_time_used (int)
 *     notes (string, one line)
 */
int save_medications(const char *filename, MedicationList *list) {
    if (!filename || !list) return 0;  // basic argument check
 
    FILE *f = fopen(filename, "w");
    if (!f) return 0;   // failed to open file for writing
    
    fprintf(f, "%d\n", list->count); // first line: how many meds we are saving

    for (int i = 0; i < list->count; i++) {
        Medication *m = &list->meds[i];

        fprintf(f, "%d\n", m->id);   
        fprintf(f, "%s\n", m->name);
        fprintf(f, "%s\n", m->dosage);
        fprintf(f, "%d\n", m->num_doses);

        // write each dose time as "hour minute"
        for (int j = 0; j < m->num_doses; j++) {
            fprintf(f, "%d %d\n", m->doses[j].hour, m->doses[j].minute);
        }

        // recurrence stored as an integer version of the enum
        fprintf(f, "%d\n", (int)m->recurrence);

        // one_time_used is stored as 0 or 1
        fprintf(f, "%d\n", m->one_time_used);

        // notes (single line)
        fprintf(f, "%s\n", m->notes);
    }

    fclose(f);
    return 1;
}


/*
 * Writes the current MedicationList to a CSV file for use in spreadsheet programs.
 *
 * Parameters: 
 * - filename: path to the CSV output file
 * - list: pointer to a MedicationList to export
 *
 * Returns 1 on success, 0 on error (invalid arguments or file open failure).
 *
 * CSV columns:
 *   Name, Dosage, Recurrence, Times, Notes
 *   Times are combined as "HH:MM; HH:MM; ..." for multiple doses
 */
int export_medications_csv(const char *filename, MedicationList *list) {
    if (!filename || !list) return 0;
 
    FILE *f = fopen(filename, "w");
    if (!f) return 0;  // could not create CSV file

    // header row for the CSV
    fprintf(f, "Name,Dosage,Recurrence,Times,Notes\n");

    for (int i = 0; i < list->count; i++) {
        Medication *m = &list->meds[i];

        // turn recurrence enum into a human-readable string
        const char *rec_str = (m->recurrence == RECURRENCE_ONCE)
                              ? "one-time"
                              : "daily";

        // Combine times into "09:00/21:00"
        char time_buf[128];
        times_buf[0] = '\0';
     
        for (int j = 0; j < m->num_doses; j++) {
            char t[16];
            snprintf(t, sizeof(t), "%02d:%02d",
                     m->doses[j].hour, m->doses[j].minute);
            strcat(times_buf, t);
            if (j < m->num_doses - 1) {
                strcat(times_buf, "; ");
            }
        }

        fprintf(f, "\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\n",
                m->name, m->dosage, rec_str, time_buf, m->notes);
    }

    fclose(f);
    return 1;
}


