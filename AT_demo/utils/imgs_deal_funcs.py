import matplotlib.pyplot as plt
import numpy as np
import torch
from torchvision import datasets, transforms
from config import ConfigLinfAttack
from torch.autograd import Variable
import torch.optim as optim
import torch.nn.functional as F
from PIL import Image
from utils.datasets import TinyImageNet


def imgs_convert_ndarray_to_tensor(images: np.ndarray):
    # the value should be in the range of [0, 255]

    assert isinstance(images, np.ndarray)
    assert images.ndim == 4 and images.shape[-1] in [1, 3], 'should be NHWC, channel number should be 3 or 1'

    images = images / 255.0
    images = torch.from_numpy(np.transpose(images, (0, 3, 1, 2))).to(torch.float)

    return images


def img_convert_tensor_to_ndarray(image: torch.Tensor):
    assert isinstance(image, torch.Tensor)
    assert torch.amax(image) <= 1.0
    assert image.ndim == 3 and image.size(0) in [1, 3], 'should be CHW, channel number should be 3 or 1'

    # CHW -> HWC
    image = image.cpu().permute(1, 2, 0) * 255.
    return image.numpy().astype(np.uint8)


def save_single_numpy_image(image: np.ndarray, path_save):
    # img = trainset.data[40]
    image = Image.fromarray(image)
    image.save(path_save)


def make_adv(model: torch.nn.Module, images: torch.Tensor, labels, adv_config: ConfigLinfAttack):
    assert isinstance(images, torch.Tensor)

    model = model.to('cuda')
    model.eval()
    random_start = True
    images = images.to('cuda')
    labels = labels.to('cuda')

    x_pgd = Variable(images.data, requires_grad=True)
    if random_start:
        random_noise = torch.FloatTensor(*x_pgd.shape).uniform_(adv_config.epsilon,
                                                                adv_config.epsilon).cuda()
        x_pgd = Variable(x_pgd.data + random_noise, requires_grad=True)

    for _ in range(adv_config.perturb_steps):
        opt = optim.SGD([x_pgd], lr=1e-3)
        opt.zero_grad()

        with torch.enable_grad():
            loss = F.cross_entropy(model(x_pgd), labels)
        loss.backward()
        eta = adv_config.step_size * x_pgd.grad.data.sign()
        x_pgd = Variable(x_pgd.data + eta, requires_grad=True)
        eta = torch.clamp(x_pgd.data - images.data, -adv_config.epsilon, adv_config.epsilon)
        x_pgd = Variable(images.data + eta, requires_grad=True)
        x_pgd = Variable(torch.clamp(x_pgd, 0, 1.0), requires_grad=True)

    return x_pgd.detach()


def show_images(images, num_per_col: int = 1, titles: list = None, first_line_title: bool = False):
    """Display a list of images(0~1) in a single figure with matplotlib.

    :param first_line_title:
    :param images: List of np.arrays compatible with plt.imshow.
    :param num_per_col: Number of columns in figure (number of rows is
                        set to np.ceil(n_images/float(cols))).
    :param titles: List of titles corresponding to each image.
    :return: None
    """

    n_images = len(images)

    if titles is None:
        # titles = [str(i) for i in range(1, n_images + 1)]
        titles = ['' for _ in range(n_images)]
    elif len(titles) < n_images:
        if first_line_title:
            assert len(titles) == n_images // num_per_col

        titles += ['' for _ in range(n_images - len(titles))]

    fig = plt.figure()
    fig.set_figwidth(10)
    fig.set_figheight(4)

    size_title = 6
    for n, (image, title) in enumerate(zip(images, titles)):
        if isinstance(image, torch.Tensor):
            image = img_convert_tensor_to_ndarray(image)

        line_num = int(np.ceil(n_images / float(num_per_col)))
        a = fig.add_subplot(num_per_col, line_num, n + 1)
        if image.ndim == 2 or image.shape[2] == 1:
            plt.gray()
        plt.axis('off')
        plt.imshow(image, vmin=0, vmax=1)

        if first_line_title:
            if n < line_num:
                a.set_title(title, size=size_title)
        else:
            a.set_title(title, size=size_title)

    plt.tight_layout()
    plt.show()


def get_cifar10_imgs(ids_selected, is_train, want_type='tensor'):
    assert want_type in ['tensor', 'ndarray']

    ds = datasets.CIFAR10(root='/home/gy/torchvision_dataset',
                          train=is_train, download=False)
    list_classes_str = ds.classes

    images = ds.data[ids_selected]
    labels = np.array(ds.targets)[ids_selected]

    if want_type == 'tensor':
        images = imgs_convert_ndarray_to_tensor(images)
        labels = torch.from_numpy(labels)

    class_list = [list_classes_str[i] for i in labels]

    return images, labels, class_list


def get_tinyimagenet_imgs(ids_selected, is_train, want_type='tensor'):
    def pil_loader(path: str) -> Image.Image:
        # open path as file to avoid ResourceWarning (https://github.com/python-pillow/Pillow/issues/835)
        with open(path, 'rb') as f:
            img = Image.open(f)
            return img.convert('RGB')

    assert want_type in ['tensor', 'ndarray']

    ds = TinyImageNet(root='/home/gy/torchvision_dataset', train=is_train)
    images = []
    labels = []
    class_list = []

    for i in ids_selected:
        path, label = ds.imgs_info[i]
        images.append(np.asarray(pil_loader(path)))
        labels.append(label)
        class_list.append(path.split('/')[-1].split('.')[0])

    images = np.asarray(images)
    labels = np.asarray(labels)

    if want_type == 'tensor':
        images = np.asarray(images)
        images = imgs_convert_ndarray_to_tensor(images)
        labels = torch.from_numpy(labels)

    return images, labels, class_list


if __name__ == '__main__':
    images, labels, class_list = get_tinyimagenet_imgs(ids_selected=[1, 2, 3, 4, 5], is_train=True, want_type='tensor')
    print(labels, class_list)
