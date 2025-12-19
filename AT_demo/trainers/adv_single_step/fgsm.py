import sys

import numpy as np
import torch
import torch.nn.functional as F
from tqdm import tqdm

from config import *
from utils.helper_wandb import WandbRecorder


class FgsmAdvTrainer:
    def __init__(self, config: Config, device: str, lr_scheduler,
                 model: torch.nn.Module, train_loader, optimizer, targets,
                 wandb_recorder: WandbRecorder = None,
                 **kwargs):

        self.model = model
        self.adv_config = config.param_atk_train
        self.optimizer = optimizer
        self.device = device
        self.total_epoch = config.total_epoch
        self.train_loader = train_loader
        self.lr_scheduler = lr_scheduler
        self.num_samples = len(targets)
        self.targets = targets

        self.wandb_recorder = wandb_recorder
        self.bs_print = 2000
        self.tqdm_bar = tqdm(total=len(self.train_loader) * self.total_epoch, file=sys.stdout, position=0, ncols=150)

    def train_epoch(self, idx_epoch):
        " idx_epoch should start from 1"
        self.model.train()
        epoch_correct_num_adv = 0

        for batch_idx, (idx, data, target) in enumerate(self.train_loader, 1):
            data, target = data.to(self.device), target.to(self.device)

            self.optimizer.zero_grad()
            lr = self.lr_scheduler.get_last_lr()[0]

            loss_adv, batch_correct_num_adv = self._train_batch(x=data, y=target)

            loss_adv.backward()
            epoch_correct_num_adv += batch_correct_num_adv

            self.optimizer.step()
            self.lr_scheduler.step()

            str_info = f'Epoch-{idx_epoch:<3} Batch-{batch_idx + 1:<3} | ' \
                       f'Loss adv-{loss_adv.item():<6.4f} | ' \
                       f'LR-{lr:.3f} '

            if batch_idx % self.bs_print == 0:
                self.tqdm_bar.write(str_info)

            self.tqdm_bar.update(1)
            self.tqdm_bar.set_description(str_info)

            if self.wandb_recorder is not None:  # record info for batch
                idx_batch_total = (idx_epoch - 1) * len(self.train_loader) + batch_idx
                self.wandb_recorder.record_train_batch_info(idx_batch_total=idx_batch_total, idx_epoch=idx_epoch,
                                                            lr=lr, loss_total=loss_adv.item(),
                                                            loss_adv=loss_adv.item(),
                                                            correct_rate_adv_batch=batch_correct_num_adv / len(data))
        if self.wandb_recorder is not None:  # record info for epoch
            self.wandb_recorder.record_train_epoch_info(idx_epoch=idx_epoch,
                                                        correct_rate_adv_epoch=epoch_correct_num_adv / self.num_samples)

        if idx_epoch >= self.total_epoch:
            self.tqdm_bar.clear()
            self.tqdm_bar.close()

    def _train_batch(self, x, y):
        self.model.eval()

        x_adv = x.clone().detach()
        x_adv.requires_grad_(True)
        loss_make_adv = F.cross_entropy(self.model(x_adv), y)
        grad = torch.autograd.grad(loss_make_adv, [x_adv])[0]
        x_adv = x_adv.detach() + self.adv_config.epsilon * torch.sign(grad.detach())
        x_adv = torch.clamp(x_adv, 0.0, 1.0)
        x_adv.requires_grad_(False)

        self.model.train()
        self.optimizer.zero_grad()

        out_adv = self.model(x_adv)
        loss_adv = F.cross_entropy(out_adv, y, reduction='mean')

        adv_correct_num = torch.sum(torch.argmax(out_adv, dim=1) == y).cpu().item()

        return loss_adv, adv_correct_num
