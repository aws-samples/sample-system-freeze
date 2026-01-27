# system-freeze - System Freeze Kernel Module

A Linux kernel module that uses `stop_machine()` to freeze all CPUs for a
specified duration, useful to simulate system behavior when impacted by
hardware or virtualization noise (for ex. SMIs, steal-time, etc.).

**Warning**: This will freeze your system. It should only be run for testing
purposes in non-production environments.

## Building

### Fedora

Install build dependencies:
```bash
sudo dnf install kernel-devel-$(uname -r) make gcc
```

### Amazon Linux 2

Install build dependencies:
```bash
sudo yum install kernel-devel-$(uname -r) make gcc
```

### Amazon Linux 2023

Install build dependencies:
```bash
sudo dnf install kernel-devel kernel-headers make gcc
```

### Ubuntu

Install build dependencies:
```bash
sudo apt update
sudo apt install linux-headers-$(uname -r) build-essential
```

### Build the module

```bash
make
```

## Usage

Load the module:
```bash
sudo insmod system_freeze.ko
```

The module creates two debugfs files under `/sys/kernel/debug/system_freeze/`:

- **duration_ms**: Set delay duration in milliseconds (0-10000, max 10 seconds)
- **start**: Trigger the delay (write `1` to start)

### Example

Set a 100ms delay and trigger it:
```bash
# Set duration to 100ms
echo 100 | sudo tee /sys/kernel/debug/system_freeze/duration_ms

# Trigger the delay (freezes all CPUs for 100ms)
echo 1 | sudo tee /sys/kernel/debug/system_freeze/start

# Read current duration
cat /sys/kernel/debug/system_freeze/duration_ms
```

Unload the module:
```bash
sudo rmmod system_freeze
```

### Periodic Freezing

To periodically freeze the system (e.g., every 5 seconds with 100ms freezes):
```bash
# Set duration once
echo 100 | sudo tee /sys/kernel/debug/system_freeze/duration_ms

# Trigger freeze every 5 seconds (Ctrl+C to stop)
while true; do
    echo 1 | sudo tee /sys/kernel/debug/system_freeze/start
    sleep 5
done
```
