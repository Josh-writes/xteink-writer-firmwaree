"""
MicroSlate Sync — One-way backup from device to PC.

Downloads all notes from the device to a local folder.  Files on
the PC that don't exist on the device are left untouched (they
serve as a backup archive).  Nothing is ever uploaded or deleted.

Usage:
  python microslate_sync.py          (foreground, console output)
  pythonw.exe microslate_sync.py     (background, log-only)

Dependencies: requests  (pip install requests)
"""

import os
import time
import logging
import requests

# --- Configuration ---
DEVICE_URL = "http://microslate.local"
POLL_INTERVAL = 5  # seconds between connection attempts
LOCAL_DIR = os.path.expanduser("~/OneDrive/Documents/MicroSlate Notes")
LOG_FILE = os.path.join(LOCAL_DIR, "microslate_sync.log")

# --- Setup ---

os.makedirs(LOCAL_DIR, exist_ok=True)

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s  %(message)s",
    datefmt="%H:%M:%S",
    handlers=[
        logging.FileHandler(LOG_FILE, encoding="utf-8"),
        logging.StreamHandler(),
    ],
)
log = logging.getLogger("sync")


def get_device_files():
    """Fetch the file list from the device. Returns list of {name, size} dicts or None on failure."""
    try:
        r = requests.get(f"{DEVICE_URL}/api/files", timeout=3)
        r.raise_for_status()
        return r.json()
    except Exception:
        return None


def download_file(name):
    """Download a file from the device to the local folder."""
    r = requests.get(f"{DEVICE_URL}/notes/{name}", timeout=10)
    r.raise_for_status()
    path = os.path.join(LOCAL_DIR, name)
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(r.text)
    log.info("  Downloaded: %s (%d bytes)", name, len(r.text))


def get_local_files():
    """Return dict of {filename: size} for .txt files in the local folder."""
    result = {}
    for name in os.listdir(LOCAL_DIR):
        if name.lower().endswith(".txt"):
            path = os.path.join(LOCAL_DIR, name)
            result[name] = os.path.getsize(path)
    return result


def sync_once(device_files):
    """One-way sync: download every device file to PC. Never upload or delete."""
    device_map = {f["name"]: f["size"] for f in device_files}
    local_map = get_local_files()
    actions = 0

    for name in sorted(device_map.keys()):
        on_local = name in local_map

        if not on_local or device_map[name] != local_map[name]:
            download_file(name)
            actions += 1

    return actions


def signal_sync_complete():
    """Tell the device we're done syncing so it can shut off WiFi."""
    try:
        r = requests.post(f"{DEVICE_URL}/api/sync-complete", timeout=3)
        r.raise_for_status()
        log.info("Signaled sync-complete to device")
    except Exception as e:
        log.warning("Could not signal sync-complete: %s", e)


def main():
    log.info("MicroSlate Sync started")
    log.info("Local folder: %s", LOCAL_DIR)
    log.info("Device URL:   %s", DEVICE_URL)

    # Wait for device to appear
    log.info("Waiting for device...")
    while True:
        device_files = get_device_files()
        if device_files is not None:
            break
        time.sleep(POLL_INTERVAL)

    log.info("Device found — syncing...")
    actions = sync_once(device_files)
    if actions > 0:
        log.info("Sync complete: %d file(s) transferred", actions)
    else:
        log.info("Sync complete: no changes needed")

    signal_sync_complete()
    log.info("Done")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        log.info("Sync stopped by user")
