#利用Firestore進行雲端database
import firebase_admin
from firebase_admin import credentials
from firebase_admin import firestore
#shape = input('please type in shape of holder')
#shape = str(shape)

#引入金鑰
cred = credentials.Certificate('E:\OneDrive\OneDrive - 長庚大學\DALab\Firebase\serviceAccount.json')
#初始化 firebase
firebase_admin.initialize_app(cred)
#初始化 firestore
db = firestore.client()
#儲存內容
#if shape == 'tetrahedron':
doc = {
           'Shape' : "Tetrahedron"
           'time' 
       }

#建立文件 給定集合名稱+文件id
doc_ref = db.collection("pyradise_students").document("student03")
doc_ref.set(doc)