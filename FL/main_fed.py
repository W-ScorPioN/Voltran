#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Python version: 3.6

import base64
import struct
import matplotlib
import zipfile
import os, tarfile

from decrypt import AES_GCM, InvalidTagException, varied_step_range
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import copy
import numpy as np
from torchvision import datasets, transforms
import torch
import gc
import os
import pickle
import requests


from utils.sampling import mnist_iid, mnist_noniid, cifar_iid, fashionMnist_iid, fashionMnist_noniid
from utils.options import args_parser
from utils.info import get_bias, save_info
from models.Update import LocalUpdate
from models.Nets import MLP, CNNMnist, CNNCifar, resnet18, Darknet, resnet101
from models.Fed import FedAvg
from models.test import test_img, test_tabular
from models.Nets import MLPAdult
from models.Update import Adult_dataloader, load_tensor
from Crypto.Cipher import AES
from Crypto.Util import Counter
from Crypto.Util.number import long_to_bytes, bytes_to_long
import base64
import time
import json
import logging

def make_zip(source_dir, output_filename):
    zipf = zipfile.ZipFile(output_filename, 'w')    
    pre_len = len(os.path.dirname(source_dir))
    for parent, dirnames, filenames in os.walk(source_dir):
        for filename in filenames:
            pathfile = os.path.join(parent, filename)
            arcname = pathfile[pre_len:].strip(os.path.sep)     #相对路径
            zipf.write(pathfile, arcname)
    zipf.close()

def getCiphertext(taskid, round, index):
    flg = True
    headers={'User-Agent': 'Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/67.0.3396.87 Safari/537.36',}
    url = 'http://10.50.146.119:9001/tee/getCiphertext'
    params = {'taskID':taskid, 'round':round, 'index':index}
    a = ''
    extra_time = 0
    while flg:
        # print('aaa')
        response = requests.post(url=url, json=params, headers=headers).text
        response_byte = response.encode('utf-8')
        response_json = json.loads(response_byte)
        opcode = response_json['code']
        if opcode != '200':
            start = time.time()
            time.sleep(1)
            end = time.time()
            extra_time += (end-start)
            continue
        flg = False
        # print('bbb')
        a = response_json['result']

        # getAuthTag(taskid, round, index)
    return a, extra_time

def getAuthTag(taskid, round, index):
    flg = True
    headers={'User-Agent': 'Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/67.0.3396.87 Safari/537.36',}
    url = 'http://10.50.146.119:9001/tee/getAuth_tag'
    params = {'taskID':taskid, 'round':round, 'index':index}
    a = ''
    while flg:
        response = requests.post(url=url, json=params, headers=headers).text
        response_byte = response.encode('utf-8')
        response_json = json.loads(response_byte)
        opcode = response_json['code']
        if opcode != '200':
            time.sleep(1)
            logging.info(f'opcode:{opcode}')
            continue
        flg = False
        a = response_json['result']

        # getAuthTag(taskid, round, index)
    return a

# def varied_step_range(start,stop,stepiter):
#     step = iter(stepiter)
#     while start < stop:
#         yield start
#         start += next(step)

if __name__ == '__main__':
    gc.collect()
    torch.cuda.empty_cache()
    args = args_parser()
    logging.basicConfig(filename=f'./save/ours/log/{args.dataset}_{args.model}_{str(args.epochs)}_{str(args.num_users)}_{args.model}_{str(args.iid)}_{str(args.frac)}_{str(args.bs)}_{args.task_id}.log', level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
    # parse args
    args = args_parser()
    args.device = torch.device('cuda:{}'.format(args.gpu) if torch.cuda.is_available() and args.gpu != -1 else 'cpu')

    global_acc = [0 for j in range(args.num_run)]
    global_loss = [0 for j in range(args.num_run)]
    global_agg = [0 for j in range(args.num_run)]
    all_time = [0 for j in range(args.num_run)]

    for i in range(args.num_run):
        start = time.time()
        agg_time = 0
        # extra_time = 0

        # load dataset and split users
        if args.dataset == 'mnist':
            trans_mnist = transforms.Compose([transforms.ToTensor(), transforms.Normalize((0.1307,), (0.3081,))])
            dataset_train = datasets.MNIST('../data/mnist/', train=True, download=True, transform=trans_mnist)
            dataset_test = datasets.MNIST('../data/mnist/', train=False, download=True, transform=trans_mnist)
            # sample users
            if args.iid:
                dict_users = mnist_iid(dataset_train, args.num_users)
            else:
                dict_users = mnist_noniid(dataset_train, args.num_users)
        elif args.dataset == 'cifar':
            trans_cifar = transforms.Compose([transforms.ToTensor(), transforms.Normalize((0.5, 0.5, 0.5), (0.5, 0.5, 0.5))])
            dataset_train = datasets.CIFAR10('../data/cifar', train=True, download=True, transform=trans_cifar)
            dataset_test = datasets.CIFAR10('../data/cifar', train=False, download=True, transform=trans_cifar)
            if args.iid:
                dict_users = cifar_iid(dataset_train, args.num_users)
            else:
                exit('Error: only consider IID setting in CIFAR10')
        elif args.dataset == 'fashion_mnist':
            trans_fashionMnist = transforms.Compose([transforms.ToTensor(), transforms.Normalize([0.5], [0.5])])
            dataset_train = datasets.FashionMNIST('../data/fashion_mnist', train=True, download=True, transform=trans_fashionMnist)
            dataset_test = datasets.FashionMNIST('../data/fashion_mnist', train=False, download=True, transform=trans_fashionMnist)
            
            if args.iid:
                dict_users = fashionMnist_iid(dataset_train, args.num_users)
            else:
                dict_users = fashionMnist_noniid(dataset_train, args.num_users)
        elif args.dataset == 'mini_imagenet':
            trans_cifar = transforms.Compose([transforms.ToTensor(), transforms.Normalize((0.5, 0.5, 0.5), (0.5, 0.5, 0.5))])
            dataset_train = datasets.CIFAR10('../../../data/cifar', train=True, download=True, transform=trans_cifar)
            dataset_test = datasets.CIFAR10('../data/cifar', train=False, download=True, transform=trans_cifar)
            if args.iid:
                dict_users = cifar_iid(dataset_train, args.num_users)
            else:
                exit('Error: only consider IID setting in CIFAR10')
        elif args.dataset == 'adult' and args.model == 'mlp':
            train = load_tensor('/home/notebook/data/group/privacy/research/adult/adult.train.npz')
            test = load_tensor('/home/notebook/data/group/privacy/research/adult/adult.test.npz')
            
            dataset_train = Adult_dataloader(train)
            dataset_test = Adult_dataloader(test)

            if args.iid:
                dict_users = mnist_iid(dataset_train, args.num_users)
            else:
                dict_users = noniid(dataset_train, args.num_users, args.num_groups)

        elif args.dataset == 'Covid':
            train_data_transforms = transforms.Compose([
                transforms.Resize
                ((224, 224)), # 缩放
                transforms.RandomCrop(32, padding=4),
                transforms.RandomHorizontalFlip(),
                transforms.ToTensor(),
                transforms.Normalize([0.5, 0.5, 0.5],
                                    [0.5, 0.5, 0.5])])

            train_data_dir = '/home/notebook/data/group/privacy/research/Covid/'

            data_train = datasets.ImageFolder(train_data_dir, transform=train_data_transforms)
            data_test = datasets.ImageFolder(train_data_dir, transform=train_data_transforms)

            test_size = 300
            train_size = len(data_train) - test_size
            dataset_train, dataset_test = torch.utils.data.random_split(data_train, [train_size, test_size])
            # val_ds = test_ds
            dict_users = mnist_iid(dataset_train, args.num_users)

            # dict_sever = mnist_iid(test_ds, 1, args.num_items_test)
            # dict_users_val = mnist_iid(train_ds, args.num_users, args.num_items_train)
            # dict_val = mnist_iid(train_ds, args.num_users, args.num_items_train)
            # elif args.dataset == "imdb":
        #     trans_imdb = transforms.Compose([transforms.ToTensor(), transforms.Normalize()])
        #     dataset_train = datasets.IMDB()
        else:
            exit('Error: unrecognized dataset')
        img_size = dataset_train[0][0].shape


        # build model
        if args.model == 'cnn' and args.dataset == 'cifar':
            net_glob = CNNCifar(args=args).to(args.device)
        elif args.model == 'cnn' and args.dataset == 'mnist':
            net_glob = CNNMnist(args=args).to(args.device)
        elif args.model == 'resnet18' and args.dataset == 'cifar':
            net_glob = resnet18(10, False).to(args.device)
        elif args.model == 'resnet18' and args.dataset == 'fashion_mnist':
            net_glob = resnet18(10, True).to(args.device)
        elif args.model == 'mlp' and args.dataset == 'adult':
            net_glob = MLPAdult().to(args.device)
        elif args.model == 'mlp':
            len_in = 1
            for x in img_size:
                len_in *= x
            net_glob = MLP(dim_in=len_in, dim_hidden=200, dim_out=args.num_classes).to(args.device)
        elif args.model == 'resnet101':
            net_glob = resnet101(3, False).to(args.device)
        else:
            exit('Error: unrecognized model')
        logging.info(net_glob)
        net_glob.train()

        # copy weights
        w_glob = net_glob.state_dict()

        # training
        loss_train, acc_train1 = [], []
        acc = []
        loss1 = []
        cv_loss, cv_acc = [], []
        val_loss_pre, counter = 0, 0
        net_best = None
        best_loss = None
        val_acc_list, net_list = [], []
        acc_train, loss_train1 = 0, 0
        acc_test, loss_test = 0, 0
        test_time = 0

        if args.all_clients: 
            logging.info("Aggregation over all clients")
            w_locals = [w_glob for i in range(args.num_users)]
        for iter in range(args.epochs):
            net_glob.train()
            logging.info(f'Round: {iter+1}')
            loss_locals = []
            if not args.all_clients:
                w_locals = []
            m = max(int(args.frac * args.num_users), 1)
            idxs_users = np.random.choice(range(args.num_users), m, replace=False)
            for idx in idxs_users:
                local = LocalUpdate(args=args, dataset=dataset_train, idxs=dict_users[idx])
                w, loss = local.train(net=copy.deepcopy(net_glob).to(args.device))
                # print(type(w))
                # # f = open('./wh/'+str(idx)+'.txt', 'wb+')
                # # f.write(w)
                # print(w)
                # torch.save(w, './wh/'+str(idx)+'.pt')
                w_copy = copy.deepcopy(w)
                w_shape = []

                key_list = list(w_copy.keys())
                index_key = {index: key for index, key in enumerate(key_list)}
                key_index = {key: index for index, key in enumerate(key_list)}
                # logging.info(f'key_list,{key_list}') 

                # count = 0
                for (k,v) in w_copy.items():
                    v1_shape = w_copy[k].shape
                    # print('v1_shape:',v1_shape)
                    w_shape.append(v1_shape)
                    # print('w_shape:',w_shape)
                    v1 = w_copy[k].cpu().flatten().numpy()
                    w_str = ','.join(str(x) for x in v1)

                    
                    # 创建的目录
                    start1 = time.time()
                    path = './wh/'+ str(args.num_users)+'/'+ args.task_id+'/'+str(iter)+'/'
                    if not os.path.exists(path):
                        os.makedirs(path)
                    end1 = time.time()
                    test_time += (end1-start1)

                    filename = './wh/'+ str(args.num_users)+'/'+ args.task_id+'/'+str(iter)+'/'+str(iter)+'_'+str(idx)+'_'+str(key_index[k])+'.txt'
                    f = open(filename, 'w+', encoding='utf-8')
                    f.write(w_str)

                if args.all_clients:
                    w_locals[idx] = copy.deepcopy(w)
                else:
                    w_locals.append(copy.deepcopy(w))
                loss_locals.append(copy.deepcopy(loss))
            # update global weights
            # w_glob = FedAvg(w_locals)
            master_key = 0xD16FDC6C12B1E8D8AB7D82417934E1BE
            master_key_reverse = 0xBEE1347941827DABD8E8B1126CDC6FD1
            init_value = 0x0
            indexnumber = 0
            my_gcm = AES_GCM(master_key_reverse)
            with open('./config.json', 'r') as f:
                config_dict = json.load(f)   
                indexnumber = config_dict['indexnumber']  
                logging.info(f'indexnumber,{indexnumber}')   
            f.close()

            start1 = time.time()
            with tarfile.open('./wh/'+ str(args.num_users)+'/'+ args.task_id+ '/' +str(iter) +'.tar.gz', "w:gz") as tar:
                tar.add('./wh/'+ str(args.num_users) + '/'+ args.task_id + '/' +str(iter), arcname=os.path.basename('./wh/'+ str(args.num_users)+'/'+ args.task_id) + '/' +str(iter))
            end1 = time.time()
            test_time += (end1-start1)
            # w_copy = copy.deepcopy(w)
            # key_list = []
            # for (k,v) in w_copy.items():
            #     key_list.append(k)
            logging.info('Round {:3d}结束'.format(iter+1))

            global_index = 0
            w_value = []
            key_index_list = []
            extra_time = 0
            agg_start = time.time()
            # print("time is: {:.3f}".format(end - start))
            
            for index in range(indexnumber):

            ###### 获取链上的新的globalModel并解析成原来的Tensor格式，生成w_glob#########
            # 用taskid和round获取w_glob的str
                ciphertext_message, tmp = getCiphertext(args.task_id, str(iter), index)
                extra_time += tmp
                auth_tag_message = getAuthTag(args.task_id, str(iter), index)
            # # 解密
            #     while True:
            #         if 
            #         break
            # ciphertext_hex = 0x2096D84B84A329920A94BE5B98
            # ciphertext_byte = long_to_bytes(ciphertext_hex)
                # ciphertext_msg_byte = ciphertext_message.encode('utf-8')
                # ciphertext_msg_json = json.loads(ciphertext_msg_byte)
                # ciphertext_json = ciphertext_msg_json['result']
                # print('ciphertext',ciphertext_json, type(ciphertext_json))
                # ciphertext_hex = int(ciphertext_message,16)
                # logging.info(f'ciphertext_hex,{ciphertext_hex}')
                # print('ciphertext',ciphertext_json, type(ciphertext_json))
                # ciphertext_byte = ciphertext_json.encode('utf-8')
                ciphertext_byte = bytes.fromhex(ciphertext_message)
                # logging.info(f'ciphertext_byte: {ciphertext_byte}')
                # print(ciphertext_byte)

                # auth_tag_byte = auth_tag_message.encode('utf-8')
                # auth_tag_msg_json = json.loads(auth_tag_byte)
                # auth_tag_json = auth_tag_msg_json['result']
                # print('auth_tag',auth_tag_json, type(auth_tag_json))
                auth_tag = int(auth_tag_message,16)
                # logging.info(f'indexnumber,{indexnumber}')
                # logging.info(f'auth_tag: {auth_tag}')
                # print(auth_tag)
                # auth_tag_test = 0x8A1703A3779A5D8B886F4564000E53ED
                # auth_tag_hex = auth_tag_json.encode('utf-8')
                # # print('ciphertext',ciphertext_json)
                # # auth_tag_hex = auth_tag_message.encode('utf-8')
                # # print('auth_tag',auth_tag_hex)
                # auth_tag_big = int.from_bytes(auth_tag_hex,'big')
                # print('auth_tag_big',auth_tag_big)
                # auth_tag_little = int.from_bytes(auth_tag_hex,'little')
                # print('auth_tag_little',auth_tag_little)

            
                try:
                # decrypted = my_gcm.decrypt(init_value, plaintext, auth_tag + 1, b'')
                    decrypted_r = my_gcm.decrypt(init_value, ciphertext_byte, auth_tag +1, b'')
                except InvalidTagException:
                # decrypted = my_gcm.decrypt(init_value, plaintext, auth_tag, b'')
                    # print('使用小端')
                    decrypted_r = my_gcm.decrypt(init_value, ciphertext_byte, auth_tag, b'')           

                # logging.info(f'decrypted_r,{decrypted_r}')
                # w_glob_json = json.loads(decrypted_r)
                btarray = bytearray(decrypted_r)
                btarray_slice = [struct.unpack('d',btarray[i:i+8])[0] for i in range(0,len(btarray),8)]

                steplist = []

                # layer_list = w.keys()

                idx = 3
                while (idx < len(btarray_slice)):
                    steplist.append(int(btarray_slice[idx])+2)
                    # logging.info(f'steplist,{steplist}')
                    idx = idx + int(btarray_slice[idx]) +2

                for num in varied_step_range(2,len(btarray_slice),steplist):
                    key_index_list.append(int(btarray_slice[num]))
                    # logging.info(f'key_index_list,{key_index_list}')
                    num = num + 1
                    length = int(btarray_slice[num])
                    w_value.append(np.array(btarray_slice[num+1:num+length+1]))

                for i in range(global_index,len(steplist) + global_index,1):              
                    w_glob_feature_tensor = torch.reshape(torch.from_numpy(w_value[i]),w_shape[key_index_list[i]])
                    w_copy[index_key[key_index_list[i]]]= w_glob_feature_tensor 
                    global_index = global_index + 1 
                    # logging.info(f'w_copy_{i},{w_copy[index_key[key_index_list[i]]]}') 

                logging.info('Round {:3d}, indexnumber {:3d}结束'.format(iter+1, indexnumber))   

                # logging.info(f'w_copy,{w_copy}') 



                # w_glob_temp = w_glob_json['globalModel']
            ########
            #需要加一步将多index整合成一个的
            ########
                # w_glob_feature_json = json.loads(w_glob_temp)
                # print(type(w_glob_feature_json))
            # ######################################

            #可用版本##
            # w_glob_feature_json = json.loads(json.loads(getGlobalModel('123', str(iter)).text)['result'])
                
                
                # for i in range(len(w_glob_temp)):
                # # for i in range(len(w_glob_feature_json)):
                #     key = w_glob_temp[i]["key"]
                #     data = np.array(w_glob_temp[i]["data"])
                #     w_glob_feature_tensor = torch.reshape(torch.from_numpy(data),w_shape[i])
                #     w[layer_list[key]]= w_glob_feature_tensor

            # print(w)
            w_glob = copy.deepcopy(w_copy)
            # logging.info(f'Aggregated model: {w_glob}')
            agg_end = time.time()
            agg_time = agg_end - agg_start - extra_time
            logging.info("Round {:3d} aggregation time is: {:.3f}".format(iter+1, agg_time))
            # w_glob = {**w_glob, **w}
            #可用版本结束##

            # copy weight to net_glob
            net_glob.load_state_dict(w_glob)
            # logging.info(f'load_state_dict: {net_glob.state_dict()}')

            # print loss
            start1 = time.time()
            loss_avg = sum(loss_locals) / len(loss_locals)
            logging.info('Round {:3d}, Average loss {:.3f}'.format(iter+1, loss_avg))
            loss_train.append(loss_avg)

            net_glob.eval()
            if args.dataset == 'adult':
                acc_train, loss_train1 = test_tabular(net_glob, dataset_train, args)
                # logging.info(f'load_state_dict: {net_glob.state_dict()}')
                acc_test, loss_test = test_tabular(net_glob, dataset_test, args)
                # logging.info(f'load_state_dict: {net_glob.state_dict()}')

                logging.info(f'[Train] loss: {loss_train1:6.6f} | acc: {acc_train:6.6f}.')
                logging.info(f'[Test ] loss: {loss_test:6.6f} | acc: {acc_test:6.6f}.')
            elif args.dataset == 'mnist' or args.dataset == 'cifar':
                acc_train, loss_train1 = test_img(net_glob, dataset_train, args)
                # logging.info(f'load_state_dict: {net_glob.state_dict()}')
                acc_test, loss_test = test_img(net_glob, dataset_test, args)
                # logging.info(f'load_state_dict: {net_glob.state_dict()}')
                logging.info(f'[Train] loss: {loss_train1:6.6f} | acc: {acc_train:6.6f}.')
                logging.info(f'[Test ] loss: {loss_test:6.6f} | acc: {acc_test:6.6f}.')
            
            loss1.append(loss_test)
            acc.append(acc_test)
            acc_train1.append(acc_train)
            end1 = time.time()
            test_time += (end1- start1)

        end = time.time()
        # plot loss curve
        plt.figure()
        plt.plot(range(len(loss_train)), loss_train)
        plt.ylabel('train_loss')
        plt.savefig('./save/fed_{}_{}_{}_C{}_iid{}.png'.format(args.dataset, args.model, args.epochs, args.frac, args.iid))

        # save accuracy, loss
        save_info(loss_train, './save/ours/train_loss/', args, i)
        save_info(loss1, './save/ours/test_loss/', args, i)
        save_info(acc, './save/ours/acc/', args, i)
        save_info(acc_train1, './save/ours/acc_train1/', args, i)


        # save model
        PATH = './model/'
        torch.save(net_glob.state_dict(), PATH + args.dataset + '.pth')
        # testing
        net_glob.eval()

        logging.info('----------------------Test Model----------------------')
        if args.dataset == 'adult':
            acc_train, loss_train1 = test_tabular(net_glob, dataset_train, args)
            acc_test, loss_test = test_tabular(net_glob, dataset_test, args)
            logging.info(f'[Train] loss: {loss_train1:6.6f} | acc: {acc_train:6.6f}.')
            logging.info(f'[Test ] loss: {loss_test:6.6f} | acc: {acc_test:6.6f} .')
        else:
            acc_train, loss_train1 = test_img(net_glob, dataset_train, args)
            acc_test, loss_test = test_img(net_glob, dataset_test, args)
            logging.info(f'[Train] loss: {loss_train1:6.6f} | acc: {acc_train:6.6f}.')
            logging.info(f'[Test ] loss: {loss_test:6.6f} | acc: {acc_test:6.6f} .')

        logging.info("Training accuracy: {:.3f}".format(acc_train))
        logging.info("Testing accuracy: {:.3f}".format(acc_test))

        
        global_acc[i] = acc_test
        global_loss[i] = loss_test
        global_agg[i] = agg_time
        
        all_time[i] = (end-start-test_time)
        logging.info("time is: {:.3f}".format(all_time[i]))

        logging.info('----------------------End----------------------')

    
    logging.info(f'accuracy: {global_acc}')
    logging.info(f'loss: {global_loss}')
    logging.info(f'loss: {global_agg}')
    logging.info(f'time: {all_time}')
    get_bias('accuracy', global_acc, args)
    get_bias('loss', global_loss, args)
    get_bias('aggregation time', global_agg, args)
    get_bias('time', all_time, args)
