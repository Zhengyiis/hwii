from dataclasses import dataclass
from enum import Enum, auto
from pathlib import Path




dir_dataset = Path('datasets')
dir_project_root = Path('AT_demo')  # '/.../AT_demo'

dir_wandb_to_sync = dir_project_root.joinpath('wandb_sync')
dir_wandb_saved_local = dir_project_root.joinpath('wandb_local')

assert dir_dataset != 'YOUR_PATH', 'change dir_dataset to your own path!'
assert dir_project_root != 'YOUR_PATH', 'change dir_project_root to your own path!'


class LrSchedulerType(Enum):
    STEP = auto()
    CYCLIC = auto()


class MethodName(Enum):
    NAT = auto()

    PGD = auto()
    TRADES = auto()

    FGSM = auto()
    RFGSM = auto()

class DataName(Enum):
    CIFAR10 = auto()
    CIFAR100 = auto()
    SVHN = auto()
    GTSRB = auto()
    STL10 = auto()
    TinyImageNet = auto()


class ModelName(Enum):
    PreActResNet_18 = auto()  # for CIFAR data
    WideResNet_28_10 = auto()  # for CIFAR data
    WideResNet_34_8 = auto()  # for TinyImageNet data


class Seed(Enum):
    SEED_1 = 1
    SEED_2 = 2
    SEED_3 = 3


class RobEvalAttack(Enum):
    PGD_my = auto()
    PGD_common = auto()
    PGD_50_10 = auto()  # 50 iterations with 10 restarts
    CW = auto()  # C&W attack
    Auto = auto()  # auto attack


@dataclass
class ConfigLinfAttack:
    epsilon: float = 8 / 255
    perturb_steps: int = None
    step_size: float = None


@dataclass
class Config:
    is_use_wandb: bool = True

    lr_init: float = 0.1
    lr_max: float = 0.1
    lr_min: float = 0.0
    momentum: float = 0.9
    weight_decay: float = 2e-4
    total_epoch: int = 100

    lr_schedule_type: LrSchedulerType = None

    method: MethodName = None
    data: DataName = None
    model: ModelName = None
    seed: Seed = None

    param_atk_train: ConfigLinfAttack = None
    param_atk_eval: ConfigLinfAttack = None
