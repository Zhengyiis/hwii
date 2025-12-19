import datetime
import pprint

import torch.optim as optim

from config import *
from utils.dataloaders import *
from utils.helper_funcs import choose_model, set_seed, get_device, get_lr_scheduler, wait_gpu, prepare_data, \
    get_trainer, set_lr_param
from utils.helper_wandb import *
from utils.log import Logger
from utils.tester import AdvTester


def main(config: Config):
    now = datetime.datetime.now()
    print(now)

    set_seed(config.seed.value)
    device = get_device()

    set_wandb_env(is_online=True)  # if offline, can be later synced with the `wandb sync` command.
    param_config = make_wandb_config(config)
    disable_debug_internal_log()  # need set internal log path in wandb init
    run = wandb.init(project="AdversarialTraining", reinit=True, dir=dir_wandb_to_sync,
                     group=f'{config.method.name}-Results',
                     job_type=f'Epoch_{config.total_epoch}-{config.data.name}-{config.model.name}',
                     notes='',
                     config=param_config,
                     settings=wandb.Settings(log_internal=str(dir_wandb_to_sync / 'null'), ))

    wandb.run.name = f'{wandb.run.id}-{config.seed.name}'

    str_run_info = f'\n=============================================\n' \
                   f'{wandb.run.group=}\n' \
                   f'{wandb.run.job_type=}\n' \
                   f'{wandb.run.name=}\n' \
                   '=============================================\n'

    print(str_run_info)

    wandb.run.log_code(".")  # walks the current directory and save files that end with .py.
    config_wandb = from_wandb_config(wandb.config)  # for parameter sweep
    print(f'Param config = \n'
          f'{pprint.pformat(config_wandb, indent=4)}')

    model = choose_model(config.data, config.model)
    model = model.to(device)
    model.train()

    train_loader, test_loader, targets, num_classes = prepare_data(config.data, train_id=True, test_id=False)
    optimizer = optim.SGD(model.parameters(), lr=config.lr_init,
                          momentum=config.momentum, weight_decay=config.weight_decay)

    lr_scheduler = get_lr_scheduler(config.total_epoch, len(train_loader), config.lr_schedule_type,
                                    optimizer, config.lr_min, config.lr_max)

    wandb_recorder = WandbRecorder()
    log = Logger(is_use_wandb=True)
    log.log_to_file(str_run_info)

    trainer = get_trainer(config.method)
    trainer = trainer(config=config, device=device,
                      model=model, train_loader=train_loader, optimizer=optimizer, num_classes=num_classes,
                      targets=targets, lr_scheduler=lr_scheduler, beta=2.5,
                      wandb_recorder=wandb_recorder)

    tester = AdvTester(config.param_atk_eval, device, model, test_loader)

    acc_all = []
    rob_all = []

    for i_epoch in range(1, config.total_epoch + 1):
        log.log_to_file(f'epoch: {i_epoch}')

        trainer.train_epoch(i_epoch)

        if i_epoch % config.total_epoch == 0:
            acc, rob = tester.eval()
            log.log_to_file(f'Acc: {acc:.2%} | Rob: {rob:.2%}')

            acc_all.append(acc)
            rob_all.append(rob)
            wandb_recorder.record_eval_info(i_epoch, acc, rob)

    save_model_to_wandb_dir(model, config.total_epoch)
    save_array_to_wandb_dir(np.array(acc_all), f'acc_all')
    save_array_to_wandb_dir(np.array(rob_all), f'rob_all')

    log.close()
    run.finish()

if __name__ == '__main__':

    config = Config(
        param_atk_train=ConfigLinfAttack(
            epsilon=10 / 255,
            perturb_steps=1,
            step_size=10 / 255
        ),
        param_atk_eval=ConfigLinfAttack(
            epsilon=8 / 255,
            perturb_steps=20,
            step_size=2 / 255),
    )

    config.model = ModelName.PreActResNet_18
    config.seed = Seed.SEED_1
    config.total_epoch = 30
    set_lr_param(config, 'cyclic-wise')
    config.data = DataName.STL10
    config.method = MethodName.FGSM
    main(config)
