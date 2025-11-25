// src/main.c
#include <stdio.h>
#include <string.h>
#include "core.h"
#include "io.h"
#include "gui.h"

int main(void) {
    MedicationList list;
    init_med_list(&list);

    Medication m = {0};
    snprintf(m.name, MAX_NAME_LEN, "Amoxicillin");
    snprintf(m.dosage, MAX_NAME_LEN, "500 mg");
    m.num_doses = 2;
    m.doses[0].hour = 9;  m.doses[0].minute = 0;
    m.doses[1].hour = 21; m.doses[1].minute = 0;
    snprintf(m.notes, MAX_NOTES_LEN, "Take with food.");
    add_medication(&list, m);

    save_medications("data/meds.txt", &list);
    export_medications_csv("data/meds.csv", &list);

    TimeOfDay now;
    get_current_time(&now);

    Medication due[10];
    int count = get_due_medications(&list, now, due, 10);
    printf("Meds due now: %d\n", count);

    return 0;
}
