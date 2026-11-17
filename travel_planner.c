/*
 * NORTHERN AREAS TRAVEL PLANNER  (Custom Trip Version)
 *
 * What it does:
 *   1. Custom trips: create / view / delete (saved in trips.dat)
 *   2. Hotel and transport cost estimation based on travellers and dates
 *   3. Customer details (name, contact number, Gmail) taken after a plan
 *      is customized, followed by a reservation confirmation statement
 *   4. Reports
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define NUM_DEST        6
#define NUM_HOTELS      25
#define NUM_TRANSPORT   3
#define TRIP_FILE       "trips.dat"
#define COMPANY         "MERA SAFAR NAMA"
#define CONTACT         "0300-9222-897"
#define DETAIL_CONTACT  "0300-0987-215"   /* shown on the reservation statement */

#define NAME_LEN        50
#define PHONE_LEN       20
#define EMAIL_LEN       60

/* ====================== DATA TYPES ====================== */

typedef struct { int day, month, year; } Date;

typedef struct {
    int id, dest;
    const char *name;
    double price;        /* per night */
    double rating;       /* 0 = not available */
} Hotel;

typedef struct {
    int id;
    const char *name;
    int seats;                         /* passengers per vehicle */
    double minPrice, maxPrice;   /* daily price range (per vehicle) */
} Transport;

typedef struct {
    int id, travellers;
    Date checkIn, checkOut;
    int destID;
    int hotelID;
    int transportID;
    double hotelCost, transportCost, totalCost;
    char custName[NAME_LEN];
    char custPhone[PHONE_LEN];
    char custEmail[EMAIL_LEN];
} Trip;

/* ====================== DATA TABLES ====================== */

static const char *destNames[NUM_DEST] = {
    "Hunza", "Skardu", "Swat", "Kashmir", "Chitral", "Naran Kaghan"
};

/* id, destination, name, price/night, guest rating */
static Hotel hotels[NUM_HOTELS] = {
    {1, 1, "The Rihla Hunza - Managed by Luxus", 31226, 9.7},
    {2, 1, "Pearl Continental Attabad Lake, Hunza", 43501, 8.8},
    {3, 1, "Hunza Serena Hotel", 35574, 9.3},
    {4, 2, "Hotel Mountain Lodge Skardu", 11266, 9.0},
    {5, 2, "Hotel DewaneKhas Skardu", 2839, 8.9},
    {6, 2, "Heaven Hotel Skardu", 6009, 8.9},
    {7, 2, "Fialong Hotel Skardu", 25222, 9.4},
    {8, 3, "Pearl Continental Hotel Malam Jabba", 35700, 8.4},
    {9, 3, "IYI Luxury Inn Swat", 33816, 6.4},
    {10, 3, "Shelton Rezidor Swat", 11810, 8.1},
    {11, 3, "Rock City Resort", 12228, 8.9},
    {14, 4, "Pearl Continental Hotel Muzaffarabad", 25000, 0.0},
    {15, 4, "Munawar Residence", 15000, 0.0},
    {16, 4, "Bridge Residency & Restaurant", 10000, 0.0},
    {17, 4, "Hotel La Orilla", 8000, 0.0},
    {18, 5, "Hindukush Heights", 29500, 0.0},
    {19, 5, "Ayun Fort Inn", 15000, 0.0},
    {20, 5, "Legendary Hotel Chitral", 12500, 0.0},
    {21, 5, "Gahirat Castle", 9500, 0.0},
    {22, 6, "Riviera Resort Naran", 25000, 0.0},
    {23, 6, "Luxus Grand Hotel Naran", 16000, 0.0},
    {24, 6, "Hotel Demanchi", 18000, 0.0},
    {25, 6, "Faletti's Grand Hotel Naran", 19500, 0.0},
};

/* id, name, seats, daily price range */
static Transport transports[NUM_TRANSPORT] = {
    {1, "Toyota Corolla", 4,  4000,  6000},
    {2, "4x4 Jeep",        6,  8000,  15000},
    {3, "Toyota Hiace",   14, 11000, 11000}
};

/* ====================== SAVED DATA IN MEMORY ====================== */

static Trip *trips = NULL;
static int tripCount = 0, tripCapacity = 0;
static int nextTripID = 1;

/* ====================== INPUT HELPERS ====================== */

static void readLine(const char *prompt, char *buf, int size)
{
    int c;
    for (;;) {
        printf("%s", prompt);
        if (fgets(buf, size, stdin) == NULL) {
            printf("\nInput ended. Exiting.\n");
            exit(0);
        }
        if (strchr(buf, '\n') != NULL) {
            buf[strcspn(buf, "\n")] = '\0';
            return;
        }
        while ((c = getchar()) != '\n' && c != EOF) { }   /* throw away the rest */
        printf("Input is too long. Please try again.\n");
    }
}

static long readInt(const char *prompt, long min, long max)
{
    char line[100], *end;
    long value;
    for (;;) {
        readLine(prompt, line, sizeof line);
        value = strtol(line, &end, 10);
        if (end != line && *end == '\0' && value >= min && value <= max)
            return value;
        printf("Invalid input. Enter a whole number from %ld to %ld.\n", min, max);
    }
}

static int askYesNo(const char *prompt)
{
    char answer[16];
    for (;;) {
        readLine(prompt, answer, sizeof answer);
        if (tolower((unsigned char)answer[0]) == 'y') return 1;
        if (tolower((unsigned char)answer[0]) == 'n') return 0;
        printf("  Please type y or n.\n");
    }
}

static void title(const char *text)
{
    printf("\n--------------------------------------------------------------------\n");
    printf("                    %s\n", text);
    printf("--------------------------------------------------------------------\n");
}

/* remove spaces at the start and end of a string */
static void trim(char *s)
{
    char *start = s;
    size_t len;

    while (isspace((unsigned char)*start)) start++;
    memmove(s, start, strlen(start) + 1);
    len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1]))
        s[--len] = '\0';
}

/* ====================== CUSTOMER DETAILS ====================== */

static int validName(const char *s)
{
    int letters = 0;
    for (; *s; s++) {
        if (isalpha((unsigned char)*s)) letters++;
        else if (*s != ' ' && *s != '.' && *s != '\'' && *s != '-') return 0;
    }
    return letters >= 2;
}

/* accepts digits with optional leading +, dashes or spaces; 10 to 13 digits */
static int validPhone(const char *s)
{
    int digits = 0;
    const char *p;

    for (p = s; *p; p++) {
        if (isdigit((unsigned char)*p)) digits++;
        else if (*p == '-' || *p == ' ' || (*p == '+' && p == s)) continue;
        else return 0;
    }
    return digits >= 10 && digits <= 13;
}

/* must look like  something@gmail.com */
static int validGmail(const char *s)
{
    static const char suffix[] = "@gmail.com";
    size_t len = strlen(s), sl = sizeof suffix - 1, i;

    if (len <= sl) return 0;
    for (i = 0; i < sl; i++)
        if (tolower((unsigned char)s[len - sl + i]) != suffix[i]) return 0;
    for (i = 0; i < len - sl; i++) {
        unsigned char c = (unsigned char)s[i];
        if (!isalnum(c) && c != '.' && c != '_' && c != '+' && c != '-') return 0;
    }
    return 1;
}

static void inputCustomerDetails(Trip *t)
{
    char buf[100];

    title("CUSTOMER DETAILS");
    for (;;) {
        readLine("Full name: ", buf, sizeof buf);
        trim(buf);
        if (validName(buf) && strlen(buf) < NAME_LEN) break;
        printf("Please enter a valid name (letters only, up to %d characters).\n", NAME_LEN - 1);
    }
    strcpy(t->custName, buf);

    for (;;) {
        readLine("Contact number (e.g. 0300-1234567): ", buf, sizeof buf);
        trim(buf);
        if (validPhone(buf) && strlen(buf) < PHONE_LEN) break;
        printf("Invalid number. Use 10 to 13 digits (dashes and a leading + are allowed).\n");
    }
    strcpy(t->custPhone, buf);

    for (;;) {
        readLine("Gmail address (e.g. name@gmail.com): ", buf, sizeof buf);
        trim(buf);
        if (validGmail(buf) && strlen(buf) < EMAIL_LEN) break;
        printf("Invalid Gmail address. It must look like name@gmail.com.\n");
    }
    strcpy(t->custEmail, buf);
}

/* ====================== DATES ====================== */

static int validDate(Date d)
{
    static const int days[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int leap = (d.year % 400 == 0) || (d.year % 4 == 0 && d.year % 100 != 0);

    if (d.year < 1900 || d.year > 9999 || d.month < 1 || d.month > 12) return 0;
    return d.day >= 1 && d.day <= days[d.month] + (d.month == 2 && leap);
}

static long dateSerial(Date d)
{
    long y = d.year, m = d.month;
    if (m <= 2) { y--; m += 12; }
    return 365L * y + y / 4 - y / 100 + y / 400 + (153 * (m - 3) + 2) / 5 + d.day;
}

static int isFuture(Date d)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    Date today;

    today.day = t->tm_mday;
    today.month = t->tm_mon + 1;
    today.year = t->tm_year + 1900;
    return dateSerial(d) > dateSerial(today);
}

static void printDate(Date d)
{
    printf("%02d-%02d-%04d", d.day, d.month, d.year);
}

static int parseDate(const char *s, Date *d)
{
    char *end;

    d->day = (int)strtol(s, &end, 10);
    if (*end != '-') return 0;
    s = end + 1;
    d->month = (int)strtol(s, &end, 10);
    if (*end != '-') return 0;
    s = end + 1;
    d->year = (int)strtol(s, &end, 10);
    return *end == '\0' && validDate(*d);
}

static Date inputDate(const char *prompt)
{
    char text[100];
    Date d;
    for (;;) {
        readLine(prompt, text, sizeof text);
        if (!parseDate(text, &d)) {
            printf("Invalid date \"%s\". Use a real date as DD-MM-YYYY.\n", text);
        } else if (!isFuture(d)) {
            printf("Past or current dates are not allowed. Please enter a future date.\n");
        } else {
            return d;
        }
    }
}

/* ====================== SAVING AND LOADING ====================== */

static void saveTrips(void)
{
    FILE *f = fopen(TRIP_FILE, "wb");
    if (f == NULL) { printf("Could not open %s for saving.\n", TRIP_FILE); return; }
    if (tripCount > 0 && fwrite(trips, sizeof(Trip), tripCount, f) != (size_t)tripCount)
        printf("Error writing trips to file.\n");
    fclose(f);
}

static int appendTrip(const Trip *t)
{
    if (tripCount == tripCapacity) {
        int newCapacity = (tripCapacity == 0) ? 4 : tripCapacity * 2;
        Trip *bigger = (Trip *)realloc(trips, newCapacity * sizeof(Trip));
        if (bigger == NULL) {
            printf("Memory allocation failed. The trip was not stored.\n");
            return 0;
        }
        trips = bigger;
        tripCapacity = newCapacity;
    }
    trips[tripCount++] = *t;
    return 1;
}

static void loadTrips(void)
{
    FILE *f = fopen(TRIP_FILE, "rb");
    Trip t;

    if (f == NULL) return;                    /* first run: no file yet */
    while (fread(&t, sizeof(Trip), 1, f) == 1) {
        if (!appendTrip(&t)) break;
        if (t.id >= nextTripID) nextTripID = t.id + 1;
    }
    fclose(f);
}

/* ====================== HOTELS ====================== */

static void swapInts(int *a, int *b)
{
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

static int findHotelRecursive(int hotelID, int index)
{
    if (index >= NUM_HOTELS) return -1;
    if (hotels[index].id == hotelID) return index;
    return findHotelRecursive(hotelID, index + 1);
}

static int chooseHotel(int dest)
{
    int list[NUM_HOTELS], count = 0, i, j, pick;
    char rating[16];

    for (i = 0; i < NUM_HOTELS; i++)
        if (hotels[i].dest == dest)
            list[count++] = i;

    /* bubble sort the list by price */
    for (i = 0; i < count - 1; i++)
        for (j = 0; j < count - i - 1; j++)
            if (hotels[list[j]].price > hotels[list[j + 1]].price)
                swapInts(&list[j], &list[j + 1]);

    printf("\nHotels in %s, cheapest first:\n", destNames[dest - 1]);
    printf("%-4s %-40s %12s %8s\n", "No.", "Hotel", "PKR/night", "Rating");
    for (i = 0; i < count; i++) {
        const Hotel *h = &hotels[list[i]];
        if (h->rating > 0) sprintf(rating, "%.1f", h->rating);
        else strcpy(rating, "N/A");
        printf("%-4d %-40.40s %12.0f %8s\n", i + 1, h->name, h->price, rating);
    }
    pick = (int)readInt("Hotel number: ", 1, count);
    return list[pick - 1];
}

/* ====================== TRANSPORT ====================== */

static int vehiclesNeeded(int travellers, int seats)
{
    return (travellers + seats - 1) / seats;
}

static int chooseTransport(int travellers)
{
    int i;
    const Transport *t;

    printf("\nTRANSPORT FOR %d TRAVELLERS\n", travellers);
    printf("%-4s %-16s %-6s %-24s %s\n", "No.", "Vehicle", "Seats", "Daily price (each)",
           "Vehicles needed");
    for (i = 0; i < NUM_TRANSPORT; i++) {
        t = &transports[i];
        printf("%-4d %-16s %-6d Rs. %.0f - %.0f    %d\n", i + 1, t->name, t->seats,
               t->minPrice, t->maxPrice, vehiclesNeeded(travellers, t->seats));
    }
    return (int)readInt("Select vehicle number: ", 1, NUM_TRANSPORT) - 1;
}

/* ====================== CUSTOM TRIPS ====================== */

static void buildTrip(Trip *t)
{
    int nights, rentalDays, i, h, tr;

    memset(t, 0, sizeof *t);

    t->travellers = (int)readInt("\nNumber of travellers: ", 1, 10000);
    t->checkIn = inputDate("Departure date (DD-MM-YYYY): ");
    for (;;) {
        t->checkOut = inputDate("Return date (DD-MM-YYYY): ");
        nights = (int)(dateSerial(t->checkOut) - dateSerial(t->checkIn));
        if (nights > 0) break;
        printf("Return date must be later than departure date.\n");
    }
    rentalDays = nights + 1;      /* counts both the departure and return days */
    printf("Trip duration: %d night(s); vehicle rental: %d day(s).\n", nights, rentalDays);

    printf("\nDESTINATIONS\n");
    for (i = 0; i < NUM_DEST; i++) printf("  %d. %s\n", i + 1, destNames[i]);
    t->destID = (int)readInt("Select destination: ", 1, NUM_DEST);

    h = chooseHotel(t->destID);
    t->hotelID = hotels[h].id;
    t->hotelCost = hotels[h].price * nights;

    tr = chooseTransport(t->travellers);
    t->transportID = transports[tr].id;
    t->transportCost = transports[tr].minPrice * rentalDays *
                        vehiclesNeeded(t->travellers, transports[tr].seats);

    t->totalCost = t->hotelCost + t->transportCost;
}

static void showTrip(const Trip *t)
{
    int rentalDays = (int)(dateSerial(t->checkOut) - dateSerial(t->checkIn)) + 1;
    int h = findHotelRecursive(t->hotelID, 0);
    const Transport *v = &transports[t->transportID - 1];
    int count = vehiclesNeeded(t->travellers, v->seats);

    title("CUSTOM TRIP SUMMARY");
    if (t->custName[0] != '\0') {
        printf("Name: %s\nContact: %s\nGmail: %s\n", t->custName, t->custPhone, t->custEmail);
    }
    printf("Booking ID: %d\nTravellers: %d\nDeparture: ", t->id, t->travellers);
    printDate(t->checkIn);
    printf("\nReturn: ");
    printDate(t->checkOut);
    printf("\nDuration: %d night(s) / %d rental day(s)\n", rentalDays - 1, rentalDays);
    printf("Destination: %s\n", destNames[t->destID - 1]);

    printf("\nHotel: %s | %d night(s) | Rs. %.0f\n", hotels[h].name, rentalDays - 1, t->hotelCost);
    printf("Vehicle: %s x %d | %d day(s) | Rs. %.0f\n", v->name, count, rentalDays, t->transportCost);

    printf("\nTOTAL ESTIMATED COST:  Rs. %.0f\n", t->totalCost);
    printf("Estimated per person:  Rs. %.0f\n", t->totalCost / t->travellers);
    printf("Not included: fuel, food, tolls, parking, entry tickets, other expenses.\n");
}

static void printReservationStatement(const Trip *t)
{
    title("RESERVATION CONFIRMED");
    printf("Dear %s,\n", t->custName);
    printf("Booking #%d  |  %s  |  ", t->id, destNames[t->destID - 1]);
    printDate(t->checkIn);
    printf(" to ");
    printDate(t->checkOut);
    printf("\nTotal estimated cost: Rs. %.0f\n\n", t->totalCost);
    printf("Your seats are reserved.\n");
    printf("More further detail contact %s\n", DETAIL_CONTACT);
    printf("--------------------------------------------------------------------\n");
}

static int findTrip(int id)
{
    int i;
    for (i = 0; i < tripCount; i++)
        if (trips[i].id == id) return i;
    return -1;
}

static void createTrip(void)
{
    Trip t;

    buildTrip(&t);
    t.id = nextTripID;
    showTrip(&t);                       /* plan summary first */

    inputCustomerDetails(&t);           /* then name, contact and Gmail */

    if (!appendTrip(&t)) return;
    nextTripID++;
    saveTrips();

    showTrip(&t);                       /* final copy including customer details */
    printReservationStatement(&t);
}

static void deleteTrip(void)
{
    int id = (int)readInt("Enter booking ID to delete: ", 1, 2000000000);
    int index = findTrip(id), i;

    if (index < 0) { printf("Booking ID not found.\n"); return; }
    if (!askYesNo("Delete this booking? (y/n): ")) return;
    for (i = index; i + 1 < tripCount; i++)
        trips[i] = trips[i + 1];
    tripCount--;
    saveTrips();
    printf("Booking #%d deleted.\n", id);
}

static void showSavedTrips(void)
{
    int i;

    if (tripCount == 0) {
        printf("\nNo saved custom trips.\n");
        return;
    }

    title("SAVED TRIPS");
    printf("%-11s %-20s %-5s %-12s %-12s %12s\n",
           "Booking ID", "Name", "Pax", "Departure", "Return", "Total (PKR)");
    for (i = 0; i < tripCount; i++) {
        printf("#%-10d %-20.20s %-5d ", trips[i].id, trips[i].custName, trips[i].travellers);
        printDate(trips[i].checkIn);
        printf("   ");
        printDate(trips[i].checkOut);
        printf("   %12.0f\n", trips[i].totalCost);
    }
}

static void tripMenu(void)
{
    int choice;
    do {
        title("TRIP MANAGEMENT");
        printf("  1. Create a Custom Trip\n  2. View Saved Trips\n");
        printf("  3. Delete a Trip\n  4. Back to Main Menu\n");
        choice = (int)readInt("Choose an option: ", 1, 4);
        switch (choice) {
            case 1: createTrip();    break;
            case 2: showSavedTrips(); break;
            case 3: deleteTrip();    break;
        }
    } while (choice != 4);
}

/* ====================== REPORTS ====================== */

static void reports(void)
{
    int i, popularity[NUM_DEST] = {0};
    double total = 0;

    if (tripCount == 0) {
        printf("\nNo saved trips available for reports.\n");
        return;
    }

    printf("\n========== TRAVEL REPORT ==========\n");
    printf("CUSTOM TRIPS: %d saved\n", tripCount);
    for (i = 0; i < tripCount; i++) {
        total += trips[i].totalCost;
        popularity[trips[i].destID - 1]++;
    }
    printf("Combined estimated cost: Rs. %.0f\n", total);
    printf("Average estimated cost:  Rs. %.0f\n", total / tripCount);
    printf("Destination popularity:\n");
    for (i = 0; i < NUM_DEST; i++)
        printf("  %-15s %d trip(s)\n", destNames[i], popularity[i]);
    printf("===================================\n");
}

/* ====================== MAIN ====================== */

int main(void)
{
    int choice;

    loadTrips();
    printf("\n\t\t\t*** %s ***\n\t\t\tHar safar ki apni kahani\n", COMPANY);

    do {
        title("NORTHERN AREAS TRAVEL PLANNER");
        printf("  1. Manage Trips (create / view / delete)\n");
        printf("  2. Generate Reports\n");
        printf("  3. Exit\n");
        choice = (int)readInt("Choose an option: ", 1, 3);

        switch (choice) {
            case 1: tripMenu();       break;
            case 2: reports();        break;
        }
    } while (choice != 3);

    free(trips);
    printf("\nThank you for using the Northern Areas Travel Planner!\n");
    return 0;
}
