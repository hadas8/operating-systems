#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/errno.h>

MODULE_LICENSE("GPL");

#include "message_slot.h"

//data structure of linked list to hold message slots
typedef struct list_node {
	unsigned int channel_id;
	char message[BUF_LEN];
	ssize_t message_len;
	struct list_node *next;
} list_node_t;


//a list of list node pointers, that demonstrates the open device files according to minor number
static list_node_t *minor_list[256];

//================== HELPER FUNCTIONS ===========================
//allocate memory and create new list node with given channel ID
list_node_t* allocate_list_node(unsigned int channel_id) {
    list_node_t *node = kmalloc(sizeof(list_node_t), GFP_KERNEL);
    if (!node) {
        return NULL;
    }
    node->channel_id = channel_id;
    node->message_len = 0;
    node->next = NULL;

    return node;
}

//free memory data structure
void free_minor_list(void) {
    int i;
    list_node_t *prev, *node;

    for(i = 0; i < 256; ++i) {
        if(minor_list[i] != NULL) {
            prev = minor_list[i];
            node = prev->next;
            kfree(prev);
            while(node != NULL) {
                prev = node;
                node = node->next;
                kfree(prev);
            }
        }
    }
}




//================== DEVICE FUNCTIONS ===========================
static int device_open(struct inode *inode, struct file *file) {
    list_node_t *head;
    unsigned int minor = iminor(inode);

    if(minor_list[minor] == NULL) {
        head = allocate_list_node(0);
        if(head == NULL) {
            return -ENOMEM;
        }
        minor_list[minor] = head;
    }

	file->private_data = minor_list[minor];

	return SUCCESS;
}


//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file) {
    return SUCCESS;
}

//---------------------------------------------------------------
static ssize_t device_read( struct file* file,
                            char __user* buffer,
                            size_t       length,
                            loff_t*      offset ) {
    ssize_t ret_val;
    list_node_t *node = file->private_data;

    if(node == NULL || node->channel_id == 0) {
        return -EINVAL;
    }

    if(node->message_len == 0) {
        return -EWOULDBLOCK;
    }

    if(length < node->message_len) {
        return -ENOSPC;
    }

    ret_val = copy_to_user(buffer, node->message, node->message_len);
    if (ret_val != 0) {
        return -EFAULT;
    }

    return node->message_len;
}

//---------------------------------------------------------------
static ssize_t device_write( struct file*       file,
                             const char __user* buffer,
                             size_t             length,
                             loff_t*            offset) {
    ssize_t ret_val;
    list_node_t *node = file->private_data;

    if(node == NULL || node->channel_id == 0) {
        return -EINVAL;
    }

    if(length == 0 || length > BUF_LEN) {
        return -EMSGSIZE;
    }

    ret_val = copy_from_user(node->message, buffer, length);
    if(ret_val != 0) {
        return -EFAULT;
    }
    node->message_len = length;

    return length;
}

//----------------------------------------------------------------
static long device_ioctl(struct file* file,
						unsigned int ioctl_command_id,
                        unsigned long ioctl_param) {
    list_node_t *node = file->private_data;

    //error cases for ioctl
	if(ioctl_command_id != MSG_SLOT_CHANNEL || !ioctl_param) {
		return -EINVAL;
	}

    if(node->channel_id == ioctl_param) {
        file->private_data = node;
        return SUCCESS;
    } else {
        if(node->channel_id == 0) {
            node->channel_id = ioctl_param;
            file->private_data = node;
            return SUCCESS;
        }
    }

	while(node->next) {
	    if(node->next->channel_id == ioctl_param) {
	        file->private_data = node->next;
	        return SUCCESS;
	    } else {
	        node = node->next;
	    }
	}

	node->next = allocate_list_node(ioctl_param);
	if(node->next == NULL) {
	    return -ENOMEM;
	}
	file->private_data = node->next;

	return SUCCESS;
}


//==================== DEVICE SETUP =============================
struct file_operations Fops =
{
	.owner = THIS_MODULE,
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.unlocked_ioctl = device_ioctl,
	.release = device_release,
};

//---------------------------------------------------------------
static int __init simple_init(void) {
	int rc = -1;

	rc = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops);

	//if module initialization fails, print error
	if(rc < 0) {
		printk(KERN_ERR "%s registration failed for  %d\n",
				DEVICE_FILE_NAME, MAJOR_NUM);
		return rc;
	}

	return 0;
}

//---------------------------------------------------------------
static void __exit simple_cleanup(void)
{
  // Unregister the device
  free_minor_list();
  unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================
