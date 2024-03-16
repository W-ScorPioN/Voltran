import torch
from torchvision import models
from torchsummary import summary

from models.Nets import CNNMnist, CNNCifar, ResNet, BasicBlock, MLPAdult
from utils.options import args_parser


args = args_parser()

model = CNNMnist(args)
# model = ResNet(block=BasicBlock,
#                 layers=[2, 2, 2, 2],
#                 num_classes=10,
#                 grayscale=False) # load ResNet-56 model
# model = MLPAdult()
param_size = 0 # initialize parameter size

for param in model.parameters(): # loop over all parameters
    param_size += param.numel() * 4 # add parameter size in bytes (4 bytes per float32)
print(f"Model file size: {param_size / (1024):.2f} KB") # print model file size in MB


summary(model, input_size=[(1, 28, 28)])