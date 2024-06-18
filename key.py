from time import sleep
import serial

buf = b""
buf = b"123456"


arduino = serial.Serial(port='/dev/ttyACM0', baudrate=9600, timeout=.1)

arduino.write(buf)
sleep(2)
arduino.write(buf)
sleep(2)
print("[+] Payload Sent")
exit()
