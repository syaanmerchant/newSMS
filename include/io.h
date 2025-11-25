#ifndef IO_H
#define IO_H

#include "core.h"

// Load medications from a file into the list.
// Returns 1 on success, 0 on failure.
int load_medications(const char *filename, MedicationList *list);

// Save medications from the list into a file.
// Returns 1 on success, 0 on failure.
int save_medications(const char *filename, MedicationList *list);

// Export medications as CSV.
// Returns 1 on success, 0 on failure.
int export_medications_csv(const char *filename, MedicationList *list);

#endif
