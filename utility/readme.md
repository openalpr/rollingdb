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
