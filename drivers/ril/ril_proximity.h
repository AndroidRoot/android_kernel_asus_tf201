#ifndef RIL_PROXIMITY_H
#define RIL_PROXIMITY_H

int ril_proximity_init(struct device *target_device, struct workqueue_struct *queue);
void ril_proximity_exit(void);
irqreturn_t ril_proximity_interrupt_handle(int irq, void *dev_id);

#endif
