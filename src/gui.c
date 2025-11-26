// src/gui.c
#include "gui.h"
#include <gtk/gtk.h>
#include "io.h"

static MedicationList *g_med_list = NULL;
static GtkWidget *main_window;
static GtkWidget *med_list_view; // e.g., GtkTreeView or GtkListBox

// Forward declarations
static void build_main_window(void);
static void refresh_med_list(void);
static gboolean reminder_timer_callback(gpointer user_data);
static void on_export_button_clicked(GtkButton *button, gpointer user_data);
static void on_add_button_clicked(GtkButton *button, gpointer user_data);

int run_gui(MedicationList *list) {
    g_med_list = list;

    gtk_init(NULL, NULL);

    build_main_window();

    // Reminder timer: check every 60 seconds
    g_timeout_add_seconds(60, reminder_timer_callback, NULL);

    gtk_main();
    return 0;
}
