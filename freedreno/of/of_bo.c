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

#include "of_priv.h"

static int of_bo_cpu_prep(struct fd_bo *bo, struct fd_pipe *pipe, uint32_t op)
{
	struct fd_device *dev = bo->dev;
	struct drm_exynos_g3d_cpu_prep req = {
			.handle = bo->handle,
			.op = op,
	};

	get_abs_timeout(&req.timeout, 5000);

	return drmCommandWrite(dev->fd, DRM_EXYNOS_G3D_CPU_PREP,
				&req, sizeof(req));
}

static void of_bo_cpu_fini(struct fd_bo *bo)
{
	struct fd_device *dev = bo->dev;
	struct drm_exynos_g3d_cpu_prep req = {
			.handle = bo->handle,
	};

	drmCommandWrite(dev->fd, DRM_EXYNOS_G3D_CPU_FINI, &req, sizeof(req));
}

static void of_bo_destroy(struct fd_bo *bo)
{
	struct of_bo *of_bo = to_of_bo(bo);
	free(of_bo);
}

static int of_bo_offset(struct fd_bo *bo, uint64_t *offset)
{
	struct of_bo *of_bo = to_of_bo(bo);

	if (!of_bo->offset) {
		struct fd_device *dev = bo->dev;
		struct drm_exynos_gem_map_off req = {
			.handle = bo->handle,
		};
		int ret;

		ret = drmCommandWriteRead(dev->fd, DRM_EXYNOS_GEM_MAP_OFFSET,
						&req, sizeof(req));
		if (ret)
			return ret;

		of_bo->offset = req.offset;
	}

	*offset = of_bo->offset;

	return 0;
}

static struct fd_bo_funcs funcs = {
		.offset = of_bo_offset,
		.cpu_prep = of_bo_cpu_prep,
		.cpu_fini = of_bo_cpu_fini,
		.destroy = of_bo_destroy,
};

/* allocate a buffer handle: */
int of_bo_new_handle(struct fd_device *dev,
		uint32_t size, uint32_t flags, uint32_t *handle)
{
	struct drm_exynos_gem_create req = {
			.size = size,
			.flags = EXYNOS_BO_WC,
	};
	int ret;

	ret = drmCommandWriteRead(dev->fd, DRM_EXYNOS_GEM_CREATE,
				  &req, sizeof(req));
	if (ret)
		return ret;

	*handle = req.handle;

	return 0;
}

/* allocate a new buffer object */
struct fd_bo * of_bo_from_handle(struct fd_device *dev,
		uint32_t size, uint32_t handle)
{
	struct of_bo *of_bo;
	struct fd_bo *bo;
	unsigned i;

	of_bo = calloc(1, sizeof(*of_bo));
	if (!of_bo)
		return NULL;

	bo = &of_bo->base;
	bo->funcs = &funcs;

	for (i = 0; i < ARRAY_SIZE(of_bo->list); i++)
		list_inithead(&of_bo->list[i]);

	return bo;
}
