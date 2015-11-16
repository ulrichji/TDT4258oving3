struct fasync_struct *gamepad_queue; // Queue of signal listeners

static int gamepad_fasync(int fd, struct file *filp, int mode);
static ssize_t gamepad_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);

struct file_operations gamepad_fops;
