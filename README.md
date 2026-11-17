# Northern Areas Travel Planner

**Course:** Programming Fundamentals (CT-175), Fall 2026
**Instructor:** Ms. Maham Ashraf

## Team
| Name | Roll Number | Section |
|------|-------------|---------|
| Maryam Rehan | CT-26207 | E |
| Ayesha Asif | CT-26211 | E |
| Humna Haq | CT-26217 | E |
| Rumaisa Shafiq | CT-26221 | E |

## About
A menu-driven C program that helps users plan a trip to Northern Pakistan.
The user enters the number of travellers and travel dates, chooses a
destination (Hunza, Skardu, Swat, Kashmir, Chitral or Naran Kaghan), picks a
hotel and a vehicle, and gets an estimated cost. Trips are saved to a file and
can be viewed, deleted and summarised in a report.

## Features
- Create, view and delete custom trips
- Hotels listed cheapest first with price per night and guest rating
- Vehicle options with the number of vehicles needed for the group
- Hotel and transport cost estimate with cost per person
- Date and input validation (real, future dates; no crashes on bad input)
- Trips saved in `trips.dat` and loaded when the program starts
- Report with total, average cost and trips per destination

## How to compile and run
Requires a C compiler such as GCC.

```
gcc "travel planner code.c" -o travel_planner
./travel_planner
```

On Windows, run `travel_planner.exe` instead.

## Notes
- Estimates do not include fuel, food, tolls, parking or entry tickets.
- `trips.dat` is created automatically the first time a trip is saved.

## Concepts used
Structures, arrays, functions, pointers, recursion, bubble sort, linear
search, dynamic memory (`realloc`), file handling and input validation.
