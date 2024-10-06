#include <libce.h>
#include <virtio.h>

bool create_virtq(virtq &queue, natw queue_size, natw avail_flags)
{
  if (queue_size & (queue_size - 1))
    return false;

  int desc_size = 16 * queue_size;
  int avail_size = 6 + 2 * queue_size;
  int used_size = 6 + 8 * queue_size;

  queue.num = 0;

  queue.desc = (virtq_desc *) alloc_aligned(desc_size, align_val_t(16));
  if (!queue.desc) {
    goto err_desc;
  }
  memset(queue.desc, 0, desc_size);

  queue.avail = (virtq_avail *) alloc_aligned(avail_size, align_val_t(2));
  if (!queue.avail) {
    goto err_avail;
  }
  memset(queue.avail, 0, avail_size);
  queue.avail->flags = avail_flags;

  queue.used = (virtq_used*) alloc_aligned(used_size, align_val_t(4));
  if (!queue.used) {
    goto err_used;
  }
  memset(queue.used, 0, used_size);

  return true;

err_used:
  dealloca(queue.avail);
err_avail:
  dealloca(queue.desc);
err_desc:
  return false;
}

bool enable_virtq(virtio_pci_common_cfg &common_cfg, natw queue_select, natw queue_size,
                  natw queue_msix_vector, virtq queue, paddr (*get_real_addr)(void *ff))
{
  common_cfg.queue_select      = queue_select;
  common_cfg.queue_size        = queue_size;
  common_cfg.queue_msix_vector = queue_msix_vector;
  common_cfg.queue_desc        = get_real_addr(queue.desc);
  common_cfg.queue_driver      = get_real_addr(queue.avail);
  common_cfg.queue_device      = get_real_addr(queue.used);

  if (common_cfg.queue_msix_vector == VIRTIO_MSI_NO_VECTOR) {
    return false;
  }
  common_cfg.queue_enable = 1;
  return true;
}

void add_buf_desc(virtq &queue, natw index, natq addr, natl len, natw flags, natw next,
                  paddr (*get_real_addr)(void *ff))
{
  queue.desc[index].addr  = get_real_addr((void *) addr);
  queue.desc[index].len   = len;
  queue.desc[index].flags = flags;
  queue.desc[index].next  = next;
}

void memory_barrier()
{
  asm(".option push");
  asm(".option arch, +zifencei");
  asm("fence");
  asm(".option pop");
}
