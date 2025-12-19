import random
import warnings

from torch.nn import Module

from config import *
from utils.dataloaders import *
from models import *
from trainers import *
from torch.optim.lr_scheduler import CyclicLR, MultiStepLR


def choose_model(dataset_name: DataName, model_name: ModelName):
    num_classes_dict = {
        DataName.CIFAR10: 10,
        DataName.CIFAR100: 100,
        DataName.SVHN: 10,
        DataName.GTSRB: 43,
        DataName.STL10: 10,
        DataName.TinyImageNet: 200,
    }

    num_classes = num_classes_dict[dataset_name]

    if model_name is ModelName.PreActResNet_18:
        if dataset_name in [DataName.TinyImageNet, DataName.STL10]:
            model = PreActResNet18LargeInput(num_classes=num_classes)
        elif dataset_name in [DataName.CIFAR10, DataName.CIFAR100, DataName.SVHN, DataName.GTSRB]:
            model = PreActResNet18(num_classes=num_classes)
        else:
            raise ValueError
    else:
        if dataset_name is DataName.TinyImageNet:
            WRN = WRN_imagenet
        else:  # for 32*32 size image
            WRN = WRN_cifar

        if model_name is ModelName.WideResNet_34_8:
            model = WRN(depth=34, widen_factor=8, num_classes=num_classes)
        elif model_name is ModelName.WideResNet_28_10:
            model = WRN(depth=28, widen_factor=10, num_classes=num_classes)
        else:
            raise ValueError

    return model


def prepare_data(dataset_name: DataName, train_id, test_id):
    if dataset_name is DataName.CIFAR10:
        get_loader = get_cifar10_loader
        num_classes = 10
    elif dataset_name is DataName.CIFAR100:
        get_loader = get_cifar100_loader
        num_classes = 100
    elif dataset_name is DataName.SVHN:
        get_loader = get_SVHN_loader
        num_classes = 10
    elif dataset_name is DataName.GTSRB:
        get_loader = get_GTSRB_loader
        num_classes = 43
    elif dataset_name is DataName.STL10:
        get_loader = get_STL10_loader
        num_classes = 10
    elif dataset_name is DataName.TinyImageNet:
        get_loader = get_tiny_imagenet_200_loader
        num_classes = 200
    else:
        raise NotImplementedError

    train_loader, test_loader, targets = get_loader(train_id=train_id, test_id=test_id)
    return train_loader, test_loader, targets, num_classes


def get_trainer(name_method: MethodName):
    method_trainer_dict = {
        MethodName.NAT: NatTrainer,

        MethodName.PGD: PgdAdvTrainer,
        MethodName.TRADES: TradesAdvTrainer,

        MethodName.FGSM: FgsmAdvTrainer,
        MethodName.RFGSM: RandomFgsmAdvTrainer,
    }

    trainer = method_trainer_dict[name_method]
    print('=========================')
    print(f'Use {trainer.__name__}!')
    print('=========================')

    return trainer


def set_seed(seed: int = 0):
    random.seed(seed)
    torch.manual_seed(seed)
    np.random.seed(seed)
    torch.cuda.manual_seed_all(seed)


def get_device():
    if torch.cuda.is_available():
        device = torch.device('cuda:0')
        torch.backends.cudnn.benchmark = True
        torch.backends.cudnn.deterministic = True
    else:
        device = torch.device('cpu')
        warnings.warn("You are using CPU!")

    return device


def set_lr_param(cfg: Config, lr_type: str):
    if lr_type == 'step-wise':
        cfg.lr_init = 0.1
        cfg.lr_max = 0.1
        cfg.lr_min = 0.001
        cfg.lr_schedule_type = LrSchedulerType.STEP
    elif lr_type == 'cyclic-wise':
        cfg.lr_init = 0.
        cfg.lr_max = 0.2
        cfg.lr_min = 0.0
        cfg.lr_schedule_type = LrSchedulerType.CYCLIC
    else:
        raise ValueError


def get_lr_scheduler(total_epoch: int, len_loader: int, lr_schedule_type: LrSchedulerType,
                     optimizer, lr_min=None, lr_max=None):
    lr_steps = total_epoch * len_loader
    if lr_schedule_type is LrSchedulerType.STEP:
        # total 100 epoch, decay in 50/75 epoch
        lr_scheduler = MultiStepLR(optimizer,
                                   milestones=[int(lr_steps / 2 - len_loader), int(lr_steps * 3 / 4 - len_loader)],
                                   gamma=(lr_min / lr_max) ** (1 / 2))
    elif lr_schedule_type is LrSchedulerType.CYCLIC:
        lr_scheduler = CyclicLR(optimizer, base_lr=lr_min, max_lr=lr_max,
                                step_size_up=int(lr_steps * 2 / 5 - len_loader),
                                step_size_down=int(lr_steps * 3 / 5 + len_loader))
    else:
        raise ValueError

    return lr_scheduler


def save_epoch_weights(save_dir: str, i_epoch: int, model: Module):
    save_dir = Path(save_dir)
    save_dir.mkdir(parents=False, exist_ok=True)
    weights_path = save_dir / f'weights_{i_epoch}.pth'
    torch.save(model.state_dict(), weights_path)


def wait_gpu(seconds_wait=5, temperature: int = 60):
    import subprocess
    import time

    def is_gpu_free():
        query_threshold = {'temperature.gpu': temperature,
                           'memory.used': 800,
                           'utilization.gpu': 30,
                           'utilization.memory': 30}

        for t in query_threshold:
            result = subprocess.run(['nvidia-smi',
                                     f'--query-gpu={t}',
                                     '--format=csv,noheader'],
                                    stdout=subprocess.PIPE)

            result = int(result.stdout[:-1].decode('utf-8').split(' ')[0])

            if result > query_threshold[t]:
                print(f'{t}: {result} > {query_threshold[t]}, not ready')
                return False

        return True

    while not is_gpu_free():
        time.sleep(seconds_wait)


def get_nat_checkpoint_for_HelperAT(dataset: DataName, model: ModelName, seed: Seed):
    from config import dir_nat_ckpts
    dir_ckpt = {
        DataName.CIFAR10: {
            ModelName.PreActResNet_18: dir_nat_ckpts.joinpath('CIFAR10_PreActResNet18/epoch_50/cyclic_lr'),
            ModelName.WideResNet_28_10: dir_nat_ckpts.joinpath('CIFAR10_WideResNet_28_10/epoch_50/cyclic_lr'),
        },
        DataName.CIFAR100: {
            ModelName.PreActResNet_18: dir_nat_ckpts.joinpath('CIFAR100_PreActResNet18/epoch_50/cyclic_lr'),
            ModelName.WideResNet_28_10: dir_nat_ckpts.joinpath('CIFAR100_WideResNet_28_10/epoch_50/cyclic_lr'),
        },
        DataName.TinyImageNet: {
            ModelName.PreActResNet_18: dir_nat_ckpts.joinpath('TinyImageNet_PreActResNet18/epoch_50/cyclic_lr'),
        },
    }

    path_ckpt = dir_project_root.joinpath(dir_ckpt[dataset][model], f'{seed.name}.pth')

    nat_model = choose_model(dataset, model).cuda()
    nat_model.load_state_dict(torch.load(path_ckpt))

    return nat_model
