# Prerequisites

* Ubuntu 18.04 or 20.04
* Python3
* Pip package manager

# Installation

1. Install the `lmdb` Python package
2. Install the `gpac` apt package (optional for `mdb_extractvideo --combine`)
  2. Confirm the installation of `MP4Box`

```bash
pip3 install lmdb
apt-get update && apt-get install -y gpac
MP4Box -version 
```

# Usage

View the command options `./mdb_[commandName] --help`

All commands expect a `.mdb` file as input.
These are produced by the Scout Agent to store image frames and video clips in a more efficient, compressed format.
The location of these files depends on your agent's configuration, which you can view by either
  1. Opening the Scout Agent GUI and going to Configure > Agent Settings > Advanced
  2. Viewing the plain text file `/etc/openalpr/alprd.conf` from terminal

In the configuration, there are two keys (with defaults) which specify the folders to save MDB files
  1. Plates: `store_plates_location = /var/lib/openalpr/plateimages/`
  2. Videos: `store_video_location = /var/lib/openalpr/videoclips/`

Note for videos, you must also have `store_video = 1` enabled in your config (by default, this is off).
