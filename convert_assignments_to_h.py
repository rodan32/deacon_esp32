import csv
from datetime import datetime

assignments = []

with open('2026 Deacons Quorum Assignments - Sheet1.csv', 'r', encoding='utf-8') as f:
    reader = csv.reader(f)
    next(reader) # skip headers
    
    for row in reader:
        if not row or not row[0]:
            continue
            
        date_str = row[0].strip()
        # Parse M/D format assuming 2026
        parts = date_str.split('/')
        if len(parts) == 2:
            parsed_date = f"2026-{int(parts[0]):02d}-{int(parts[1]):02d}"
        else:
            continue
            
        lesson = row[2].strip() if len(row) > 2 else ""
        messenger = row[3].strip() if len(row) > 3 else ""
        class_list = row[4].strip() if len(row) > 4 else ""
        
        assignments.append({
            'date': parsed_date,
            'lesson': lesson,
            'messenger': messenger,
            'class': class_list
        })

assignments.sort(key=lambda x: x['date'])

out = "#ifndef ASSIGNMENTS_H\n#define ASSIGNMENTS_H\n\n#include <Arduino.h>\n\n"
out += "struct Assignment {\n  const char* date;\n  const char* lesson;\n  const char* messenger;\n  const char* classList;\n};\n\n"
out += f"const int ASSIGNMENTS_LENGTH = {len(assignments)};\n\n"
out += "// Storing the assignments in flash memory (PROGMEM)\n"
out += "const Assignment deacon_assignments[] PROGMEM = {\n"

for item in assignments:
    out += f'  {{"{item["date"]}", "{item["lesson"]}", "{item["messenger"]}", "{item["class"]}"}},\n'

out += "};\n\n#endif\n"

with open('src/assignments.h', 'w', encoding='utf-8') as f:
    f.write(out)

print("Created src/assignments.h successfully")
