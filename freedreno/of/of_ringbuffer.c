/*
 * Copyright (C) 2014 Tomasz Figa <tomasz.figa@gmail.com>
 *
 * Parts shamelessly copied from msm backend:
 *
 * Copyright (C) 2013 Rob Clark <robclark@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <assert.h>

#include "freedreno_ringbuffer.h"
#include "of_priv.h"

struct of_ringbuffer {
	struct fd_ringbuffer base;
	struct fd_bo *ring_bo;

	struct list_head submit_list;
};

static inline struct of_ringbuffer * to_of_ringbuffer(struct fd_ringbuffer *x)
{
	return (struct of_ringbuffer *)x;
}

static void * of_ringbuffer_hostptr(struct fd_ringbuffer *ring)
{
	struct of_ringbuffer *of_ring = to_of_ringbuffer(ring);
	return fd_bo_map(of_ring->ring_bo);
}

static uint32_t offset_bytes(void *end, void *start)
{
	return ((char *)end) - ((char *)start);
}

static int of_ringbuffer_flush(struct fd_ringbuffer *ring, uint32_t *last_start)
{
	struct of_ringbuffer *of_ring = to_of_ringbuffer(ring);
	struct fd_bo *ring_bo = of_ring->ring_bo;
	struct drm_exynos_g3d_submit req = {
			.pipe = to_of_pipe(ring->pipe)->pipe,
			.handle = fd_bo_handle(ring_bo),
	};
	struct of_bo *of_bo = NULL, *tmp;
	int ret, id = ring->pipe->id;
	unsigned long cmd;

	req.offset = offset_bytes(last_start, ring->start);
	req.length = offset_bytes(ring->cur, last_start);

	if (id == FD_PIPE_3D)
		cmd = DRM_EXYNOS_G3D_SUBMIT;
	else
		cmd = DRM_EXYNOS_G2D_SUBMIT;

	ret = drmCommandWriteRead(ring->pipe->dev->fd, cmd, &req, sizeof(req));
	if (ret) {
		ERROR_MSG("submit failed: %d (%s)", ret, strerror(errno));
	} else {
		/* update timestamp on all rings associated with submit: */
		ring->last_timestamp = req.fence;
	}

	LIST_FOR_EACH_ENTRY_SAFE(of_bo, tmp, &of_ring->submit_list, list[id]) {
		struct list_head *list = &of_bo->list[id];
		list_delinit(list);
		of_bo->indexp1[id] = 0;
		fd_bo_del(&of_bo->base);
	}

	return ret;
}

static void of_ringbuffer_reset(struct fd_ringbuffer *ring)
{
	/* Nothing to do here */
}

static void of_ringbuffer_emit_reloc(struct fd_ringbuffer *ring,
		const struct fd_reloc *r)
{
	/* Nothing to do here */
}

static void of_ringbuffer_emit_reloc_ring(struct fd_ringbuffer *ring,
		struct fd_ringmarker *target, struct fd_ringmarker *end)
{
	/* Nothing to do here */
}

static void of_ringbuffer_destroy(struct fd_ringbuffer *ring)
{
	struct of_ringbuffer *of_ring = to_of_ringbuffer(ring);
	if (of_ring->ring_bo)
		fd_bo_del(of_ring->ring_bo);
	free(of_ring);
}

static struct fd_ringbuffer_funcs funcs = {
		.hostptr = of_ringbuffer_hostptr,
		.flush = of_ringbuffer_flush,
		.reset = of_ringbuffer_reset,
		.emit_reloc = of_ringbuffer_emit_reloc,
		.emit_reloc_ring = of_ringbuffer_emit_reloc_ring,
		.destroy = of_ringbuffer_destroy,
};

struct fd_ringbuffer * of_ringbuffer_new(struct fd_pipe *pipe,
		uint32_t size)
{
	struct of_ringbuffer *of_ring;
	struct fd_ringbuffer *ring = NULL;

	of_ring = calloc(1, sizeof(*of_ring));
	if (!of_ring) {
		ERROR_MSG("allocation failed");
		goto fail;
	}

	ring = &of_ring->base;
	ring->funcs = &funcs;

	list_inithead(&of_ring->submit_list);

	of_ring->ring_bo = fd_bo_new(pipe->dev, size, 0);
	if (!of_ring->ring_bo) {
		ERROR_MSG("ringbuffer allocation failed");
		goto fail;
	}

	return ring;
fail:
	if (ring)
		fd_ringbuffer_del(ring);
	return NULL;
}
