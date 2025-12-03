
/* gtk-based graphical user interface for the medmate application.
 * this file connects the medicationlist core logic to the ui.
 * it handles:
 * - listing medications
 * - adding, editing, and deleting medications
 * - exporting data
 * - showing today's schedule
 * - simple reminder popups
 * - wikipedia lookup for a medication name (via a helper script)
 *
 * author: syaan merchant
 * date: 2025/12/03
 * version: v1.1.1
 */
#include "gui.h"
#include "io.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>

static MedicationList *g_med_list = NULL;
static GtkWidget *g_list_box = NULL;
static GtkWidget *g_main_window = NULL;

/* forward declarations for internal helpers */
static void refresh_med_list(void);
static void on_add_med_clicked(GtkButton *button, gpointer user_data);
static void on_export_clicked(GtkButton *button, gpointer user_data);
static void on_edit_med_clicked(GtkButton *button, gpointer user_data);
static void on_delete_med_clicked(GtkButton *button, gpointer user_data);
static void on_schedule_clicked(GtkButton *button, gpointer user_data);
static gboolean reminder_check_callback(gpointer user_data);
static GtkWidget* create_med_row(Medication *m);
static void show_todays_schedule(void);
static void on_wiki_lookup_clicked(GtkButton *button, gpointer user_data);

/* ---------- helpers ---------- */

/* single row widget for one medication entry. */
static GtkWidget* create_med_row(Medication *m) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    const char *rec_str = (m->recurrence == RECURRENCE_ONCE)
                          ? "one-time"
                          : "daily";

    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s — %s (%s)",
             m->name, m->dosage, rec_str);

    GtkWidget *label = gtk_label_new(buffer);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(row), label, TRUE, TRUE, 5);

    GtkWidget *edit_btn = gtk_button_new_with_label("Edit");
    g_object_set_data(G_OBJECT(edit_btn), "med-id", GINT_TO_POINTER(m->id));
    g_signal_connect(edit_btn, "clicked", G_CALLBACK(on_edit_med_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(row), edit_btn, FALSE, FALSE, 2);

    GtkWidget *delete_btn = gtk_button_new_with_label("Delete");
    g_object_set_data(G_OBJECT(delete_btn), "med-id", GINT_TO_POINTER(m->id));
    g_signal_connect(delete_btn, "clicked", G_CALLBACK(on_delete_med_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(row), delete_btn, FALSE, FALSE, 2);

    return row;
}

/* clears and repopulates the medication list ui. */
static void refresh_med_list(void) {
    if (!g_list_box || !g_med_list) return;

    GList *children = gtk_container_get_children(GTK_CONTAINER(g_list_box));
    for (GList *iter = children; iter != NULL; iter = iter->next) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    for (int i = 0; i < g_med_list->count; i++) {
        GtkWidget *row = create_med_row(&g_med_list->meds[i]);
        gtk_box_pack_start(GTK_BOX(g_list_box), row, FALSE, FALSE, 5);
    }

    gtk_widget_show_all(g_list_box);
}

/* opens a dialog asking for a medication name, then runs a python helper
 * script that looks it up on wikipedia. the result is shown in a message dialog.
 */
static void on_wiki_lookup_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;

    //  medication name
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Wikipedia Lookup",
        GTK_WINDOW(g_main_window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Search", GTK_RESPONSE_OK,
        NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(content), box);

    GtkWidget *label = gtk_label_new("Enter a medication name:");
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 2);

    GtkWidget *entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 2);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *name = gtk_entry_get_text(GTK_ENTRY(entry));

        if (name && *name) {
            char command[512];
            snprintf(command, sizeof(command),
                     "python3 scripts/wiki_lookup \"%s\" > wiki_tmp.txt",
                     name);

            /* run the python helper script */
            system(command);

            // read result of the lookup helper script
            FILE *f = fopen("wiki_tmp.txt", "r");
            if (f) {
                char buffer[2048];
                fread(buffer, 1, sizeof(buffer)-1, f);
                fclose(f);
                buffer[sizeof(buffer)-1] = '\0';

                GtkWidget *msg = gtk_message_dialog_new(
                    GTK_WINDOW(g_main_window),
                    GTK_DIALOG_MODAL,
                    GTK_MESSAGE_INFO,
                    GTK_BUTTONS_OK,
                    "%s",
                    buffer[0] ? buffer : "No information found."
                );
                gtk_dialog_run(GTK_DIALOG(msg));
                gtk_widget_destroy(msg);
            }
        }
    }

    gtk_widget_destroy(dialog);
}


/* ---------- reminder timer ---------- */

/* timer callback that checks if any medications are due and shows a popup.
 * runs periodically (once per minute). */
static gboolean reminder_check_callback(gpointer user_data) {
    (void)user_data;

    if (!g_med_list || g_med_list->count == 0) {
        return TRUE;
    }

    TimeOfDay now;
    get_current_time(&now);

    Medication due[10];
    int count = get_due_medications(g_med_list, now, due, 10);

    if (count > 0) {
        Medication *first = &due[0];

        GtkWidget *msg = gtk_message_dialog_new(
            GTK_WINDOW(g_main_window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
            "It is time to take:\n%s (%s)",
            first->name,
            first->dosage
        );
        gtk_dialog_run(GTK_DIALOG(msg));
        gtk_widget_destroy(msg);

        // If this was a one-time med, mark as used
        Medication *orig = find_med_by_id(g_med_list, first->id);
        if (orig && orig->recurrence == RECURRENCE_ONCE) {
            orig->one_time_used = 1;
            save_medications("data/meds.txt", g_med_list);
            refresh_med_list();
        }
    }

    return TRUE;
}

/* ---------- add medication dialog ---------- */

/* opens a dialog that lets the user create a new medication and add it
 * to the shared list. */
static void on_add_med_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Add Medication",
        GTK_WINDOW(g_main_window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Add", GTK_RESPONSE_OK,
        NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_container_add(GTK_CONTAINER(content), grid);

    /* name */
    GtkWidget *lbl_name = gtk_label_new("Name:");
    GtkWidget *entry_name = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), lbl_name, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_name, 1, 0, 1, 1);

    /* dosage */
    GtkWidget *lbl_dosage = gtk_label_new("Dosage:");
    GtkWidget *entry_dosage = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), lbl_dosage, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_dosage, 1, 1, 1, 1);

    /* recurrence (daily / one-time) */
    GtkWidget *lbl_rec = gtk_label_new("Recurrence:");
    GtkWidget *rec_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *rb_daily = gtk_radio_button_new_with_label(NULL, "Daily");
    GtkWidget *rb_once  = gtk_radio_button_new_with_label_from_widget(
                              GTK_RADIO_BUTTON(rb_daily),
                              "One-time (today)");
    gtk_box_pack_start(GTK_BOX(rec_box), rb_daily, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(rec_box), rb_once, FALSE, FALSE, 2);

    gtk_grid_attach(GTK_GRID(grid), lbl_rec, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), rec_box, 1, 2, 1, 1);

    /* doses per day */
    GtkWidget *lbl_doses = gtk_label_new("Doses per day (1–4):");
    GtkAdjustment *adj = gtk_adjustment_new(1, 1, 4, 1, 1, 0);
    GtkWidget *spin_doses = gtk_spin_button_new(adj, 1, 0);
    gtk_grid_attach(GTK_GRID(grid), lbl_doses, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), spin_doses, 1, 3, 1, 1);

    /* time entries */
    GtkWidget *lbl_time1 = gtk_label_new("Time 1 (HH:MM):");
    GtkWidget *entry_time1 = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_time1), "09:00");

    GtkWidget *lbl_time2 = gtk_label_new("Time 2 (HH:MM):");
    GtkWidget *entry_time2 = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_time2), "13:00");

    GtkWidget *lbl_time3 = gtk_label_new("Time 3 (HH:MM):");
    GtkWidget *entry_time3 = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_time3), "17:00");

    GtkWidget *lbl_time4 = gtk_label_new("Time 4 (HH:MM):");
    GtkWidget *entry_time4 = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_time4), "21:00");

    gtk_grid_attach(GTK_GRID(grid), lbl_time1, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_time1, 1, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), lbl_time2, 0, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_time2, 1, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), lbl_time3, 0, 6, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_time3, 1, 6, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), lbl_time4, 0, 7, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_time4, 1, 7, 1, 1);

    /* notes */
    GtkWidget *lbl_notes = gtk_label_new("Notes:");
    GtkWidget *entry_notes = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), lbl_notes, 0, 8, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_notes, 1, 8, 1, 1);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *name = gtk_entry_get_text(GTK_ENTRY(entry_name));
        const char *dosage = gtk_entry_get_text(GTK_ENTRY(entry_dosage));
        const char *notes = gtk_entry_get_text(GTK_ENTRY(entry_notes));
        int num_doses = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_doses));

        RecurrenceType rec =
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_once))
            ? RECURRENCE_ONCE
            : RECURRENCE_DAILY;

        /* basic validation for name and dosage */
        if (!name || strlen(name) == 0 || !dosage || strlen(dosage) == 0) {
            GtkWidget *msg = gtk_message_dialog_new(
                GTK_WINDOW(g_main_window),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_OK,
                "Name and dosage cannot be empty.");
            gtk_dialog_run(GTK_DIALOG(msg));
            gtk_widget_destroy(msg);
            gtk_widget_destroy(dialog);
            return;
        }

        Medication m;
        memset(&m, 0, sizeof(Medication));
        snprintf(m.name, MAX_NAME_LEN, "%s", name);
        snprintf(m.dosage, MAX_NAME_LEN, "%s", dosage);
        snprintf(m.notes, MAX_NOTES_LEN, "%s", notes ? notes : "");
        m.num_doses = num_doses;
        m.recurrence = rec;
        m.one_time_used = 0;

        GtkWidget *time_entries[4] = {
            entry_time1, entry_time2, entry_time3, entry_time4
        };

        for (int i = 0; i < num_doses; i++) {
            const char *tstr = gtk_entry_get_text(GTK_ENTRY(time_entries[i]));
            int h, min;
            if (sscanf(tstr, "%d:%d", &h, &min) != 2 ||
                h < 0 || h > 23 || min < 0 || min > 59) {
                GtkWidget *msg = gtk_message_dialog_new(
                    GTK_WINDOW(g_main_window),
                    GTK_DIALOG_MODAL,
                    GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_OK,
                    "Invalid time in slot %d. Use HH:MM.",
                    i + 1);
                gtk_dialog_run(GTK_DIALOG(msg));
                gtk_widget_destroy(msg);
                gtk_widget_destroy(dialog);
                return;
            }
            m.doses[i].hour = h;
            m.doses[i].minute = min;
        }

        add_medication(g_med_list, m);
        save_medications("data/meds.txt", g_med_list);
        refresh_med_list();
    }

    gtk_widget_destroy(dialog);
}

/* ---------- edit / delete ---------- */

/* opens a dialog to edit an existing medication. */
static void on_edit_med_clicked(GtkButton *button, gpointer user_data) {
    (void)user_data;

    int id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "med-id"));
    Medication *m = find_med_by_id(g_med_list, id);
    if (!m) return;

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Edit Medication",
        GTK_WINDOW(g_main_window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Save", GTK_RESPONSE_OK,
        NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_container_add(GTK_CONTAINER(content), grid);

    /* name */
    GtkWidget *lbl_name = gtk_label_new("Name:");
    GtkWidget *entry_name = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry_name), m->name);
    gtk_grid_attach(GTK_GRID(grid), lbl_name, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_name, 1, 0, 1, 1);

    /* dosage */
    GtkWidget *lbl_dosage = gtk_label_new("Dosage:");
    GtkWidget *entry_dosage = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry_dosage), m->dosage);
    gtk_grid_attach(GTK_GRID(grid), lbl_dosage, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_dosage, 1, 1, 1, 1);

    /* recurrence */
    GtkWidget *lbl_rec = gtk_label_new("Recurrence:");
    GtkWidget *rec_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *rb_daily = gtk_radio_button_new_with_label(NULL, "Daily");
    GtkWidget *rb_once  = gtk_radio_button_new_with_label_from_widget(
                              GTK_RADIO_BUTTON(rb_daily),
                              "One-time (today)");

    if (m->recurrence == RECURRENCE_ONCE) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_once), TRUE);
    } else {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_daily), TRUE);
    }

    gtk_box_pack_start(GTK_BOX(rec_box), rb_daily, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(rec_box), rb_once, FALSE, FALSE, 2);
    gtk_grid_attach(GTK_GRID(grid), lbl_rec, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), rec_box, 1, 2, 1, 1);

    /* doses per day */
    GtkWidget *lbl_doses = gtk_label_new("Doses per day (1–4):");
    GtkAdjustment *adj = gtk_adjustment_new(m->num_doses, 1, 4, 1, 1, 0);
    GtkWidget *spin_doses = gtk_spin_button_new(adj, 1, 0);
    gtk_grid_attach(GTK_GRID(grid), lbl_doses, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), spin_doses, 1, 3, 1, 1);

    /* times */
    GtkWidget *lbl_time1 = gtk_label_new("Time 1 (HH:MM):");
    GtkWidget *entry_time1 = gtk_entry_new();
    GtkWidget *lbl_time2 = gtk_label_new("Time 2 (HH:MM):");
    GtkWidget *entry_time2 = gtk_entry_new();
    GtkWidget *lbl_time3 = gtk_label_new("Time 3 (HH:MM):");
    GtkWidget *entry_time3 = gtk_entry_new();
    GtkWidget *lbl_time4 = gtk_label_new("Time 4 (HH:MM):");
    GtkWidget *entry_time4 = gtk_entry_new();

    char buf[16];
    if (m->num_doses > 0) {
        snprintf(buf, sizeof(buf), "%02d:%02d", m->doses[0].hour, m->doses[0].minute);
        gtk_entry_set_text(GTK_ENTRY(entry_time1), buf);
    }
    if (m->num_doses > 1) {
        snprintf(buf, sizeof(buf), "%02d:%02d", m->doses[1].hour, m->doses[1].minute);
        gtk_entry_set_text(GTK_ENTRY(entry_time2), buf);
    }
    if (m->num_doses > 2) {
        snprintf(buf, sizeof(buf), "%02d:%02d", m->doses[2].hour, m->doses[2].minute);
        gtk_entry_set_text(GTK_ENTRY(entry_time3), buf);
    }
    if (m->num_doses > 3) {
        snprintf(buf, sizeof(buf), "%02d:%02d", m->doses[3].hour, m->doses[3].minute);
        gtk_entry_set_text(GTK_ENTRY(entry_time4), buf);
    }

    gtk_grid_attach(GTK_GRID(grid), lbl_time1, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_time1, 1, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), lbl_time2, 0, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_time2, 1, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), lbl_time3, 0, 6, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_time3, 1, 6, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), lbl_time4, 0, 7, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_time4, 1, 7, 1, 1);

    /* notes */
    GtkWidget *lbl_notes = gtk_label_new("Notes:");
    GtkWidget *entry_notes = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry_notes), m->notes);
    gtk_grid_attach(GTK_GRID(grid), lbl_notes, 0, 8, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_notes, 1, 8, 1, 1);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *name = gtk_entry_get_text(GTK_ENTRY(entry_name));
        const char *dosage = gtk_entry_get_text(GTK_ENTRY(entry_dosage));
        const char *notes = gtk_entry_get_text(GTK_ENTRY(entry_notes));
        int num_doses = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_doses));

        RecurrenceType rec =
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rb_once))
            ? RECURRENCE_ONCE
            : RECURRENCE_DAILY;

        /* basic validation again in case user clears fields */
        if (!name || strlen(name) == 0 || !dosage || strlen(dosage) == 0) {
            GtkWidget *msg = gtk_message_dialog_new(
                GTK_WINDOW(g_main_window),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_OK,
                "Name and dosage cannot be empty.");
            gtk_dialog_run(GTK_DIALOG(msg));
            gtk_widget_destroy(msg);
            gtk_widget_destroy(dialog);
            return;
        }

        snprintf(m->name, MAX_NAME_LEN, "%s", name);
        snprintf(m->dosage, MAX_NAME_LEN, "%s", dosage);
        snprintf(m->notes, MAX_NOTES_LEN, "%s", notes ? notes : "");
        m->num_doses = num_doses;
        m->recurrence = rec;

        /* if it becomes a one-time med again, reset the used flag */
        if (m->recurrence == RECURRENCE_ONCE && m->one_time_used != 0) {
            m->one_time_used = 0;
        }

        GtkWidget *time_entries[4] = {
            entry_time1, entry_time2, entry_time3, entry_time4
        };

        for (int i = 0; i < num_doses; i++) {
            const char *tstr = gtk_entry_get_text(GTK_ENTRY(time_entries[i]));
            int h, min;
            if (sscanf(tstr, "%d:%d", &h, &min) != 2 ||
                h < 0 || h > 23 || min < 0 || min > 59) {
                GtkWidget *msg = gtk_message_dialog_new(
                    GTK_WINDOW(g_main_window),
                    GTK_DIALOG_MODAL,
                    GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_OK,
                    "Invalid time in slot %d. Use HH:MM.",
                    i + 1);
                gtk_dialog_run(GTK_DIALOG(msg));
                gtk_widget_destroy(msg);
                gtk_widget_destroy(dialog);
                return;
            }
            m->doses[i].hour = h;
            m->doses[i].minute = min;
        }

        save_medications("data/meds.txt", g_med_list);
        refresh_med_list();
    }

    gtk_widget_destroy(dialog);
}
/* confirms and deletes a medication selected from the list. */
static void on_delete_med_clicked(GtkButton *button, gpointer user_data) {
    (void)user_data;

    int id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "med-id"));
    Medication *m = find_med_by_id(g_med_list, id);
    if (!m) return;

    GtkWidget *confirm = gtk_message_dialog_new(
        GTK_WINDOW(g_main_window),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_WARNING,
        GTK_BUTTONS_OK_CANCEL,
        "Delete medication \"%s\"?", m->name);

    int resp = gtk_dialog_run(GTK_DIALOG(confirm));
    gtk_widget_destroy(confirm);

    if (resp == GTK_RESPONSE_OK) {
        remove_medication(g_med_list, id);
        save_medications("data/meds.txt", g_med_list);
        refresh_med_list();
    }
}

/* ---------- export ---------- */

/* exports the current medication list as a csv file. */
static void on_export_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;

    if (export_medications_csv("data/meds_export.csv", g_med_list)) {
        GtkWidget *msg = gtk_message_dialog_new(
            GTK_WINDOW(g_main_window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
            "Exported medications to data/meds_export.csv");
        gtk_dialog_run(GTK_DIALOG(msg));
        gtk_widget_destroy(msg);
    } else {
        GtkWidget *msg = gtk_message_dialog_new(
            GTK_WINDOW(g_main_window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "Failed to export CSV.");
        gtk_dialog_run(GTK_DIALOG(msg));
        gtk_widget_destroy(msg);
    }
}

/* ---------- today's schedule ---------- */

typedef struct {
    Medication *med;
    TimeOfDay time;
    int minutes_diff;
} ScheduleEntry;

/* shows a dialog with today's medication schedule. */
static void show_todays_schedule(void) {
    if (!g_med_list || g_med_list->count == 0) {
        GtkWidget *msg = gtk_message_dialog_new(
            GTK_WINDOW(g_main_window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
            "No medications in the list.");
        gtk_dialog_run(GTK_DIALOG(msg));
        gtk_widget_destroy(msg);
        return;
    }

    ScheduleEntry entries[MAX_MEDICATIONS * MAX_DOSES_PER_MED];
    int entry_count = 0;

    TimeOfDay now;
    get_current_time(&now);
    int now_total = now.hour * 60 + now.minute;

    for (int i = 0; i < g_med_list->count; i++) {
        Medication *m = &g_med_list->meds[i];

        /* skip one-time meds that have already been used */
        if (m->recurrence == RECURRENCE_ONCE && m->one_time_used) {
            continue;
        }

        /* add doses to the schedule */
        for (int j = 0; j < m->num_doses; j++) {
            if (entry_count >= MAX_MEDICATIONS * MAX_DOSES_PER_MED) break;

            ScheduleEntry e;
            e.med = m;
            e.time = m->doses[j];

            int dose_total = e.time.hour * 60 + e.time.minute;
            e.minutes_diff = dose_total - now_total;

            entries[entry_count++] = e;
        }
    }

    if (entry_count == 0) {
        GtkWidget *msg = gtk_message_dialog_new(
            GTK_WINDOW(g_main_window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
            "No doses scheduled.");
        gtk_dialog_run(GTK_DIALOG(msg));
        gtk_widget_destroy(msg);
        return;
    }

    /* simple sort by time of day (ascending) */
    for (int i = 0; i < entry_count - 1; i++) {
        for (int j = i + 1; j < entry_count; j++) {
            int ti = entries[i].time.hour * 60 + entries[i].time.minute;
            int tj = entries[j].time.hour * 60 + entries[j].time.minute;
            if (tj < ti) {
                ScheduleEntry tmp = entries[i];
                entries[i] = entries[j];
                entries[j] = tmp;
            }
        }
    }

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Today's Schedule",
        GTK_WINDOW(g_main_window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scroller, 400, 300);
    gtk_container_add(GTK_CONTAINER(content), scroller);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(scroller), vbox);

    for (int i = 0; i < entry_count; i++) {
        ScheduleEntry *e = &entries[i];
        int dose_total = e->time.hour * 60 + e->time.minute;
        int diff = dose_total - now_total;

        const char *rec_str = (e->med->recurrence == RECURRENCE_ONCE)
                              ? "one-time"
                              : "daily";

        char line[512];

        if (diff >= 0 && diff <= 60) {
            snprintf(line, sizeof(line),
                     "<b>%02d:%02d — %s (%s, %s) — in %d min</b>",
                     e->time.hour, e->time.minute,
                     e->med->name, e->med->dosage, rec_str,
                     diff);
        } else if (diff < 0) {
            snprintf(line, sizeof(line),
                     "%02d:%02d — %s (%s, %s) — %d min ago",
                     e->time.hour, e->time.minute,
                     e->med->name, e->med->dosage, rec_str,
                     -diff);
        } else {
            snprintf(line, sizeof(line),
                     "%02d:%02d — %s (%s, %s)",
                     e->time.hour, e->time.minute,
                     e->med->name, e->med->dosage, rec_str);
        }

        GtkWidget *label = gtk_label_new(NULL);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0);
        gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
        gtk_label_set_markup(GTK_LABEL(label), line);
        gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 3);
    }

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void on_schedule_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    show_todays_schedule();
}

/* ---------- main gui entry ---------- */

/* sets up the main window and starts the gtk main loop. */
int run_gui(MedicationList *list) {
    g_med_list = list;

    gtk_init(NULL, NULL);

    g_main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(g_main_window), "MedMate");
    gtk_window_set_default_size(GTK_WINDOW(g_main_window), 600, 400);

    g_signal_connect(g_main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(g_main_window), vbox);

    GtkWidget *top_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), top_bar, FALSE, FALSE, 5);

    GtkWidget *add_button = gtk_button_new_with_label("Add Medication");
    GtkWidget *schedule_button = gtk_button_new_with_label("Today's Schedule");
    GtkWidget *wiki_button = gtk_button_new_with_label("Wikipedia Lookup");
    GtkWidget *export_button = gtk_button_new_with_label("Export CSV");

    g_signal_connect(add_button, "clicked", G_CALLBACK(on_add_med_clicked), NULL);
    g_signal_connect(schedule_button, "clicked", G_CALLBACK(on_schedule_clicked), NULL);
    g_signal_connect(wiki_button, "clicked", G_CALLBACK(on_wiki_lookup_clicked), NULL);
    g_signal_connect(export_button, "clicked", G_CALLBACK(on_export_clicked), NULL);

    gtk_box_pack_start(GTK_BOX(top_bar), add_button, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(top_bar), schedule_button, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(top_bar), wiki_button, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(top_bar), export_button, TRUE, TRUE, 5);

    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), scroller, TRUE, TRUE, 5);

    g_list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(scroller), g_list_box);

    refresh_med_list();

     /* check for reminders once every 60 seconds */
    g_timeout_add_seconds(60, reminder_check_callback, NULL);

    gtk_widget_show_all(g_main_window);
    gtk_main();
    return 0;
}