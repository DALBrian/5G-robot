# -*- coding: utf-8 -*-
"""
Created on Fri Jul 23 15:39:49 2021

@author: Bobo
"""

#將時間戳記加入firebase中
#初始化
import time 
import datetime
import firebase_admin
from firebase_admin import credentials
from firebase_admin import firestore
#引入金鑰
cred = credentials.Certificate('E:\OneDrive\OneDrive - 長庚大學\DALab\Firebase\serviceAccount.json')
#初始化 firebase
firebase_admin.initialize_app(cred)
#初始化 firestore
db = firestore.client()
#現在時間
nowTime = datetime.datetime.now() 
print(nowTime)
print(type(nowTime))

doc = {
            'time' : nowTime
            }

doc_ref = db.collection("input_test").document("0723test")
doc_ref.set(doc)  
#or method as below
'''doc = {
       'time': firestore.SERVER_TIMESTAMP
       }'''
