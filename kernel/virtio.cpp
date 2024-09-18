#include <libce.h>
#include <virtio.h>

bool crea_virtq(virtq &queue, natw queue_size, natw avail_flags)
{
  if (queue_size & (queue_size - 1))
    return false;

  int desc_size = 16 * queue_size;
  int avail_size = 6 + 2 * queue_size;
  int used_size = 6 + 8 * queue_size;

  queue.num = queue_size;

  queue.desc = (virtq_desc *) alloca(desc_size);
  if (!queue.desc) {
    goto err_desc;
  }
  memset(queue.desc, 0, desc_size);

  queue.avail = (virtq_avail *) alloca(avail_size);
  if (!queue.avail) {
    goto err_avail;
  }
  memset(queue.avail, 0, avail_size);
  queue.avail->flags = avail_flags;

  queue.used = (virtq_used*) alloca(used_size);
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

void assign_virtq(virtq queue, virtio_pci_common_cfg *comm_cfg)
{
  comm_cfg->queue_desc = (natq) queue.desc;
  comm_cfg->queue_driver = (natq) queue.avail;
  comm_cfg->queue_device = (natq) queue.used;
}
