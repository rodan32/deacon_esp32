import json

with open('cfm_2026.json', 'r', encoding='utf-8') as f:
    data = json.load(f)

out = "#ifndef SCHEDULE_H\n#define SCHEDULE_H\n\n#include <Arduino.h>\n\n"
out += "struct WeekSchedule {\n  const char* start_date;\n  const char* end_date;\n  const char* scriptures;\n  const char* topic;\n};\n\n"
out += f"const int SCHEDULE_LENGTH = {len(data)};\n\n"
out += "// Storing the schedule in flash memory (PROGMEM)\n"
out += "const WeekSchedule cfm_schedule[] PROGMEM = {\n"

for item in data:
    out += f'  {{"{item["start_date"]}", "{item["end_date"]}", "{item["scriptures"]}", "{item["topic"]}"}},\n'

out += "};\n\n#endif\n"

with open('src/schedule.h', 'w', encoding='utf-8') as f:
    f.write(out)

print("Created src/schedule.h successfully")
