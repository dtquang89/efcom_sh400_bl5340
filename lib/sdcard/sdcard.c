/*
 * Copyright (c) 2021, Legrand North America, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <errno.h>
#include <zephyr/logging/log.h>

#include <sdcard.h>

LOG_MODULE_REGISTER(sdcard);

/* List dir entry by path
 *
 * @param path Absolute path to list
 *
 * @return Negative errno code on error, number of listed entries on
 *         success.
 */
int lsdir(const char* path)
{
    int res;
    struct fs_dir_t dirp;
    static struct fs_dirent entry;
    int count = 0;

    fs_dir_t_init(&dirp);

    /* Verify fs_opendir() */
    res = fs_opendir(&dirp, path);
    if (res) {
        LOG_ERR("Error opening dir %s [%d]", path, res);
        return res;
    }

    LOG_INF("Listing dir %s ...", path);
    for (;;) {
        /* Verify fs_readdir() */
        res = fs_readdir(&dirp, &entry);

        /* entry.name[0] == 0 means end-of-dir */
        if (res || entry.name[0] == 0) {
            break;
        }

        if (entry.type == FS_DIR_ENTRY_DIR) {
            LOG_INF("[DIR ] %s", entry.name);
        }
        else {
            LOG_INF("[FILE] %s (size = %zu)", entry.name, entry.size);
        }
        count++;
    }

    /* Verify fs_closedir() */
    res = fs_closedir(&dirp);
    if (res) {
        LOG_ERR("Error closing dir %s [%d]", path, res);
        return res;
    }

    return res;
}

int check_file_dir_exists(const char* path)
{
    int res;
    struct fs_dirent entry;

    /* Verify fs_stat() */
    res = fs_stat(path, &entry);

    return !res;
}

int sd_card_file_write(struct fs_file_t* file, const char* file_path, char* text_str, size_t text_str_size)
{
    ssize_t brw;
    int res;

    fs_file_t_init(file);

    if (check_file_dir_exists(file_path)) {
        LOG_DBG("Opening existing file %s", file_path);
        res = fs_open(file, file_path, FS_O_APPEND | FS_O_RDWR);
    }
    else {
        LOG_DBG("Creating new file %s", file_path);
        res = fs_open(file, file_path, FS_O_CREATE | FS_O_RDWR);
    }

    /* Verify fs_open() */
    if (res) {
        LOG_ERR("Failed opening file [%d]", res);
        fs_close(file);
        return res;
    }

    /* Verify fs_seek() */
    res = fs_seek(file, 0, FS_SEEK_SET);
    if (res) {
        LOG_ERR("fs_seek failed [%d]", res);
        fs_close(file);
        return res;
    }

    LOG_INF("Data written:\"%s\"", text_str);

    /* Verify fs_write() */
    brw = fs_write(file, (char*)text_str, text_str_size);
    if (brw < 0) {
        LOG_ERR("Failed writing to file [%zd]", brw);
        fs_close(file);
        return brw;
    }

    if (brw < text_str_size) {
        LOG_ERR("Unable to complete write. Volume full.");
        LOG_ERR("Number of bytes written: [%zd]", brw);
        fs_close(file);
        return -1;
    }

    LOG_DBG("Data successfully written!");

    /* Verify fs_sync() */
    res = fs_sync(file);
    if (res) {
        LOG_ERR("Error syncing file [%d]", res);
        return res;
    }

    LOG_DBG("Data successfully synced!");

    res = fs_close(file);
    if (res) {
        LOG_ERR("Error closing file [%d]", res);
        return res;
    }

    LOG_DBG("Closed file.");

    return res;
}

int sd_card_file_open(struct fs_file_t* file, const char* file_path, off_t skip_bytes)
{
    int res;

    fs_file_t_init(file);

    if (!check_file_dir_exists(file_path)) {
        LOG_INF("File does not exist %s", file_path);
        return -EIO;
    }

    /* Verify fs_open() */
    res = fs_open(file, file_path, FS_O_READ);
    if (res) {
        LOG_ERR("Failed opening file [%d]", res);
        fs_close(file);
        return res;
    }

    res = fs_seek(file, skip_bytes, FS_SEEK_SET);
    if (res) {
        LOG_ERR("fs_seek failed [%d]", res);
        fs_close(file);
        return res;
    }

    LOG_INF("Opened file %s", file_path);

    return res;
}

int sd_card_file_read(struct fs_file_t* file, void* buffer, size_t size)
{
    ssize_t read_bytes;

    /* Verify fs_read() */
    read_bytes = fs_read(file, buffer, size);
    if (read_bytes < 0) {
        LOG_ERR("Failed reading file [%zd]", read_bytes);
        fs_close(file);
    }

    return read_bytes;
}

void sd_card_file_close(struct fs_file_t* file)
{
    int ret = fs_close(file);
    if (ret < 0) {
        LOG_ERR("Failed to close file, err: %d", ret);
    }
    else {
        LOG_INF("Closed file");
    }
}

int sd_card_mkdir(char* folder_name)
{
    int res;

    res = fs_mkdir(folder_name);
    if (res) {
        LOG_ERR("Error create folder [%d]", res);
        return res;
    }

    return res;
}