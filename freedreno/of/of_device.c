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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "of_priv.h"

static void of_device_destroy(struct fd_device *dev)
{
	struct of_device *of_dev = to_of_device(dev);
	free(of_dev);
}

static struct fd_device_funcs funcs = {
		.bo_new_handle = of_bo_new_handle,
		.bo_from_handle = of_bo_from_handle,
		.pipe_new = of_pipe_new,
		.destroy = of_device_destroy,
};

struct fd_device * of_device_new(int fd)
{
	struct of_device *of_dev;
	struct fd_device *dev;

	of_dev = calloc(1, sizeof(*of_dev));
	if (!of_dev)
		return NULL;

	dev = &of_dev->base;
	dev->funcs = &funcs;

	return dev;
}
