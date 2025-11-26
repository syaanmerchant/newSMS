#include "gui.h"
#include "io.h"          // for save_medications, export_medications_csv
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>

static MedicationList *g_med_list = NULL;
static GtkWidget *g_list_box = NULL;
static GtkWidget *g_main_window = NULL;

// ---------- Helpers ----------

static GtkWidget* create_med_row(Medication *m) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s — %s", m->name, m->dosage);

    GtkWidget *label = gtk_label_new(buffer);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    return label;
}

static void refresh_med_list(void) {
    if (!g_list_box || !g_med_list) return;

    // Remove existing children
    GList *children = gtk_container_get_children(GTK_CONTAINER(g_list_box));
    for (GList *iter = children; iter != NULL; iter = iter->next) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    // Re-add from list
    for (int i = 0; i < g_med_list->count; i++) {
        GtkWidget *row = create_med_row(&g_med_list->meds[i]);
        gtk_box_pack_start(GTK_BOX(g_list_box), row, FALSE, FALSE, 5);
    }

    gtk_widget_show_all(g_list_box);
}

// ---------- Reminder timer ----------

static gboolean reminder_check_callback(gpointer user_data) {
    (void)user_data;

    if (!g_med_list || g_med_list->count == 0) {
        return TRUE; // keep timer running
    }

    TimeOfDay now;
    get_current_time(&now);

    Medication due[10];
    int count = get_due_medications(g_med_list, now, due, 10);

    if (count > 0) {
        GtkWidget *msg = gtk_message_dialog_new(
            GTK_WINDOW(g_main_window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
            "It is time to take:\n%s (%s)",
            due[0].name,
            due[0].dosage
        );
        gtk_dialog_run(GTK_DIALOG(msg));
        gtk_widget_destroy(msg);
    }

    return TRUE; // keep timer running
}

// ---------- Callbacks ----------

// Full-feature Add Medication dialog
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

    // Name
    GtkWidget *lbl_name = gtk_label_new("Name:");
    GtkWidget *entry_name = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), lbl_name, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_name, 1, 0, 1, 1);

    // Dosage
    GtkWidget *lbl_dosage = gtk_label_new("Dosage:");
    GtkWidget *entry_dosage = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), lbl_dosage, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_dosage, 1, 1, 1, 1);

    // Number of doses per day (1–4)
    GtkWidget *lbl_doses = gtk_label_new("Doses per day (1–4):");
    GtkAdjustment *adj = gtk_adjustment_new(1, 1, 4, 1, 1, 0);
    GtkWidget *spin_doses = gtk_spin_button_new(adj, 1, 0);
    gtk_grid_attach(GTK_GRID(grid), lbl_doses, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), spin_doses, 1, 2, 1, 1);

    // Time entries (up to 4)
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

    gtk_grid_attach(GTK_GRID(grid), lbl_time1, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_time1, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), lbl_time2, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_time2, 1, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), lbl_time3, 0, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_time3, 1, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), lbl_time4, 0, 6, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_time4, 1, 6, 1, 1);

    // Notes
    GtkWidget *lbl_notes = gtk_label_new("Notes:");
    GtkWidget *entry_notes = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), lbl_notes, 0, 7, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_notes, 1, 7, 1, 1);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *name = gtk_entry_get_text(GTK_ENTRY(entry_name));
        const char *dosage = gtk_entry_get_text(GTK_ENTRY(entry_dosage));
        const char *notes = gtk_entry_get_text(GTK_ENTRY(entry_notes));
        int num_doses = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_doses));

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

        // Get times
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
                    "Invalid time in slot %d. Please use HH:MM (00–23:00–59).",
                    i + 1);
                gtk_dialog_run(GTK_DIALOG(msg));
                gtk_widget_destroy(msg);
                gtk_widget_destroy(dialog);
                return;
            }
            m.doses[i].hour = h;
            m.doses[i].minute = min;
        }

        // Add to list, save, refresh GUI
        add_medication(g_med_list, m);
        save_medications("data/meds.txt", g_med_list);
        refresh_med_list();
    }

    gtk_widget_destroy(dialog);
}

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

// ---------- Main GUI entry ----------

int run_gui(MedicationList *list) {
    g_med_list = list;

    gtk_init(NULL, NULL);

    // Main window
    g_main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(g_main_window), "MedMate");
    gtk_window_set_default_size(GTK_WINDOW(g_main_window), 600, 400);

    g_signal_connect(g_main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Vertical layout
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(g_main_window), vbox);

    // Top bar
    GtkWidget *top_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), top_bar, FALSE, FALSE, 5);

    GtkWidget *add_button = gtk_button_new_with_label("Add Medication");
    GtkWidget *export_button = gtk_button_new_with_label("Export CSV");

    g_signal_connect(add_button, "clicked", G_CALLBACK(on_add_med_clicked), NULL);
    g_signal_connect(export_button, "clicked", G_CALLBACK(on_export_clicked), NULL);

    gtk_box_pack_start(GTK_BOX(top_bar), add_button, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(top_bar), export_button, TRUE, TRUE, 5);

    // Scrollable list
    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), scroller, TRUE, TRUE, 5);

    g_list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(scroller), g_list_box);

    // Populate from existing meds
    refresh_med_list();

    // Start reminder timer (every 60 seconds)
    g_timeout_add_seconds(60, reminder_check_callback, NULL);

    gtk_widget_show_all(g_main_window);
    gtk_main();
    return 0;
}