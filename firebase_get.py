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
#document_name = "student01"
#doc_ref = db.collection(collection_name).document(document_name)

docs = collection_ref.get()

for doc in docs:
    print('文件內容 {}' .format(doc.to_dict()))
    
#docs = collection_ref.where('欄位名','運算式', '值').get()
docs = collection_ref.where('number', '>', 2).get()
for doc in docs:
    print('處理後文件內容 {}' .format(doc.to_dict()))
