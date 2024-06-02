#include "filesystem.h"

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

#include "log.h"
#include "type.h"
#include "utils.h"

list_t *append_mount_options(list_t **head, const char *source,
                             const char *target, const char *filesystem,
                             unsigned long flags, const char *data) {
  struct mount_options *new = malloc(sizeof(struct mount_options));
  new->source = source ? strdup(source) : NULL;
  new->target = target ? strdup(target) : NULL;
  new->filesystem = filesystem ? strdup(filesystem) : NULL;
  new->flags = flags;
  new->data = data ? strdup(data) : NULL;
  return append(head, new);
}

int setup_container_data(const char *container_data_path,
                         const char *image_path) {
  if (access(container_data_path, F_OK) != 0)
    err(EXIT_FAILURE, "access %s", container_data_path);
  if (access(image_path, F_OK) != 0) err(EXIT_FAILURE, "access %s", image_path);

  char lower_dir[PATH_MAX];
  snprintf(lower_dir, PATH_MAX, "%s/lower", container_data_path);
  if (mkdir(lower_dir, 0700)) err(EXIT_FAILURE, "mkdir %s", lower_dir);

  char merged_root[PATH_MAX];
  snprintf(merged_root, PATH_MAX, "%s/merged", container_data_path);
  if (mkdir(merged_root, 0700)) err(EXIT_FAILURE, "mkdir %s", merged_root);

  char diff_dir[PATH_MAX];
  snprintf(diff_dir, PATH_MAX, "%s/diff", container_data_path);
  if (mkdir(diff_dir, 0700)) err(EXIT_FAILURE, "mkdir %s", diff_dir);

  char work_dir[PATH_MAX];
  snprintf(work_dir, PATH_MAX, "%s/work", container_data_path);
  if (mkdir(work_dir, 0700)) err(EXIT_FAILURE, "mkdir %s", work_dir);

  // mount overlayfs
  if (mount(image_path, lower_dir, NULL, MS_BIND, NULL) == -1)
    err(EXIT_FAILURE, "bindmount-rootfs");
  char options[PATH_MAX * 4];
  snprintf(options, PATH_MAX * 4, "lowerdir=%s,upperdir=%s,workdir=%s",
           lower_dir, diff_dir, work_dir);
  if (mount("overlay", merged_root, "overlay", 0, options) == -1)
    err(EXIT_FAILURE, "mount-overlay");

  debug("Container data setup completed\n");

  return 0;
}

int setup_mounts(const char *merged_root, list_t *mounts) {
  char mount_point[PATH_MAX];
  snprintf(mount_point, PATH_MAX, "%s/proc", merged_root);
  if (mount("proc", mount_point, "proc", 0, NULL) == -1)
    err(EXIT_FAILURE, "mount-proc");

  snprintf(mount_point, PATH_MAX, "%s/dev", merged_root);
  if (mount("tmpfs", mount_point, "tmpfs", MS_NOSUID | MS_STRICTATIME,
            "mode=0755,size=65536k") == -1)
    err(EXIT_FAILURE, "mount-dev");

  snprintf(mount_point, PATH_MAX, "%s/dev/pts", merged_root);
  if (mkdir(mount_point, 0700)) err(EXIT_FAILURE, "mkdir-dev/pts");
  if (mount("devpts", mount_point, "devpts", MS_NOEXEC | MS_NOSUID,
            "newinstance,ptmxmode=0666,mode=620") == -1)
    err(EXIT_FAILURE, "mount-dev/pts");

  snprintf(mount_point, PATH_MAX, "%s/dev/shm", merged_root);
  if (mkdir(mount_point, 0700)) err(EXIT_FAILURE, "mkdir-dev/shm");
  if (mount("shm", mount_point, "tmpfs", MS_NOEXEC | MS_NOSUID | MS_NODEV,
            "mode=1777,size=65536k") == -1)
    err(EXIT_FAILURE, "mount-dev/shm");

  snprintf(mount_point, PATH_MAX, "%s/dev/mqueue", merged_root);
  if (mkdir(mount_point, 0700)) err(EXIT_FAILURE, "mkdir-dev/mqueue");
  if (mount("mqueue", mount_point, "mqueue", MS_NOEXEC | MS_NOSUID | MS_NODEV,
            NULL) == -1)
    err(EXIT_FAILURE, "mount-dev/mqueue");

  snprintf(mount_point, PATH_MAX, "%s/sys", merged_root);
  if (mount("sysfs", mount_point, "sysfs",
            MS_NOEXEC | MS_NOSUID | MS_NODEV | MS_RDONLY, NULL) == -1)
    err(EXIT_FAILURE, "mount-sysfs");

  // snprintf(mount_point, PATH_MAX, "%s/sys/fs/cgroup", merged_root);
  // if (mount("none", mount_point, "cgroup2",
  //           MS_NOSUID | MS_NODEV | MS_NOEXEC | MS_RELATIME, NULL) == -1)
  //   err(EXIT_FAILURE, "mount-cgroup");

  const char *devs[] = {"/dev/null", "/dev/zero",   "/dev/full",
                        "/dev/tty",  "/dev/random", "/dev/urandom"};
  for (int i = 0; i < 6; i++) {
    snprintf(mount_point, PATH_MAX, "%s%s", merged_root, devs[i]);
    int fd;
    if (((fd = open(mount_point, O_CREAT, 0644)) == -1) || close(fd))
      err(EXIT_FAILURE, "create-dev");
    if (mount(devs[i], mount_point, NULL, MS_BIND, NULL) == -1)
      err(EXIT_FAILURE, "mount-dev/null");
  }
  snprintf(mount_point, PATH_MAX, "%s/etc/resolv.conf", merged_root);
  if (mount("/etc/resolv.conf", mount_point, NULL,
            MS_BIND | MS_NOATIME | MS_RDONLY, NULL) == -1)
    err(EXIT_FAILURE, "mount-resolv.conf");

  // mount user specified mounts
  for (list_t *node = mounts; node; node = node->next) {
    struct mount_options *mount_option = (struct mount_options *)node->data;
    debug("Mounting %s to %s, fstype: %s, flags: 0x%x, options: %s\n",
          mount_option->source, mount_option->target, mount_option->filesystem,
          mount_option->flags, mount_option->data);
    snprintf(mount_point, PATH_MAX, "%s%s", merged_root, mount_option->target);
    if (access(mount_point, F_OK) == -1) {
      struct stat st;
      if (stat(mount_option->source, &st) == -1)
        err(EXIT_FAILURE, "stat %s", mount_option->source);
      int perm = st.st_mode & 0777;
      if (S_ISDIR(st.st_mode)) {
        if (mkdir(mount_point, perm))
          err(EXIT_FAILURE, "mkdir-%s", mount_point);
      } else {
        int fd = open(mount_point, O_CREAT, perm);
        if (fd == -1) err(EXIT_FAILURE, "create-%s", mount_point);
        close(fd);
      }
    }
    if (mount(mount_option->source, mount_point, mount_option->filesystem,
              MS_BIND, mount_option->data) == -1)
      err(EXIT_FAILURE, "mount-%s:%s", mount_option->source,
          mount_option->target);
    if (mount(NULL, mount_point, NULL, mount_option->flags | MS_REMOUNT,
              NULL) == -1)
      err(EXIT_FAILURE, "mount-remount-%s", mount_point);
  }

  return 0;
}

int setup_filesystem(const char *image_path, const char *container_id,
                     const char *container_base, list_t *mounts) {
  debug("Image path: %s\n", image_path);
  if (image_path == NULL || container_base == NULL) {
    error("Neither rootfs nor container_base should be NULL\n");
    exit(EXIT_FAILURE);
  }

  // remount old rootfs to MS_PRIVATE
  if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) == -1)
    err(EXIT_FAILURE, "mount-MS_PRIVATE");

  char container_path[PATH_MAX - MOUNT_POINT_LEN_MAX];
  snprintf(container_path, PATH_MAX, "%s/%s", container_base, container_id);
  if (mkdir(container_path, 0700))
    err(EXIT_FAILURE, "mkdir %s", container_path);
  debug("Container path: %s\n", container_path);

  setup_container_data(container_path, image_path);

  char merged_root[PATH_MAX];
  snprintf(merged_root, PATH_MAX, "%s/merged", container_path);

  setup_mounts(merged_root, mounts);

  // char cgroup_path[PATH_MAX + 30];
  // snprintf(cgroup_path, PATH_MAX + 30, "/sys/fs/cgroup/system.slice/%s",
  //          container_id);
  // char mount_point[PATH_MAX + 20];
  // snprintf(mount_point, PATH_MAX + 20, "%s/sys/fs/cgroup", merged_root);
  // if (mount(cgroup_path, mount_point, NULL, MS_BIND | MS_RDONLY, NULL) == -1)
  //   err(EXIT_FAILURE, "mount-cgroup");
  // if (mount(NULL, mount_point, NULL, MS_REMOUNT | MS_BIND | MS_RDONLY, NULL)
  // ==
  //     -1)
  //   err(EXIT_FAILURE, "mount-cgroup-remount");

  char put_old[PATH_MAX + 10];
  snprintf(put_old, PATH_MAX + 10, "%s/old_root", merged_root);
  if (mkdir(put_old, 0700)) err(EXIT_FAILURE, "mkdir-put_old");
  if (pivot_root(merged_root, put_old) == -1) err(EXIT_FAILURE, "pivot_root");

  // remove old_root
  if (umount2("/old_root", MNT_DETACH) == -1) perror("umount2");
  if (rmdir("/old_root") == -1) perror("rmdir");

  if (symlink("/dev/pts/ptmx", "/dev/ptmx") == -1)
    err(EXIT_FAILURE, "symlink-ptmx");

  if (mount("tmpfs", "/tmp", "tmpfs", 0, NULL) == -1)
    err(EXIT_FAILURE, "mount-tmpfs");

  if (mount("none", "/sys/fs/cgroup", "cgroup2",
            MS_NOSUID | MS_RDONLY | MS_NODEV | MS_NOEXEC | MS_RELATIME,
            NULL) == -1)
    err(EXIT_FAILURE, "mount-sys/fs/cgroup");

  if (chdir("/") == -1) err(EXIT_FAILURE, "chdir");
  debug("Filesystem setup completed\n");
  return 0;
}

int parse_bind_mount_option(const char *options) {
  if (!options) return 0;
  char *options_copy = strdup(options);
  char *option = strtok(options_copy, ",");
  int flags = 0;
  while (option) {
    if (strncmp(option, "ro", 2) == 0) {
      flags |= MS_RDONLY;
    } else if (strncmp(option, "rw", 2) == 0) {
      flags &= ~MS_RDONLY;
    } else {
      err(EXIT_FAILURE, "Invalid bind mount option: %s\n", option);
    }
    option = strtok(NULL, ",");
  }
  return flags;
}