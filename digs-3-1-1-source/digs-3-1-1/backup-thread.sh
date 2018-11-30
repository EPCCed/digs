#!/bin/bash
#

# Function to mirror one config file from the main node
mirrorConfigFile() {
    CFG_FILE_NAME=$1

    # Copy the file with a ".new" extension
    echo "Copying from gsiftp://${QCDGRID_MAIN_NODE}${QCDGRID_MAIN_NODE_PATH}/${CFG_FILE_NAME} to file://${QCDGRID_BINARIES_PATH}/${CFG_FILE_NAME}.new"

    globus-url-copy gsiftp://${QCDGRID_MAIN_NODE}${QCDGRID_MAIN_NODE_PATH}/${CFG_FILE_NAME} file://${QCDGRID_BINARIES_PATH}/${CFG_FILE_NAME}.new

    # and then move it into place, to avoid race conditions
    if [ $? -eq 0 ] ; then
	mv ${QCDGRID_BINARIES_PATH}/${CFG_FILE_NAME}.new ${QCDGRID_BINARIES_PATH}/${CFG_FILE_NAME}
    else
	echo "Error copying ${CFG_FILE_NAME}, not mirrored"
    fi
}

# determine QCDgrid path
QCDGRID_BINARIES_PATH=`which digs-list | sed 's|/digs\-list||'`

# look for nodes.conf
if [ ! -f $QCDGRID_BINARIES_PATH/nodes.conf ] ; then
    echo "Can't find QCDgrid installation"
    exit 1
fi
echo "QCDgrid path: $QCDGRID_BINARIES_PATH"

# Determine main node name
QCDGRID_MAIN_NODE=`cat $QCDGRID_BINARIES_PATH/nodes.conf | grep node | grep -v backup | sed 's/.*=//' | sed 's/#.*//' | tr -d ' \t'`
echo "Main node name: $QCDGRID_MAIN_NODE"

# and path to QCDgrid software on that node
QCDGRID_MAIN_NODE_PATH=`cat $QCDGRID_BINARIES_PATH/nodes.conf | grep path | grep -v backup | sed 's/.*=//' | sed 's/#.*//' | tr -d ' \t'`
echo "QCDgrid path on main node: $QCDGRID_MAIN_NODE_PATH"

# Loop forever
while /bin/true ; do

    # Check proxy exists and is valid for at least an hour
    grid-proxy-info > /dev/null
    if [ $? -ne 0 ] ; then
	grid-proxy-init
    fi
    
    PROXY_TIMELEFT=`grid-proxy-info -timeleft`
    if [ $PROXY_TIMELEFT -le 3600 ] ; then
	grid-proxy-init
    fi

    # Copy config files
    mirrorConfigFile mainnodelist.conf
    mirrorConfigFile deadnodes.conf
    mirrorConfigFile disablednodes.conf
    mirrorConfigFile retiringnodes.conf
    mirrorConfigFile qcdgrid.conf

    # Wait 5 minutes
    sleep 300
done
