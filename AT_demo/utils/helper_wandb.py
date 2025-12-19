import os
from dataclasses import asdict, is_dataclass
from pathlib import Path
from typing import get_type_hints

import numpy as np
import torch
import wandb

from config import Config, dir_wandb_to_sync, dir_wandb_saved_local


class WandbRecorder:
    def __init__(self):
        self._get_metric_step()
        self._get_metrics_for_batch()
        self._get_metrics_for_epoch()
        self._get_metrics_for_eval()

    def _get_metric_step(self):
        self.batch_step = 'batch_step'
        self.epoch_step = 'epoch_step'
        wandb.define_metric(self.batch_step)
        wandb.define_metric(self.epoch_step)

    def _get_metrics_for_batch(self):
        self.epoch = 'train/epoch'  # for align batch
        self.lr = 'train/lr'
        self.loss_total = 'train/loss_total'
        self.loss_nat = 'train/loss_nat'
        self.loss_adv = 'train/loss_adv'
        self.correct_rate_nat_batch = 'train/correct_rate_nat_batch'
        self.correct_rate_adv_batch = 'train/correct_rate_adv_batch'

        wandb.define_metric(self.lr, step_metric=self.batch_step)
        wandb.define_metric(self.loss_total, step_metric=self.batch_step)
        wandb.define_metric(self.loss_nat, step_metric=self.batch_step)
        wandb.define_metric(self.loss_adv, step_metric=self.batch_step)
        wandb.define_metric(self.epoch, step_metric=self.batch_step)
        wandb.define_metric(self.correct_rate_nat_batch, step_metric=self.batch_step)
        wandb.define_metric(self.correct_rate_adv_batch, step_metric=self.batch_step)

    def _get_metrics_for_epoch(self):
        self.correct_rate_nat_epoch = 'train/correct_rate_nat_epoch'
        self.correct_rate_adv_epoch = 'train/correct_rate_adv_epoch'
        wandb.define_metric(self.correct_rate_nat_epoch, step_metric=self.epoch_step)
        wandb.define_metric(self.correct_rate_adv_epoch, step_metric=self.epoch_step)

    def _get_metrics_for_eval(self):
        self.test_acc = 'test/acc'
        self.test_rob = 'test/rob'
        wandb.define_metric(self.test_acc, step_metric=self.epoch_step)
        wandb.define_metric(self.test_rob, step_metric=self.epoch_step)

    def record_train_batch_info(self, idx_batch_total, idx_epoch, lr, loss_total, loss_nat=None, loss_adv=None,
                                correct_rate_nat_batch=None, correct_rate_adv_batch=None):
        metric_batch_step = {self.batch_step: idx_batch_total}
        wandb.log({self.epoch: idx_epoch, **metric_batch_step})
        wandb.log({self.lr: lr, **metric_batch_step})
        wandb.log({self.loss_total: loss_total, **metric_batch_step})
        wandb.log({self.loss_nat: loss_nat, **metric_batch_step})
        wandb.log({self.loss_adv: loss_adv, **metric_batch_step})
        wandb.log({self.correct_rate_nat_batch: correct_rate_nat_batch, **metric_batch_step})
        wandb.log({self.correct_rate_adv_batch: correct_rate_adv_batch, **metric_batch_step})

    def record_train_epoch_info(self, idx_epoch, correct_rate_nat_epoch=None, correct_rate_adv_epoch=None):
        metric_epoch_step = {self.epoch_step: idx_epoch}
        wandb.log({self.correct_rate_nat_epoch: correct_rate_nat_epoch, **metric_epoch_step})
        wandb.log({self.correct_rate_adv_epoch: correct_rate_adv_epoch, **metric_epoch_step})

    def record_eval_info(self, idx_epoch, acc, rob=None):
        metric_epoch_step = {self.epoch_step: idx_epoch}
        wandb.log({self.test_acc: acc, **metric_epoch_step})
        wandb.log({self.test_rob: rob, **metric_epoch_step})


def set_wandb_env(is_online: bool = True):
    # https://docs.wandb.ai/guides/track/advanced/environment-variables

    # ignore files when upload, no spaces after the comma separator. no effect in syncing offline run
    os.environ['WANDB_IGNORE_GLOBS'] = '*.pth,*.npy'

    # https://github.com/wandb/wandb/issues/4872
    os.environ['WANDB_DISABLE_SERVICE'] = 'true'

    # https://github.com/wandb/wandb/issues/4872#issuecomment-1459094979
    os.environ['WANDB_CONSOLE'] = 'off'

    if is_online:
        os.environ['WANDB_MODE'] = 'online'
    else:
        # can be later synced with the `wandb sync` command.
        os.environ['WANDB_MODE'] = 'offline'


def disable_debug_internal_log():
    # https://community.wandb.ai/t/the-debug-internal-log-file-is-too-large-500mb/3589/1
    # https://github.com/wandb/wandb/issues/4223

    if not dir_wandb_to_sync.exists():
        dir_wandb_to_sync.mkdir(parents=True)

    if dir_wandb_to_sync.joinpath('null').exists():
        return
    else:
        os.system(f"ln -s /dev/null {str(dir_wandb_to_sync)}/null")


def define_wandb_train_metrics():
    batch_step = 'batch_step'
    epoch_step = 'epoch_step'
    wandb.define_metric(batch_step)
    wandb.define_metric(epoch_step)

    # === metrics for batch ===
    lr = 'train/lr'
    loss_total = 'train/loss_total'
    loss_nat = 'train/loss_nat'
    loss_adv = 'train/loss_adv'
    epoch = 'train/epoch'  # for align batch
    correct_rate_nat_batch = 'train/correct_rate_nat_batch'
    correct_rate_adv_batch = 'train/correct_rate_adv_batch'
    wandb.define_metric(lr, step_metric=batch_step)
    wandb.define_metric(loss_total, step_metric=batch_step)
    wandb.define_metric(loss_nat, step_metric=batch_step)
    wandb.define_metric(loss_adv, step_metric=batch_step)
    wandb.define_metric(epoch, step_metric=batch_step)
    wandb.define_metric(correct_rate_nat_batch, step_metric=batch_step)
    wandb.define_metric(correct_rate_adv_batch, step_metric=batch_step)

    # return

    # === metrics for epoch ===
    correct_rate_nat_epoch = 'train/correct_rate_nat_epoch'
    correct_rate_adv_epoch = 'train/correct_rate_adv_epoch'
    wandb.define_metric(correct_rate_nat_epoch, step_metric=epoch_step)
    wandb.define_metric(correct_rate_adv_epoch, step_metric=epoch_step)

    return batch_step, epoch_step, epoch, lr, loss_total, loss_nat, loss_adv, \
           correct_rate_nat_batch, correct_rate_adv_batch, correct_rate_nat_epoch, correct_rate_adv_epoch


def define_wandb_test_metrics():
    epoch_step = 'epoch_step'
    test_acc = 'test/acc'
    test_rob = 'test/rob'

    wandb.define_metric(epoch_step)
    wandb.define_metric(test_acc, step_metric=epoch_step)
    wandb.define_metric(test_rob, step_metric=epoch_step)

    return epoch_step, test_acc, test_rob


def define_wandb_epoch_metrics():
    epoch_step = 'epoch_step'
    test_acc = 'test/acc'
    test_rob = 'test/rob'
    correct_rate_nat_epoch = 'train/correct_rate_nat_epoch'
    correct_rate_adv_epoch = 'train/correct_rate_adv_epoch'

    wandb.define_metric(epoch_step)
    wandb.define_metric(test_acc, step_metric=epoch_step)
    wandb.define_metric(test_rob, step_metric=epoch_step)
    wandb.define_metric(correct_rate_nat_epoch, step_metric=epoch_step)
    wandb.define_metric(correct_rate_adv_epoch, step_metric=epoch_step)

    return epoch_step, test_acc, test_rob, correct_rate_nat_epoch, correct_rate_adv_epoch


def define_wandb_batch_metrics():
    batch_step = 'batch_step'
    train_lr = 'train/lr'
    loss_total = 'train/loss_total'
    loss_nat = 'train/loss_nat'
    loss_adv = 'train/loss_adv'
    epoch = 'train/epoch'
    correct_rate_nat_batch = 'train/correct_rate_nat_batch'
    correct_rate_adv_batch = 'train/correct_rate_adv_batch'

    wandb.define_metric(batch_step)
    wandb.define_metric(train_lr, step_metric=batch_step)
    wandb.define_metric(loss_total, step_metric=batch_step)
    wandb.define_metric(loss_nat, step_metric=batch_step)
    wandb.define_metric(loss_adv, step_metric=batch_step)
    wandb.define_metric(epoch, step_metric=batch_step)
    wandb.define_metric(correct_rate_nat_batch, step_metric=batch_step)
    wandb.define_metric(correct_rate_adv_batch, step_metric=batch_step)

    return batch_step, epoch, train_lr, loss_total, loss_nat, loss_adv, correct_rate_nat_batch, correct_rate_adv_batch


def make_wandb_config(my_config: Config):
    wandb_config = asdict(my_config)

    return wandb_config


def from_wandb_config(cfg: dict) -> Config:
    """convert wandb config to dataclass config"""
    config_new = Config(**cfg)
    hints = get_type_hints(Config)
    for name_var, type_var in hints.items():

        if is_dataclass(type_var):
            if cfg[name_var] is not None:
                config_new.__dict__[name_var] = type_var(**cfg[name_var])

    return config_new


def get_wandb_dir_stem() -> str:
    return Path(wandb.run.dir).parent.stem


def save_model_to_wandb_dir(model: torch.nn.Module, idx_epoch: int):
    run_dir_name = get_wandb_dir_stem()
    weights_path = dir_wandb_saved_local.joinpath(run_dir_name, 'weight')

    weights_path.mkdir(exist_ok=True, parents=True)
    torch.save(model.state_dict(), weights_path.joinpath(f'epoch_{idx_epoch}.pth'))
    print(f'Save epoch-{idx_epoch} checkpoints to {weights_path}')


def save_array_to_wandb_dir(array: np.ndarray, name: str):
    run_dir_name = get_wandb_dir_stem()
    array_path = dir_wandb_saved_local.joinpath(run_dir_name, 'array')
    array_path.mkdir(exist_ok=True, parents=True)
    np.save(array_path.joinpath(name).__str__(), array)
    print(f'Save array {name} to {array_path}')


def get_run_obj(run_instance):
    """
    :param run_instance: string, <entity>/<project>/<run_id>
    :return: run object
    """
    # https://docs.wandb.ai/ref/python/public-api/api
    api = wandb.Api()
    run = api.run(run_instance)

    return run


def export_wandb_run_data(run_instance, keys):
    """
    :param run_instance: string, <entity>/<project>/<run_id>
    :param is_from_cloud: bool
    :return:
    """
    run = get_run_obj(run_instance)

    if run.state == "finished":
        # https://github.com/wandb/wandb/blob/latest/wandb/apis/public.py#L1968
        return run.history(keys=keys, pandas=False)
    else:
        print('Run is not finished!')


def export_wandb_run_config(run_instance):
    run = get_run_obj(run_instance)

    if run.state == "finished":
        return run.config
    else:
        print('Run is not finished!')


def export_files_name(run_instance):
    run = get_run_obj(run_instance)

    if run.state == "finished":
        return [i for i in run.files()]
        # download
        # for file in run.files():
        #     file.download()
    else:
        print('Run is not finished!')


def update_wandb_exist_run_config(run_path: str, key, value):
    """
    run_path: like geyao/Salient Feature Influence Test/2m4mcw7b
    """
    api = wandb.Api()
    run = api.run(run_path)
    run.config[key] = value
    run.update()


def update_wandb_group_name(name_project: str, name_old_group: str, name_new_group: str):
    api = wandb.Api()
    runs_in_project = api.runs(name_project)
    for run in runs_in_project:
        # print(run.name)
        if run.group == name_old_group:
            run.group = name_new_group
        run.update()


def get_wandb_run_path(run_id: str):
    " get local dir according to wandb run id "
    run_dir_mathc_list = [str(i) for i in dir_wandb_saved_local.glob(f'*-{run_id}')]
    assert len(run_dir_mathc_list) == 1, 'Should be single match result, check it!'

    return next(dir_wandb_saved_local.glob(f'*-{run_id}'))


def get_path_for_checkpoints(run_id: str):
    run_dir = get_wandb_run_path(run_id)
    checkpoints_path_list = [i for i in run_dir.joinpath('files', 'weights').glob('*.pth')]
    checkpoints_path_list = sorted(checkpoints_path_list, key=lambda x: int(x.stem.split('_')[-1]))
    return checkpoints_path_list


def check_and_get_path_for_single_checkpoint(run_id: str, num_epoch: int):
    run_dir = get_wandb_run_path(run_id)
    checkpoint_path = run_dir.joinpath('weight', f'epoch_{num_epoch}.pth')
    if checkpoint_path.exists():
        is_checkpoint_exist = True
    else:
        is_checkpoint_exist = False
    return is_checkpoint_exist, checkpoint_path


if __name__ == '__main__':
    disable_debug_internal_log(Path('/home/gy/code_disk/NewRCAT/wandb'))
