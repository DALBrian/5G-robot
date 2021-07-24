# -*- coding: utf-8 -*-
"""
Created on Sat Jul 24 16:13:00 2021

@author: Bobo
"""

#get data in the firebase
#update to firebase
import firebase_admin
from firebase_admin import credentials
from firebase_admin import firestore
#引入金鑰
cred = credentials.Certificate('E:\OneDrive\OneDrive - 長庚大學\DALab\Firebase\serviceAccount.json')
firebase_admin.initialize_app(cred)
db = firestore.client()

collection_name = "pyradise_students"
collection_ref = db.collection(collection_name)
document_name = "student01"
document_ref = db.collection(collection_name).document(document_name)

document_ref.delete()

students_ref = db.collection('pyradise_students')
docs = students_ref.get()
#小批次刪除
#for doc in docs:
#    doc.reference.delete()

#指定批次大小刪除
def delete_collection(coll_ref, batch_size):
    docs = coll_ref.limit(batch_size).get()
    deleted = 0
    for doc in docs:
        doc.reference.delete()
        deleted = deleted + 1
    if deleted >= batch_size:
        return delete_collection(coll_ref, batch_size)
students_ref = db.collection('pyradise_students')
delete_collection(students_ref, 10)