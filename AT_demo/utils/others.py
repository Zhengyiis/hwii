import numpy as np
import torch
from functools import reduce


def slice_sorted_array_and_get_bool_idx(array: np.ndarray, interval_num: int):
    # array: 1-D
    # sort array, split array, make bool idx (len(array)) for each interval
    assert isinstance(interval_num, int)

    array_sorted = np.sort(array)
    idx_threshold = [int(i * len(array) / interval_num) for i in range(1, interval_num)]
    thresholds = [array_sorted[i - 1] for i in idx_threshold]

    thresholds_pair = []
    if interval_num == 2:
        thresholds_pair.append((array_sorted[0], thresholds[0]))
        thresholds_pair.append((thresholds[0], array_sorted[-1]))
    elif interval_num > 2:
        for i, t in enumerate(thresholds):
            if i == 0:
                thresholds_pair.append((0, t))
                thresholds_pair.append((t, thresholds[i + 1]))
            elif i == interval_num - 2:
                thresholds_pair.append((t, array_sorted[-1]))
            else:
                thresholds_pair.append((t, thresholds[i + 1]))
    else:
        raise ValueError('wrong interval_num')

    select_bool_list = []
    for i, (v_min, v_max) in enumerate(thresholds_pair, 1):
        if i == len(thresholds_pair):
            select_bool = np.logical_and(v_min <= array, array <= v_max)
        else:
            select_bool = np.logical_and(v_min <= array, array < v_max)
        select_bool_list.append(select_bool)

    return select_bool_list


def array_scale_unit(array):
    # array: 1-D
    # scale to 0-1
    if isinstance(array, np.ndarray):
        array_temp = array - np.min(array)
        return array_temp / np.max(array_temp)
    elif isinstance(array, torch.Tensor):
        array_temp = array - torch.amin(array)
        return array_temp / torch.amax(array_temp)
    else:
        raise TypeError


def logic_and(*bool_arrays):
    if isinstance(bool_arrays[0], np.ndarray):
        return reduce(np.logical_and, bool_arrays)
    elif isinstance(bool_arrays[0], torch.Tensor):
        return reduce(torch.logical_and, bool_arrays)
    else:
        raise TypeError


def logic_or(*bool_arrays):
    if isinstance(bool_arrays[0], np.ndarray):
        return reduce(np.logical_or, bool_arrays)
    elif isinstance(bool_arrays[0], torch.Tensor):
        return reduce(torch.logical_or, bool_arrays)
    else:
        raise TypeError

if __name__ == '__main__':
    array = np.load('/home/gy/code_disk/NewRCAT/probs_info/CIFAR10/TRADES_lam_6/train/global_probs_epoch_50.npy')
    # slice_sorted_array_and_get_bool_idx(array, 3)
    a = array_scale_unit(torch.tensor([0.2, 0.8, 0.9]))
    print((a - 0.5) * 0.4 + 2)
