import numpy as np
import cv2
import serial
import time
import socket
import msvcrt as m
import math
from threading import Thread, Lock
from queue import Queue

# def connect():
#     # Arduino
#     serialPort = 'COM12' 
#     baudRate = 9600  
#     ser = serial.Serial(serialPort, baudRate)

    # while (True):
    #     word = ser.readline().decode(encoding='utf-8')
    #     q.put(word) 

cap = cv2.VideoCapture(0)#開啟相機
while (True): 
	 q = Queue()
	 word = q.get()
	 if len(word) > 0:
		  word = int(word)
		  print(word)
		  if word < 10 :
			  ret2, frame = cap.read()
			  cv2.imshow('frame',frame)
			  cv2.waitKey()
			  break

   
    
        
        

cap.release()#關閉相機
cv2.destroyAllWindows()#