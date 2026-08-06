import serial, time, threading, subprocess

def capture_uart():
    try:
        ser = serial.Serial('/dev/ttyUSB0', 1500000, timeout=1)
        with open('tz_diag_run.log', 'wb') as f:
            start = time.time()
            while time.time() - start < 180: # 3 mins
                if ser.in_waiting:
                    chunk = ser.read(ser.in_waiting)
                    f.write(chunk)
                    f.flush()
                time.sleep(0.1)
        ser.close()
    except Exception as e:
        print(f"UART Error: {e}")

t = threading.Thread(target=capture_uart)
t.start()

print("Started UART capture...")
time.sleep(1)

print("Connecting hdc...")
subprocess.run(["hdc", "tconn", "192.168.1.224:8710"])

print("Launching test...")
cmd = 'hdc -t 192.168.1.224:8710 shell "cd /data/ssd/rknpu && LD_LIBRARY_PATH=/data/ssd/rknpu/ /data/ssd/rknpu/ld-linux-aarch64.so.1 /data/ssd/rknpu/fake -c 0 -l 0 -m tinyllama -n 64 -s 0 > /data/tz.log 2>&1 & echo LAUNCHED"'
subprocess.run(cmd, shell=True)

print("Waiting for UART capture to finish (180s max, but we can stop earlier if needed)...")
t.join()
print("Done.")
