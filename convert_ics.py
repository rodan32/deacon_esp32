import re
import json
import datetime
import sys

def parse_ics_to_json(ics_path, output_path):
    with open(ics_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    events = []
    current_event = {}

    for line in lines:
        line = line.strip()
        if line == 'BEGIN:VEVENT':
            current_event = {}
        elif line == 'END:VEVENT':
            if current_event:
                events.append(current_event)
        elif line.startswith('DTSTART'):
            # DTSTART;VALUE=DATE:20260810
            match = re.search(r'DATE:(\d{8})', line)
            if match:
                date_str = match.group(1)
                # Parse to date object
                date_obj = datetime.datetime.strptime(date_str, '%Y%m%d').date()
                current_event['start_date'] = date_obj.isoformat()
        elif line.startswith('DTEND'):
            match = re.search(r'DATE:(\d{8})', line)
            if match:
                date_str = match.group(1)
                date_obj = datetime.datetime.strptime(date_str, '%Y%m%d').date()
                # ICS DTEND is exclusive, so we subtract 1 day right here so it's inclusive
                date_obj -= datetime.timedelta(days=1)
                current_event['end_date'] = date_obj.isoformat()
        elif line.startswith('SUMMARY'):
            # SUMMARY:CFM: Job 1-3\; 12-14\; 19\; 21-24\; 38-40\; 42 - Yet Will I Trust in Him
            
            # handle folded lines in ICS (where the next line starts with a space)
            # This is a simple parser, so we'll just join the split line back if we see it later
            
            summary = line.split(':', 1)[1]
            summary = summary.replace('\\;', ';').replace('\\,', ',')
            
            if " - " in summary:
                parts = summary.split(" - ", 1)
                refs = parts[0].replace('CFM: ', '')
                topic = parts[1]
                current_event['scriptures'] = refs.strip()
                current_event['topic'] = topic.strip()
            else:
                 current_event['scriptures'] = summary.replace('CFM: ', '').strip()
                 current_event['topic'] = ""
                 
    # Sort events by start date
    events.sort(key=lambda x: x.get('start_date', ''))
    
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(events, f, indent=2)
        
    print(f"Successfully converted {len(events)} events to {output_path}")

if __name__ == '__main__':
    ics_file = "I:\\Agrav\\esp32-deacon\\Come Follow Me_f058ad2699288fdfa244e01faad37af9d123dce000412df89dffa3d5a5b170e3@group.calendar.google.com.ics"
    json_file = "I:\\Agrav\\esp32-deacon\\cfm_2026.json"
    parse_ics_to_json(ics_file, json_file)
