
import firebase_admin
from firebase_admin import credentials
from firebase_admin import firestore

#path to serviceaccount.json
cred = credentials.Certificate('E:\OneDrive\OneDrive - 長庚大學\DALab\Firebase\serviceAccount.json')

#initialization firebase
firebase_admin.initialize_app(cred)
#initialization firestore
db = firestore.client()

#main
doc = {
       'name' : "Poyan"
      # 'email': "mailaddress"
   }

#建立文件 給定集合名稱+文件id
#doc_ref = db.collection("集合名稱").document("文件ID")
doc_ref = db.collection("pyradise_students").document("student03")

#指定文件的路徑, doc_path指向文件
#doc_path = "/pyradise_students/student_02"
#透過路徑產生參考
#doc_ref = db.document(doc_path)
#doc_ref提供一個set的方法，input需是dictionary
doc_ref.set(doc)
