from pathlib import Path

from PIL import Image
from torch.utils.data import Dataset
from torchvision import datasets
from torchvision.datasets import ImageFolder
import numpy as np


class CIFAR10WithID(Dataset):
    def __init__(self, dir_dataset, train=True, transform=None):
        super(CIFAR10WithID, self).__init__()
        self.ds_train = datasets.CIFAR10(root=dir_dataset,
                                         train=train, download=True)
        self.transform = transform
        self.data = self.ds_train.data
        self.targets = self.ds_train.targets

    def __len__(self):
        return len(self.ds_train)

    def __getitem__(self, item):
        image = self.data[item]
        image = Image.fromarray(image)
        image = self.transform(image)
        label = self.targets[item]

        return item, image, label


class CIFAR100WithID(Dataset):
    def __init__(self, dir_dataset, train=True, transform=None):
        super(CIFAR100WithID, self).__init__()
        self.ds_train = datasets.CIFAR100(root=dir_dataset,
                                          train=train, download=True)
        self.transform = transform
        self.data = self.ds_train.data
        self.targets = self.ds_train.targets

    def __len__(self):
        return len(self.ds_train)

    def __getitem__(self, item):
        image = self.data[item]
        image = Image.fromarray(image)
        image = self.transform(image)
        label = self.targets[item]

        return item, image, label


class SVHNWithID(Dataset):
    def __init__(self, dir_dataset, train=True, transform=None):
        super(SVHNWithID, self).__init__()
        split = 'train' if train else 'test'
        self.ds_train = datasets.SVHN(root=dir_dataset.joinpath('SVHN'),
                                      split=split, download=True)
        self.transform = transform
        self.data = self.ds_train.data
        self.targets = self.ds_train.labels

    def __len__(self):
        return len(self.ds_train)

    def __getitem__(self, item):
        image, label = self.data[item], int(self.targets[item])
        image = Image.fromarray(np.transpose(image, (1, 2, 0)))
        image = self.transform(image)

        return item, image, label


class GTSRBWithID(Dataset):
    def __init__(self, dir_dataset, train=True, transform=None):
        super(GTSRBWithID, self).__init__()
        split = 'train' if train else 'test'
        self.ds = datasets.GTSRB(root=dir_dataset,
                                 split=split, download=True)
        self._samples = self.ds._samples
        self.transform = transform

    def __len__(self):
        return len(self.ds)

    def get_targets(self):
        targets = []
        for i in range(self.__len__()):
            targets.append(self._samples[i][1])
        return targets

    def __getitem__(self, item):
        path, target = self._samples[item]
        sample = Image.open(path).convert("RGB")

        if self.transform is not None:
            sample = self.transform(sample)

        return item, sample, target


class STL10WithID(Dataset):
    def __init__(self, dir_dataset, train=True, transform=None):
        super(STL10WithID, self).__init__()
        split = 'train' if train else 'test'
        self.ds = datasets.STL10(root=dir_dataset,
                                 split=split, download=True)
        self.data = self.ds.data
        self.targets = self.ds.labels
        self.transform = transform

    def __len__(self):
        return len(self.ds)

    def __getitem__(self, item):
        img, target = self.data[item], int(self.targets[item])
        img = Image.fromarray(np.transpose(img, (1, 2, 0)))
        if self.transform is not None:
            img = self.transform(img)

        return item, img, target


# https://github.com/Harry24k/catastrophic-overfitting/blob/22e7bbfdfc45427834779d3dcc2e96b2da10ef12/defenses/loaders/datasets.py#L285

def pil_loader(path: str) -> Image.Image:
    # open path as file to avoid ResourceWarning (https://github.com/python-pillow/Pillow/issues/835)
    with open(path, 'rb') as f:
        img = Image.open(f)
        return img.convert('RGB')


# TODO: specify the return type
def accimage_loader(path: str):
    import accimage
    try:
        return accimage.Image(path)
    except IOError:
        # Potentially a decoding problem, fall back to PIL.Image
        return pil_loader(path)


def default_loader(path: str):
    from torchvision import get_image_backend
    if get_image_backend() == 'accimage':
        return accimage_loader(path)
    else:
        return pil_loader(path)


class TinyImageNet(Dataset):
    def __init__(self, root, train=False, transform=None, **kwargs):
        if train:
            self.data = ImageFolder(root.__str__() + '/tiny-imagenet-200/train',
                                    transform=transform)
        else:
            self.data = ImageFolder(root.__str__() + '/tiny-imagenet-200/val_fixed',
                                    transform=transform)
        self.imgs_info = self.data.imgs
        self.transform = transform

    def __len__(self):
        return len(self.imgs_info)

    def __getitem__(self, item):
        path, label = self.imgs_info[item]
        image = default_loader(path)
        if self.transform is not None:
            image = self.transform(image)

        return image, label


class TinyImageNetWithID(Dataset):
    def __init__(self, root: Path, train=False, transform=None):
        if train:
            self.data = ImageFolder(root.__str__() + '/tiny-imagenet-200/train',
                                    transform=transform)
        else:
            self.data = ImageFolder(root.__str__() + '/tiny-imagenet-200/val_fixed',
                                    transform=transform)
        self.imgs_info = self.data.imgs
        self.transform = transform

    def __len__(self):
        return len(self.imgs_info)

    def __getitem__(self, item):
        path, label = self.imgs_info[item]
        image = default_loader(path)
        if self.transform is not None:
            image = self.transform(image)

        return item, image, label
