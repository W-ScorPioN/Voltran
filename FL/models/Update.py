#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Python version: 3.6

import torch
from torch import nn, autograd
from torch.utils.data import DataLoader, Dataset, TensorDataset
import numpy as np
import random
from sklearn import metrics
import pandas as pd
from PIL import Image
import os

class DatasetSplit(Dataset):
    def __init__(self, dataset, idxs):
        self.dataset = dataset
        self.idxs = list(idxs)

    def __len__(self):
        return len(self.idxs)

    def __getitem__(self, item):
        image, label = self.dataset[self.idxs[item]]
        return image, label


class LocalUpdate(object):
    def __init__(self, args, dataset=None, idxs=None):
        self.args = args
        if args.dataset == 'adult':
            self.loss_func = nn.BCEWithLogitsLoss()
        else:
            self.loss_func = nn.CrossEntropyLoss()
        self.selected_clients = []
        self.ldr_train = DataLoader(DatasetSplit(dataset, idxs), batch_size=self.args.local_bs, shuffle=True, num_workers=4)

    def train(self, net):
        net.train()
        # train and update
        optimizer = torch.optim.SGD(net.parameters(), lr=self.args.lr, momentum=self.args.momentum)

        epoch_loss = []
        for iter in range(self.args.local_ep):
            batch_loss = []
            for batch_idx, (images, labels) in enumerate(self.ldr_train):
                images, labels = images.to(self.args.device), labels.to(self.args.device)
                net.zero_grad()
                # log_probs, x = net(images)
                # import pdb
                # pdb.set_trace()
                log_probs = net(images)
                
                # python debug
                # if self.args.dataset == 'Covid':
                    # log_probs = torch.Tensor(log_probs)
                loss = self.loss_func(log_probs, labels)
                loss.backward()
                optimizer.step()
                if self.args.verbose and batch_idx % 10 == 0:
                    print('Update Epoch: {} [{}/{} ({:.0f}%)]\tLoss: {:.6f}'.format(
                        iter, batch_idx * len(images), len(self.ldr_train.dataset),
                               100. * batch_idx / len(self.ldr_train), loss.item()))
                batch_loss.append(loss.item())
            epoch_loss.append(sum(batch_loss)/len(batch_loss))
        return net.state_dict(), sum(epoch_loss) / len(epoch_loss)



def load_tensor(filename):
    arr = np.load(filename)
    if filename.endswith(".npz"):
        arr = arr["arr_0"]
    tensor = torch.tensor(arr)
    return tensor


def Adult_dataloader(dataset):
    data, targets = dataset[:, :-1], dataset[:, -1]
    data = data.float()
    targets = targets.float()
    datasets = TensorDataset(data, targets)
    return datasets

class Celeba_Dataset(Dataset):
    """Custom Dataset for loading CelebA face images"""

    def __init__(self, csv_path, img_dir, transform=None):
    
        df = pd.read_csv(csv_path, index_col=0)#index_col=0 ：表示将第一列设置为index值
        self.img_dir = img_dir #图片所在的文件夹
        self.csv_path = csv_path #性别对应图片的关系
        self.img_names = df.index.values #such as：list_img[] = [XXXX.jpg]
        self.y = df['Male'].values      # such as:list[]=[0 or 1]
        self.transform = transform

    def __getitem__(self, index):
        img = Image.open(os.path.join(self.img_dir,self.img_names[index]))
        if self.transform is not None:
            img = self.transform(img)
        
        label = self.y[index]
        return img, label

    def __len__(self):
        return self.y.shape[0]