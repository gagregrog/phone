import sys

FIRMWARE_DEFINES = {"OTA_PASSWORD"}

flags = []
try:
    with open(".env") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, _, value = line.partition("=")
            key = key.strip()
            if key in FIRMWARE_DEFINES:
                flags.append('-D {}=\\"{}\\"'.format(key, value.strip()))
except FileNotFoundError:
    print("WARNING: .env file not found — create one with OTA_PASSWORD=yourpassword", file=sys.stderr)

print(" ".join(flags))
