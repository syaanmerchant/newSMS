/* file input/output helpers for medmate.
 * handles loading, saving, and exporting the medication list.
 *
 * author: Shifa Zaman
 * date: 2025/12/03
 * version: v2.0.0
 */
#ifndef IO_H
#define IO_H

#include "core.h"

/* loads medications from the given file into the list.
 *
 * params:
 *   filename - path to the data file
 *   list     - pointer to the medication list to fill
 *
 * on success, overwrites the current list contents.
 * returns 1 on success and 0 on failure (e.g., file not found or parse error).
 */
int load_medications(const char *filename, MedicationList *list);

/* saves all medications from list into the given file.
 *
 * params:
 *   filename - path to the data file
 *   list     - pointer to the medication list to save
 *
 * returns 1 on success and 0 on failure (e.g., file could not be opened).
 */
int save_medications(const char *filename, MedicationList *list);

/* exports the medications as csv so they can be opened in a spreadsheet.
 *
 * params:
 *   filename - path to the csv file to write
 *   list     - pointer to the medication list to export
 *
 * returns 1 on success and 0 on failure.
 */
int export_medications_csv(const char *filename, MedicationList *list);

#endif
