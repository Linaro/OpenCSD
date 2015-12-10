/*
 * Copyright(C) 2016 Linaro Limited. All rights reserved.
 * Author: Mathieu Poirier <mathieu.poirier@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/coresight.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>

#include "coresight-priv.h"
#include "coresight-tmc.h"

/**
 * struct etr_page - DMA'able and virtual address representation for a page
 * @daddr:		DMA'able page address returned by dma_map_page()
 * @vaddr:		Virtual address returned by page_address()
 */
struct etr_page {
	dma_addr_t	daddr;
	u64		vaddr;
};

/**
 * struct cs_etr_buffer - keep track of a recording session' specifics
 * @dev:		device reference to be used with the DMA API
 * @tmc:		generic portion of the TMC buffers
 * @etr_nr_pages:	number of memory pages for the ETR-SG trace storage
 * @pt_vaddr:		the virtual address of the first page table entry
 * @page_addr:		quick access to all the pages held in the page table
 */
struct cs_etr_buffers {
	struct device		*dev;
	struct cs_buffers	tmc;
	unsigned int		etr_nr_pages;
	void __iomem		*pt_vaddr;
	struct etr_page		page_addr[0];
};

#define TMC_ETR_ENTRIES_PER_PT (PAGE_SIZE / sizeof(u32))

/*
 * Helpers for scatter-gather descriptors.  Descriptors are defined as follow:
 *
 * ---Bit31------------Bit4-------Bit1-----Bit0--
 * |     Address[39:12]    | SBZ |  Entry Type  |
 * ----------------------------------------------
 *
 * Address: Bits [39:12] of a physical page address. Bits [11:0] are
 *	    always zero.
 *
 * Entry type:	b10 - Normal entry
 *		b11 - Last entry in a page table
 *		b01 - Last entry
 */
#define TMC_ETR_SG_LST_ENT(phys_pte)	(((phys_pte >> PAGE_SHIFT) << 4) | 0x1)
#define TMC_ETR_SG_ENT(phys_pte)	(((phys_pte >> PAGE_SHIFT) << 4) | 0x2)
#define TMC_ETR_SG_NXT_TBL(phys_pte)	(((phys_pte >> PAGE_SHIFT) << 4) | 0x3)

#define TMC_ETR_SG_ENT_TO_PG(entry)	((entry >> 4) << PAGE_SHIFT)

static void tmc_etr_enable_hw_cnt_mem(struct tmc_drvdata *drvdata)
{
	u32 axictl;

	/* Zero out the memory to help with debug */
	memset(drvdata->vaddr, 0, drvdata->size);

	CS_UNLOCK(drvdata->base);

	/* Wait for TMCSReady bit to be set */
	tmc_wait_for_tmcready(drvdata);

	writel_relaxed(drvdata->size / 4, drvdata->base + TMC_RSZ);
	writel_relaxed(TMC_MODE_CIRCULAR_BUFFER, drvdata->base + TMC_MODE);

	axictl = readl_relaxed(drvdata->base + TMC_AXICTL);
	axictl |= TMC_AXICTL_WR_BURST_16;
	writel_relaxed(axictl, drvdata->base + TMC_AXICTL);
	axictl &= ~TMC_AXICTL_SCT_GAT_MODE;
	writel_relaxed(axictl, drvdata->base + TMC_AXICTL);
	axictl = (axictl &
		  ~(TMC_AXICTL_PROT_CTL_B0 | TMC_AXICTL_PROT_CTL_B1)) |
		  TMC_AXICTL_PROT_CTL_B1;
	writel_relaxed(axictl, drvdata->base + TMC_AXICTL);

	writel_relaxed(drvdata->paddr, drvdata->base + TMC_DBALO);
	writel_relaxed(0x0, drvdata->base + TMC_DBAHI);
	writel_relaxed(TMC_FFCR_EN_FMT | TMC_FFCR_EN_TI |
		       TMC_FFCR_FON_FLIN | TMC_FFCR_FON_TRIG_EVT |
		       TMC_FFCR_TRIGON_TRIGIN,
		       drvdata->base + TMC_FFCR);
	writel_relaxed(drvdata->trigger_cntr, drvdata->base + TMC_TRG);
	tmc_enable_hw(drvdata);

	CS_LOCK(drvdata->base);
}

static void tmc_etr_enable_hw_sg_mem(struct tmc_drvdata *drvdata)
{
	u32 axictl;

	CS_UNLOCK(drvdata->base);

	/* Wait for TMCSReady bit to be set */
	tmc_wait_for_tmcready(drvdata);

	writel_relaxed(TMC_MODE_CIRCULAR_BUFFER, drvdata->base + TMC_MODE);

	axictl = readl_relaxed(drvdata->base + TMC_AXICTL);
	/* half the write buffer depth */
	axictl |= TMC_AXICTL_WR_BURST_08;
	/* enable scatter-gather mode */
	axictl |= TMC_AXICTL_SCT_GAT_MODE;
	/* enable non-secure, priviledged access */
	axictl |= (TMC_AXICTL_PROT_CTL_B0 | TMC_AXICTL_PROT_CTL_B1);

	writel_relaxed(axictl, drvdata->base + TMC_AXICTL);

	writel_relaxed(drvdata->paddr, drvdata->base + TMC_DBALO);

	/*
	 * DBAHI Holds the upper eight bits of the 40-bit address used to
	 * locate the trace buffer in system memory.
	 */
	writel_relaxed((drvdata->paddr >> 32) & 0xFF,
			drvdata->base + TMC_DBAHI);

	writel_relaxed(TMC_FFCR_EN_FMT | TMC_FFCR_EN_TI |
		       TMC_FFCR_FON_FLIN | TMC_FFCR_FON_TRIG_EVT |
		       TMC_FFCR_TRIGON_TRIGIN,
		       drvdata->base + TMC_FFCR);
	writel_relaxed(drvdata->trigger_cntr, drvdata->base + TMC_TRG);
	tmc_enable_hw(drvdata);

	CS_LOCK(drvdata->base);
}

static void tmc_etr_dump_hw_cnt_mem(struct tmc_drvdata *drvdata)
{
	u32 rwp, val;

	rwp = readl_relaxed(drvdata->base + TMC_RWP);
	val = readl_relaxed(drvdata->base + TMC_STS);

	/*
	 * Adjust the buffer to point to the beginning of the trace data
	 * and update the available trace data.
	 */
	if (val & TMC_STS_FULL) {
		drvdata->buf = drvdata->vaddr + rwp - drvdata->paddr;
		drvdata->len = drvdata->size;
	} else {
		drvdata->buf = drvdata->vaddr;
		drvdata->len = rwp - drvdata->paddr;
	}
}

static void tmc_etr_disable_hw(struct tmc_drvdata *drvdata)
{
	CS_UNLOCK(drvdata->base);

	tmc_flush_and_stop(drvdata);
	/*
	 * When operating in sysFS mode the content of the buffer needs to be
	 * read before the TMC is disabled.
	 */
	if (local_read(&drvdata->mode) == CS_MODE_SYSFS)
		tmc_etr_dump_hw_cnt_mem(drvdata);

	tmc_disable_hw(drvdata);

	CS_LOCK(drvdata->base);
}

static int tmc_enable_etr_sink_sysfs(struct coresight_device *csdev, u32 mode)
{
	int ret = 0;
	bool used = false;
	long val;
	unsigned long flags;
	void __iomem *vaddr = NULL;
	dma_addr_t paddr;
	struct tmc_drvdata *drvdata = dev_get_drvdata(csdev->dev.parent);

	 /* This shouldn't be happening */
	if (WARN_ON(mode != CS_MODE_SYSFS))
		return -EINVAL;

	/*
	 * If we don't have a buffer release the lock and allocate memory.
	 * Otherwise keep the lock and move along.
	 */
	spin_lock_irqsave(&drvdata->spinlock, flags);
	if (!drvdata->vaddr) {
		spin_unlock_irqrestore(&drvdata->spinlock, flags);

		/*
		 * Contiguous  memory can't be allocated while a spinlock is
		 * held.  As such allocate memory here and free it if a buffer
		 * has already been allocated (from a previous session).
		 */
		vaddr = dma_alloc_coherent(drvdata->dev, drvdata->size,
					   &paddr, GFP_KERNEL);
		if (!vaddr)
			return -ENOMEM;

		/* Let's try again */
		spin_lock_irqsave(&drvdata->spinlock, flags);
	}

	if (drvdata->reading) {
		ret = -EBUSY;
		goto out;
	}

	val = local_xchg(&drvdata->mode, mode);
	/*
	 * In sysFS mode we can have multiple writers per sink.  Since this
	 * sink is already enabled no memory is needed and the HW need not be
	 * touched.
	 */
	if (val == CS_MODE_SYSFS)
		goto out;

	/*
	 * If drvdata::buf == NULL, use the memory allocated above.
	 * Otherwise a buffer still exists from a previous session, so
	 * simply use that.
	 */
	if (drvdata->buf == NULL) {
		used = true;
		drvdata->vaddr = vaddr;
		drvdata->paddr = paddr;
		drvdata->buf = drvdata->vaddr;
	}

	memset(drvdata->vaddr, 0, drvdata->size);

	tmc_etr_enable_hw_cnt_mem(drvdata);
out:
	spin_unlock_irqrestore(&drvdata->spinlock, flags);

	/* Free memory outside the spinlock if need be */
	if (!used && vaddr)
		dma_free_coherent(drvdata->dev, drvdata->size, vaddr, paddr);

	if (!ret)
		dev_info(drvdata->dev, "TMC-ETR enabled\n");

	return ret;
}

static int tmc_enable_etr_sink_perf(struct coresight_device *csdev, u32 mode)
{
	int ret = 0;
	long val;
	unsigned long flags;
	struct tmc_drvdata *drvdata = dev_get_drvdata(csdev->dev.parent);

	 /* This shouldn't be happening */
	if (WARN_ON(mode != CS_MODE_PERF))
		return -EINVAL;

	spin_lock_irqsave(&drvdata->spinlock, flags);
	if (drvdata->reading) {
		ret = -EINVAL;
		goto out;
	}

	val = local_xchg(&drvdata->mode, mode);
	/*
	 * In Perf mode there can be only one writer per sink.  There
	 * is also no need to continue if the ETR is already operated
	 * from sysFS.
	 */
	if (val != CS_MODE_DISABLED) {
		ret = -EINVAL;
		goto out;
	}

	tmc_etr_enable_hw_sg_mem(drvdata);
out:
	spin_unlock_irqrestore(&drvdata->spinlock, flags);

	return ret;
}

static int tmc_enable_etr_sink(struct coresight_device *csdev, u32 mode)
{
	switch (mode) {
	case CS_MODE_SYSFS:
		return tmc_enable_etr_sink_sysfs(csdev, mode);
	case CS_MODE_PERF:
		return tmc_enable_etr_sink_perf(csdev, mode);
	}

	/* We shouldn't be here */
	return -EINVAL;
}

static void tmc_disable_etr_sink(struct coresight_device *csdev)
{
	long val;
	unsigned long flags;
	struct tmc_drvdata *drvdata = dev_get_drvdata(csdev->dev.parent);

	spin_lock_irqsave(&drvdata->spinlock, flags);
	if (drvdata->reading) {
		spin_unlock_irqrestore(&drvdata->spinlock, flags);
		return;
	}

	val = local_xchg(&drvdata->mode, CS_MODE_DISABLED);
	/* Disable the TMC only if it needs to */
	if (val != CS_MODE_DISABLED)
		tmc_etr_disable_hw(drvdata);

	spin_unlock_irqrestore(&drvdata->spinlock, flags);

	dev_info(drvdata->dev, "TMC-ETR disabled\n");
}

/*
 * The default perf ring buffer size is 32 and 1024 pages for user and kernel
 * space respectively.  The size of the intermediate SG list is allowed
 * to match the size of the perf ring buffer but cap it to the default
 * kernel size.
 */
#define DEFAULT_NR_KERNEL_PAGES	1024
static int tmc_get_etr_pages(int nr_pages)
{
	if (nr_pages <= DEFAULT_NR_KERNEL_PAGES)
		return nr_pages;

	return DEFAULT_NR_KERNEL_PAGES;
}

/*
 * Go through all the pages in the SG list and check if @phys_addr
 * falls within one of those.  If so record the information in
 * @page and @offset.
 */
static int
tmc_get_sg_page_index(struct cs_etr_buffers *etr_buffer,
		      u64 phys_addr, u32 *page, u32 *offset)
{
	int i = 0, pte = 0, nr_pages = etr_buffer->etr_nr_pages;
	u32 *page_table_itr = etr_buffer->pt_vaddr;
	phys_addr_t phys_page_addr;

	/* Circle through all the pages in the SG list */
	while (pte < nr_pages) {
		phys_page_addr = TMC_ETR_SG_ENT_TO_PG((u64)*page_table_itr);

		/* Does @phys_addr falls within this page? */
		if (phys_addr >= phys_page_addr &&
		    phys_addr < (phys_page_addr + PAGE_SIZE)) {
			*page = pte;
			*offset = phys_addr - phys_page_addr;
			return 0;
		}

		if (pte == nr_pages - 1) {
			/* The last page in the SG list */
			pte++;
		} else if (i == TMC_ETR_ENTRIES_PER_PT - 1) {
			/*
			 * The last entry in this page table - get a reference
			 * on the next page table and do _not_ increment @pte
			 */
			page_table_itr = phys_to_virt(phys_page_addr);
			i = 0;
		} else {
			/* A normal page in the SG list */
			page_table_itr++;
			pte++;
			i++;
		}
	}

	return -EINVAL;
}

static void tmc_sg_page_sync(struct cs_etr_buffers *etr_buffer,
			     int start_page, u64 to_sync)
{
	int i, index;
	int pages_to_sync = DIV_ROUND_UP_ULL(to_sync, PAGE_SIZE);
	dma_addr_t daddr;
	struct device *dev = etr_buffer->dev;

	for (i = start_page; i < (start_page + pages_to_sync); i++) {
		/* Wrap around the etr page list if need be */
		index = i % etr_buffer->etr_nr_pages;
		daddr = etr_buffer->page_addr[index].daddr;
		dma_sync_single_for_cpu(dev, daddr, PAGE_SIZE, DMA_FROM_DEVICE);
	}
}

static void tmc_free_sg_buffer(struct cs_etr_buffers *etr_buffer, int nr_pages)
{
	int i = 0, pte = 0;
	u32 *page_addr, *page_table_itr;
	u32 *page_table_addr = etr_buffer->pt_vaddr;
	phys_addr_t phys_page_addr;
	dma_addr_t daddr;
	struct device *dev = etr_buffer->dev;

	if (!page_table_addr)
		return;

	page_table_itr = page_table_addr;
	while (pte < nr_pages) {
		phys_page_addr = TMC_ETR_SG_ENT_TO_PG((u64)*page_table_itr);
		page_addr = phys_to_virt(phys_page_addr);

		if (pte == nr_pages - 1) {
			/* The last page in the SG list */
			daddr = etr_buffer->page_addr[pte].daddr;
			page_addr = (u32 *)etr_buffer->page_addr[pte].vaddr;

			dma_unmap_page(dev, daddr, PAGE_SIZE,
				       DMA_FROM_DEVICE);

			/* Free the current page */
			free_page((unsigned long)page_addr);
			/* Free the current page table */
			free_page((unsigned long)page_table_addr);

			pte++;
		} else if (i == TMC_ETR_ENTRIES_PER_PT - 1) {
			/* The last entry in this page table */
			page_addr = phys_to_virt(phys_page_addr);

			/* Free the current page table */
			free_page((unsigned long)page_table_addr);
			/* Move along to the next one */
			page_table_addr = page_addr;
			page_table_itr = page_table_addr;

			i = 0;
		} else {
			/* A normal page in the SG list */
			daddr = etr_buffer->page_addr[pte].daddr;
			page_addr = (u32 *)etr_buffer->page_addr[pte].vaddr;

			dma_unmap_page(dev, daddr, PAGE_SIZE,
				       DMA_FROM_DEVICE);

			/* Free the current page */
			free_page((unsigned long)page_addr);

			page_table_itr++;
			pte++;
			i++;
		}
	}
}

static dma_addr_t tmc_setup_dma_page(struct device *dev, struct page *page)
{
	dma_addr_t daddr;

	/*
	 * No data is communicated to the device, as such there is no point
	 * in setting the direction to DMA_BIDIRECTIONAL.  See
	 * Documentation/DMA-API-HOWTO.txt for details.
	 */
	daddr = dma_map_page(dev, page, 0, PAGE_SIZE, DMA_FROM_DEVICE);
	if (dma_mapping_error(dev, daddr)) {
		__free_page(page);
		return -EINVAL;
	}

	return daddr;
}

static int
tmc_alloc_sg_buffer(struct cs_etr_buffers *etr_buffer, int cpu, int nr_pages)
{
	int i = 0, node, pte = 0, ret = 0;
	dma_addr_t dma_page_addr;
	u32 *page_table_addr, *page_addr;
	struct page *page;
	struct device *dev = etr_buffer->dev;

	if (cpu == -1)
		cpu = smp_processor_id();
	node = cpu_to_node(cpu);

	/* Allocate the first page table */
	page = alloc_pages_node(node, GFP_KERNEL | __GFP_ZERO, 0);
	if (!page)
		return -ENOMEM;

	page_table_addr = page_address(page);
	/*
	 * Keep track of the first page table, the rest will be chained
	 * in the last page table entry.
	 */
	etr_buffer->pt_vaddr = page_table_addr;

	while (pte < nr_pages) {
		page = alloc_pages_node(node,
					GFP_KERNEL | __GFP_ZERO, 0);
		if (!page) {
			ret = -ENOMEM;
			goto err;
		}

		page_addr = page_address(page);

		if (pte == nr_pages - 1) {
			/* The last page in the list */
			dma_page_addr = tmc_setup_dma_page(dev, page);
			if (dma_page_addr == -EINVAL) {
				ret = -EINVAL;
				goto err;
			}

			*page_table_addr = TMC_ETR_SG_LST_ENT(dma_page_addr);

			etr_buffer->page_addr[pte].vaddr = (u64)page_addr;
			etr_buffer->page_addr[pte].daddr = dma_page_addr;

			pte++;
		} else if (i == TMC_ETR_ENTRIES_PER_PT - 1) {
			/* The last entry in this page table */
			*page_table_addr =
				TMC_ETR_SG_NXT_TBL(virt_to_phys(page_addr));
			/* Move on to the next page table */
			page_table_addr = page_addr;

			i = 0;
		} else {
			/* A normal page in the SG list */
			dma_page_addr = tmc_setup_dma_page(dev, page);
			if (dma_page_addr == -EINVAL) {
				ret = -EINVAL;
				goto err;
			}

			*page_table_addr = TMC_ETR_SG_ENT(dma_page_addr);

			etr_buffer->page_addr[pte].vaddr = (u64)page_addr;
			etr_buffer->page_addr[pte].daddr = dma_page_addr;

			page_table_addr++;
			pte++;
			i++;
		}
	}

	return 0;

err:
	tmc_free_sg_buffer(etr_buffer, pte);
	etr_buffer->pt_vaddr = NULL;
	return ret;
}

static void *tmc_alloc_etr_buffer(struct coresight_device *csdev, int cpu,
				  void **pages, int nr_pages, bool overwrite)
{
	int etr_pages, node;
	struct device *dev = csdev->dev.parent;
	struct cs_etr_buffers *buf;

	if (cpu == -1)
		cpu = smp_processor_id();
	node = cpu_to_node(cpu);

	/* Register DBALO and DBAHI form a 40-bit address range */
	if (dma_set_mask(dev, DMA_BIT_MASK(40)))
		return NULL;

	/*
	 * The HW can't start collecting data in the middle of the SG list,
	 * it must start at the beginning.  As such we can't use the ring
	 * buffer provided by perf as entries into the page tables since
	 * it is not guaranteed that user space will have the chance to
	 * consume the data before the next trace run begins.
	 *
	 * To work around this reserve a set of pages that will be used as
	 * and intermediate (SG) buffer.  This isn't optimal but the best we
	 * can do with the current HW revision.
	 */
	etr_pages = tmc_get_etr_pages(nr_pages);

	/* Allocate memory structure for interaction with Perf */
	buf = kzalloc_node(offsetof(struct cs_etr_buffers,
			   page_addr[etr_pages]),
			   GFP_KERNEL, node);
	if (!buf)
		return NULL;

	buf->dev = dev;

	if (tmc_alloc_sg_buffer(buf, cpu, etr_pages)) {
		kfree(buf);
		return NULL;
	}

	buf->etr_nr_pages = etr_pages;
	buf->tmc.snapshot = overwrite;
	buf->tmc.nr_pages = nr_pages;
	buf->tmc.data_pages = pages;

	return buf;
}

static void tmc_free_etr_buffer(void *config)
{
	struct cs_etr_buffers *buf = config;

	tmc_free_sg_buffer(buf, buf->etr_nr_pages);
	kfree(buf);
}

static int tmc_set_etr_buffer(struct coresight_device *csdev,
			      struct perf_output_handle *handle,
			      void *sink_config)
{
	unsigned long head;
	struct cs_etr_buffers *buf = sink_config;
	struct tmc_drvdata *drvdata = dev_get_drvdata(csdev->dev.parent);

	/* wrap head around to the amount of space we have */
	head = handle->head & ((buf->tmc.nr_pages << PAGE_SHIFT) - 1);

	/* find the page to write to */
	buf->tmc.cur = head / PAGE_SIZE;

	/* and offset within that page */
	buf->tmc.offset = head % PAGE_SIZE;

	local_set(&buf->tmc.data_size, 0);

	/* Keep track of how big the internal SG list is */
	drvdata->size = buf->etr_nr_pages << PAGE_SHIFT;

	/* Tell the HW where to put the trace data */
	drvdata->paddr = virt_to_phys(buf->pt_vaddr);

	return 0;
}

static unsigned long tmc_reset_etr_buffer(struct coresight_device *csdev,
					  struct perf_output_handle *handle,
					  void *sink_config, bool *lost)
{
	long size = 0;
	struct cs_etr_buffers *buf = sink_config;
	struct tmc_drvdata *drvdata = dev_get_drvdata(csdev->dev.parent);

	if (buf) {
		/*
		 * In snapshot mode ->data_size holds the new address of the
		 * ring buffer's head.  The size itself is the whole address
		 * range since we want the latest information.
		 */
		if (buf->tmc.snapshot) {
			size = buf->tmc.nr_pages << PAGE_SHIFT;
			handle->head = local_xchg(&buf->tmc.data_size, size);
		}

		/*
		 * Tell the tracer PMU how much we got in this run and if
		 * something went wrong along the way.  Nobody else can use
		 * this cs_etr_buffers instance until we are done.  As such
		 * resetting parameters here and squaring off with the ring
		 * buffer API in the tracer PMU is fine.
		 */
		*lost = !!local_xchg(&buf->tmc.lost, 0);
		size = local_xchg(&buf->tmc.data_size, 0);
	}

	/* Get ready for another run */
	drvdata->vaddr = NULL;
	drvdata->paddr = 0;

	return size;
}

static void tmc_update_etr_buffer(struct coresight_device *csdev,
				  struct perf_output_handle *handle,
				  void *sink_config)
{
	bool full;
	int i, rb_index, sg_index = 0;
	u32 rwplo, rwphi, rb_offset, sg_offset = 0;
	u32 stop_index, stop_offset, to_copy, sg_size;
	u32 *rb_ptr, *sg_ptr;
	u64 rwp, to_read;
	struct cs_etr_buffers *etr_buf = sink_config;
	struct cs_buffers *cs_buf = &etr_buf->tmc;
	struct tmc_drvdata *drvdata = dev_get_drvdata(csdev->dev.parent);

	if (!etr_buf)
		return;

	/* This shouldn't happen */
	if (WARN_ON_ONCE(local_read(&drvdata->mode) != CS_MODE_PERF))
		return;

	CS_UNLOCK(drvdata->base);

	tmc_flush_and_stop(drvdata);

	rwplo = readl_relaxed(drvdata->base + TMC_RWP);
	rwphi = readl_relaxed(drvdata->base + TMC_RWPHI);
	full = (readl_relaxed(drvdata->base + TMC_STS) & TMC_STS_FULL);

	/* Combine the high and low part of the rwp to make a full address */
	rwp = (u64)rwphi << 32;
	rwp |= rwplo;

	/* Convert the stop address in RAM to a page and an offset */
	if (tmc_get_sg_page_index(etr_buf, rwp, &stop_index, &stop_offset))
		goto out;

	if (full) {
		/*
		 * The buffer head has wrapped around.  As such the size
		 * is the entire buffer length and the index and offset in
		 * the scatter-gather list are moved forward.
		 */
		local_inc(&cs_buf->lost);
		to_read = drvdata->size;
		sg_index = stop_index;
		sg_offset = stop_offset;
	} else {
		to_read = (stop_index * PAGE_SIZE) + stop_offset;
	}

	/*
	 * The TMC RAM buffer may be bigger than the space available in the
	 * perf ring buffer (handle->size).  If so advance the RRP so that we
	 * get the latest trace data.
	 */
	if (to_read > handle->size) {
		u64 rrp;

		/*
		 * Compute where we should start reading from
		 * relative to rwp.
		 */
		rrp = rwp + drvdata->size;
		/* Go back just enough */
		rrp -= handle->size;
		/* Make sure we are still within our limits */
		rrp %= drvdata->size;

		/* Get a new index and offset based on rrp */
		if (tmc_get_sg_page_index(etr_buf, rrp,
					  &stop_index, &stop_offset))
			goto out;

		/* Tell user space we lost data */
		local_inc(&cs_buf->lost);
		to_read = handle->size;
		/* Adjust start index and offset */
		sg_index = stop_index;
		sg_offset = stop_offset;
	}

	/* Get a handle on where the Perf ring buffer is */
	rb_index = cs_buf->cur;
	rb_offset = cs_buf->offset;

	/* Refresh the SG list */
	tmc_sg_page_sync(etr_buf, sg_index, to_read);

	for (i = to_read; i > 0; ) {
		/* Get current location of the perf ring buffer */
		rb_ptr = cs_buf->data_pages[rb_index] + rb_offset;
		/* Get current location in the ETR SG list */
		sg_ptr = (u32 *)(etr_buf->page_addr[sg_index].vaddr +
				 sg_offset);

		/*
		 * First figure out the maximum amount of data we can get out
		 * of the ETR SG list.
		 */
		if (i < PAGE_SIZE)
			sg_size = i;
		else
			sg_size = PAGE_SIZE - sg_offset;

		/*
		 * We have two page table buffer, one is the Perf ring
		 * buffer while the other one is the internal ETR SG list.
		 * Get the maximum amount of information we can copy from the
		 * ETR SG list to the Perf ring buffer, which happens to be
		 * the minimum space available in the current pages
		 * (both of them).
		 */
		to_copy = min((u32)(PAGE_SIZE - rb_offset), sg_size);

		/* Transfer trace data from ETR SG list to Perf ring buffer */
		memcpy(rb_ptr, sg_ptr, to_copy);

		rb_offset += to_copy;
		sg_offset += to_copy;
		i -= to_copy;

		/* If a page is full, move to the next one */
		if (rb_offset == PAGE_SIZE) {
			rb_offset = 0;
			rb_index++;
			rb_index %= cs_buf->nr_pages;
		}

		if (sg_offset == PAGE_SIZE) {
			sg_offset = 0;
			sg_index++;
			sg_index %= etr_buf->etr_nr_pages;
		}
	}

	/*
	 * In snapshot mode all we have to do is communicate to
	 * perf_aux_output_end() the address of the current head.  In full
	 * trace mode the same function expects a size to move rb->aux_head
	 * forward.
	 */
	if (etr_buf->tmc.snapshot)
		local_set(&etr_buf->tmc.data_size,
			  stop_index * PAGE_SIZE + stop_offset);
	else
		local_add(to_read, &etr_buf->tmc.data_size);

out:
	CS_LOCK(drvdata->base);
}

static const struct coresight_ops_sink tmc_etr_sink_ops = {
	.enable		= tmc_enable_etr_sink,
	.disable	= tmc_disable_etr_sink,
	.alloc_buffer	= tmc_alloc_etr_buffer,
	.free_buffer	= tmc_free_etr_buffer,
	.set_buffer	= tmc_set_etr_buffer,
	.reset_buffer	= tmc_reset_etr_buffer,
	.update_buffer	= tmc_update_etr_buffer,
};

const struct coresight_ops tmc_etr_cs_ops = {
	.sink_ops	= &tmc_etr_sink_ops,
};

int tmc_read_prepare_etr(struct tmc_drvdata *drvdata)
{
	int ret = 0;
	long val;
	unsigned long flags;

	/* config types are set a boot time and never change */
	if (WARN_ON_ONCE(drvdata->config_type != TMC_CONFIG_TYPE_ETR))
		return -EINVAL;

	spin_lock_irqsave(&drvdata->spinlock, flags);
	if (drvdata->reading) {
		ret = -EBUSY;
		goto out;
	}

	val = local_read(&drvdata->mode);
	/* Don't interfere if operated from Perf */
	if (val == CS_MODE_PERF) {
		ret = -EINVAL;
		goto out;
	}

	/* If drvdata::buf is NULL the trace data has been read already */
	if (drvdata->buf == NULL) {
		ret = -EINVAL;
		goto out;
	}

	/* Disable the TMC if need be */
	if (val == CS_MODE_SYSFS)
		tmc_etr_disable_hw(drvdata);

	drvdata->reading = true;
out:
	spin_unlock_irqrestore(&drvdata->spinlock, flags);

	return ret;
}

int tmc_read_unprepare_etr(struct tmc_drvdata *drvdata)
{
	unsigned long flags;
	dma_addr_t paddr;
	void __iomem *vaddr = NULL;

	/* config types are set a boot time and never change */
	if (WARN_ON_ONCE(drvdata->config_type != TMC_CONFIG_TYPE_ETR))
		return -EINVAL;

	spin_lock_irqsave(&drvdata->spinlock, flags);

	/* RE-enable the TMC if need be */
	if (local_read(&drvdata->mode) == CS_MODE_SYSFS) {
		/*
		 * The trace run will continue with the same allocated trace
		 * buffer. The trace buffer is cleared in
		 * tmc_etr_enable_hw_cnt_mem(), so we don't have to explicitly
		 * clear it. Also, since the tracer is still enabled
		 * drvdata::buf can't be NULL.
		 */
		tmc_etr_enable_hw_cnt_mem(drvdata);
	} else {
		/*
		 * The ETR is not tracing and the buffer was just read.
		 * As such prepare to free the trace buffer.
		 */
		vaddr = drvdata->vaddr;
		paddr = drvdata->paddr;
		drvdata->buf = drvdata->vaddr = NULL;
	}

	drvdata->reading = false;
	spin_unlock_irqrestore(&drvdata->spinlock, flags);

	/* Free allocated memory out side of the spinlock */
	if (vaddr)
		dma_free_coherent(drvdata->dev, drvdata->size, vaddr, paddr);

	return 0;
}
