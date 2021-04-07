# Installation

## Linux (Ubuntu 18.04 or 20.04)

Install the following apt packages

* Python3 and Pip package manager (may already be satisfied)
* Optional `gpac` for MP4Box (only needed if using `mdb_extractvideo --combine`)

```bash
sudo apt-get update
sudo apt-get install python3 python3-pip
sudo apt-get install gpac  # Optional
```

## Windows

1. Download and run the Miniconda [installer](https://docs.conda.io/en/latest/miniconda.html) to setup Python3
    1. Pip comes with the Anaconda Prompt which can be opened by searching from the Windows start button
2. MP4Box is only available for Linux, so this can be skipped

## All Operating Systems

Complete the following steps after finishing the OS-specific instructions above

1. Install the LMDB Python package: `pip3 install lmdb`
2. Download the scripts from Github (or use `git clone`)

# Usage

View the command options `./mdb_[commandName] --help`

All commands expect a `.mdb` file as input.
These are produced by the Scout Agent to store image frames and video clips in a more efficient, compressed format.
The location of these files depends on your agent's configuration, which you can view by either
  1. Opening the Scout Agent GUI and going to Configure > Agent Settings > Advanced
  2. Viewing the raw text file
      2. Linux: `/etc/openalpr/alprd.conf`
      2. Windows: `C:\OpenALPR\Agent\etc\openalpr\alprd.conf`

In the configuration, there are two keys (with defaults) which specify the folders to save MDB files
  1. Plates: `store_plates_location = /var/lib/openalpr/plateimages/`
  2. Videos: `store_video_location = /var/lib/openalpr/videoclips/`

Note for videos, you must also have `store_video = 1` enabled in your config (by default, this is off).
