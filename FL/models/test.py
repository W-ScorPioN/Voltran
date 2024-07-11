#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @python: 3.6

import torch
from torch import nn
import torch.nn.functional as F
from torch.utils.data import DataLoader
from sklearn.metrics import accuracy_score


def test_img(net_g, datatest, args):
    net_g.eval()
    
    # testing
    test_loss = 0
    correct = 0
    data_loader = DataLoader(datatest, batch_size=args.bs)
    l = len(data_loader)
    for idx, (data, target) in enumerate(data_loader):
        if args.gpu != -1:
            data, target = data.cuda(), target.cuda()
        # log_probs, x = net_g(data)
        log_probs = net_g(data)
        # sum up batch loss
        test_loss += F.cross_entropy(log_probs, target, reduction='sum').item()
        # get the index of the max log-probability
        y_pred = log_probs.data.max(1, keepdim=True)[1]
        correct += y_pred.eq(target.data.view_as(y_pred)).long().cpu().sum()

    test_loss /= len(data_loader.dataset)
    accuracy = 100.00 * correct / len(data_loader.dataset)
    if args.verbose:
        print('\nTest set: Average loss: {:.4f} \nAccuracy: {}/{} ({:.2f}%)\n'.format(
            test_loss, correct, len(data_loader.dataset), accuracy))
    return accuracy, test_loss


def test_tabular(net_g, datatest, args):  
    net_g.eval()
    dataloader = DataLoader(datatest, batch_size=args.local_bs)

    count = len(dataloader)
    total_loss = 0
    pred_probs = []
    true_ys = []
    for _, (xs, ys) in enumerate(dataloader):
        if args.gpu != -1:
            xs, ys = xs.cuda(), ys.cuda()  
        out = net_g(xs)
        loss = torch.nn.BCEWithLogitsLoss()
        loss_val = loss(out, ys)

        pred_prob = torch.sigmoid(out)
        pred_probs.extend(torch.flatten(pred_prob).tolist())
        true_ys.extend(torch.flatten(ys).int().tolist())
        total_loss += loss_val.item()
    pred_ys = [int(p > 0.5) for p in pred_probs]
    return accuracy_score(true_ys, pred_ys)*100, total_loss/count
	

