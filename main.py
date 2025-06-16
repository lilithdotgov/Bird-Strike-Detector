import communication as comm
import accelerometer as acc
import storage as stor
import analysis as anal
import machine
from picozero import pico_led
import time

acc.AccTest() #Tests accelerometer can be communicated with
acc.ReadState(1) #Turns on data reading
acc.IntrState(1) #Turns on interrupts
acc.ResetIntrState() #Ensures interrupt condition is reset

def test(pin):
    a = acc.FastStream()
    b = anal.StripData(a)
    name = stor.CreateBin(b)
    comm.SendMsg(name, msg="hopefully this works!")
    comm.SendData(name)
    comm.Disconnect()
    acc.ResetIntrState()
    Sleep()
    

def test2(pin):
    pico_led.on()
    time.sleep(0.1)
    pico_led.off()
    acc.ResetIntrState()
    Sleep()


acc.intr.irq(trigger=machine.Pin.IRQ_RISING,handler=test)

#while True:
#    print(f'waiting for falling! Pin = {acc.intr.value()}')
#    time.sleep(0.3)

def Sleep():
    while True:
        machine.deepsleep(1000000)


#if acc.intr.value() == 0:
#    Sleep()