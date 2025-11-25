#include "io.h"
#include <stdio.h>
 
int load_medications(const char *filename, MedicationList *list) {
    // For now, do nothing and pretend success.
    // We'll fill this in later.
    (void)filename; // avoid unused parameter warnings
    if (list) {
        init_med_list(list);
    }
    return 1;
}
 
int save_medications(const char *filename, MedicationList *list) {
    // For now, just print a message.
    (void)list;
    printf("Saving medications to %s (stub).\n", filename);
    return 1;
}
 
int export_medications_csv(const char *filename, MedicationList *list) {
    // For now, just print a message.
    (void)list;
    printf("Exporting medications to CSV: %s (stub).\n", filename);
    return 1;
}
