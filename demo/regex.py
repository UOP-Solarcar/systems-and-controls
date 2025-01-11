#!/usr/bin/env python3

import re
from collections import defaultdict
import pandas as pd
import sys

data = sys.stdin.read()

# Step 1: Split the data into records
# We'll use a regex to split the data whenever we encounter 'Cell ID' or 'Ah Used' at the start of a line
record_split_pattern = r"(?m)(?=^(?:Cell ID|Ah Used):)"
records = re.split(record_split_pattern, data.strip())

# Remove any empty strings from the list
records = [record.strip() for record in records if record.strip()]

# Step 2: Define the regex pattern for key-value pairs
kv_pattern = r"(?P<key>[\w ]+):\s*(?P<value>[^:\n]+?)(?=\s+(?:[\w ]+:|$))"

# Step 3: Parse each record and collect the data
parsed_records = []
for record in records:
    matches = re.finditer(kv_pattern, record)
    record_dict = {}
    for match in matches:
        key = match.group("key").strip()
        value = match.group("value").strip()
        record_dict[key] = value
    parsed_records.append(record_dict)

# Step 4: Create a DataFrame from the parsed records
df = pd.DataFrame(parsed_records)

# Display the DataFrame
print(df)

# Write DataFrame to csv file
df.to_csv("output_data.csv")
