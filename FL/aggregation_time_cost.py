import time
import copy

from models.Nets import resnet50, resnet18
from models.Fed import FedAvg

if __name__ == "__main__":
    model = resnet50(num_classes=2000000, GRAYSCALE=False) # load ResNet-56 model
    # model = resnet18(num_classes=10, GRAYSCALE=False)
    communication_round = 50
    num_client = 100

    net_parameters_dict = copy.deepcopy(model.state_dict())
    client_models_list = [[net_parameters_dict for _ in range(num_client)] for i in range(communication_round)]
    time1 = time.time()
    for i in range(communication_round):
        global_model = FedAvg(client_models_list[i])
        print(f"Communication round {i} finished!")
    time2 = time.time()
    time_cost = time2 - time1
    print(f"Time cost is : {time_cost}")