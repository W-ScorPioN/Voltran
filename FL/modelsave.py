# import os
# import pickle
# import glob
# def save(content, path):
#     "save content into path."
#     with open(path, 'wb') as f:
#         pickle.dump(content, f)
    

# def save_model(dict_, models_dir, idx):
#     if not os.path.exists(models_dir):
#         os.makedirs(models_dir)
        
#     model_path = f"{models_dir}/model_{dict_['epoch']}_{str(IndexError())}.model"
#     model_paras = {
#             'weight': dict_['weight']
#             # 'bias': dict_['bias'],
#         }
#     save(model_paras, model_path)

# def load_model(model_path):
#     print('model_path', model_path)
#     with open(model_path, 'rb') as f:
#         data = pickle.load(f)
#         print('load model successfully')
#         return data