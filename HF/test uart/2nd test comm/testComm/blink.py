from machine import Pin,UART
from utime import sleep_ms

pin = Pin("LED", Pin.OUT)


def lectureReception ():
    print(rec.read())


print("LED starts flashing...")
Tx = Pin(12)
comm = UART(0,9600)
comm.init(9600,bits=8,parity=None,tx=Tx)
rec = UART(1,9600)
rec.init(9600,bits=8,parity=None,rx = Pin(9))

while True:
    comm.write(b'54')
    rec.irq(lectureReception(),rec.IRQ_RXIDLE)
    sleep_ms(50)



