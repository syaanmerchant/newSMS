#include <stdio.h>
#include <string.h>
#include "core.h"
#include "io.h"
#include "gui.h"

static void print_help(const char *progname) {
    printf("MedMate - Medication Reminder (C Project)\n");
    printf("Usage:\n");
    printf("  %s            Start the GUI (stub for now)\n", progname);
    printf("  %s --help     Show this help message\n", progname);
    printf("  %s --export   Export medications to CSV (stub)\n", progname);
}

int main(int argc, char *argv[]) {
    MedicationList list;
    init_med_list(&list);

    // Try to load existing data (stub)
    load_medications("data/meds.txt", &list);

    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[1], "--export") == 0) {
            export_medications_csv("data/meds_export.csv", &list);
            return 0;
        } else {
            printf("Unknown option: %s\n", argv[1]);
            printf("Use --help for usage.\n");
            return 1;
        }
    }

        /*TEMP: Add a med to test save/load
    Medication m = {0};
    snprintf(m.name, MAX_NAME_LEN, "TestMed");
    snprintf(m.dosage, MAX_NAME_LEN, "100 mg");
    m.num_doses = 1;
    m.doses[0].hour = 9;
    m.doses[0].minute = 30;
    snprintf(m.notes, MAX_NOTES_LEN, "Sample notes.");
*/
    //add_medication(&list, m);


    run_gui(&list);

    // On exit, save (stub)
    save_medications("data/meds.txt", &list);

    return 0;
}