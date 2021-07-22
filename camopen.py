import cv2

cap = cv2.VideoCapture(1)#開啟相機

while(True):
    ret,frame = cap.read()#捕獲一幀影象
    cv2.imshow('frame',frame)
    #判斷按鍵，如果按鍵為q，退出迴圈
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()#關閉相機
cv2.destroyAllWindows()#關閉視窗
