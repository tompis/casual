#!/bin/bash

REPOSITORY_ROOT=$(git rev-parse --show-toplevel)
GATEWAY=$REPOSITORY_ROOT/middleware/gateway

if [[ -z $CASUAL_HOME ]]
then
   export CASUAL_HOME=/tmp/casual/unittest
fi

export LD_LIBRARY_PATH=$GATEWAY/bin:$REPOSITORY_ROOT/middleware/common/bin:$REPOSITORY_ROOT/middleware/serviceframework/bin

echo "markdown"
$GATEWAY/bin/casual-gateway-markdown-protocol > $GATEWAY/documentation/protocol/protocol.md

echo "binary"
$GATEWAY/bin/casual-gateway-binary-protocol --base $GATEWAY/documentation/protocol/example --format json

