# # # import requests

# # # def getGlobalModel(taskid, round):
# # #     headers={'User-Agent': 'Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/67.0.3396.87 Safari/537.36',}
# # #     url = 'http://10.114.100.154:9001/tee/getGlobalModel'
# # #     params = {"taskid":taskid, "round":round}
# # #     print(params)
# # #     response = requests.post(url=url, data=params, headers=headers).text
# # #     return response

# # # w_glob_str = getGlobalModel("123", "0")
# # from decrypt import AES_GCM, InvalidTagException
# # import json
# # import logging
# # import requests
# # from Crypto.Cipher import AES
# # # logging.basicConfig(filename=f'logging.log', level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

# # def getCiphertext(taskid, round, index):
# #     flg = True
# #     headers={'User-Agent': 'Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/67.0.3396.87 Safari/537.36',}
# #     url = 'http://10.50.146.119:9001/tee/getCiphertext'
# #     params = {'taskID':taskid, 'round':round, 'index':index}
# #     a = ''
# #     while flg:
# #         # print('aaa')
# #         response = requests.post(url=url, json=params, headers=headers).text
# #         response_byte = response.encode('utf-8')
# #         response_json = json.loads(response_byte)
# #         opcode = response_json['code']
# #         if opcode != '200':
# #             time.sleep(1)
# #             continue
# #         flg = False
# #         # print('bbb')
# #         a = response_json['result']

# #         # getAuthTag(taskid, round, index)
# #     return a

# # def getAuthTag(taskid, round, index):
# #     flg = True
# #     headers={'User-Agent': 'Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/67.0.3396.87 Safari/537.36',}
# #     url = 'http://10.50.146.119:9001/tee/getAuth_tag'
# #     params = {'taskID':taskid, 'round':round, 'index':index}
# #     a = ''
# #     while flg:
# #         response = requests.post(url=url, json=params, headers=headers).text
# #         response_byte = response.encode('utf-8')
# #         response_json = json.loads(response_byte)
# #         opcode = response_json['code']
# #         if opcode != '200':
# #             time.sleep(1)
# #             print(opcode)
# #             continue
# #         flg = False
# #         a = response_json['result']

# #         # getAuthTag(taskid, round, index)
# #     return a


# # ciphertext_message = getCiphertext('12345', '0', '0')
# # auth_tag_message = getAuthTag('12345', '0', '0')
# # # ciphertext_hex = int(ciphertext_message,16)
# # # print(ciphertext_hex)
# # ciphertext_byte = bytes.fromhex(ciphertext_message)
# # auth_tag = int(auth_tag_message,16)
# # # print(ciphertext_byte)

# # master_key = 0xD16FDC6C12B1E8D8AB7D82417934E1BE
# # master_key_reverse = 0xBEE1347941827DABD8E8B1126CDC6FD1
# # init_value = 0x0
# # indexnumber = 0
# # my_gcm = AES_GCM(master_key_reverse)

# # try:
# # # decrypted = my_gcm.decrypt(init_value, plaintext, auth_tag + 1, b'')
# #     decrypted_r = my_gcm.decrypt(init_value, ciphertext_byte, auth_tag +1, b'')
# #     print('成功')
# # except InvalidTagException:
# # # decrypted = my_gcm.decrypt(init_value, plaintext, auth_tag, b'')
    
# #     decrypted_r = my_gcm.decrypt(init_value, ciphertext_byte, auth_tag, b'')   
# #     print('成功')

# # w_copy = {'a': 1, 'b': 2, 'c': 3}
# # key_list = list(w_copy.keys())
# # indexed_keys = {index: key for index, key in enumerate(key_list)}
# # print(indexed_keys)

# import numpy as np

# def varied_step_range(start,stop,stepiter):
#     step = iter(stepiter)
#     while start < stop:
#         yield start
#         start += next(step)

# index_key = {0:'a', 1:'b',2:'c', 3:'d', 4:'e', 5:'f'}
# global_index = 0
# btarray_slice_list = [[0,3,0,1,0,1,2,1,1,4,3,4,4,4], [0,3,3,1,3,2,2,2,2,5,3,5,5,5]]


# key_index_list = []
# w_value = []
# steplist = []

# for i in range(2):

#     btarray_slice = btarray_slice_list[i]

#     idx = 3
#     while (idx < len(btarray_slice)):
#         steplist.append(btarray_slice[idx]+2)
#         # logging.info(f'steplist,{steplist}')
#         idx = idx + int(btarray_slice[idx]) +2


#     for num in varied_step_range(2,len(btarray_slice),steplist):
#         key_index_list.append(int(btarray_slice[num]))
#         # logging.info(f'key_index_list,{key_index_list}')
#         num = num + 1
#         length = int(btarray_slice[num])
#         w_value.append(np.array(btarray_slice[num+1:num+length+1]))
    

#     for i in range(global_index,len(steplist) + global_index,1):              

#         print(index_key[key_index_list[i]])
#         global_index = global_index + 1 
#         print(global_index)
# filename = f'./wh/{args.num_users}/'+str(iter)+'_'+str(idx)+'_'+str(key_index[k])+'.txt'
# f = open(filename, 'w+', encoding='utf-8')