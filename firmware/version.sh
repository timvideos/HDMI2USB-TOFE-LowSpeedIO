#!/bin/bash
cat > $1 <<EOF
#ifndef VERSION_H
#define VERSION_H

#define GIT_VERSION_HUMAN \\
	"$(git describe --long --dirty)"

#define GIT_VERSION_HASH \\
	"$(git rev-parse HEAD)"

#endif  /* VERSION_H */
EOF
