// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2026 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 */

#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/stop_machine.h>
#include <linux/delay.h>

static struct dentry *dir;
static u32 duration_ms;

static int stop_fn(void *data)
{
    mdelay(duration_ms);
    return 0;
}

static int start_set(void *data, u64 val)
{
    if (val != 1)
        return -EINVAL;
    if (duration_ms > 0)
        stop_machine(stop_fn, NULL, NULL);
    return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(start_fops, NULL, start_set, "%llu\n");

static int duration_ms_set(void *data, u64 val)
{
    if (val > 10000)
        return -EINVAL;
    duration_ms = val;
    return 0;
}

static int duration_ms_get(void *data, u64 *val)
{
    *val = duration_ms;
    return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(duration_ms_fops, duration_ms_get, duration_ms_set, "%llu\n");

static int __init system_freeze_init(void)
{
    dir = debugfs_create_dir("system_freeze", NULL);
    debugfs_create_file("duration_ms", 0600, dir, NULL, &duration_ms_fops);
    debugfs_create_file("start", 0200, dir, NULL, &start_fops);
    return 0;
}

static void __exit system_freeze_exit(void)
{
    debugfs_remove_recursive(dir);
}

module_init(system_freeze_init);
module_exit(system_freeze_exit);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("System Freeze Driver");
MODULE_AUTHOR("Amazon.com, Inc. or its affiliates");
