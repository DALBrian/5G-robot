import serial
 
serialPort = 'COM7'  # 串列埠
baudRate = 9600  # 波特率
ser = serial.Serial(serialPort, baudRate, timeout=0.5)
print("引數設定：串列埠=%s ，波特率=%d" % (serialPort, baudRate))
 
while 1:
     word = str(ser.readline())
     print(word[2])
 
ser.close()
