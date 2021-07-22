import numpy as np
import cv2
import serial
import time
import socket
import msvcrt as m
import math
from threading import Thread, Lock
from queue import Queue

def connect():

    # Arduino
    serialPort = 'COM7' 
    baudRate = 9600  
    ser = serial.Serial(serialPort, baudRate)

    while (True):
        word = ser.readline().decode(encoding='utf-8')
        q.put(word) # q 輸出 arduino 的 word
       
# Socket
HOST = "192.168.225.56"
PORT = 7000
 
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind((HOST, PORT))
server.listen(0)
connection, address = server.accept()
print(connection, address)
cap = cv2.VideoCapture(0 )    

q = Queue()
thread_1 = Thread(target = connect)
thread_1.start() 
 
while(True):
    recv_str=connection.recv(1024)[0:10]
    recv_str=recv_str.decode("ascii")
    print("Recieve: "+recv_str)
    if recv_str[0]=='s':
        
        while (True): 
            q = Queue()
            word = q.get()
            if len(word) > 0:
                word = int(word)
                print(word)
                if word < 10 :   # 0:雷射沒照到
                    ret2, frame = cap.read()
					   # cv2.imshow('frame',frame)#
                    cv2.waitKey()
                    break

        img_gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        img_th = cv2.inRange(img_gray,190,255)
        cv2.imshow('img_th',img_th)
        cnts_t1, h1 = cv2.findContours(img_th.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
         
        #找最大面積
        areas = [cv2.contourArea(c) for c in cnts_t1]   
        max_index = np.argmax(areas)    #argmax返回值0、1、2...(第一個東西為0 第二個東西為1)
        cnt = cnts_t1[max_index]    #cnt = 最大面積的contours
        maxArea = areas[max_index]  #儲存最大面積的數值

        img_c1 = frame.copy()
        box_points = list()
        
        rect_rot = cv2.minAreaRect(cnt) # ((x, y), (w, h), θ )
        rect_pt = cv2.boxPoints (rect_rot) # 4個頂點
        box = np.int0(rect_pt) #int0會省略小數點後方的數字
        box_points.append(box) #記錄所有物件頂點
        cv2.drawContours(img_c1, [box], -1, (0, 255, 0), 2) #畫出矩形
    
        a_x = box_points[0][0][0]
        a_y = box_points[0][0][1]
        b_x = box_points[0][1][0]
        b_y = box_points[0][1][1]
        c_x = box_points[0][2][0]
        c_y = box_points[0][2][1] 
        AB = math.sqrt((a_x-b_x)**2+(a_y-b_y)**2)
        BC = math.sqrt((b_x-c_x)**2+(b_y-c_y)**2)
        if AB > BC: 
            angle = (math.atan((a_y-b_y)/(a_x-b_x)))*180/math.pi
            print(angle)
        else:
            angle = (math.atan((b_y-c_y)/(b_x-c_x)))*180/math.pi
            print(angle)

        M = cv2.moments(cnt)
        cx = int(M['m10']/M['m00'])
        cy = int(M['m01']/M['m00']) 
        cv2.circle(img_c1, (cx, cy), 5, (255, 0, 0), -1)
        cv2.imshow('c1',img_c1)#

        if maxArea > 30000:
            z=200
        else : 
            z=250
		
        # waittime = (((cy-c_y)/52.2)+13.62-9)*1000
		
        cx_ = str(cx)
        cy_ = str(cy)
        z_ = str(z)
        angle_ = str(angle)
        # time_ = str(waittime)
        information = cx_+','+cy_+','+z_+','+angle_
        print(information) 
        connection.send(bytes(information,encoding="ascii"))
        # cv2.waitKey(0)
        cv2.destroyAllWindows()

    else:
        connection.send(bytes('end',encoding="ascii"))
        server.close()
        break

cap.release()
cv2.destroyAllWindows()
cv2.imshow(img_c1,img)
