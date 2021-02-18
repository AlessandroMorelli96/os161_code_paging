# os161_PAGING
OS161 project for Programmazione di sistema POLITO

You need to clone this repository in this path:

```
cd /home/pds/os161/
rm -rf os161-base-2.0.2/
git clone https://github.com/AlessandroMorelli96/os161_PAGING.git os161-base-2.0.2/
```

This will sobstitute the default folder with this one.

At the start of every new exercise remember to do:

```
git pull
```

This will update your local folder.

At the end of every new features to update the remote version you need to:

```
git add <file1> <folder1> ...
git commit -m "Message"
git push
```

# STUTTURE IMPORTANTI

## VNODE
``` c
/*
 * A struct vnode is an abstract representation of a file.
 *
 * It is an interface in the Java sense that allows the kernel's
 * filesystem-independent code to interact usefully with multiple sets
 * of filesystem code.
 */

/*
 * Abstract low-level file.
 *
 * Note: vn_fs may be null if the vnode refers to a device.
 */
struct vnode {
    int vn_refcount;                /* Reference count */
    struct spinlock vn_countlock;   /* Lock for vn_refcount */

    struct fs *vn_fs;               /* Filesystem vnode belongs to */

    void *vn_data;                  /* Filesystem-specific data */

    const struct vnode_ops *vn_ops; /* Functions on this vnode */
};
```
