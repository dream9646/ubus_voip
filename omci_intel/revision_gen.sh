#!/bin/sh
REVISION=$(git log 2>/dev/null | head -1 | cut -c8-14)
echo "#define TAG_REVISION \"$REVISION\"" > cli/revision.h
