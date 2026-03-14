import random
import time
from datetime import datetime, timedelta

# Define possible values for log entries
IP_ADDRESSES = ["192.168.1.1", "192.168.1.2", "192.168.1.3", "192.168.1.4", "192.168.1.5"]
METHODS = ["GET", "POST", "PUT", "DELETE"]
URLS = ["/index.html", "/about.html", "/contact", "/login", "/dashboard", "/api/data"]
STATUS_CODES = [200, 403, 404, 500]

# Generate a starting timestamp
start_time = datetime(2024, 2, 10, 10, 20, 30)

# Open file to write logs
with open("access.log", "w") as f:
    for i in range(500):
        ip = random.choice(IP_ADDRESSES)
        method = random.choice(METHODS)
        url = random.choice(URLS)
        status = random.choice(STATUS_CODES)

        # Increment time slightly for each log entry
        timestamp = start_time + timedelta(seconds=i)
        timestamp_str = timestamp.strftime("%d/%b/%Y:%H:%M:%S")

        # Construct log entry
        log_entry = f'{ip} - - [{timestamp_str}] "{method} {url}" {status}\n'

        # Write to file
        f.write(log_entry)

print("Generated 'access.log' with 20000000 entries.")
