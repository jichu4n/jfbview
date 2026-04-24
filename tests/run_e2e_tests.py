import os
import subprocess
import socket
import json
import time
import sys
import shutil
import argparse

WORKSPACE_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
QEMU_DIR = os.path.join(WORKSPACE_ROOT, "tests", "qemu")
OUT_DIR = os.path.join(QEMU_DIR, "out")
UPLOAD_DIR = os.path.join(WORKSPACE_ROOT, "upload")
STATUS_FILE = os.path.join(OUT_DIR, "status.log")
QMP_SOCK = os.path.join(OUT_DIR, "qmp.sock")

def wait_for_status(f, timeout=30):
    print("Waiting for status update...")
    start_time = time.time()
    
    while True:
        line = f.readline()
        if line:
            if "render_complete" in line:
                return
        else:
            if time.time() - start_time > timeout:
                raise TimeoutError("Timeout waiting for render_complete in status file.")
            time.sleep(0.1)

def wait_for_file_to_stabilize(path, timeout=10):
    start_time = time.time()
    last_size = -1
    while True:
        time.sleep(0.5)
        if time.time() - start_time > timeout:
            raise TimeoutError(f"Timeout waiting for file {path} to stabilize.")
        if os.path.exists(path):
            current_size = os.path.getsize(path)
            if current_size == last_size and current_size > 0:
                break
            last_size = current_size

def send_qmp_cmd(sock, cmd):
    sock.sendall(json.dumps(cmd).encode('utf-8') + b'\n')
    # Read response
    buf = b""
    while b'\n' not in buf:
        data = sock.recv(1024)
        if not data:
            break
        buf += data
    
    lines = buf.split(b'\n')
    for line in lines:
        if line:
            resp = json.loads(line)
            if 'return' in resp:
                return resp['return']
            elif 'error' in resp:
                raise Exception(f"QMP Error: {resp['error']}")
    return None

def compare_or_update_golden(screenshot_path, golden_name, update_goldens=False):
    golden_path = os.path.join(QEMU_DIR, "testdata", golden_name)
    os.makedirs(os.path.dirname(golden_path), exist_ok=True)

    if update_goldens:
        print(f"Updating golden: {golden_path}")
        shutil.copy(screenshot_path, golden_path)
        return

    print(f"Comparing {screenshot_path} against {golden_path}...")
    if not os.path.exists(golden_path):
        print(f"Error: Golden {golden_path} not found. Run with --update-goldens.")
        sys.exit(1)

    if shutil.which("compare"):
        diff_path = os.path.join(OUT_DIR, f"diff_{golden_name}.png")
        ret = subprocess.run(["compare", "-metric", "RMSE", screenshot_path, golden_path, diff_path], capture_output=True)
        
        output = ret.stderr.decode('utf-8').strip()
        print(f"Compare metric: {output}")

        try:
            norm_error_str = output.split("(")[1].split(")")[0]
            norm_error = float(norm_error_str)
            if norm_error > 0.05:
                print(f"Error: Screenshot differs from golden significantly ({norm_error * 100:.2f}%). Diff saved to {diff_path}")
                sys.exit(1)
            else:
                print(f"Screenshot matches golden (error: {norm_error * 100:.2f}%).")
        except IndexError:
            print(f"Failed to parse compare output: {output}")
            if ret.returncode != 0:
                print("Error: Screenshots differ.")
                sys.exit(1)
    else:
        print("ImageMagick 'compare' not found. Skipping image comparison.")

def main():
    parser = argparse.ArgumentParser(description="Run E2E UI tests for jfbview via QEMU.")
    parser.add_argument("--update-goldens", action="store_true", help="Update golden reference images instead of comparing.")
    args = parser.parse_args()

    # Clean up previous state
    if os.path.exists(STATUS_FILE):
        os.remove(STATUS_FILE)
    if os.path.exists(QMP_SOCK):
        os.remove(QMP_SOCK)

    os.makedirs(OUT_DIR, exist_ok=True)

    print("Starting QEMU...")
    qemu_script = os.path.join(QEMU_DIR, "run-qemu-ui.sh")
    process = subprocess.Popen([qemu_script, "--test"])

    try:
        # Wait for QMP socket
        start_time = time.time()
        while not os.path.exists(QMP_SOCK):
            if time.time() - start_time > 120: # Building might take a while if deb is missing
                raise TimeoutError("Timeout waiting for QMP socket.")
            time.sleep(1)

        print("Connecting to QMP...")
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.connect(QMP_SOCK)
        
        # QMP handshake
        greeting = sock.recv(1024)
        send_qmp_cmd(sock, {"execute": "qmp_capabilities"})

        print(f"Waiting for status file to be created: {STATUS_FILE}...")
        start_time = time.time()
        while not os.path.exists(STATUS_FILE):
            if time.time() - start_time > 120:
                raise TimeoutError("Timeout waiting for status file to be created.")
            time.sleep(0.5)

        with open(STATUS_FILE, "r") as status_f:
            # Wait for initial render
            wait_for_status(status_f, timeout=120)  # VM boot takes time
            print("Initial render complete.")
            
            # Take screenshot
            screenshot1_path = os.path.join(OUT_DIR, "screenshot1.ppm")
            print(f"Taking screenshot: {screenshot1_path}")
            if os.path.exists(screenshot1_path):
                os.remove(screenshot1_path)
            send_qmp_cmd(sock, {"execute": "screendump", "arguments": {"filename": "screenshot1.ppm"}})
            wait_for_file_to_stabilize(screenshot1_path)
            compare_or_update_golden(screenshot1_path, "page1.ppm", args.update_goldens)
            
            # Send Page Down
            print("Sending Page Down...")
            send_qmp_cmd(sock, {"execute": "input-send-event", "arguments": {"events": [{"type": "key", "data": {"down": True, "key": {"type": "qcode", "data": "pgdn"}}}]}})
            send_qmp_cmd(sock, {"execute": "input-send-event", "arguments": {"events": [{"type": "key", "data": {"down": False, "key": {"type": "qcode", "data": "pgdn"}}}]}})

            # Wait for second render
            wait_for_status(status_f, timeout=10)
            print("Second render complete.")
            
            # Take second screenshot
            screenshot2_path = os.path.join(OUT_DIR, "screenshot2.ppm")
            print(f"Taking second screenshot: {screenshot2_path}")
            if os.path.exists(screenshot2_path):
                os.remove(screenshot2_path)
            send_qmp_cmd(sock, {"execute": "screendump", "arguments": {"filename": "screenshot2.ppm"}})
            wait_for_file_to_stabilize(screenshot2_path)
            compare_or_update_golden(screenshot2_path, "page2.ppm", args.update_goldens)

        print("Test passed successfully!")
    finally:
        print("Terminating QEMU...")
        process.terminate()
        process.wait()

if __name__ == "__main__":
    main()
