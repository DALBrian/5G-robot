# -*- coding: utf-8 -*-
"""
Created on Sat Jul 24 13:42:32 2021

@author: Bobo
"""

#update to firebase
import firebase_admin
from firebase_admin import credentials
from firebase_admin import firestore
#引入金鑰
cred = credentials.Certificate('E:\OneDrive\OneDrive - 長庚大學\DALab\Firebase\serviceAccount.json')
firebase_admin.initialize_app(cred)
db = firestore.client()

#collection_name = "pyradise_students"
#document_name = "student_01"
#doc_ref = db.collection(collection_name).document(document_name)
path = "pyradise_students/student01"
doc_ref = db.document(path)

doc = {
       'number': 1
       }
doc_ref.update(doc)

path = "pyradise_students/student02"
doc_ref = db.document(path)

doc = {
       'number': 2
       }
doc_ref.update(doc)

path = "pyradise_students/student03"
doc_ref = db.document(path)

doc = {
       'number': 3
       }
doc_ref.update(doc)