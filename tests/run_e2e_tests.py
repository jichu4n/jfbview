#!/usr/bin/env python3
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
GOLDEN_DIR = os.path.join(QEMU_DIR, "testdata")
STATUS_FILE = os.path.join(OUT_DIR, "status.log")
QMP_SOCK = os.path.join(OUT_DIR, "qmp.sock")

class JfbviewE2ETester:
    def __init__(self, sock, status_file, update_goldens=False):
        self.sock = sock
        self.status_file = status_file
        self.update_goldens = update_goldens
        self.failures = []
        self.status_f = open(status_file, "r")
        # Seek to end to ignore boot-time renders
        self.status_f.seek(0, os.SEEK_END)

    def __del__(self):
        if hasattr(self, "status_f"):
            self.status_f.close()

    def send_qmp_cmd(self, cmd):
        self.sock.sendall(json.dumps(cmd).encode('utf-8') + b'\n')
        buf = b""
        while b'\n' not in buf:
            data = self.sock.recv(1024)
            if not data:
                break
            buf += data
        
        for line in buf.split(b'\n'):
            if line:
                resp = json.loads(line)
                if 'return' in resp:
                    return resp['return']
                elif 'error' in resp:
                    raise Exception(f"QMP Error: {resp['error']}")
        return None

    def send_key(self, key):
        print(f"Sending key: {key}")
        # Use human-monitor-command for simpler key sending
        self.send_qmp_cmd({
            "execute": "human-monitor-command",
            "arguments": {"command-line": f"sendkey {key}"}
        })
        # Short sleep to let the app process the key
        time.sleep(0.1)

    def send_keys(self, keys):
        for key in keys:
            self.send_key(key)

    def drain_renders(self, timeout=10, silence_duration=1.5):
        """Wait until no more renders occur for silence_duration seconds."""
        print("Waiting for renders to stabilize...")
        start_time = time.time()
        last_render_time = time.time()
        
        while True:
            line = self.status_f.readline()
            if line:
                if "render_complete" in line:
                    last_render_time = time.time()
            else:
                if time.time() - last_render_time > silence_duration:
                    return
                if time.time() - start_time > timeout:
                    raise TimeoutError("Timeout waiting for renders to stabilize.")
                time.sleep(0.1)

    def take_screenshot_and_compare(self, golden_name):
        screenshot_path = os.path.join(OUT_DIR, golden_name)
        if os.path.exists(screenshot_path):
            os.remove(screenshot_path)

        print(f"Taking screenshot for {golden_name}...")
        # Give QEMU VGA buffer a moment to sync
        time.sleep(1)
        self.send_qmp_cmd({"execute": "screendump", "arguments": {"filename": golden_name}})
        
        # Wait for file to exist and stop growing
        start_time = time.time()
        last_size = -1
        while True:
            time.sleep(0.5)
            if time.time() - start_time > 10:
                raise TimeoutError(f"Timeout waiting for screenshot {screenshot_path}")
            if os.path.exists(screenshot_path):
                current_size = os.path.getsize(screenshot_path)
                if current_size == last_size and current_size > 0:
                    break
                last_size = current_size

        golden_path = os.path.join(GOLDEN_DIR, golden_name)
        if self.update_goldens:
            print(f"Updating golden: {golden_path}")
            shutil.copy(screenshot_path, golden_path)
            return

        print(f"Comparing against {golden_path}...")
        if not os.path.exists(golden_path):
            msg = f"Golden {golden_path} not found. Run with --update-goldens."
            print(f"Error: {msg}")
            self.failures.append((golden_name, msg))
            return

        diff_path = os.path.join(OUT_DIR, f"diff_{golden_name}.png")
        ret = subprocess.run(["compare", "-metric", "RMSE", screenshot_path, golden_path, diff_path], capture_output=True)
        output = ret.stderr.decode('utf-8').strip()
        print(f"Compare metric: {output}")

        try:
            norm_error_str = output.split("(")[1].split(")")[0]
            norm_error = float(norm_error_str)
            if norm_error > 0.05:
                msg = f"Screenshot differs from golden significantly ({norm_error * 100:.2f}%). Diff saved to {diff_path}"
                print(f"Error: {msg}")
                self.failures.append((golden_name, msg))
            else:
                print(f"Screenshot matches golden (error: {norm_error * 100:.2f}%).")
        except (IndexError, ValueError):
            print(f"Failed to parse compare output: {output}")
            if ret.returncode != 0:
                msg = f"Screenshots differ (unparseable compare output: {output})"
                print(f"Error: {msg}")
                self.failures.append((golden_name, msg))

def run_test_scenario(tester):
    # 1. Initial boot
    tester.drain_renders(timeout=120)
    tester.take_screenshot_and_compare("page1.ppm")

    # 2. Scroll down 5 times
    print("Scrolling down...")
    for _ in range(5):
        tester.send_key("pgdn")
    tester.drain_renders()
    tester.take_screenshot_and_compare("scroll_down.ppm")

    # 3. Scroll up 2 times
    print("Scrolling up...")
    for _ in range(2):
        tester.send_key("pgup")
    tester.drain_renders()
    tester.take_screenshot_and_compare("scroll_up.ppm")

    # 4. Go to page 170
    print("Jumping to page 170...")
    tester.send_keys(["1", "7", "0", "g"])
    tester.drain_renders()
    tester.take_screenshot_and_compare("page170.ppm")

    # 5. Zoom to fit
    tester.send_key("a")
    tester.drain_renders()
    tester.take_screenshot_and_compare("zoom_fit.ppm")

    # 6. Zoom to width
    tester.send_key("s")
    tester.drain_renders()
    tester.take_screenshot_and_compare("zoom_width.ppm")

    # 7. Zoom in 5 times
    tester.send_keys(["5", "equal"]) # '5='
    tester.drain_renders()
    tester.take_screenshot_and_compare("zoom_in.ppm")

    # 8. Zoom out 5 times
    tester.send_keys(["5", "minus"]) # '5-'
    tester.drain_renders()
    tester.take_screenshot_and_compare("zoom_out.ppm")

    # 9. Rotate right
    tester.send_key("dot") # '.'
    tester.drain_renders()
    tester.take_screenshot_and_compare("rotate_right.ppm")

    # 10. Rotate left
    tester.send_key("comma") # ','
    tester.drain_renders()
    tester.take_screenshot_and_compare("rotate_left.ppm")

    # 11. Outline view
    tester.send_key("tab")
    tester.drain_renders()
    tester.take_screenshot_and_compare("outline_view.ppm")

    # 12. Unfold all
    tester.send_keys(["z", "shift-r"]) # 'zR'
    tester.drain_renders()
    tester.take_screenshot_and_compare("outline_unfold.ppm")

    # 13. Exit outline
    tester.send_key("esc")
    tester.drain_renders()
    tester.take_screenshot_and_compare("outline_exit.ppm")

    # 14. Outline navigate and jump
    tester.send_key("tab")
    tester.drain_renders()
    for _ in range(10):
        tester.send_key("down")
    tester.drain_renders()
    tester.take_screenshot_and_compare("outline_down10.ppm")
    tester.send_key("ret") # Enter
    tester.drain_renders()
    tester.take_screenshot_and_compare("outline_jump.ppm")

    # 15. Search view
    tester.send_key("slash") # '/'
    tester.drain_renders()
    tester.take_screenshot_and_compare("search_view.ppm")

    # 16. Search for "kernel"
    tester.send_keys(["k", "e", "r", "n", "e", "l", "ret"])
    # Background search might take time, so drain renders carefully
    tester.drain_renders(timeout=20, silence_duration=2.0)
    tester.take_screenshot_and_compare("search_kernel.ppm")

    # 17. Go to second result
    tester.send_key("down")
    tester.drain_renders()
    tester.take_screenshot_and_compare("search_second.ppm")

    # 18. Jump to page from search
    tester.send_key("ret")
    tester.drain_renders()
    tester.take_screenshot_and_compare("search_jump.ppm")

def main():
    parser = argparse.ArgumentParser(description="Run E2E UI tests for jfbview via QEMU.")
    parser.add_argument("--update-goldens", action="store_true", help="Update golden reference images instead of comparing.")
    args = parser.parse_args()

    # Clean up previous QMP state
    if os.path.exists(QMP_SOCK):
        os.remove(QMP_SOCK)
    if os.path.exists(STATUS_FILE):
        os.remove(STATUS_FILE)

    os.makedirs(OUT_DIR, exist_ok=True)

    print("Starting QEMU...")
    qemu_script = os.path.join(QEMU_DIR, "run-qemu-ui.sh")
    process = subprocess.Popen([qemu_script, "--test"])

    try:
        # Wait for QMP socket
        start_time = time.time()
        while not os.path.exists(QMP_SOCK):
            if time.time() - start_time > 150:
                raise TimeoutError("Timeout waiting for QMP socket.")
            time.sleep(1)

        print("Connecting to QMP...")
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.connect(QMP_SOCK)
        
        # QMP handshake
        sock.recv(1024)
        sock.sendall(json.dumps({"execute": "qmp_capabilities"}).encode('utf-8') + b'\n')
        sock.recv(1024)

        print(f"Waiting for status file to be created: {STATUS_FILE}...")
        start_time = time.time()
        while not os.path.exists(STATUS_FILE):
            if time.time() - start_time > 120:
                raise TimeoutError("Timeout waiting for status file to be created.")
            time.sleep(1)

        tester = JfbviewE2ETester(sock, STATUS_FILE, args.update_goldens)
        run_test_scenario(tester)

        print("Test scenario complete!")
    finally:
        print("Terminating QEMU...")
        process.terminate()
        process.wait()

    if tester.failures:
        print(f"\n{len(tester.failures)} screenshot mismatch(es):")
        for name, msg in tester.failures:
            print(f"  - {name}: {msg}")
        sys.exit(1)

if __name__ == "__main__":
    main()
