#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <clock.h>
#include <syscall.h>
#include <lib.h>

#if OPT_FILE
#include <copyinout.h>
#include <current.h>
#include <vnode.h>
#include <vfs.h>
#include <limits.h>
#include <uio.h>
#include <proc.h>

/* max num of system wide open files */
#define SYSTEM_OPEN_MAX (10*OPEN_MAX)

#define USE_KERNEL_BUFFER 1

/* system open file table */
struct openfile {
  struct vnode *vn;
  off_t offset;	
  unsigned int countRef;
};

struct openfile systemFileTable[SYSTEM_OPEN_MAX];
#endif

#if OPT_FILE
void openfileIncrRefCount(struct openfile *of) {
  if (of!=NULL)
    of->countRef++;
}
#endif

//#if USE_KERNEL_BUFFER

#if OPT_FILE
static int
file_read(int fd, userptr_t buf_ptr, size_t size) {
  struct iovec iov;
  int result;
  struct vnode *vn;
  struct openfile *of;
#if USE_KERNEL_BUFFER
  struct uio ku;
  int nread;
  void *kbuf;
#else
  struct uio u;
#endif
  if (fd<0||fd>OPEN_MAX) return -1;
  of = curproc->fileTable[fd];
  if (of==NULL) return -1;
  vn = of->vn;
  if (vn==NULL) return -1;

#if USE_KERNEL_BUFFER
  kbuf = kmalloc(size);
  uio_kinit(&iov, &ku, kbuf, size, of->offset, UIO_READ);
  result = VOP_READ(vn, &ku);
  if (result) {
    return result;
  }
  of->offset = ku.uio_offset;
  nread = size - ku.uio_resid;
  copyout(kbuf,buf_ptr,nread);
  kfree(kbuf);
  return (nread);
#else
  iov.iov_ubase = buf_ptr;
  iov.iov_len = size;

  u.uio_iov = &iov;
  u.uio_iovcnt = 1;
  u.uio_resid = size;          // amount to read from the file
  u.uio_offset = of->offset;
  u.uio_segflg =UIO_USERISPACE;
  u.uio_rw = UIO_READ;
  u.uio_space = curproc->p_addrspace;

  result = VOP_READ(vn, &u);
  if (result) {
    return result;
  }

  of->offset = u.uio_offset;
  return (size - u.uio_resid);
#endif
}
#endif

#if OPT_FILE
static int
file_write(int fd, userptr_t buf_ptr, size_t size) {
  struct iovec iov;
  int result, nwrite;
  struct vnode *vn;
  struct openfile *of;
#if USE_KERNEL_BUFFER
  struct uio ku;
  void *kbuf;
#else
  struct uio u;
#endif

  if (fd<0||fd>OPEN_MAX) return -1;
  of = curproc->fileTable[fd];
  if (of==NULL) return -1;
  vn = of->vn;
  if (vn==NULL) return -1;

#if USE_KERNEL_BUFFER
  kbuf = kmalloc(size);
  copyin(buf_ptr,kbuf,size);
  uio_kinit(&iov, &ku, kbuf, size, of->offset, UIO_WRITE);
  result = VOP_WRITE(vn, &ku);
  if (result) {
    return result;
  }
  kfree(kbuf);
  of->offset = ku.uio_offset;
  nwrite = size - ku.uio_resid;
  return (nwrite);
#else
  iov.iov_ubase = buf_ptr;
  iov.iov_len = size;

  u.uio_iov = &iov;
  u.uio_iovcnt = 1;
  u.uio_resid = size;          // amount to read from the file
  u.uio_offset = of->offset;
  u.uio_segflg =UIO_USERISPACE;
  u.uio_rw = UIO_WRITE;
  u.uio_space = curproc->p_addrspace;

  result = VOP_WRITE(vn, &u);
  if (result) {
    return result;
  }
  of->offset = u.uio_offset;
  nwrite = size - u.uio_resid;
  return (nwrite);
#endif
}
#endif

#if OPT_FILE
int
sys_open(userptr_t path, int openflags, mode_t mode, int *errp)
{
  int fd, i;
  struct vnode *v;
  struct openfile *of=NULL;; 	
  int result;

  result = vfs_open((char *)path, openflags, mode, &v);
  if (result) {
    *errp = ENOENT;
    return -1;
  }
  /* search system open file table */
  for (i=0; i<SYSTEM_OPEN_MAX; i++) {
    if (systemFileTable[i].vn==NULL) {
      of = &systemFileTable[i];
      of->vn = v;
      of->offset = 0; // TODO: handle offset with append
      of->countRef = 1;
      break;
    }
  }
  if (of==NULL) { 
    // no free slot in system open file table
    *errp = ENFILE;
  }
  else {
    for (fd=STDERR_FILENO+1; fd<OPEN_MAX; fd++) {
      if (curproc->fileTable[fd] == NULL) {
	curproc->fileTable[fd] = of;
	return fd;
      }
    }
    // no free slot in process open file table
    *errp = EMFILE;
  }
  
  vfs_close(v);
  return -1;
}
#endif

#if OPT_FILE
/*
 * file system calls for open/close
 */
int
sys_close(int fd)
{
  struct openfile *of=NULL; 
  struct vnode *vn;

  if (fd<0||fd>OPEN_MAX) return -1;
  of = curproc->fileTable[fd];
  if (of==NULL) return -1;
  curproc->fileTable[fd] = NULL;

  if (--of->countRef > 0) return 0; // just decrement ref cnt
  vn = of->vn;
  of->vn = NULL;
  if (vn==NULL) return -1;

  vfs_close(vn);	
  return 0;
}
#endif

#if OPT_SYSCALLS
int
sys_write(int fd, userptr_t buf_ptr, size_t size)
{
  int i;
  char *p = (char *)buf_ptr;

  if (fd!=STDOUT_FILENO && fd!=STDERR_FILENO) {
#if OPT_FILE
    return file_write(fd, buf_ptr, size);
#else
    kprintf("sys_write supported only to stdout\n");
    return -1;
#endif
  }
  for (i=0; i<(int)size; i++) {
    putch(p[i]);
  }

  return (int)size;
}
#endif

#if OPT_SYSCALLS
int
sys_read(int fd, userptr_t buf_ptr, size_t size)
{
  int i;
  char *p = (char *)buf_ptr;
  
  if (fd!=STDIN_FILENO) {
#if OPT_FILE
	int r=file_read(fd, buf_ptr, size);
    return r;
#else
    kprintf("sys_read supported only to stdin\n");
    return -1;
#endif
  }

  for (i=0; i<(int)size; i++) {
    p[i] = getch();
    if (p[i] < 0) 
      return i;
  }

  return (int)size;
}
#endif
