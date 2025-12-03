# MedMate

MedMate is a C + GTK desktop application that helps users manage their medications, daily schedules, and reminders. It supports recurring and one-time medications, local data storage, CSV export, and both offline and online (Wikipedia) medication lookup.

---

## Features

### Core Medication Management

- **Add Medication**
  - Name
  - Dosage
  - 1–4 doses per day
  - Exact time(s) for each dose (HH:MM)
  - Notes
  - Recurrence type:
    - **Daily** (repeats every day)
    - **One-time** (today only)

- **Edit Medication**
  - Change name, dosage, times, notes, and recurrence type.
  - All fields are validated (e.g., time format).

- **Delete Medication**
  - Remove a medication from the list.
  - Confirmation dialog to prevent accidental deletion.

---

### Reminders & Scheduling

- **Automatic Reminders**
  - Background timer checks every 60 seconds.
  - When a dose is due (± 5 minutes), a popup appears:
    > “It is time to take: \<name> (\<dosage>)”
  - Works for all daily medications.

- **One-time Medication Logic**
  - One-time meds trigger a reminder **once**.
  - After the reminder fires, they are marked as “used” and excluded from future reminders and the daily schedule.

- **Today’s Schedule View**
  - Shows all doses for today in a scrollable window.
  - Sorted from earliest to latest.
  - Each entry shows:
    - Time (HH:MM)
    - Name
    - Dosage
    - Recurrence (daily / one-time)
  - Doses within the next 60 minutes are highlighted in bold and show “in X min”.
  - Past doses show “X min ago”.

---

### Data Storage & Export

- **Persistent Local Storage**
  - Medications are saved to `data/meds.txt`.
  - Custom text format stores:
    - Name
    - Dosage
    - Number of doses
    - Recurrence type
    - One-time “used” flag
    - Dose times
    - Notes
  - Medications are automatically loaded from the file on startup.

- **CSV Export**
  - Exports medications to `data/meds_export.csv`.
  - Each row contains:
    - Name
    - Dosage
    - Time
    - Recurrence (daily / one-time)
    - Notes
  - Can be opened in Excel / Google Sheets.

---

### Lookup Features

- **Offline Quick Info**
  - Small built-in C “database” with brief descriptions for a few example medications.
  - Used by the “Lookup Med Info” button.

- **Wikipedia Lookup (Online)**
  - “Wikipedia Lookup” button opens a dialog to enter a medication name.
  - A Python helper script (`scripts/wiki_lookup`) calls the Wikipedia API:
    - Searches for the best matching page title.
    - Fetches that page’s summary.
  - The summary is shown inside a GTK dialog in the app.
  - Works for multi-word queries (e.g., “amoxicillin 500 mg”, “quetiapine medication”).

> Note: Wikipedia lookup requires an internet connection and Python 3 with the `requests` library installed.

> Note on Code Coverage: The gcov results show low line coverage for core.c, io.c, and especially gui.c. This is expected because much of the code, particularly GUI event-handling in gui.c, requires interactive user input to execute. All aspects of the code were thoroughly tested to ensure they work together.

---
