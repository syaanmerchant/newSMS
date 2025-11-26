#include "io.h"
#include <stdio.h>
#include <string.h>
 
int load_medications(const char *filename, MedicationList *list) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        // File might not exist yet, that's OK.
        init_med_list(list);
        return 1;
    }
    init_med_list(list);

    int total;

    if (fscanf(f, "%d\n", &total) != 1) {
        fclose(f);
        return 0;
    }

    for (int i = 0; i < total; i++) {
        Medication m;
        memset(&m, 0, sizeof(Medication));

        // Read name
        if (!fgets(m.name, MAX_NAME_LEN, f)) break;
        m.name[strcspn(m.name, "\n")] = '\0';

        // Read dosage
        if (!fgets(m.dosage, MAX_NAME_LEN, f)) break;
        m.dosage[strcspn(m.dosage, "\n")] = '\0';

        // Read number of doses
        if (fscanf(f, "%d\n", &m.num_doses) != 1) break;

        // Read times
        for (int j = 0; j < m.num_doses; j++) {
            fscanf(f, "%d %d\n", &m.doses[j].hour, &m.doses[j].minute);
        }

        // Read notes (single line)
        if (!fgets(m.notes, MAX_NOTES_LEN, f)) break;
        m.notes[strcspn(m.notes, "\n")] = '\0';

        add_medication(list, m);
    }

    fclose(f);
    return 1;
}
 
int save_medications(const char *filename, MedicationList *list) {
    FILE *f = fopen(filename, "w");
    if (!f) return 0;
    
    fprintf(f, "%d\n", list->count);

    for (int i = 0; i < list->count; i++) {
        Medication *m = &list->meds[i];

        fprintf(f, "%s\n", m->name);
        fprintf(f, "%s\n", m->dosage);
        fprintf(f, "%d\n", m->num_doses);

        for (int j = 0; j < m->num_doses; j++) {
            fprintf(f, "%02d %02d\n", m->doses[j].hour, m->doses[j].minute);
        }

        fprintf(f, "%s\n", m->notes);
    }

    fclose(f);
    return 1;
}
 
int export_medications_csv(const char *filename, MedicationList *list) {
    FILE *f = fopen(filename, "w");
    if (!f) return 0;

    fprintf(f, "Name,Dosage,DoseTimes,Notes\n");

    for (int i = 0; i < list->count; i++) {
        Medication *m = &list->meds[i];

        // Combine times into "09:00/21:00"
        char time_buf[128] = "";

        for (int j = 0; j < m->num_doses; j++) {
            char t[16];
            snprintf(t, sizeof(t), "%02d:%02d",
                     m->doses[j].hour, m->doses[j].minute);

            strcat(time_buf, t);
            if (j < m->num_doses - 1) {
                strcat(time_buf, "/");
            }
        }

        fprintf(f, "\"%s\",\"%s\",\"%s\",\"%s\"\n",
                m->name, m->dosage, time_buf, m->notes);
    }

    fclose(f);
    return 1;
}
