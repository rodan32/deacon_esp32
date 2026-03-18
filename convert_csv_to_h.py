import csv
from datetime import datetime
import re

def parse_date(date_str):
    # Try multiple formats
    date_str = date_str.strip()
    # Handle weird "May 14 2026 **Thurs"
    date_str = re.sub(r'\*\*.*$', '', date_str).strip()
    # Handle "July 27-31, 2026", just take start date
    if '-' in date_str and ',' in date_str:
        parts = date_str.split(',')
        month_days = parts[0].split('-')
        date_str = f"{month_days[0]} {parts[1]}"
    
    formats = ['%B %d, %Y', '%B %d %Y', '%b %d, %Y', '%B %d,%Y']
    for fmt in formats:
        try:
            return datetime.strptime(date_str, fmt).strftime('%Y-%m-%d')
        except ValueError:
            continue
    raise ValueError(f"Could not parse date: {date_str}")

activities = []
with open('Youth Activites Calendar 2026 - Deacons.csv', 'r', encoding='utf-8') as f:
    reader = csv.reader(f)
    for _ in range(4): # Skip header rows
        next(reader)
        
    for row in reader:
        if not row or len(row) < 2 or not row[0]:
            continue
        date_str = row[0]
        activity = row[1] if len(row) > 1 else ""
        if not activity: 
            activity = "No Activity"
        
        parsed_date = parse_date(date_str)
        time_str = row[2] if len(row) > 2 else ""
        location = row[3] if len(row) > 3 else ""
        
        activities.append({
            'date': parsed_date,
            'title': activity,
            'time': time_str,
            'location': location
        })

# Sort chronologically just in case
activities.sort(key=lambda x: x['date'])

out = "#ifndef ACTIVITIES_H\n#define ACTIVITIES_H\n\n#include <Arduino.h>\n\n"
out += "struct Activity {\n  const char* date;\n  const char* title;\n  const char* time;\n  const char* location;\n};\n\n"
out += f"const int ACTIVITIES_LENGTH = {len(activities)};\n\n"
out += "// Storing the schedule in flash memory (PROGMEM)\n"
out += "const Activity deacon_activities[] PROGMEM = {\n"

for item in activities:
    out += f'  {{"{item["date"]}", "{item["title"]}", "{item["time"]}", "{item["location"]}"}},\n'

out += "};\n\n#endif\n"

with open('src/activities.h', 'w', encoding='utf-8') as f:
    f.write(out)

print("Created src/activities.h successfully")
