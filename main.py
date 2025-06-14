import communication as comm
import accelerometer as acc
import storage as stor
import analysis as anal
import machine
from picozero import pico_led
import time


acc.ReadState(1)
acc.IntrState(1)
acc.ResetIntrState()

def test(pin):
    a = acc.FastStream()
    b = anal.StripData(a)
    name = stor.CreateBin(b)
    comm.SendData(name, msg="It's working!!!")
    comm.Disconnect()
    #acc.ResetIntrState()
    

def test2(pin):
    pico_led.on()
    time.sleep(0.1)
    pico_led.off()
    acc.ResetIntrState()
    Sleep()


acc.intr.irq(trigger=machine.Pin.IRQ_RISING,handler=test2)

#while True:
#    print(f'waiting for falling! Pin = {acc.intr.value()}')
#    time.sleep(0.3)

def Sleep():
    while True:
        machine.deepsleep(1000000)


#if acc.intr.value() == 0:
#    Sleep()