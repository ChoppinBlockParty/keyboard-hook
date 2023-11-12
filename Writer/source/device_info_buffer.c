#include "device_info_buffer.h"

#include "output_keyboard.h"

#define KEYBOARD_HOOK_WRITER_DEVICE_INFO_BUFFER_DEVICE_NAME "keyboard_hook_writer_device_info_buffer"

struct device_info_buffer* device_info_buffer;

struct device_info_buffer* get_device_info_buffer(void) {
    return device_info_buffer;
}

int open_device_info_buffer(struct inode* inode, struct file* filp) {
    unsigned int mj = imajor(inode);
    unsigned int mn = iminor(inode);

    if (mutex_lock_killable(&device_info_buffer->mutex)) {
        return -EINTR;
    }

    if (mj != device_info_buffer->major || mn != device_info_buffer->minor) {
        printk(KERN_WARNING "[target] "
                            "No device found with minor=%d and major=%d\n",
               mj,
               mn);
        return -ENODEV; /* No such device */
    }

    if (inode->i_cdev != &device_info_buffer->cdev) {
        printk(KERN_WARNING "[target] open: internal error\n");
        return -ENODEV; /* No such device */
    }

    return 0;
}

ssize_t write_to_device_info_buffer(struct file* filp, const char __user* buf, size_t count, loff_t* f_pos) {
    ssize_t retval = 0;

    if (device_info_buffer->bufferPosition >= device_info_buffer->buffer_size) {
        /* Writing beyond the end of the buffer is not allowed. */
        retval = -EINVAL;
        goto out;
    }

    if ((device_info_buffer->bufferPosition + count) >= device_info_buffer->buffer_size) {
        count = device_info_buffer->buffer_size - device_info_buffer->bufferPosition;
    }

    if (copy_from_user(&(device_info_buffer->data[device_info_buffer->bufferPosition]), buf, count) != 0) {
        retval = -EFAULT;
        goto out;
    }

    device_info_buffer->bufferPosition += count;
    *f_pos = device_info_buffer->bufferPosition;
    retval = count;

out:
    return retval;
}

int release_device_info_buffer(struct inode* inode, struct file* filp) {
    int err = create_output_keyboard(device_info_buffer->major,
                                     device_info_buffer->minor + 1,
                                     device_info_buffer->class);
    device_info_buffer->bufferPosition = 0;

    mutex_unlock(&device_info_buffer->mutex);

    if (err) {
        printk(KERN_WARNING "Failed to create_output_keyboard");
        return err;
    }

    return 0;
}

struct file_operations device_info_buffer_fops = {
    .owner = THIS_MODULE,
    .open = open_device_info_buffer,
    .write = write_to_device_info_buffer,
    .release = release_device_info_buffer,
};

int create_device_info_buffer(unsigned int major, unsigned int minor, struct class* class) {
    int errror = 0;
    dev_t devno;
    struct device* device = NULL;

    /* Allocate the array of devices */
    device_info_buffer = (struct device_info_buffer*)kzalloc(sizeof(struct device_info_buffer), GFP_KERNEL);

    if (device_info_buffer == NULL) {
        errror = -ENOMEM;
        return errror;
    }

    devno = MKDEV(major, minor);

    BUG_ON(device_info_buffer == NULL || class == NULL);

    /* Memory is to be allocated when the device is opened the first time */
    device_info_buffer->data = NULL;
    device_info_buffer->buffer_size = 40000;
    device_info_buffer->major = major;
    device_info_buffer->class = class;
    device_info_buffer->minor = minor;
    mutex_init(&device_info_buffer->mutex);

    cdev_init(&device_info_buffer->cdev, &device_info_buffer_fops);
    device_info_buffer->cdev.owner = THIS_MODULE;

    errror = cdev_add(&device_info_buffer->cdev, devno, 1);

    if (errror) {
        printk(KERN_WARNING "[target] Error %d while trying to add %s (minor %d)",
               errror,
               KEYBOARD_HOOK_WRITER_DEVICE_INFO_BUFFER_DEVICE_NAME,
               minor);
        return errror;
    }

    device = device_create(class,
                           NULL,
                           /* no parent device */
                           devno,
                           NULL,
                           /* no additional data */
                           KEYBOARD_HOOK_WRITER_DEVICE_INFO_BUFFER_DEVICE_NAME);

    if (IS_ERR(device)) {
        errror = PTR_ERR(device);
        printk(KERN_WARNING "[target] Error %d while trying to create %s (minor %d)",
               errror,
               KEYBOARD_HOOK_WRITER_DEVICE_INFO_BUFFER_DEVICE_NAME,
               minor);
        cdev_del(&device_info_buffer->cdev);
        return errror;
    }

    /* allocate the buffer */
    if (device_info_buffer->data == NULL) {
        device_info_buffer->data = (unsigned char*)kzalloc(device_info_buffer->buffer_size, GFP_KERNEL);

        if (device_info_buffer->data == NULL) {
            printk(KERN_WARNING "[target] open(): out of memory\n");
            return -ENOMEM;
        }
    }

    device_info_buffer->bufferPosition = 0;

    return 0;
}

void destroy_device_info_buffer(void) {
    BUG_ON(device_info_buffer == NULL || device_info_buffer->class == NULL);
    release_all_output_keyboards();
    device_destroy(device_info_buffer->class, MKDEV(device_info_buffer->major, device_info_buffer->minor));
    cdev_del(&device_info_buffer->cdev);
    kfree(device_info_buffer->data);
    return;
}
