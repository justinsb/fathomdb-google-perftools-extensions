#!/bin/bash

rsync -r --links --verbose --times --include="*.h" --include="*/" --exclude="*" src/main/cpp/fathomdb/ /usr/local/include/fathomdb/

rsync -r --verbose --times --include="*.so" --exclude="*" bin/ /usr/local/lib/




