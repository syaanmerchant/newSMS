// Medication and CSV export

#include "io.h"
#include <stdio.h>
#include <string.h>
 
int load_medications(const char *filename, MedicationList *list) {
    if (!filename || !list) return 0;
 
    FILE *f = fopen(filename, "r");
    if (!f) {
     // No file yet is not a hard error
        return 0;
    }
    init_med_list(list);

    int count = 0;
    if (fscanf(f, "%d\n", &count) != 1) {
        fclose(f);
        return 0;
    }

    for (int i = 0; i < count; i++) {
        Medication m;
        memset(&m, 0, sizeof(Medication));

        if (fscanf(f, "%d\n", &m.id) != 1) break;

        // Read name
        if (!fgets(m.name, MAX_NAME_LEN, f)) break;
        m.name[strcspn(m.name, "\r\n")] = '\0';

        // Read dosage
        if (!fgets(m.dosage, MAX_NAME_LEN, f)) break;
        m.dosage[strcspn(m.dosage, "\r\n")] = '\0';
     
        if (fscanf(f, "%d\n", &m.num_doses) != 1) break;
        if (m.num_doses < 0) m.num_doses = 0;
        if (m.num_doses > MAX_DOSES_PER_MED) m.num_doses = MAX_DOSES_PER_MED;

        for (int j = 0; j < m.num_doses; j++) {
            if (fscanf(f, "%d %d\n",
                       &m.doses[j].hour,
                       &m.doses[j].minute) != 2) {
                m.doses[j].hour = 0;
                m.doses[j].minute = 0;
            }
        }

        int rec_int = 0;
        int used_int = 0;

        if (fscanf(f, "%d\n", &rec_int) != 1) {
            rec_int = 0;
        }

        if (fscanf(f, "%d\n", &used_int) != 1) {
            used_int = 0;
        }

        m.recurrence = (rec_int == (int)RECURRENCE_ONCE)
                       ? RECURRENCE_ONCE
                       : RECURRENCE_DAILY;
        m.one_time_used = used_int ? 1 : 0;

        if (!fgets(m.notes, MAX_NOTES_LEN, f)) {
            m.notes[0] = '\0';
        } else {
            m.notes[strcspn(m.notes, "\r\n")] = '\0';
        }

        // add into list preserving id
        if (list->count < MAX_MEDICATIONS) {
            list->meds[list->count++] = m;
            if (m.id >= list->next_id) {
                list->next_id = m.id + 1;
            }
        }
    }

    fclose(f);
    return 1;
}


 
int save_medications(const char *filename, MedicationList *list) {
    if (!filename || !list) return 0;
 
    FILE *f = fopen(filename, "w");
    if (!f) return 0;
    
    fprintf(f, "%d\n", list->count);

    for (int i = 0; i < list->count; i++) {
        Medication *m = &list->meds[i];

        fprintf(f, "%d\n", m->id);   
        fprintf(f, "%s\n", m->name);
        fprintf(f, "%s\n", m->dosage);
        fprintf(f, "%d\n", m->num_doses);

        for (int j = 0; j < m->num_doses; j++) {
            fprintf(f, "%d %d\n", m->doses[j].hour, m->doses[j].minute);
        }

        fprintf(f, "%d\n", (int)m->recurrence);
        fprintf(f, "%d\n", m->one_time_used);
        fprintf(f, "%s\n", m->notes);
    }

    fclose(f);
    return 1;
}
 
int export_medications_csv(const char *filename, MedicationList *list) {
    if (!filename || !list) return 0;
 
    FILE *f = fopen(filename, "w");
    if (!f) return 0;

    fprintf(f, "Name,Dosage,Recurrence,Times,Notes\n");

    for (int i = 0; i < list->count; i++) {
        Medication *m = &list->meds[i];

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

