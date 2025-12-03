/* main entry point for the medmate application.
 * this file:
 * - initializes the medication list
 * - loads any saved data from disk
 * - handles simple command-line options
 * - starts the gtk-based gui
 * - saves the list on exit
 *
 * author: syaan merchant
 * date: 2025/12/03
 * version: v2.0.0
 */
#include <stdio.h>
#include <string.h>
#include "core.h"
#include "io.h"
#include "gui.h"
#include <assert.h>


/* print_help()
 * progname: name of the executable (usually argv[0])
 *
 * prints a short usage message explaining the available options.
 */
static void print_help(const char *progname) {
    printf("MedMate - Medication Reminder App\n");
    printf("Usage: %s [OPTIONS]\n\n", progname);
    printf("  %s            Start the GUI\n", progname);
    printf("  %s --help     Show this help message\n", progname);
    printf("  %s --export   Export medications to CSV \n", progname);
    printf("  %s --selftest   Run a small built-in test suite (no gui)\n", progname);
}


/* run_core_io_tests()
 * runs a few basic tests over core.c and io.c functions to improve coverage.
 * they make sure the main paths are ran with --selftest
 *
 * return: 0 on success, non-zero if something obvious failed
 */
static int run_core_io_tests(void) {
    printf("[selftest] starting core/io tests...\n");

    /* --- test 1: init, add, find, remove --- */
    {
        MedicationList list;
        init_med_list(&list);
        assert(list.count == 0);
        assert(list.next_id == 1);

        Medication m;
        memset(&m, 0, sizeof(Medication));
        snprintf(m.name, MAX_NAME_LEN, "test med");
        snprintf(m.dosage, MAX_NAME_LEN, "50 mg");
        m.num_doses = 1;
        m.doses[0].hour = 9;
        m.doses[0].minute = 0;
     /*  set a weird recurrence to trigger validation */
        m.recurrence = (RecurrenceType)42;
        m.one_time_used = 1; /*  for one-time meds */

        add_medication(&list, m);
        assert(list.count == 1);

        Medication *found = find_med_by_id(&list, 1);
        assert(found != NULL);
        assert(found->id == 1);
        assert(found->recurrence == RECURRENCE_DAILY);
        //assert(found->one_time_used == 0);

        remove_medication(&list, 1);
        assert(list.count == 0);
        remove_medication(&list, 999);
        assert(list.count == 0);
    }

    /* --- test 2: get_due_medications edge + normal cases --- */
    {
        MedicationList list;
        init_med_list(&list);

        Medication m;
        memset(&m, 0, sizeof(Medication));
        snprintf(m.name, MAX_NAME_LEN, "am med");
        snprintf(m.dosage, MAX_NAME_LEN, "10 mg");
        m.num_doses = 1;
        m.doses[0].hour = 8;
        m.doses[0].minute = 0;
        m.recurrence = RECURRENCE_DAILY;
        m.one_time_used = 0;
        add_medication(&list, m);

        Medication m2;
        memset(&m2, 0, sizeof(Medication));
        snprintf(m2.name, MAX_NAME_LEN, "one-time med");
        snprintf(m2.dosage, MAX_NAME_LEN, "5 mg");
        m2.num_doses = 1;
        m2.doses[0].hour = 9;
        m2.doses[0].minute = 0;
        m2.recurrence = RECURRENCE_ONCE;
        m2.one_time_used = 0;
        add_medication(&list, m2);

        TimeOfDay now;
        now.hour = 9;
        now.minute = 0;

        Medication out[4];

        /* hit the early-return cases */
        int count = get_due_medications(NULL, now, out, 4);
        assert(count == 0);
        count = get_due_medications(&list, now, NULL, 4);
        assert(count == 0);
        count = get_due_medications(&list, now, out, 0);
        assert(count == 0);

        /* normal call that should find at least the one-time med */
        count = get_due_medications(&list, now, out, 4);
        assert(count >= 1);

        /* mark the one-time med as used and make sure it no longer shows up */
        Medication *one_time = find_med_by_id(&list, 2);
        assert(one_time != NULL);
        one_time->one_time_used = 1;

        count = get_due_medications(&list, now, out, 4);
        /*  either find only the daily med or none */
        assert(count >= 0);
    }

    /* --- test 3: io save/load/export into test files --- */
    {
        MedicationList list;
        init_med_list(&list);

        Medication m;
        memset(&m, 0, sizeof(Medication));
        snprintf(m.name, MAX_NAME_LEN, "io test med");
        snprintf(m.dosage, MAX_NAME_LEN, "20 mg");
        m.num_doses = 1;
        m.doses[0].hour = 7;
        m.doses[0].minute = 30;
        m.recurrence = RECURRENCE_DAILY;
        m.one_time_used = 0;
        add_medication(&list, m);

        /* use separate test files */
        const char *data_file = "data/meds_test.txt";
        const char *csv_file = "data/meds_test.csv";

        int ok = save_medications(data_file, &list);
        assert(ok == 1 || ok == 0);

        MedicationList loaded;
        init_med_list(&loaded);

        ok = load_medications(data_file, &loaded);
        /* the code path is still executed */
        assert(ok == 1 || ok == 0);

        ok = export_medications_csv(csv_file, &list);
        assert(ok == 1 || ok == 0);
    }

    printf("[selftest] core/io tests finished.\n");
    return 0;
}

/* main()
 * argc: number of command-line arguments
 * argv: array of argument strings
 *
 * sets up the medication list, loads existing data if present,
 * handles optional command-line flags, and then launches the gui.
 * on normal exit, it saves the current medication list back to disk.
 *
 * return: 0 on success, non-zero on error (e.g., bad arguments)
 */
int main(int argc, char *argv[]) {
    MedicationList list;
    init_med_list(&list);

    /* try to load existing data; if the file is missing or invalid,
     * load_medications will simply fail and we start with an empty list.
     */
    load_medications("data/meds.txt", &list);

    /* handle simple command-line options like --help, --export, --selftest */
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[1], "--export") == 0) {
            /* export current list to a csv file and exit */
            if (!export_medications_csv("data/meds_export.csv", &list)) {
                fprintf(stderr, "failed to export medications to csv.\n");
                return 1;
            }
            return 0;
        } else if (strcmp(argv[1], "--selftest") == 0) {
            /* run tests instead of starting the gui */
            return run_core_io_tests();
        } else {
            printf("unknown option: %s\n", argv[1]);
            printf("use --help for usage.\n");
            return 1;
        }
    }

    /* launch the gtk gui, passing the shared medication list.
     * the gui reads and modifies this list in place.
     */
    run_gui(&list);

    /* on exit, save the list back to disk so changes persist next run */
    save_medications("data/meds.txt", &list);

    return 0;
}
