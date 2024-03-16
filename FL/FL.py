#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Python version: 3.6

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import copy
import numpy as np
from torchvision import datasets, transforms
import torch

from utils.sampling import mnist_iid, mnist_noniid, cifar_iid
from utils.options import args_parser
from models.Update import LocalUpdate
from models.Nets import MLP, CNNMnist, CNNCifar, resnet18, resnet101, resnet50, AlexNet
from models.Fed import FedAvg
from models.test import test_img, test_tabular
from models.Nets import MLPAdult
from models.Update import Adult_dataloader, load_tensor, Celeba_Dataset
from utils.info import get_bias, save_info

import logging
import time
import torchvision.models as models


if __name__ == '__main__':
    # parse args
    args = args_parser()
    args.device = torch.device('cuda:{}'.format(args.gpu) if torch.cuda.is_available() and args.gpu != -1 else 'cpu')
    logging.basicConfig(filename=f'./save/FL/log/{args.dataset}_{args.model}_{str(args.epochs)}_{str(args.num_users)}_{args.model}_{str(args.iid)}_{str(args.frac)}_{str(args.bs)}_{str(args.lr)}_{args.task_id}.log', level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

    global_acc = [0 for j in range(args.num_run)]
    global_loss = [0 for j in range(args.num_run)]
    all_time = [0 for j in range(args.num_run)]
    agg = [0 for j in range(args.num_run)]

    for i in range(args.num_run):
        start = time.time()

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
        # elif args.dataset == 'cifar' and args.model == 'alexnet':
        #     trans_cifar = transforms.Compose([  
        #         transforms.Resize((32, 32)),  
        #         transforms.ToTensor(),  
        #         transforms.Normalize(mean=[0.5, 0.5, 0.5], std=[0.5, 0.5, 0.5])  
        #     ])
        #     # trans_cifar = transforms.Compose([transforms.ToTensor(), transforms.Normalize((0.5, 0.5, 0.5), (0.5, 0.5, 0.5))])
        #     dataset_train = datasets.CIFAR10('../data/cifar', train=True, download=True, transform=trans_cifar)
        #     dataset_test = datasets.CIFAR10('../data/cifar', train=False, download=True, transform=trans_cifar)
        #     if args.iid:
        #         dict_users = cifar_iid(dataset_train, args.num_users)
        #     else:
        #         exit('Error: only consider IID setting in CIFAR10')
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
            print(len(data_train))
            train_size = len(data_train) - test_size
            dataset_train, dataset_test = torch.utils.data.random_split(data_train, [train_size, test_size])
            # val_ds = test_ds
            dict_users = mnist_iid(dataset_train, args.num_users)
        elif args.dataset == 'celeba':
            # celeba_transform = transforms.Compose([transforms.Resize((224, 224)), transforms.ToTensor(), transforms.Normalize([0.485, 0.456, 0.406], [0.229, 0.224, 0.225])])
            # celeba_transform = transforms.Compose([transforms.CenterCrop((178, 178)), transforms.Resize((128, 128)), transforms.ToTensor()])
            celeba_transform=transforms.Compose([
                                        transforms.Resize((224, 224)),  # 调整图像尺寸为 224x224
                                        transforms.ToTensor(),  # 将图像转换为张量
                                        transforms.Normalize(mean=[0.5, 0.5, 0.5], std=[0.5, 0.5, 0.5])])  # 标准化图像张量  
            
            # dataset_train = datasets.CelebA(root='/home/notebook/data/group/privacy/research', split='train', transform=celeba_transform, download=False)
            # dataset_valid = datasets.CelebA(root='/home/notebook/data/group/privacy/research', split='valid', transform=celeba_transform, download=False)
            # dataset_test = datasets.CelebA(root='/home/notebook/data/group/privacy/research', split='test', transform=celeba_transform, download=False)

            img_dir='/home/notebook/data/group/privacy/research/celeba/img_align_celeba'
            data_train = Celeba_Dataset('/home/notebook/data/group/privacy/research/celeba/celeba-gender-train.csv', img_dir, transform=celeba_transform)           
            dataset_train, _ = torch.utils.data.random_split(data_train, [int(len(data_train)*0.6), len(data_train)-int(len(data_train)*0.6)])
            # dataset_valid = Celeba_Dataset('/home/notebook/data/group/privacy/research/celeba/celeba-gender-valid.csv', img_dir, transform=celeba_transform)
            data_test = Celeba_Dataset('/home/notebook/data/group/privacy/research/celeba/celeba-gender-test.csv', img_dir, transform=celeba_transform)
            dataset_test, _ = torch.utils.data.random_split(data_test, [int(len(data_test)*0.6), len(data_test)-int(len(data_test)*0.6)])
            dict_users = mnist_iid(dataset_train, args.num_users)
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
        elif args.model == 'mlp' and args.dataset == 'adult':
            net_glob = MLPAdult().to(args.device)
        elif args.model == 'alexnet' and args.dataset == 'cifar':
            net_glob = AlexNet(10).to(args.device)  
        elif args.model == 'mlp':
            len_in = 1
            for x in img_size:
                len_in *= x
            net_glob = MLP(dim_in=len_in, dim_hidden=200, dim_out=args.num_classes).to(args.device)
        elif args.model == 'resnet101' and args.dataset == 'Covid':
            net_glob = resnet101(3, False).to(args.device)
        elif args.model == 'resnet101' and args.dataset == 'celeba':
            net_glob = resnet101(2, False).to(args.device)
        elif args.model == 'resnet50' and args.dataset == 'celeba':
            net_glob = resnet50(2, False).to(args.device)
            # 加载预训练的 ResNet-101 模型  
            # net_glob = models.resnet101(pretrained=True)  
      
            # 获取最后一层全连接层  
            # fc_in_features = net_glob.fc.in_features  
            # 修改全连接层的输出维度  
            # num_attributes = 1  # 二分类任务，所以输出维度为 1  
            # net_glob.fc = torch.nn.Linear(fc_in_features, num_attributes)  
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
        agg_time = 0
        if args.all_clients: 
            logging("Aggregation over all clients")
            w_locals = [w_glob for i in range(args.num_users)]

        for iter in range(args.epochs):
            net_glob.train()
            loss_locals = []
            if not args.all_clients:
                w_locals = []
            m = max(int(args.frac * args.num_users), 1)
            idxs_users = np.random.choice(range(args.num_users), m, replace=False)
            for idx in idxs_users:
                local = LocalUpdate(args=args, dataset=dataset_train, idxs=dict_users[idx])
                w, loss = local.train(net=copy.deepcopy(net_glob).to(args.device))
                if args.all_clients:
                    w_locals[idx] = copy.deepcopy(w)
                else:
                    w_locals.append(copy.deepcopy(w))
                loss_locals.append(copy.deepcopy(loss))
            # update global weights
            time1 = time.time()
            w_glob = FedAvg(w_locals)
            time2 = time.time()
            agg_time += (time2 - time1)
            # copy weight to net_glob
            net_glob.load_state_dict(w_glob)

            # logging loss
            loss_avg = sum(loss_locals) / len(loss_locals)
            logging.info('Round {:3d}, Average loss {:.3f}'.format(iter, loss_avg))
            loss_train.append(loss_avg)

            start1 = time.time()
            if args.dataset == 'adult':
                acc_train, loss_train1 = test_tabular(net_glob, dataset_train, args)
                acc_test, loss_test = test_tabular(net_glob, dataset_test, args)
                logging.info(f'[Train] loss: {loss_train1:6.6f} | acc: {acc_train:6.6f}.')
                logging.info(f'[Test ] loss: {loss_test:6.6f} | acc: {acc_test:6.6f}.')
            else:
                acc_train, loss_train1 = test_img(net_glob, dataset_train, args)
                acc_test, loss_test = test_img(net_glob, dataset_test, args)
                logging.info(f'[Train] loss: {loss_train1:6.6f} | acc: {acc_train:6.6f}.')
                logging.info(f'[Test ] loss: {loss_test:6.6f} | acc: {acc_test:6.6f}.')
            end1 = time.time()
            test_time += (end1 - start1)

            loss1.append(loss_test)
            acc.append(acc_test)
            acc_train1.append(acc_train)
        end = time.time()
        all_time[i] = (end-start-test_time)
        agg[i] = agg_time

        # plot loss curve
        plt.figure()
        plt.plot(range(len(loss_train)), loss_train)
        plt.ylabel('train_loss')
        plt.savefig('./save/fed_{}_{}_{}_C{}_iid{}.png'.format(args.dataset, args.model, args.epochs, args.frac, args.iid))

        # save accuracy, loss
        save_info(loss_train, './save/FL/train_loss/', args, i)
        save_info(loss1, './save/FL/test_loss/', args, i)
        save_info(acc, './save/FL/acc/', args, i)
        save_info(acc_train1, './save/FL/acc_train1/', args, i)
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

        
        logging.info("time is: {:.3f}".format(all_time[i]))

    logging.info(f'accuracy: {global_acc}')
    logging.info(f'loss: {global_loss}')
    logging.info(f'time: {all_time}')
    get_bias('accuracy', global_acc, args)
    get_bias('loss', global_loss, args)
    get_bias('time', all_time, args)
    get_bias('agg_time', agg, args)

