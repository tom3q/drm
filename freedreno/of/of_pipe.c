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


static int of_pipe_get_param(struct fd_pipe *pipe,
		enum fd_param_id param, uint64_t *value)
{
	ERROR_MSG("invalid param id: %d", param);
	return -1;
}

static int of_pipe_wait(struct fd_pipe *pipe, uint32_t timestamp)
{
	struct of_pipe *of_pipe = to_of_pipe(pipe);
	struct fd_device *dev = pipe->dev;
	struct drm_exynos_g3d_wait req = {
			.pipe = of_pipe->pipe,
			.fence = timestamp,
	};
	int ret;

	get_abs_timeout(&req.timeout, 5000);

	ret = drmCommandWrite(dev->fd, DRM_EXYNOS_G3D_WAIT, &req, sizeof(req));
	if (ret) {
		ERROR_MSG("wait-fence failed! %d (%s)", ret, strerror(errno));
		return ret;
	}

	return 0;
}

static void of_pipe_destroy(struct fd_pipe *pipe)
{
	struct of_pipe *of_pipe = to_of_pipe(pipe);
	int ret;

	if (of_pipe->pipe) {
		struct drm_exynos_g3d_pipe req = {
				.pipe = of_pipe->pipe,
		};

		ret = drmCommandWriteRead(pipe->dev->fd,
						DRM_EXYNOS_G3D_DESTROY_PIPE,
						&req, sizeof(req));
		if (ret)
			ERROR_MSG("G3D_DESTROY_PIPE failed! %d (%s)",
					ret, strerror(errno));
	}

	free(of_pipe);
}

static struct fd_pipe_funcs funcs = {
		.ringbuffer_new = of_ringbuffer_new,
		.get_param = of_pipe_get_param,
		.wait = of_pipe_wait,
		.destroy = of_pipe_destroy,
};

struct fd_pipe * of_pipe_new(struct fd_device *dev, enum fd_pipe_id id)
{
	struct drm_exynos_g3d_pipe req;
	struct of_pipe *of_pipe = NULL;
	struct fd_pipe *pipe = NULL;
	int ret;

	of_pipe = calloc(1, sizeof(*of_pipe));
	if (!of_pipe) {
		ERROR_MSG("allocation failed");
		goto fail;
	}

	pipe = &of_pipe->base;
	pipe->funcs = &funcs;

	memset(&req, 0, sizeof(req));

	ret = drmCommandWriteRead(dev->fd, DRM_EXYNOS_G3D_CREATE_PIPE,
					&req, sizeof(req));
	if (ret) {
		ERROR_MSG("G3D_CREATE_PIPE failed! %d (%s)",
				ret, strerror(errno));
		goto fail;
	}

	of_pipe->pipe = req.pipe;

	return pipe;

fail:
	if (pipe)
		fd_pipe_del(pipe);
	return NULL;
}
