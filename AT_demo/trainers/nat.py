import sys

import torch
import torch.nn.functional as F
import wandb
from tqdm import tqdm

from config import *
from utils.helper_wandb import WandbRecorder


class NatTrainer:
    def __init__(self, config: Config, device: str, lr_scheduler,
                 model: torch.nn.Module, train_loader, optimizer,
                 targets, wandb_recorder: WandbRecorder = None, **kwargs):

        self.model = model
        self.optimizer = optimizer
        self.device = device
        self.total_epoch = config.total_epoch
        self.train_loader = train_loader
        self.lr_scheduler = lr_scheduler
        self.num_samples = len(targets)

        self.wandb_recorder = wandb_recorder

        self.bs_print = 200
        self.tqdm_bar = tqdm(total=len(self.train_loader) * self.total_epoch, file=sys.stdout, position=0, ncols=120)

    def train_epoch(self, idx_epoch):
        " idx_epoch should start from 1"
        self.model.train()
        epoch_correct_num = 0
        for batch_idx, (idx, data, target) in enumerate(self.train_loader, 1):
            data, target = data.to(self.device), target.to(self.device)

            self.optimizer.zero_grad()
            lr = self.lr_scheduler.get_last_lr()[0]

            # calculate robust loss
            loss, batch_correct_num = self._train_batch(idx=idx, x_natural=data, y=target)
            loss.backward()

            epoch_correct_num += batch_correct_num

            self.optimizer.step()
            self.lr_scheduler.step()

            if batch_idx % self.bs_print == 0:
                self.tqdm_bar.write(f'[{idx_epoch:<2}, {batch_idx + 1:<5}] '
                                    f'loss: {loss:<6.4f} '
                                    f'lr: {lr:.4f} ')

            self.tqdm_bar.update(1)
            self.tqdm_bar.set_description(f'epoch-{idx_epoch:<3} '
                                          f'batch-{batch_idx + 1:<3} '
                                          f'loss-{loss:<.2f} '
                                          f'lr-{lr:.3f} ')

            if self.wandb_recorder is not None:  # record info for batch
                idx_batch_total = (idx_epoch - 1) * len(self.train_loader) + batch_idx
                self.wandb_recorder.record_train_batch_info(idx_batch_total=idx_batch_total, idx_epoch=idx_epoch,
                                                            lr=lr, loss_total=loss.item(),
                                                            loss_nat=loss.item(),
                                                            correct_rate_nat_batch=batch_correct_num / len(data))
        if self.wandb_recorder is not None:  # record info for epoch
            self.wandb_recorder.record_train_epoch_info(idx_epoch=idx_epoch,
                                                        correct_rate_nat_epoch=epoch_correct_num / self.num_samples)

        if idx_epoch >= self.total_epoch:
            self.tqdm_bar.clear()
            self.tqdm_bar.close()

    def _train_batch(self, idx, x_natural, y):
        # define CE-loss
        self.model.train()
        # zero gradient
        self.optimizer.zero_grad()
        output = self.model(x_natural)
        loss_mean = F.cross_entropy(output, y)

        correct_num = torch.sum(torch.argmax(output, dim=1) == y).cpu().item()

        return loss_mean, correct_num
