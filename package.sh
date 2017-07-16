#!/bin/bash

# Purpose of the package script is to compile everything and put the output into 
# "/debout/"

#!/bin/bash

CLONE_DIR=$(pwd)
TAR_PREFIX=$(cat distros/debian/control | grep Source | awk '{print $2;}')
OUT_DIR=/debout/
CPU_CORES=$(grep -c ^processor /proc/cpuinfo)

rm -R $OUT_DIR
mkdir -p $OUT_DIR

## NEED TO GET PRIVATE KEY
# Import the private key in the output dir
# gpg --allow-secret-key-import --import ${VOLUME_DIR}/matthill_openalpr.gpg_private.key
# echo "cert-digest-algo SHA256" >> ~/.gnupg/gpg.conf
# echo "digest-algo SHA256" >> ~/.gnupg/gpg.conf


VERSION=`dpkg-parsechangelog -ldistros/debian/changelog --show-field Version | sed 's/\-.*$//'`
#VERSION=`cat $CMAKELISTS_PATH | grep -E '^\s*SET\(OPENALPR[A-Z]*_.*_VERSION\s*"[0-9]+"' | tail -n 3  | grep -oE '"[0-9]+"' | grep -oE '[0-9]+' | paste -sd "." -`

#Swap in a build number based on the current date/time.
if [ -z "$BUILD_NUMBER" ]; then
    BUILD_NUMBER=`date "+%Y%m%d%H%M%S"`
fi
sed -i "s/$VERSION-[0-9\.]*/$VERSION-$BUILD_NUMBER/" distros/debian/changelog

ARCHIVE_PREFIX=${TAR_PREFIX}_${VERSION}.orig
git archive --prefix=${ARCHIVE_PREFIX}/ HEAD | gzip -9 > ${OUT_DIR}/${ARCHIVE_PREFIX}.tar.gz

cd $OUT_DIR
tar xvfz ./${ARCHIVE_PREFIX}.tar.gz
cp -R ${CLONE_DIR}/distros/debian/ ${ARCHIVE_PREFIX}/
cd ${ARCHIVE_PREFIX}/


debuild -eDEB_BUILD_OPTIONS="parallel=$CPU_CORES" || exit 1
