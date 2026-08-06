import serial, time
try:
    ser = serial.Serial('/dev/ttyUSB0', 1500000, timeout=1)
    with open('tz_diag_run2.log', 'wb') as f:
        start = time.time()
        while time.time() - start < 300: # 5 mins
            if ser.in_waiting:
                chunk = ser.read(ser.in_waiting)
                f.write(chunk)
                f.flush()
            time.sleep(0.1)
    ser.close()
except Exception as e:
    print(f"UART Error: {e}")
