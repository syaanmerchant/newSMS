/* graphical user interface for medmate.
 * connects the medication list to the gtk-based ui.
 *
 * author: syaan merchant
 * date: 2025/12/03
 * version: v2.0.0
 */

#ifndef GUI_H
#define GUI_H

#include "core.h"

/* runs the gui event loop using the provided medication list.
 *
 * params:
 *   list - shared medication list used by the ui
 *
 * the gui is responsible for reading and modifying the list in-place.
 * returns 0 on a normal exit. other values can be used for error codes.
 */
int run_gui(MedicationList *list);

#endif
