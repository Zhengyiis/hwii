import sys

import torch
import torch.nn.functional as F
from tqdm import tqdm

from config import *
from utils.helper_wandb import WandbRecorder


class TradesAdvTrainer:
    def __init__(self, config: Config, device: str, lr_scheduler,
                 model: torch.nn.Module, train_loader, optimizer, targets, beta=2.5,
                 wandb_recorder: WandbRecorder = None, **kwargs):

        self.model = model
        self.adv_config = config.param_atk_train
        self.optimizer = optimizer
        self.device = device
        self.total_epoch = config.total_epoch
        self.train_loader = train_loader
        self.lr_scheduler = lr_scheduler
        self.num_samples = len(targets)
        self.beta = beta
        self.targets = targets

        self.wandb_recorder = wandb_recorder
        self.bs_print = 2000
        self.tqdm_bar = tqdm(total=len(self.train_loader) * self.total_epoch, file=sys.stdout, position=0, ncols=150)

    def train_epoch(self, idx_epoch):
        " idx_epoch should start from 1"
        self.model.train()
        epoch_correct_num_nat = 0
        epoch_correct_num_adv = 0

        for batch_idx, (idx, data, target) in enumerate(self.train_loader, 1):
            data, target = data.to(self.device), target.to(self.device)

            self.optimizer.zero_grad()
            lr = self.lr_scheduler.get_last_lr()[0]

            # calculate robust loss
            loss_total, loss_nat, loss_adv, batch_correct_num_nat, batch_correct_num_adv \
                = self._train_batch(x=data, y=target)

            loss_total.backward()
            epoch_correct_num_nat += batch_correct_num_nat
            epoch_correct_num_adv += batch_correct_num_adv

            self.optimizer.step()
            self.lr_scheduler.step()

            str_info = f'Epoch-{idx_epoch:<3} Batch-{batch_idx + 1:<3} | ' \
                       f'Loss: total-{loss_total:<6.4f} nat-{loss_nat:<6.4f} adv-{loss_adv:<6.4f} | ' \
                       f'LR-{lr:.3f} '

            if batch_idx % self.bs_print == 0:
                self.tqdm_bar.write(str_info)

            self.tqdm_bar.update(1)
            self.tqdm_bar.set_description(str_info)

            if self.wandb_recorder is not None:  # record info for batch
                idx_batch_total = (idx_epoch - 1) * len(self.train_loader) + batch_idx
                self.wandb_recorder.record_train_batch_info(idx_batch_total=idx_batch_total, idx_epoch=idx_epoch,
                                                            lr=lr, loss_total=loss_total.item(),
                                                            loss_nat=loss_nat.item(), loss_adv=loss_adv.item(),
                                                            correct_rate_nat_batch=batch_correct_num_nat / len(data),
                                                            correct_rate_adv_batch=batch_correct_num_adv / len(data))

        if idx_epoch >= self.total_epoch:
            self.tqdm_bar.clear()
            self.tqdm_bar.close()

        if self.wandb_recorder is not None:  # record info for epoch
            self.wandb_recorder.record_train_epoch_info(idx_epoch=idx_epoch,
                                                        correct_rate_nat_epoch=epoch_correct_num_nat / self.num_samples,
                                                        correct_rate_adv_epoch=epoch_correct_num_adv / self.num_samples, )

    def _train_batch(self, x, y):
        # reference: https://github.com/yaodongyu/TRADES/blob/master/trades.py
        self.model.eval()

        x_adv = x.detach() + 0.001 * torch.randn(x.shape).cuda().detach()

        with torch.no_grad():
            p_natural = F.softmax(self.model(x), dim=1)

        for _ in range(self.adv_config.perturb_steps):
            x_adv.requires_grad_(True)
            with torch.enable_grad():
                loss_make_adv = F.kl_div(F.log_softmax(self.model(x_adv), dim=1),
                                         p_natural, reduction='sum')
            grad = torch.autograd.grad(loss_make_adv, [x_adv])[0]
            x_adv = x_adv.detach() + self.adv_config.step_size * torch.sign(grad.detach())
            x_adv = torch.min(torch.max(x_adv, x - self.adv_config.epsilon),
                              x + self.adv_config.epsilon)
            x_adv = torch.clamp(x_adv, 0.0, 1.0)

        self.model.train()

        x_adv.requires_grad_(False)
        self.optimizer.zero_grad()

        out_nat, out_adv = self.model(x), self.model(x_adv)
        loss_nat = F.cross_entropy(out_nat, y, reduction='mean')
        loss_adv = (1 / len(x)) * F.kl_div(F.log_softmax(out_adv, dim=1), F.softmax(out_nat, dim=1),
                                           reduction='sum')

        loss = loss_nat + self.beta * loss_adv

        nat_correct_num = torch.sum(torch.argmax(out_nat, dim=1) == y).cpu().item()
        adv_correct_num = torch.sum(torch.argmax(out_adv, dim=1) == y).cpu().item()

        return loss, loss_nat.detach().cpu(), loss_adv.detach().cpu(), nat_correct_num, adv_correct_num
